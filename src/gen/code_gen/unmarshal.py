# code_gen/unmarshal.py
import logging
from type_utils import get_c_type_str, is_lvgl_struct_ptr, get_unmarshal_signature_type

logger = logging.getLogger(__name__)

MAX_USER_ENUMS = 64 # Max user-defined enum mappings

# Map C types to the specific unmarshal function names we will generate
# *** Remove lv_coord_t from this map ***
UNMARSHAL_FUNC_MAP = {
    "int": "unmarshal_int",
    "int8_t": "unmarshal_int8",
    "uint8_t": "unmarshal_uint8",
    "int16_t": "unmarshal_int16",
    "uint16_t": "unmarshal_uint16",
    "int32_t": "unmarshal_int32",
    "uint32_t": "unmarshal_uint32",
    "int64_t": "unmarshal_int64",
    "uint64_t": "unmarshal_uint64",
    "size_t": "unmarshal_size_t", # Map to appropriate int size
    # "lv_coord_t": "unmarshal_int32", # REMOVED - Handled specially
    "lv_opa_t": "unmarshal_uint8",  # Assuming lv_opa_t is uint8_t
    "float": "unmarshal_float",
    "double": "unmarshal_double",
    "bool": "unmarshal_bool",
    "_Bool": "unmarshal_bool",
    "const char *": "unmarshal_string_ptr",
    "char *": "unmarshal_string_ptr", # Non-const strings might need malloc+copy? For now, treat as const char* from JSON
    "char": "unmarshal_char", # How is single char encoded in JSON? Number or string[0]? Assume string[0]
    "lv_color_t": "unmarshal_color", # Custom '#' prefix
    # Pointers are handled via prefix '@' -> unmarshal_registered_ptr
    # Enums are handled by unmarshal_enum_value
}

# Function to generate the get_unmarshal_call string remains the same,
# it will now call unmarshal_value for lv_coord_t, which will then dispatch correctly.
def get_unmarshal_call(c_type, pointer_level, is_array, json_var_name, dest_var_name):
    """Gets the C function call string for unmarshaling a specific type."""
    c_type_str = get_c_type_str(c_type, pointer_level)

    if pointer_level > 0 or is_array:
         # Most pointers are looked up via '@' prefix handled by unmarshal_value dispatcher
         # Special case: char* is treated as string
         if c_type == "char" and pointer_level == 1:
              # Use main dispatcher for consistency, it handles strings.
              return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name}, NULL)"
         else:
              # Use the generic pointer unmarshaler which expects '@' prefix string
              # or direct call for nested results. Let unmarshal_value handle it.
              return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name}, NULL)"

    # Handle enums explicitly if we can identify them (e.g. starts with LV_, or check against api_info['enums'])
    # This heuristic relies on type name, might need refinement
    if c_type.startswith("LV_") and c_type.endswith("_T"):
        return f"unmarshal_value({json_var_name}, \"{c_type}\", {dest_var_name}, NULL)" # Let dispatcher handle enums too
    if c_type.startswith("lv_") and c_type.endswith("_t") and c_type not in UNMARSHAL_FUNC_MAP and c_type != "lv_coord_t":
         # Assume LVGL typedefs not explicitly mapped (and not coord) are enums
         return f"unmarshal_value({json_var_name}, \"{c_type}\", {dest_var_name}, NULL)" # Let dispatcher handle enums too

    # Base types - use specific unmarshal func defined in UNMARSHAL_FUNC_MAP or dispatch via unmarshal_value
    func_name = UNMARSHAL_FUNC_MAP.get(c_type)
    if func_name:
        # Direct call to primitive unmarshaler (can be faster)
        # return f"{func_name}({json_var_name}, ({c_type_str}*){dest_var_name})"
        # OR consistently use the dispatcher for simpler logic:
        return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name}, NULL)"
    elif c_type == "lv_coord_t":
         # lv_coord_t uses the dispatcher which calls unmarshal_coord
         return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name}, NULL)"
    else:
        # Fallback for unknown types - use the dispatcher
        logger.warning(f"No specific unmarshal function found for C type '{c_type_str}'. Using generic dispatcher.")
        return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name}, NULL)"


