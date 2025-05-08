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
              return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"
         else:
              # Use the generic pointer unmarshaler which expects '@' prefix string
              # or direct call for nested results. Let unmarshal_value handle it.
              return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"

    # Handle enums explicitly if we can identify them (e.g. starts with LV_, or check against api_info['enums'])
    # This heuristic relies on type name, might need refinement
    if c_type.startswith("LV_") and c_type.endswith("_T"):
        return f"unmarshal_value({json_var_name}, \"{c_type}\", {dest_var_name})" # Let dispatcher handle enums too
    if c_type.startswith("lv_") and c_type.endswith("_t") and c_type not in UNMARSHAL_FUNC_MAP and c_type != "lv_coord_t":
         # Assume LVGL typedefs not explicitly mapped (and not coord) are enums
         return f"unmarshal_value({json_var_name}, \"{c_type}\", {dest_var_name})" # Let dispatcher handle enums too

    # Base types - use specific unmarshal func defined in UNMARSHAL_FUNC_MAP or dispatch via unmarshal_value
    func_name = UNMARSHAL_FUNC_MAP.get(c_type)
    if func_name:
        # Direct call to primitive unmarshaler (can be faster)
        # return f"{func_name}({json_var_name}, ({c_type_str}*){dest_var_name})"
        # OR consistently use the dispatcher for simpler logic:
        return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"
    elif c_type == "lv_coord_t":
         # lv_coord_t uses the dispatcher which calls unmarshal_coord
         return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"
    else:
        # Fallback for unknown types - use the dispatcher
        logger.warning(f"No specific unmarshal function found for C type '{c_type_str}'. Using generic dispatcher.")
        return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"


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
    code += "extern void* lvgl_json_get_registered_ptr(const char *name);\n\n"
    code += f"static bool unmarshal_registered_ptr(cJSON *node, void **dest) {{\n"
    code += f"    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '@') {{ LOG_ERR_JSON(node, \"Expected pointer string starting with @\"); return false; }}\n"
    code += f"    const char *name = node->valuestring + 1;\n"
    code += f"    *dest = lvgl_json_get_registered_ptr(name);\n"
    code += f"    if (!(*dest)) {{\n"
    code += f"       LOG_ERR_JSON(node, \"Pointer Unmarshal Error: Registered pointer '@%s' not found.\", name);\n"
    code += f"       return false;\n"
    code += f"    }}\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    return code

def generate_main_unmarshaler():
    """Generates the core unmarshal_value function."""
    code = "// --- Main Value Unmarshaler ---\n\n"
    code += "// Forward declarations for specific type unmarshalers\n"
    code += "static const invoke_table_entry_t* find_invoke_entry(const char *name);\n"
    code += "static bool unmarshal_enum_value(cJSON *json_value, const char *enum_type_name, int *dest);\n"
    code += "static bool unmarshal_color(cJSON *node, lv_color_t *dest);\n"
    code += "static bool unmarshal_coord(cJSON *node, lv_coord_t *dest);\n" # Added forward decl
    code += "static bool unmarshal_registered_ptr(cJSON *node, void **dest);\n"
    # Add forwards for all primitive unmarshalers generated
    code += "static bool unmarshal_int(cJSON *node, int *dest);\n"
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
    code += "static bool unmarshal_char(cJSON *node, char *dest);\n"

    code += "\n"
    code += "// The core dispatcher for unmarshaling any value from JSON based on expected C type.\n"
    code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest) {\n"
    code += "    if (!json_value || !expected_c_type || !dest) {\n"
    code += "        LOG_ERR(\"Unmarshal Error: NULL argument passed to unmarshal_value (%p, %s, %p)\", json_value, expected_c_type ? expected_c_type : \"NULL\", dest);\n"
    code += "        return false;\n"
    code += "     }\n\n"

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
    code += "            // Make the nested call. Result goes into 'dest'. target_obj_ptr is NULL.\n"
    code += "            if (!entry->invoke(entry, NULL, dest, args_item)) {\n"
    code += "                 LOG_ERR_JSON(json_value, \"Unmarshal Error: Nested call to '%s' failed.\", func_name);\n"
    code += "                 return false;\n"
    code += "            }\n"
    # TODO: Check if return type of nested call matches expected_c_type? Requires more info.
    code += "            return true; // Nested call successful\n"
    code += "        }\n"
    code += "        // If it's an object but not a 'call' object, it's an error unless expecting a specific struct type?\n"
    code += "        // Currently, we don't handle passing structs directly via JSON objects other than 'call'.\n"
    code += "    }\n\n"

    code += "    // 2. Handle custom pre- and post-fixes in strings ('#', '@', '%')\n"
    code += "    // Also handle non-prefixed strings that might be enums or regular strings.\n"
    code += "    if (cJSON_IsString(json_value) && json_value->valuestring) {\n"
    code += "        char *str_val = json_value->valuestring;\n"
    code += "        size_t len = strlen(str_val);\n"
    code += "        // Check for '#' color prefix\n"
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
    code += "               // Ensure expected type is some kind of pointer\n"
    code += "               if (strchr(expected_c_type, '*')) {\n"
    code += "                    return unmarshal_registered_ptr(json_value, (void**)dest);\n"
    code += "               } else {\n"
    code += "                   LOG_ERR_JSON(json_value, \"Unmarshal Error: Found registered pointer string '%s' but expected non-pointer type '%s'\", str_val, expected_c_type);\n"
    code += "                   //return false;\n"
    code += "               }\n"
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

    code += "        // Finally, try as a regular string pointer if expected type is `const char *`\n"
    code += "        if (strcmp(expected_c_type, \"const char *\") == 0 || strcmp(expected_c_type, \"char *\") == 0) {\n"
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
    code += "                          (strncmp(expected_c_type, \"LV_\", 3) == 0 && strstr(expected_c_type, \"_T\"))  || strcmp(expected_c_type, \"uint32_t\") == 0;\n"
    code += "        if (maybe_enum && strcmp(expected_c_type, \"lv_obj_t\") != 0 /* add others */) {\n"
    code += "             if (unmarshal_enum_value(json_value, expected_c_type, (int*)dest)) return true;\n"
    code += "        }\n\n"
    code += "        LOG_ERR_JSON(json_value, \"Unmarshal Error: Unhandled expected C type '%s' or invalid JSON value type (%d)\", expected_c_type, json_value->type);\n"
    code += "        return false;\n"
    code += "    }\n"
    code += "}\n\n"

    return code