def generate_enum_unmarshalers(hashed_and_sorted_enum_members, all_enum_members_map_for_type_check):
    """
    Generates C code for parsing enum strings using a hybrid approach:
    1. A small, runtime-configurable user table (linear scan).
    2. A large, pre-hashed, sorted table of generated enums (bsearch + strcmp for collisions).

    Args:
        hashed_and_sorted_enum_members (list): List of dicts, pre-sorted by hash then name.
            Each dict: {'name': str, 'value': int, 'hash': int, 'original_type_name': str}
        all_enum_members_map_for_type_check (dict): The old map, can be used by unmarshal_value's
                                                    heuristic if needed, but primary lookup is here.
    """
    c_code = "// --- Enum Unmarshaling (Hybrid Hashed Approach) ---\n\n"

    # --- 1. C Hash Function ---
    c_code += "static uint32_t djb2_hash_c(const char *str) {\n"
    c_code += "    uint32_t hash = 5381;\n"
    c_code += "    unsigned char c;\n" # Use unsigned char for safety with char values > 127
    c_code += "    while ((c = (unsigned char)*str++)) {\n"
    c_code += "        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */\n"
    c_code += "    }\n"
    c_code += "    return hash;\n"
    c_code += "}\n\n"

    # --- 2. User-Defined Enum Table ---
    c_code += "// User-defined enum mappings (runtime configurable)\n"
    c_code += "typedef struct {\n"
    c_code += "    const char *name; // Copied string, user responsible for lifetime if not literal\n"
    c_code += "    uint32_t hash;\n"
    c_code += "    int value;\n"
    c_code += "} user_enum_entry_t;\n\n"
    c_code += f"static user_enum_entry_t g_user_enum_table[{MAX_USER_ENUMS}];\n"
    c_code += f"static size_t g_num_user_enums = 0;\n\n"

    c_code += "// Function to add user-defined enum mappings (exposed via lvgl_json_renderer.h if needed)\n"
    c_code += "// For now, keep it static. If exposing, ensure thread safety if applicable.\n"
    c_code += "bool lvgl_json_add_user_enum_mapping(const char* name, int value) {\n"
    c_code += f"    if (g_num_user_enums >= {MAX_USER_ENUMS}) {{\n"
    c_code += "        LOG_ERR(\"User enum table full. Cannot add '%s'.\", name);\n"
    c_code += "        return false;\n"
    c_code += "    }\n"
    c_code += "    // Consider if 'name' should be strdup'd if not a literal. For now, assume it's persistent.\n"
    c_code += "    g_user_enum_table[g_num_user_enums].name = name; \n"
    c_code += "    g_user_enum_table[g_num_user_enums].hash = djb2_hash_c(name);\n"
    c_code += "    g_user_enum_table[g_num_user_enums].value = value;\n"
    c_code += "    g_num_user_enums++;\n"
    c_code += "    return true;\n"
    c_code += "}\n\n"
    c_code += "void lvgl_json_clear_user_enum_mappings() {\n"
    c_code += "    // If names were strdup'd, free them here.\n"
    c_code += "    g_num_user_enums = 0;\n"
    c_code += "}\n\n"


    # --- 3. Generated (Hard-coded) Enum Table ---
    c_code += "// Generated enum table (sorted by hash, then name)\n"
    c_code += "typedef struct {\n"
    c_code += "    uint32_t hash;\n"
    c_code += "    const char *original_name; // For collision resolution\n"
    c_code += "    int c_value;\n"
    c_code += "    // const char *original_type_name; // Optional: for type checking, adds to size\n"
    c_code += "} generated_enum_entry_t;\n\n"

    c_code += "static const generated_enum_entry_t g_generated_enum_table[] = {\n"
    if not hashed_and_sorted_enum_members:
        c_code += "    // No enum members were processed or included.\n"
    for member in hashed_and_sorted_enum_members:
        # Ensure name string is C-escaped if it could contain special chars (though unlikely for enum names)
        c_code += f"    {{0x{member['hash']:08x}, \"{member['name']}\", {member['value']:#04x} }}, // Type: {member['original_type_name']}\n"
    c_code += "};\n"
    c_code += "#define G_GENERATED_ENUM_TABLE_SIZE (sizeof(g_generated_enum_table) / sizeof(g_generated_enum_table[0]))\n\n"

    # --- 4. bsearch Comparator for Generated Table ---
    c_code += "// Comparator for bsearch on g_generated_enum_table (compares only hash for initial range find)\n"
    c_code += "static int compare_generated_enum_hash(const void *key_hash_ptr, const void *element_ptr) {\n"
    c_code += "    uint32_t key_hash = *(const uint32_t*)key_hash_ptr;\n"
    c_code += "    const generated_enum_entry_t *element = (const generated_enum_entry_t *)element_ptr;\n"
    c_code += "    if (key_hash < element->hash) return -1;\n"
    c_code += "    if (key_hash > element->hash) return 1;\n"
    c_code += "    return 0; // Hashes match\n"
    c_code += "}\n\n"

    # --- 5. The unmarshal_enum_value function ---
    c_code += "// Main enum unmarshaling function\n"
    c_code += "static bool unmarshal_enum_value(cJSON *json_value, const char *expected_enum_type_name, int *dest) {\n"
    c_code += "    if (!json_value || !dest) return false;\n\n"
    c_code += "    // Allow integer values directly for enums\n"
    c_code += "    if(cJSON_IsNumber(json_value)) {\n"
    c_code += "        *dest = (int)json_value->valuedouble;\n"
    c_code += "        return true;\n"
    c_code += "    }\n\n"
    c_code += "    if (!cJSON_IsString(json_value) || !json_value->valuestring) {\n"
    c_code += "       LOG_ERR_JSON(json_value, \"Enum Unmarshal Error: Expected string or number for enum type '%s', got type %d\", expected_enum_type_name ? expected_enum_type_name : \"unknown\", json_value->type);\n"
    c_code += "       return false;\n"
    c_code += "    }\n"
    c_code += "    const char *str_value = json_value->valuestring;\n"
    c_code += "    uint32_t input_hash = djb2_hash_c(str_value);\n\n"

    c_code += "    // 1. Search user-defined table first (allows overriding generated)\n"
    c_code += "    for (size_t i = 0; i < g_num_user_enums; ++i) {\n"
    c_code += "        if (g_user_enum_table[i].hash == input_hash) {\n"
    c_code += "            if (strcmp(g_user_enum_table[i].name, str_value) == 0) {\n"
    c_code += "                *dest = g_user_enum_table[i].value;\n"
    c_code += "                return true;\n"
    c_code += "            }\n"
    c_code += "        }\n"
    c_code += "    }\n\n"

    c_code += "    // 2. Search generated table using bsearch + strcmp for collisions\n"
    c_code += "    if (G_GENERATED_ENUM_TABLE_SIZE > 0) { // Only search if table is not empty\n"
    c_code += "        const generated_enum_entry_t *found_any_hash_match = (const generated_enum_entry_t *)bsearch(\n"
    c_code += "            &input_hash,\n"
    c_code += "            g_generated_enum_table,\n"
    c_code += "            G_GENERATED_ENUM_TABLE_SIZE,\n"
    c_code += "            sizeof(generated_enum_entry_t),\n"
    c_code += "            compare_generated_enum_hash);\n\n"
    c_code += "        if (found_any_hash_match) {\n"
    c_code += "            // bsearch found an element with matching hash. Now check original_name for exact match.\n"
    c_code += "            // Since table is sorted by hash then name, all collisions are contiguous.\n"
    c_code += "            // Iterate backwards from found_any_hash_match.\n"
    c_code += "            const generated_enum_entry_t *current_entry = found_any_hash_match;\n"
    c_code += "            while (current_entry >= g_generated_enum_table && current_entry->hash == input_hash) {\n"
    c_code += "                if (strcmp(current_entry->original_name, str_value) == 0) {\n"
    c_code += "                    // Optional: Check if current_entry->original_type_name matches expected_enum_type_name if type safety is critical\n"
    c_code += "                    *dest = current_entry->c_value;\n"
    c_code += "                    return true;\n"
    c_code += "                }\n"
    c_code += "                // Optimization: if original_name is already \"lesser\" than str_value (and table is sorted by name within hash groups), we can stop early for this direction\n"
    c_code += "                // but simple iteration is fine.\n"
    c_code += "                if (current_entry == g_generated_enum_table) break; // Boundary condition for first element\n"
    c_code += "                current_entry--;\n"
    c_code += "            }\n\n"
    c_code += "            // Iterate forwards from found_any_hash_match + 1 (found_any_hash_match itself was checked or was part of backward scan if it was the first match)\n"
    c_code += "            current_entry = found_any_hash_match + 1;\n"
    c_code += "            while (current_entry < (g_generated_enum_table + G_GENERATED_ENUM_TABLE_SIZE) && current_entry->hash == input_hash) {\n"
    c_code += "                if (strcmp(current_entry->original_name, str_value) == 0) {\n"
    c_code += "                    *dest = current_entry->c_value;\n"
    c_code += "                    return true;\n"
    c_code += "                }\n"
    c_code += "                current_entry++;\n"
    c_code += "            }\n"
    c_code += "        }\n"
    c_code += "    }\n\n"

    c_code += "    LOG_ERR_JSON(json_value, \"Enum Unmarshal Error: Unknown string value '%s' for enum type '%s' (hash 0x%08x)\", str_value, expected_enum_type_name ? expected_enum_type_name : \"unknown\", input_hash);\n"
    c_code += "    return false;\n"
    c_code += "}\n\n"

    return c_code



def generate_primitive_unmarshalers():
    """Generates C unmarshalers for basic C types (excluding lv_coord_t)."""
    code = "// --- Primitive Unmarshalers ---\n\n"

    # Integers (handle potential overflow if json number > C type limits?)
    # C standard behavior for double-to-int cast handles truncation.
    int_types = [
        ("int", "int"), ("int8", "int8_t"), ("uint8", "uint8_t"),
        ("int16", "int16_t"), ("uint16", "uint16_t"),
        ("int32", "int32_t"), ("uint32", "uint32_t"),
        ("int64", "int64_t"), ("uint64", "uint64_t"),
        ("size_t", "size_t"), # Assuming size_t fits in double for json value
        ("opa", "lv_opa_t") # Unmarshal opa type (usually uint8)
    ]
    for name, ctype in int_types:
        # Skip lv_coord_t if it happens to resolve to a base int type here
        if ctype == "lv_coord_t": continue

        code += f"static bool unmarshal_{name}(cJSON *node, {ctype} *dest) {{\n"
        code += f"    if (!cJSON_IsNumber(node)) {{ LOG_ERR_JSON(node, \"Expected number for {ctype}\"); return false; }}\n"
        # Add range checks here if necessary based on ctype limits vs double
        code += f"    *dest = ({ctype})node->valuedouble;\n"
        code += f"    // TODO: Add range check for {ctype}?\n"
        code += f"    return true;\n"
        code += f"}}\n\n"

    # Floats
    float_types = [("float", "float"), ("double", "double")]
    for name, ctype in float_types:
        code += f"static bool unmarshal_{name}(cJSON *node, {ctype} *dest) {{\n"
        code += f"    if (!cJSON_IsNumber(node)) {{ LOG_ERR_JSON(node, \"Expected number for {ctype}\"); return false; }}\n"
        code += f"    *dest = ({ctype})node->valuedouble;\n"
        code += f"    return true;\n"
        code += f"}}\n\n"

    # Bool
    code += f"static bool unmarshal_bool(cJSON *node, bool *dest) {{\n"
    code += f"    if (!cJSON_IsBool(node)) {{ LOG_ERR_JSON(node, \"Expected boolean\"); return false; }}\n"
    code += f"    *dest = cJSON_IsTrue(node);\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    # String Pointer (const char *)
    # Assumes string lifetime is managed by cJSON object or is literal
    code += f"static bool unmarshal_string_ptr(cJSON *node, const char **dest) {{\n"
    code += f"    if (!cJSON_IsString(node)) {{ LOG_ERR_JSON(node, \"Expected string\"); return false; }}\n"
    code += f"    *dest = node->valuestring;\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    # Char (from string[0]?) - This is ambiguous
    code += f"static bool unmarshal_char(cJSON *node, char *dest) {{\n"
    code += f"    if (cJSON_IsString(node) && node->valuestring && node->valuestring[0] != '\\0') {{ *dest = node->valuestring[0]; return true; }}\n"
    # Allow single char from number?
    code += f"    if (cJSON_IsNumber(node)) {{ *dest = (char)node->valuedouble; return true; }}\n"
    code += f"    LOG_ERR_JSON(node, \"Expected single-character string or number for char\");\n"
    code += f"    return false;\n"
    code += f"}}\n\n"

    return code

def generate_coord_unmarshaler(api_info):
    """ Generates the unmarshaler for lv_coord_t, handling numbers and percentages. """
    # Check if lv_pct is available
    lv_pct_available = any(f['name'] == 'lv_pct' for f in api_info['functions'])
    if not lv_pct_available:
        logger.warning("lv_pct function not found in filtered API. Percentage strings ('N%') for lv_coord_t will not work.")

    code = "// --- Coordinate (lv_coord_t) Unmarshaler ---\n\n"
    # Include lv_pct prototype if it's available
    if lv_pct_available:
        code += "// We assume lv_pct(int32_t) returns lv_coord_t (or compatible)\n"
        code += "extern lv_coord_t lv_pct(int32_t v);\n\n"

    code += "static bool unmarshal_coord(cJSON *node, lv_coord_t *dest) {\n"
    code += "    if (!node || !dest) return false;\n\n"
    code += "    // Try as number first\n"
    code += "    if (cJSON_IsNumber(node)) {\n"
    code += "        *dest = (lv_coord_t)node->valuedouble;\n"
    code += "        // TODO: Add range check for lv_coord_t?\n"
    code += "        return true;\n"
    code += "    }\n\n"
    code += "    // Try as percentage string \"N%\"\n"
    code += "    if (cJSON_IsString(node) && node->valuestring) {\n"
    code += "        const char *str = node->valuestring;\n"
    code += "        size_t len = strlen(str);\n"
    code += "        if (len > 2 && str[len - 1] == '%') {\n"
    if lv_pct_available:
        code += "            // Found '%', try parsing the number part\n"
        code += "            // Create a temporary null-terminated string for the number part\n"
        code += "            char num_part[32]; // Should be large enough for typical percentages\n"
        code += "            size_t num_len = len - 1;\n"
        code += "            if (num_len >= sizeof(num_part)) num_len = sizeof(num_part) - 1;\n"
        code += "            strncpy(num_part, str, num_len);\n"
        code += "            num_part[num_len] = '\\0';\n\n"
        code += "            // Parse the number (long is safe enough for int32_t)\n"
        code += "            char *endptr;\n"
        code += "            long val = strtol(num_part, &endptr, 10);\n\n"
        code += "            // Check if parsing was successful (endptr should point to the null terminator)\n"
        code += "            if (*endptr == '\\0') {\n"
        code += "                *dest = lv_pct((int32_t)val);\n"
        code += "                return true;\n"
        code += "            } else {\n"
        code += "                LOG_ERR_JSON(node, \"Coord Unmarshal Error: Invalid number format in percentage string '%s'\", str);\n"
        code += "                return false;\n"
        code += "            }\n"
    else:
        code += "            LOG_ERR_JSON(node, \"Coord Unmarshal Error: Percentage string '%%' found, but lv_pct function was not included in build.\");\n"
        code += "            return false;\n"
    code += "        }\n"
    code += "    }\n\n"
    code += "    // If not a number or valid percentage string\n"
    code += "    LOG_ERR_JSON(node, \"Coord Unmarshal Error: Expected number or percentage string ('N%%') for lv_coord_t, got type %d\", node->type);\n"
    code += "    return false;\n"
    code += "}\n\n"
    return code


def generate_custom_unmarshalers(api_info):
    """Generates unmarshalers for '#' color and '@' pointer syntaxes."""
    code = "// --- Custom Unmarshalers ---\n\n"

    # Color (#RRGGBB or #AARRGGBB)
    # Check if lv_color_hex is available
    lv_color_hex_available = any(f['name'] == 'lv_color_hex' for f in api_info['functions'])

    code += f"static bool unmarshal_color(cJSON *node, lv_color_t *dest) {{\n"
    code += f"    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '#') {{ LOG_ERR_JSON(node, \"Expected color string starting with #\"); return false; }}\n"
    code += f"    const char *hex_str = node->valuestring + 1;\n"
    code += f"    uint32_t hex_val = (uint32_t)strtoul(hex_str, NULL, 16);\n" # TODO: Error check strtoul?
    if lv_color_hex_available:
         code += f"    // Assuming lv_color_hex handles 6-digit (RRGGBB) correctly.\n"
         code += f"    // LVGL's lv_color_hex might ignore alpha if present.\n"
         code += f"    if (strlen(hex_str) == 6 || strlen(hex_str) == 8) {{ // Accept RRGGBB or AARRGGBB\n"
         # For AARRGGBB, lv_color_hex might only use bottom 24 bits.
         code += f"       *dest = lv_color_hex(hex_val & 0xFFFFFF); // Mask out potential alpha for lv_color_hex\n"
         code += f"       return true;\n"
         code += f"    }} else if (strlen(hex_str) == 3) {{ // Handle #RGB shorthand -> #RRGGBB\n"
         code += f"        unsigned int r = (hex_val >> 8) & 0xF;\n"
         code += f"        unsigned int g = (hex_val >> 4) & 0xF;\n"
         code += f"        unsigned int b = hex_val & 0xF;\n"
         code += f"        *dest = lv_color_hex( (r << 20 | r << 16) | (g << 12 | g << 8) | (b << 4 | b) );\n"
         code += f"        return true;\n"
         code += f"    }}\n"
    else:
         # Manual conversion if lv_color_hex not included (assuming 32bit color depth for simplicity)
         code += f"    // Manual conversion (assuming lv_color_t has r,g,b fields)\n"
         code += f"    if (strlen(hex_str) == 6) {{ // RRGGBB\n"
         code += f"       dest->red = (hex_val >> 16) & 0xFF;\n"
         code += f"       dest->green = (hex_val >> 8) & 0xFF;\n"
         code += f"       dest->blue = hex_val & 0xFF;\n"
         # code += f"       // dest->alpha = 0xFF; // If lv_color_t has alpha\n"
         code += f"       return true;\n"
         code += f"    }} else if (strlen(hex_str) == 3) {{ // #RGB\n"
         code += f"        unsigned int r = (hex_val >> 8) & 0xF; r = r | (r << 4);\n"
         code += f"        unsigned int g = (hex_val >> 4) & 0xF; g = g | (g << 4);\n"
         code += f"        unsigned int b = hex_val & 0xF; b = b | (b << 4);\n"
         code += f"        dest->red = r;\n"
         code += f"        dest->green = g;\n"
         code += f"        dest->blue = b;\n"
         # code += f"       // dest->alpha = 0xFF; // If lv_color_t has alpha\n"
         code += f"        return true;\n"
         code += f"    }}\n"
         # Add support for #AARRGGBB if lv_color_t has alpha? Requires knowing struct def.

    code += f"    LOG_ERR_JSON(node, \"Color Unmarshal Error: Invalid hex format '%s'. Expected #RGB, #RRGGBB.\", node->valuestring);\n"
    code += f"    return false;\n"
    code += f"}}\n\n"

    # Registered Pointer (@name)
    code += "// Forward declaration\n"
    code += "extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);\n\n"
    code += f"static bool unmarshal_registered_ptr(cJSON *node, const char *expected_ptr_type, void **dest) {{\n"
    code += f"    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '@') {{ LOG_ERR_JSON(node, \"Expected pointer string starting with @\"); return false; }}\n"
    code += f"    const char *name = node->valuestring + 1;\n"
    code += f"    *dest = lvgl_json_get_registered_ptr(name, expected_ptr_type);\n"
    code += f"    if (!(*dest)) {{\n"
    # Error message is already logged by lvgl_json_get_registered_ptr if type mismatch or not found
    # code += f"       LOG_ERR_JSON(node, \"Pointer Unmarshal Error: Registered pointer '@%s' (type %s) not found or type mismatch.\", name, expected_ptr_type ? expected_ptr_type : \"any\");\n"
    code += f"       return false;\n"
    code += f"    }}\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    code += "// Context Value ($variable_name)\n"
    code += "static cJSON* get_current_context(void); // Forward declaration from renderer code\n"
    code += "// unmarshal_value is also forward declared later or should be available\n\n"
    code += "static bool unmarshal_context_value(cJSON *json_source_node, const char *expected_c_type, void *dest) {\n"
    code += "    if (!cJSON_IsString(json_source_node) || !json_source_node->valuestring || json_source_node->valuestring[0] != '$') {\n"
    code += "        LOG_ERR_JSON(json_source_node, \"Context Unmarshal Error: Expected string starting with '$'\");\n"
    code += "        return false;\n"
    code += "    }\n"
    code += "    const char *var_name = json_source_node->valuestring + 1; // Skip '$'\n"
    code += "    if (strlen(var_name) == 0) {\n"
    code += "        LOG_ERR_JSON(json_source_node, \"Context Unmarshal Error: Empty variable name after '$'.\");\n"
    code += "        return false;\n"
    code += "    }\n\n"
    code += "    cJSON *current_ctx = get_current_context();\n"
    code += "    if (!current_ctx) {\n"
    code += "        LOG_ERR_JSON(json_source_node, \"Context Unmarshal Error: No context active for variable '%s'.\", var_name);\n"
    code += "        return false;\n"
    code += "    }\n\n"
    code += "    cJSON *value_from_context = cJSON_GetObjectItemCaseSensitive(current_ctx, var_name);\n"
    code += "    if (!value_from_context) {\n"
    code += "        LOG_ERR_JSON(json_source_node, \"Context Unmarshal Error: Variable '%s' not found in current context.\", var_name);\n"
    # Log the current context for debugging
    # code += "        char* ctx_str = json_node_to_string(current_ctx);\n"
    # code += "        LOG_DEBUG(\"Current context for failed lookup of '%s': %s\", var_name, ctx_str ? ctx_str : \"N/A\");\n"
    # code += "        if (ctx_str) cJSON_free(ctx_str);\n"
    code += "        return false;\n"
    code += "    }\n\n"
    code += "    // Recursively call unmarshal_value with the node found in the context\n"
    code += "    // This allows context values to be numbers, strings, booleans, or even other context/pointer refs.\n"
    code += "    if (!unmarshal_value(value_from_context, expected_c_type, dest, NULL)) {\n"
    # unmarshal_value would have logged the specific error
    code += "        LOG_ERR_JSON(json_source_node, \"Context Unmarshal Error: Failed to unmarshal context variable '%s' as type '%s'.\", var_name, expected_c_type);\n"
    code += "        return false;\n"
    code += "    }\n"
    code += "    return true;\n"
    code += "}\n\n"

    return code

    return code

# --- Functions for Transpilation Support ---

def generate_unmarshal_value_to_c_string_prototype():
    """Returns the C prototype for unmarshal_value_to_c_string."""
    return "static bool unmarshal_value_to_c_string(cJSON *json_value, const char *expected_c_type, char **dest_c_code_str, void *implicit_parent_for_context);"

def generate_unmarshal_value_to_c_string_helpers():
    """Generates helper C functions for unmarshal_value_to_c_string."""
    c_code = "// --- Transpilation String Helper Functions ---\n\n"
    # Helper to escape a string for C code output
    c_code += """
static char* escape_c_string_for_transpile(const char* input_str) {
    if (!input_str) return NULL;
    size_t len = strlen(input_str);
    // Estimate required buffer size: worst case each char needs escaping + quotes + null terminator
    char *escaped_str = (char*)lv_malloc(len * 2 + 3); // len * 2 for escapes, +2 for quotes, +1 for null
    if (!escaped_str) return NULL;

    char *p = escaped_str;
    *p++ = '"'; // Start with a quote

    for (size_t i = 0; i < len; ++i) {
        switch (input_str[i]) {
            case '\\\\': *p++ = '\\\\'; *p++ = '\\\\'; break;
            case '"':  *p++ = '\\\\'; *p++ = '"';  break;
            case '\\n': *p++ = '\\\\'; *p++ = 'n';  break;
            case '\\r': *p++ = '\\\\'; *p++ = 'r';  break;
            case '\\t': *p++ = '\\\\'; *p++ = 't';  break;
            // Add other escapes as needed (e.g., \\f, \\b, \\v, \\?, \\', \\0NNN, \\xHH)
            default:
                // Check for non-printable characters (optional, depends on desired strictness)
                // if (input_str[i] < 32 || input_str[i] > 126) {
                //    sprintf(p, "\\\\x%02X", (unsigned char)input_str[i]); p += 4;
                // } else {
                    *p++ = input_str[i];
                // }
                break;
        }
    }
    *p++ = '"'; // End with a quote
    *p++ = '\\0';
    return escaped_str; // Caller must lv_free this
}
"""
    c_code += "\n// --- End Transpilation String Helper Functions ---\n\n"
    return c_code

def generate_unmarshal_value_to_c_string():
    """Generates the C code for unmarshal_value_to_c_string."""
    c_code = "// --- JSON to C Code String Unmarshaler ---\n\n"
    c_code += "static bool unmarshal_value_to_c_string(cJSON *json_value, const char *expected_c_type, char **dest_c_code_str, void *implicit_parent_for_context) {\n"
    c_code += "    if (!json_value || !expected_c_type || !dest_c_code_str) {\n"
    c_code += "        LOG_ERR(\"Transpile Unmarshal Error: NULL argument passed.\");\n"
    c_code += "        if (dest_c_code_str) *dest_c_code_str = NULL;\n"
    c_code += "        return false;\n"
    c_code += "    }\n"
    c_code += "    *dest_c_code_str = NULL; // Initialize\n"
    c_code += "    char temp_buf[512]; // For sprintf operations\n\n"

    c_code += "    if (cJSON_IsNull(json_value)) {\n"
    c_code += "        *dest_c_code_str = lv_strdup(\"NULL\");\n"
    c_code += "        return (*dest_c_code_str != NULL);\n"
    c_code += "    }\n\n"

    c_code += "    if (cJSON_IsBool(json_value)) {\n"
    c_code += "        *dest_c_code_str = lv_strdup(cJSON_IsTrue(json_value) ? \"true\" : \"false\");\n"
    c_code += "        return (*dest_c_code_str != NULL);\n"
    c_code += "    }\n\n"

    c_code += "    if (cJSON_IsNumber(json_value)) {\n"
    c_code += "        if (strcmp(expected_c_type, \"float\") == 0 || strcmp(expected_c_type, \"double\") == 0) {\n"
    c_code += "            snprintf(temp_buf, sizeof(temp_buf), \"%f\", json_value->valuedouble);\n"
    c_code += "        } else { // Assume integer or lv_coord_t (which is often int)\n"
    c_code += "            snprintf(temp_buf, sizeof(temp_buf), \"%lld\", (long long)json_value->valuedouble);\n"
    c_code += "        }\n"
    c_code += "        *dest_c_code_str = lv_strdup(temp_buf);\n"
    c_code += "        return (*dest_c_code_str != NULL);\n"
    c_code += "    }\n\n"

    c_code += "    if (cJSON_IsString(json_value) && json_value->valuestring) {\n"
    c_code += "        const char *str_val = json_value->valuestring;\n"
    c_code += "        size_t len = strlen(str_val);\n\n"

    c_code += "        if (len > 0 && str_val[0] == '$') {\n"
    c_code += "            // Context variable: use variable name directly (assuming it's a valid C identifier)\n"
    c_code += "            snprintf(temp_buf, sizeof(temp_buf), \"%s\", str_val + 1);\n"
    c_code += "            *dest_c_code_str = lv_strdup(temp_buf);\n"
    c_code += "            return (*dest_c_code_str != NULL);\n"
    c_code += "        }\n\n"

    c_code += "        if (len > 0 && str_val[0] == '#') {\n"
    c_code += "            uint32_t hex_val = (uint32_t)strtoul(str_val + 1, NULL, 16);\n"
    c_code += "            // TODO: Handle #RGB shorthand if lv_color_hex doesn't\n"
    c_code += "            snprintf(temp_buf, sizeof(temp_buf), \"lv_color_hex(0x%06X)\", hex_val & 0xFFFFFF);\n"
    c_code += "            *dest_c_code_str = lv_strdup(temp_buf);\n"
    c_code += "            return (*dest_c_code_str != NULL);\n"
    c_code += "        }\n\n"

    c_code += "        if (len > 0 && str_val[0] == '@') {\n"
    c_code += "            const char *name = str_val + 1;\n"
    c_code += "            // Try to get the C variable name registered for this path\n"
    c_code += "            char* c_var_name = (char*)lvgl_json_get_registered_ptr(name, \"c_var_name\");\n"
    c_code += "            if (c_var_name) {\n"
    c_code += "                *dest_c_code_str = lv_strdup(c_var_name);\n"
    c_code += "            } else {\n"
    c_code += "                // Fallback: use the name directly, assuming it's a global C variable/macro or will be resolved by C compiler\n"
    c_code += "                LOG_WARN(\"Transpile Unmarshal: Registered C variable name for '@%s' not found. Using name directly.\", name);\n"
    c_code += "                snprintf(temp_buf, sizeof(temp_buf), \"%s\", name);\n"
    c_code += "                *dest_c_code_str = lv_strdup(temp_buf);\n"
    c_code += "            }\n"
    c_code += "            return (*dest_c_code_str != NULL);\n"
    c_code += "        }\n\n"

    c_code += "        if (len > 0 && str_val[0] == '!') { // Static string for registry\n"
    c_code += "             // This should resolve to a string literal that was previously registered and will be part of the transpiled binary.\n"
    c_code += "             // For now, just treat it like a normal string to be escaped. The registry itself isn't part of the transpiled output directly.\n"
    c_code += "             *dest_c_code_str = escape_c_string_for_transpile(str_val + 1);\n"
    c_code += "             return (*dest_c_code_str != NULL);\n"
    c_code += "        }\n\n"

    c_code += "        if (len > 1 && str_val[len - 1] == '%') { // Coordinate percentage\n"
    c_code += "            if (strcmp(expected_c_type, \"lv_coord_t\") == 0 || strcmp(expected_c_type, \"int32_t\") == 0) {\n"
    c_code += "                char num_part[32];\n"
    c_code += "                size_t num_len = len - 1;\n"
    c_code += "                if (num_len >= sizeof(num_part)) num_len = sizeof(num_part) - 1;\n"
    c_code += "                strncpy(num_part, str_val, num_len);\n"
    c_code += "                num_part[num_len] = '\\0';\n"
    c_code += "                snprintf(temp_buf, sizeof(temp_buf), \"lv_pct(%s)\", num_part);\n"
    c_code += "                *dest_c_code_str = lv_strdup(temp_buf);\n"
    c_code += "                return (*dest_c_code_str != NULL);\n"
    c_code += "            }\n"
    c_code += "        }\n\n"

    c_code += "        // Default string: Could be an enum or a literal string\n"
    c_code += "        int temp_int_for_validation;\n"
    c_code += "        // Create a temporary cJSON string node to pass to unmarshal_enum_value\n"
    c_code += "        cJSON* temp_json_str_node = cJSON_CreateString(str_val);\n"
    c_code += "        if (!temp_json_str_node) { LOG_ERR(\"Failed to create temp cJSON node for enum check\"); return false; }\n"
    c_code += "        bool is_enum = unmarshal_enum_value(temp_json_str_node, expected_c_type, &temp_int_for_validation);\n"
    c_code += "        cJSON_Delete(temp_json_str_node); // Clean up temp node\n"
    c_code += "        if (is_enum) {\n"
    c_code += "            // It's a valid enum, use its C name directly\n"
    c_code += "            *dest_c_code_str = lv_strdup(str_val);\n"
    c_code += "            return (*dest_c_code_str != NULL);\n"
    c_code += "        } else if (strcmp(expected_c_type, \"const char *\") == 0 || strcmp(expected_c_type, \"char *\") == 0) {\n"
    c_code += "            // It's a literal string for a char* type\n"
    c_code += "            *dest_c_code_str = escape_c_string_for_transpile(str_val);\n"
    c_code += "            return (*dest_c_code_str != NULL);\n"
    c_code += "        }\n"
    c_code += "        // If it's a string but not an enum and not for a char* type, it might be an error or unhandled case.\n"
    c_code += "        // For example, a string value for an int type that wasn't caught by other specific prefixes.\n"
    c_code += "        // LOG_WARN_JSON(json_value, \"Transpile Unmarshal: String '%s' is not a recognized enum for type '%s' and not a char* type.\", str_val, expected_c_type);\n"
    c_code += "        // Fall through to general error handling for strings not meeting criteria.\n"
    c_code += "\n        // If the string was not handled by any specific prefix or type check above:\n"
    c_code += "        LOG_ERR_JSON(json_value, \"Transpile Unmarshal Error: String value '%s' could not be interpreted for expected type '%s'.\", str_val, expected_c_type);\n"
    c_code += "        *dest_c_code_str = lv_strdup(\"/* UNHANDLED_STRING_FORMAT_FOR_TYPE */\");\n"
    c_code += "        return false; // Explicitly return false for unhandled strings\n"
    c_code += "    }\n\n    // End of cJSON_IsString block\n"

    c_code += "    if (cJSON_IsObject(json_value) && cJSON_GetObjectItemCaseSensitive(json_value, \"call\")) {\n"
    c_code += "        LOG_ERR_JSON(json_value, \"Transpile Error: 'call' object should be handled by invoker, not unmarshal_value_to_c_string directly when it's a direct argument.\");\n"
    c_code += "        *dest_c_code_str = lv_strdup(\"/* ERROR_CALL_OBJ_UNEXPECTED_IN_UNMARSHAL_TO_C_STRING */\");\n"
    c_code += "        return false;\n"
    c_code += "    }\n\n"
    # If it's an object but not a "call" object, it's an unhandled case for direct C string conversion.
    c_code += "    if (cJSON_IsObject(json_value)) {\n"
    c_code += "        LOG_ERR_JSON(json_value, \"Transpile Unmarshal Error: Cannot convert generic JSON object to C string for type '%s' unless it's a 'call' handled by invoker.\", expected_c_type);\n"
    c_code += "        *dest_c_code_str = lv_strdup(\"/* ERROR_GENERIC_OBJECT_UNEXPECTED */\");\n"
    c_code += "        return false;\n"
    c_code += "    }\n\n"

    c_code += "    LOG_ERR_JSON(json_value, \"Transpile Unmarshal Error: Could not format value to C string for type '%s' (JSON type %d)\", expected_c_type, json_value->type);\n"
    c_code += "    *dest_c_code_str = lv_strdup(\"/* UNABLE_TO_TRANSPILE_ARG */\");\n"
    c_code += "    return false; // Indicate failure\n"
    c_code += "}\n\n"
    c_code += "// --- End JSON to C Code String Unmarshaler ---\n\n"
    return c_code

def generate_main_unmarshaler():
    """Generates the core unmarshal_value function."""
    code = "// --- Main Value Unmarshaler ---\n\n"
    code += generate_unmarshal_value_to_c_string_prototype() + "\n\n"; # Add prototype
    code += "// Forward declarations for specific type unmarshalers\n"
    code += "static const invoke_table_entry_t* find_invoke_entry(const char *name);\n"
    # unmarshal_enum_value is already generated by generate_enum_unmarshalers
    # code += "static bool unmarshal_enum_value(cJSON *json_value, const char *enum_type_name, int *dest);\n"
    # unmarshal_color is generated by generate_custom_unmarshalers
    # code += "static bool unmarshal_color(cJSON *node, lv_color_t *dest);\n"
    # unmarshal_coord is generated by generate_coord_unmarshaler
    # code += "static bool unmarshal_coord(cJSON *node, lv_coord_t *dest);\n"
    # unmarshal_registered_ptr and unmarshal_context_value are generated by generate_custom_unmarshalers
    # code += "static bool unmarshal_registered_ptr(cJSON *node, const char* expected_ptr_type, void **dest);\n"
    # code += "static bool unmarshal_context_value(cJSON *json_source_node, const char *expected_c_type, void *dest);\n"

    # Primitive unmarshalers are already static and generated before this, no need for forward decls here
    # if they are in the same C file and defined before unmarshal_value.
    # However, explicit forward declarations don't hurt.
    # code += "static bool unmarshal_int(cJSON *node, int *dest);\n"
    code += "static bool unmarshal_int8(cJSON *node, int8_t *dest);\n"
    code += "static bool unmarshal_uint8(cJSON *node, uint8_t *dest);\n"
    code += "static bool unmarshal_int16(cJSON *node, int16_t *dest);\n"
    code += "static bool unmarshal_uint16(cJSON *node, uint16_t *dest);\n"
    code += "static bool unmarshal_int32(cJSON *node, int32_t *dest);\n"
    code += "static bool unmarshal_uint32(cJSON *node, uint32_t *dest);\n"
    code += "static bool unmarshal_int64(cJSON *node, int64_t *dest);\n"
    code += "static bool unmarshal_uint64(cJSON *node, uint64_t *dest);\n"
    code += "static bool unmarshal_size_t(cJSON *node, size_t *dest);\n"
    code += "static bool unmarshal_opa(cJSON *node, lv_opa_t *dest);\n"
    code += "static bool unmarshal_float(cJSON *node, float *dest);\n"
    code += "static bool unmarshal_double(cJSON *node, double *dest);\n"
    code += "static bool unmarshal_bool(cJSON *node, bool *dest);\n"
    code += "static bool unmarshal_string_ptr(cJSON *node, const char **dest);\n"
    # code += "static bool unmarshal_char(cJSON *node, char *dest);\n"

    code += "\n"
    code += "// The core dispatcher for unmarshaling any value from JSON based on expected C type (RENDER_MODE).\n"
    code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent) {\n"
    code += "    if (!json_value || !expected_c_type || !dest) {\n"
    code += "        LOG_ERR(\"Unmarshal Error: NULL argument passed to unmarshal_value (%p, %s, %p)\", json_value, expected_c_type ? expected_c_type : \"NULL\", dest);\n"
    code += "        return false;\n"
    code += "     }\n\n"

    code += "    if (cJSON_IsNull(json_value) && strcmp(expected_c_type, \"void *\") == 0) {\n"
    code += "      *((char **) dest) = NULL;\n"
    code += "      return true;\n"
    code += "    }\n\n"

    code += "    // 1. Handle nested function calls { \"call\": \"func_name\", \"args\": [...] }\n"
    code += "    if (cJSON_IsObject(json_value)) {\n"
    code += "        cJSON *call_item = cJSON_GetObjectItemCaseSensitive(json_value, \"call\");\n"
    code += "        cJSON *args_item = cJSON_GetObjectItemCaseSensitive(json_value, \"args\");\n"
    code += "        if (call_item && cJSON_IsString(call_item) && args_item && cJSON_IsArray(args_item)) {\n"
    code += "            const char *func_name = call_item->valuestring;\n"
    code += "            const invoke_table_entry_t* entry = find_invoke_entry(func_name);\n"
    code += "            if (!entry) {\n"
    code += "                LOG_ERR_JSON(json_value, \"Unmarshal Error: Nested call function '%s' not found in invoke table.\", func_name);\n"
    code += "                return false;\n"
    code += "            }\n"
    code += "            size_t args = 0; for (args = 0; entry->arg_types[args] != NULL; ++args) {}\n"
    code += "            lv_obj_t *target_obj_ptr = NULL;\n"
    code += "            if (entry->arg_types[0] && strcmp(entry->arg_types[0], \"lv_obj_t *\") == 0 && cJSON_GetArraySize(args_item) < args) { target_obj_ptr = implicit_parent; }\n" 
    code += "            // Make the nested call. Result goes into 'dest'.\n"
    code += "            // This is in RENDER_MODE context of unmarshal_value, so transpile params are NULL.\n"
    code += "            if (!entry->invoke(entry, target_obj_ptr, NULL /*target_obj_c_name_transpile*/, dest /*result_dest_render_mode*/, NULL /*result_c_name_transpile*/, args_item)) {\n"
    code += "                 LOG_ERR_JSON(json_value, \"Unmarshal Error: Nested call to '%s' failed.\", func_name);\n"
    code += "                 return false;\n"
    code += "            }\n"
    # TODO: Check if return type of nested call matches expected_c_type? Requires more info.
    code += "            return true; // Nested call successful\n"
    code += "        }\n"
    code += "        // If it's an object but not a 'call' object, it's an error unless expecting a specific struct type?\n"
    code += "        // Currently, we don't handle passing structs directly via JSON objects other than 'call'.\n"
    code += "    }\n\n"

    code += "    // 2. Handle custom pre- and post-fixes in strings ('$', '#', '@', '%', '!')\n"
    code += "    // Also handle non-prefixed strings that might be enums or regular strings.\n"
    code += "    if (cJSON_IsString(json_value) && json_value->valuestring) {\n"
    code += "        char *str_val = json_value->valuestring;\n"
    code += "        size_t len = strlen(str_val);\n"
    code += "        // Check for '$', '@', '%', '#' prefix\n"
    code += "        if (len) {\n"
    code += "            if (len > 2 && str_val[len - 1] == '%') {\n"
    code += "               if (str_val[len - 2] != '%') {\n"
    code += "                 if (strcmp(expected_c_type, \"lv_coord_t\") == 0 || strcmp(expected_c_type, \"int32_t\") == 0) {\n"
    code += "                     return unmarshal_coord(json_value, (lv_coord_t *)dest);\n"
    code += "                 } else {\n"
    code += "                     LOG_ERR_JSON(json_value, \"Unmarshal Error: Found percent string '%s' but expected type '%s'\", str_val, expected_c_type);\n"
    code += "                     //return false;\n"
    code += "                 }\n"
    code += "               } else {\n"
    code += "                 str_val[--len] = '\\0';\n"
    code += "               }\n"
    code += "            }\n"
    code += "            if (str_val[0] == '#') {\n"
    code += "               if (strcmp(expected_c_type, \"lv_color_t\") == 0) {\n"
    code += "                   return unmarshal_color(json_value, (lv_color_t*)dest);\n"
    code += "               } else {\n"
    code += "                   LOG_ERR_JSON(json_value, \"Unmarshal Error: Found color string '%s' but expected type '%s'\", str_val, expected_c_type);\n"
    code += "                   //return false;\n"
    code += "               }\n"
    code += "            }\n"
    code += "            // Check for '@' pointer prefix\n"
    code += "            if (str_val[0] == '@') {\n"
    code += "               if (str_val[len - 1] != '@') {\n"
    code += "                 return unmarshal_registered_ptr(json_value, expected_c_type, (void**)dest);\n"
    #                         Too many special cases and type-defs for this simlpe check to work
    # code += "                 // Ensure expected type is some kind of pointer or a special component type\n"
    # code += "                 if (strchr(expected_c_type, '*') || strcmp(expected_c_type, \"component_json_node\") == 0) {\n"
    # code += "                      return unmarshal_registered_ptr(json_value, expected_c_type, (void**)dest);\n"
    # code += "                 } else {\n"
    # code += "                     LOG_ERR_JSON(json_value, \"Unmarshal Error: Found registered pointer string '%s' but expected non-pointer type '%s'\", str_val, expected_c_type);\n"
    # code += "                     //return false;\n"
    # code += "                 }\n"
    code += "               } else { str_val[--len] = '\\0'; }\n"
    code += "            }\n"
    code += "            // Check for '!' (registered, static string) prefix\n"
    code += "            if (str_val[0] == '!') {\n"
    code += "               if (str_val[len - 1] != '!') {\n"
    code += "                 if (strcmp(expected_c_type, \"const char *\") == 0 || strcmp(expected_c_type, \"char *\") == 0) {\n"
    code += "                      const char *res = NULL; unmarshal_string_ptr(json_value, (const char **) &res);\n"
    code += "                      *((char **) dest) = lvgl_json_register_str(res + 1);\n"
    code += "                      LOG_INFO(\"Unmarshaled static string '%s'\", res);\n"
    code += "                      return res;\n"
    code += "                 } else {\n"
    code += "                     LOG_ERR_JSON(json_value, \"Unmarshal Error: Found registered static string '%s' but expected non-string type '%s'\", str_val, expected_c_type);\n"
    code += "                     //return false;\n"
    code += "                 }\n"
    code += "               } else { str_val[--len] = '\\0'; }\n"
    code += "            }\n"
    code += "            // Check for '$' context variable prefix FIRST\n"
    code += "            if (str_val[0] == '$') {\n"
    code += "                if (str_val[len - 1] == '$') {\n"
    code += "                   str_val[--len] = '\\0';\n"
    code += "                } else { return unmarshal_context_value(json_value, expected_c_type, dest); }\n"
    code += "            }\n"
    code += "        }\n"

    # --- String Fallback Handling (Enums or Regular Strings) ---
    code += "        // If no prefix, it could be an enum name or a regular string.\n"
    code += "        // Try enum first if the type looks like an LVGL type/typedef\n"
    code += "        // Heuristic: Check if expected_c_type looks like an enum name (e.g. lv_align_t, LV_...)\n"
    code += "        // We need a reliable way to know if expected_c_type IS an enum.\n"
    code += "        // For now, let's try enum unmarshal if it looks like lv_..._t or LV_..._T\n"
    code += "        bool maybe_enum = (strncmp(expected_c_type, \"lv_\", 3) == 0 && strstr(expected_c_type, \"_t\")) || \n"
    code += "                          (strncmp(expected_c_type, \"LV_\", 3) == 0 && strstr(expected_c_type, \"_T\")) || \n"
    code += "                          (strncmp(expected_c_type, \"int\", 3) == 0) || (strncmp(expected_c_type, \"uint\", 4) == 0);\n"
    code += "        if (maybe_enum && strcmp(expected_c_type, \"lv_obj_t\") != 0 && strcmp(expected_c_type, \"lv_style_t\") != 0 /* add other known non-enums */) {\n"
    code += "            if (unmarshal_enum_value(json_value, expected_c_type, (int*)dest)) {\n"
    code += "                 return true; // Successfully parsed as enum string\n"
    code += "            }\n"
    code += "            // If unmarshal_enum_value failed, it logged the error. Fall through?\n"
    code += "            // Maybe don't fall through, enum name was expected but invalid.\n"
    code += "            LOG_WARN_JSON(json_value, \"Enum parse failed for '%s' as type %s, falling back to string?\", str_val, expected_c_type);\n"
    code += "            // return false; // Strict: If it looked like an enum, it must parse as one.\n"
    code += "        }\n\n"

    code += "        // Finally, try as a regular string pointer if expected type is `const char *` (or rarely void * - DANGER!)\n"
    code += "        if (strcmp(expected_c_type, \"const char *\") == 0 || strcmp(expected_c_type, \"char *\") == 0 || strcmp(expected_c_type, \"void *\") == 0) {\n"
    code += "             return unmarshal_string_ptr(json_value, (const char **)dest);\n"
    code += "        }\n"

    code += "        // If it's a string but none of the above matched, it's an error.\n"
    code += "        // Except if expecting lv_coord_t, handled below.\n"
    code += "        if (strcmp(expected_c_type, \"lv_coord_t\") != 0) { // Don't error here for coord % strings\n"
    code += "           LOG_ERR_JSON(json_value, \"Unmarshal Error: Got string '%s' but couldn't interpret as color, ptr, enum, or expected string type '%s'\", str_val, expected_c_type);\n"
    code += "           return false;\n"
    code += "        }\n"
    code += "    }\n\n" # End of cJSON_IsString block


    code += "    // 3. Dispatch based on explicitly known C types\n"
    code += "    if (strcmp(expected_c_type, \"lv_coord_t\") == 0) {\n"
    code += "        return unmarshal_coord(json_value, (lv_coord_t*)dest);\n"
    code += "    }\n"
    # Add lv_color_t here too for completeness, although '#' prefix usually catches it.
    code += "    else if (strcmp(expected_c_type, \"lv_color_t\") == 0) {\n"
    code += "        return unmarshal_color(json_value, (lv_color_t*)dest);\n"
    code += "    }\n"
    # Check other base types defined in UNMARSHAL_FUNC_MAP
    first = True
    for ctype, funcname in UNMARSHAL_FUNC_MAP.items():
         # Skip types already handled (color, pointers implicitly, coord explicitly)
         if ctype == "lv_color_t" or "*" in ctype: continue
         code += f"    else if (strcmp(expected_c_type, \"{ctype}\") == 0) {{\n"
         code += f"        return {funcname}(json_value, ({ctype.replace('const ', '')}*)dest);\n"
         code += f"    }}\n"
         first = False # Not strictly needed as we use else if

    # --- Final Fallback / Error ---
    code += "    else {\n"
    code += "        // We might reach here if:\n"
    code += "        // - expected_c_type is an enum not caught by the heuristic string check above.\n"
    code += "        // - expected_c_type is a pointer not using '@' syntax.\n"
    code += "        // - expected_c_type is an unhandled struct/union/etc.\n"
    code += "        // Let's try one last time to parse as an enum if it looks like one.\n"
    code += "        bool maybe_enum = (strncmp(expected_c_type, \"lv_\", 3) == 0 && strstr(expected_c_type, \"_t\")) || \n"
    code += "                          (strncmp(expected_c_type, \"LV_\", 3) == 0 && strstr(expected_c_type, \"_T\"))  || strcmp(expected_c_type, \"uint32_t\") == 0 || strcmp(expected_c_type, \"int\") == 0 ;\n" # Added int
    code += "        if (maybe_enum && strcmp(expected_c_type, \"lv_obj_t\") != 0 /* add others */) {\n"
    code += "             // Ensure json_value is string before trying enum parse for non-numeric types\n"
    code += "             if (cJSON_IsString(json_value) || cJSON_IsNumber(json_value)) { // Allow numbers for enums too\n"
    code += "                 if (unmarshal_enum_value(json_value, expected_c_type, (int*)dest)) return true;\n"
    code += "             }\n"
    code += "        }\n\n"
    code += "        LOG_ERR_JSON(json_value, \"Unmarshal Error: Unhandled expected C type '%s' or invalid JSON value type (%d)\", expected_c_type, json_value->type);\n"
    code += "        return false;\n"
    code += "    }\n"
    code += "}\n\n"

    # Append helper C functions and the main C string unmarshaler
    code += generate_unmarshal_value_to_c_string_helpers()
    code += generate_unmarshal_value_to_c_string()

    return code