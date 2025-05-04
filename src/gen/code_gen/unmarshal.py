# code_gen/unmarshal.py
import logging
from type_utils import get_c_type_str, is_lvgl_struct_ptr

logger = logging.getLogger(__name__)

# Map C types to the specific unmarshal function names we will generate
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
    "lv_coord_t": "unmarshal_int32", # Assuming lv_coord_t is int32_t
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
    # Enums are handled by unmarshal_enum_type
}

def get_unmarshal_call(c_type, pointer_level, is_array, json_var_name, dest_var_name):
    """Gets the C function call string for unmarshaling a specific type."""
    c_type_str = get_c_type_str(c_type, pointer_level)

    if pointer_level > 0 or is_array:
         # Most pointers are looked up via '@' prefix handled by unmarshal_value dispatcher
         # Special case: char* is treated as string
         if c_type == "char" and pointer_level == 1:
              func_name = UNMARSHAL_FUNC_MAP.get("const char *", "unmarshal_string_ptr")
              return f"{func_name}({json_var_name}, ({c_type_str}*){dest_var_name})"
         else:
              # Use the generic pointer unmarshaler which expects '@' prefix string
              # Or, if the JSON contains a direct address (less likely/safe), handle that.
              # The main dispatcher `unmarshal_value` should handle '@'
              # If we reach here for a pointer, it might be an error or need different handling.
              # Let's assume `unmarshal_value` handles it based on JSON type.
              # This specific function call generation might need rethinking.
              # Let's make `unmarshal_value` the primary entry point.
              # The invocation helpers will call `unmarshal_value` with the C type string.
              return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"

    # Handle enums - use type name
    if c_type.startswith("LV_") and c_type.endswith("_T"): # Heuristic for enum types by naming convention if not typedef'd
        return f"unmarshal_enum_value({json_var_name}, \"{c_type}\", (int*){dest_var_name})" # Cast dest to int* ? depends on enum base type
    if c_type.startswith("lv_") and c_type.endswith("_t") and c_type not in UNMARSHAL_FUNC_MAP:
        # Assume LVGL typedefs not explicitly mapped are enums
        # Need a way to confirm if a typedef is an enum. Check api_info['enums']
        # For now, assume they are enums and pass the type name
         return f"unmarshal_enum_value({json_var_name}, \"{c_type}\", (int*){dest_var_name})" # Assuming base type int

    # Base types
    func_name = UNMARSHAL_FUNC_MAP.get(c_type)
    if func_name:
        return f"{func_name}({json_var_name}, ({c_type_str}*){dest_var_name})"

    # Fallback if no specific function - use generic dispatcher again?
    logger.warning(f"No specific unmarshal function found for C type '{c_type_str}'. Using generic dispatcher.")
    return f"unmarshal_value({json_var_name}, \"{c_type_str}\", {dest_var_name})"


def generate_enum_unmarshalers(filtered_enums, enum_member_map):
    """Generates C code for parsing enum strings."""
    c_code = "// --- Enum Unmarshaling ---\n\n"

    # Option 1: Single function with strcmp chains per enum type
    # Requires knowing the type name at runtime.
    c_code += "static bool unmarshal_enum_value(cJSON *json_value, const char *enum_type_name, int *dest) {\n"
    c_code += "    if (!cJSON_IsString(json_value)) {\n"
    c_code += "       // Allow integer values directly for enums? Maybe.\n"
    c_code += "       if(cJSON_IsNumber(json_value)) { *dest = (int)json_value->valuedouble; return true; } \n"
    c_code += "       LOG_ERR(\"Enum Unmarshal Error: Expected string or number for %s, got %d\", enum_type_name, json_value->type);\n"
    c_code += "       return false;\n"
    c_code += "    }\n"
    c_code += "    const char *str_value = json_value->valuestring;\n"
    c_code += "    if (!str_value) { return false; } // Should not happen if IsString passed\n\n"

    first_type = True
    for enum_type in filtered_enums:
        type_name = enum_type.get('name') # e.g., lv_align_t
        if not type_name:
            # Try to infer from first member if no typedef name
            members = enum_type.get('members', [])
            if members and members[0].get('type'):
                 type_name = members[0]['type'].get('name')
            if not type_name:
                 logger.warning(f"Skipping enum without usable type name: {enum_type.get('members', [{}])[0].get('name', 'Unknown')}")
                 continue

        c_code += f"    {'else ' if not first_type else ''}if (strcmp(enum_type_name, \"{type_name}\") == 0) {{\n"
        first_member = True
        for member in enum_type.get('members', []):
            member_name = member.get('name')
            if member_name and member_name in enum_member_map:
                val = enum_member_map[member_name]
                c_code += f"        {'else ' if not first_member else ''}if (strcmp(str_value, \"{member_name}\") == 0) {{ *dest = {val}; return true; }}\n"
                first_member = False
        c_code += "        LOG_ERR(\"Enum Unmarshal Error: Unknown value '%s' for enum type %s\", str_value, enum_type_name);\n"
        c_code += "        return false;\n"
        c_code += "    }\n"
        first_type = False

    c_code += "    else {\n"
    c_code += "        LOG_ERR(\"Enum Unmarshal Error: Unknown enum type '%s'\", enum_type_name);\n"
    c_code += "        return false;\n"
    c_code += "    }\n"
    c_code += "}\n\n"

    # Also store enum names -> values map if needed elsewhere
    # `enum_member_map` already holds this.

    return c_code


def generate_primitive_unmarshalers():
    """Generates C unmarshalers for basic C types."""
    code = "// --- Primitive Unmarshalers ---\n\n"

    # Integers (handle potential overflow if json number > C type limits?)
    # C standard behavior for double-to-int cast handles truncation.
    int_types = [
        ("int", "int"), ("int8", "int8_t"), ("uint8", "uint8_t"),
        ("int16", "int16_t"), ("uint16", "uint16_t"),
        ("int32", "int32_t"), ("uint32", "uint32_t"),
        ("int64", "int64_t"), ("uint64", "uint64_t"),
        ("size_t", "size_t") # Assuming size_t fits in double for json value
    ]
    for name, ctype in int_types:
        code += f"static bool unmarshal_{name}(cJSON *node, {ctype} *dest) {{\n"
        code += f"    if (!cJSON_IsNumber(node)) return false;\n"
        # Add range checks here if necessary
        code += f"    *dest = ({ctype})node->valuedouble;\n"
        code += f"    return true;\n"
        code += f"}}\n\n"

    # Floats
    float_types = [("float", "float"), ("double", "double")]
    for name, ctype in float_types:
        code += f"static bool unmarshal_{name}(cJSON *node, {ctype} *dest) {{\n"
        code += f"    if (!cJSON_IsNumber(node)) return false;\n"
        code += f"    *dest = ({ctype})node->valuedouble;\n"
        code += f"    return true;\n"
        code += f"}}\n\n"

    # Bool
    code += f"static bool unmarshal_bool(cJSON *node, bool *dest) {{\n"
    code += f"    if (!cJSON_IsBool(node)) return false;\n"
    code += f"    *dest = cJSON_IsTrue(node);\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    # String Pointer (const char *)
    code += f"static bool unmarshal_string_ptr(cJSON *node, char **dest) {{\n"
    code += f"    if (!cJSON_IsString(node)) return false;\n"
    code += f"    *dest = node->valuestring;\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    # Char (from string[0]?) - This is ambiguous
    code += f"static bool unmarshal_char(cJSON *node, char *dest) {{\n"
    code += f"    if (cJSON_IsString(node) && node->valuestring && node->valuestring[0] != '\\0') {{ *dest = node->valuestring[0]; return true; }}\n"
    # Allow single char from number?
    code += f"    if (cJSON_IsNumber(node)) {{ *dest = (char)node->valuedouble; return true; }}\n"
    code += f"    return false;\n"
    code += f"}}\n\n"

    return code

def generate_custom_unmarshalers(api_info):
    """Generates unmarshalers for '#' color and '@' pointer syntaxes."""
    code = "// --- Custom Unmarshalers ---\n\n"

    # Color (#RRGGBB or #AARRGGBB)
    # Check if lv_color_hex is available
    lv_color_hex_available = any(f['name'] == 'lv_color_hex' for f in api_info['functions'])

    code += f"static bool unmarshal_color(cJSON *node, lv_color_t *dest) {{\n"
    code += f"    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '#') return false;\n"
    code += f"    const char *hex_str = node->valuestring + 1;\n"
    code += f"    uint32_t hex_val = (uint32_t)strtoul(hex_str, NULL, 16);\n"
    if lv_color_hex_available:
         # Check length? lv_color_hex expects RRGGBB.
         code += f"    if (strlen(hex_str) == 6) {{ *dest = lv_color_hex(hex_val); return true; }}\n"
         # Handle alpha? #AARRGGBB? LVGL doesn't have direct lv_color_hexA.
         # Fallback or ignore alpha for now.
         code += f"    // Add support for other hex lengths? (e.g. #RGB, #AARRGGBB)\n"
    else:
         # Manual conversion if lv_color_hex not included (assuming 32bit color depth for simplicity)
         code += f"    // Assuming 32-bit color format (ARGB8888)\n"
         code += f"    if (strlen(hex_str) == 8) {{ // AARRGGBB\n"
         code += f"       // lv_color_t might not have alpha. Extract RGB.\n"
         code += f"       dest->red = (hex_val >> 16) & 0xFF;\n"
         code += f"       dest->green = (hex_val >> 8) & 0xFF;\n"
         code += f"       dest->blue = hex_val & 0xFF;\n"
         code += f"       // Alpha ((hex_val >> 24) & 0xFF) is ignored if lv_color_t has no alpha field\n"
         code += f"       return true;\n"
         code += f"    }} else if (strlen(hex_str) == 6) {{ // RRGGBB\n"
         code += f"       dest->red = (hex_val >> 16) & 0xFF;\n"
         code += f"       dest->green = (hex_val >> 8) & 0xFF;\n"
         code += f"       dest->blue = hex_val & 0xFF;\n"
         code += f"       // Assuming full alpha if lv_color_t has alpha field\n"
         code += f"       return true;\n"
         code += f"    }}\n"

    code += f"    LOG_ERR(\"Color Unmarshal Error: Invalid hex format '%s'\", node->valuestring);\n"
    code += f"    return false;\n"
    code += f"}}\n\n"

    # Registered Pointer (@name)
    code += "// Forward declaration\n"
    code += "extern void* lvgl_json_get_registered_ptr(const char *name);\n\n"
    code += f"static bool unmarshal_registered_ptr(cJSON *node, void **dest) {{\n"
    code += f"    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '@') return false;\n"
    code += f"    const char *name = node->valuestring + 1;\n"
    code += f"    *dest = lvgl_json_get_registered_ptr(name);\n"
    code += f"    if (!(*dest)) {{\n"
    code += f"       LOG_ERR(\"Pointer Unmarshal Error: Registered pointer '@%s' not found.\", name);\n"
    code += f"       return false;\n"
    code += f"    }}\n"
    code += f"    return true;\n"
    code += f"}}\n\n"

    return code

def generate_main_unmarshaler():
    """Generates the core unmarshal_value function."""
    code = "// --- Main Value Unmarshaler ---\n\n"
    code += "// Forward declarations\n"
    code += "static const invoke_table_entry_t* find_invoke_entry(const char *name);\n"
    code += "static bool unmarshal_enum_value(cJSON *json_value, const char *enum_type_name, int *dest);\n"
    code += "static bool unmarshal_color(cJSON *node, lv_color_t *dest);\n"
    code += "static bool unmarshal_registered_ptr(cJSON *node, void **dest);\n"
    # Add forwards for all primitive unmarshalers generated
    code += "static bool unmarshal_int(cJSON *node, int *dest);\n"
    code += "static bool unmarshal_int8(cJSON *node, int8_t *dest);\n"
    # ... etc for all primitives ...
    code += "static bool unmarshal_float(cJSON *node, float *dest);\n"
    code += "static bool unmarshal_double(cJSON *node, double *dest);\n"
    code += "static bool unmarshal_bool(cJSON *node, bool *dest);\n"
    code += "static bool unmarshal_string_ptr(cJSON *node, char **dest);\n"
    code += "static bool unmarshal_char(cJSON *node, char *dest);\n"
    code += "static bool unmarshal_size_t(cJSON *node, size_t *dest);\n"
    code += "static bool unmarshal_int32(cJSON *node, int32_t *dest);\n"
    code += "static bool unmarshal_uint8(cJSON *node, uint8_t *dest);\n"
    # ... add others as needed ...

    code += "\n"
    code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest) {\n"
    code += "    if (!json_value || !expected_c_type || !dest) return false;\n\n"

    code += "    // 1. Handle nested function calls { \"call\": \"func_name\", \"args\": [...] }\n"
    code += "    if (cJSON_IsObject(json_value)) {\n"
    code += "        cJSON *call_item = cJSON_GetObjectItemCaseSensitive(json_value, \"call\");\n"
    code += "        cJSON *args_item = cJSON_GetObjectItemCaseSensitive(json_value, \"args\");\n"
    code += "        if (call_item && cJSON_IsString(call_item) && args_item && cJSON_IsArray(args_item)) {\n"
    code += "            const char *func_name = call_item->valuestring;\n"
    code += "            const invoke_table_entry_t* entry = find_invoke_entry(func_name);\n"
    code += "            if (!entry) {\n"
    # _code += "                LOG_ERR(\"Unmarshal Error: Nested call function '%s' not found in invoke table.\", func_name);\n"
    code += "                LOG_ERR_JSON(json_value, \"Unmarshal Error: Nested call function '%s' not found in invoke table.\", func_name);\n"
    code += "                return false;\n"
    code += "            }\n"
    code += "            // Invoke the nested function, result goes into 'dest'\n"
    code += "            // Note: target_obj_ptr is NULL for nested calls (unless context implies otherwise)\n"
    code += "            if (!entry->invoke(NULL, dest, args_item, entry->func_ptr)) {\n"
    # code += "                 LOG_ERR(\"Unmarshal Error: Nested call to '%s' failed.\", func_name);\n"
    code += "                 LOG_ERR_JSON(json_value, \"Unmarshal Error: Nested call to '%s' failed.\", func_name);\n"
    code += "                 return false;\n"
    code += "            }\n"
    code += "            // TODO: Check if the return type of the nested call matches expected_c_type?\n"
    code += "            // This requires getting return type info from the invoke_table_entry or function signature.\n"
    code += "            return true; // Nested call successful\n"
    code += "        }\n"
    code += "    }\n\n"

    code += "    // 2. Handle custom prefixes in strings\n"
    code += "    if (cJSON_IsString(json_value) && json_value->valuestring) {\n"
    code += "        const char *str_val = json_value->valuestring;\n"
    code += "        if (str_val[0] == '#') {\n"
    # Make sure expected type is compatible with lv_color_t
    code += "           if (strcmp(expected_c_type, \"lv_color_t\") == 0) {\n"
    code += "               return unmarshal_color(json_value, (lv_color_t*)dest);\n"
    code += "           } else { /* Fall through if type mismatch */ }\n"
    code += "        }\n"
    code += "        if (str_val[0] == '@') {\n"
    # Make sure expected type is a pointer
    code += "           if (strchr(expected_c_type, '*')) {\n" # Simple check for pointer type
    code += "                return unmarshal_registered_ptr(json_value, (void**)dest);\n"
    code += "           } else { /* Fall through if type mismatch */ }\n"
    code += "        }\n"
    code += "        // Fall through to check if it's an enum name or regular string\n"
    code += "    }\n\n"

    code += "    // 3. Dispatch based on expected C type\n"
    # Generate if/else if chain for all known types
    code += "    // Using strcmp for type matching. Could be optimized (e.g., hash map if many types).\n"
    first = True
    for ctype, funcname in UNMARSHAL_FUNC_MAP.items():
        code += f"    {'else ' if not first else ''}if (strcmp(expected_c_type, \"{ctype}\") == 0) {{\n"
        code += f"        return {funcname}(json_value, ({ctype.replace('const ', '')}*)dest);\n" # Pass non-const pointer to dest
        code += f"    }}\n"
        first = False

    # Add catch-all for LVGL enums/types not explicitly mapped above
    code += "    else if (strncmp(expected_c_type, \"lv_\", 3) == 0 && strstr(expected_c_type, \"_t\")) {\n"
    # Assume it's an enum if it's not a known struct ptr etc. This is heuristic.
    code += "       // Assuming an enum type based on naming convention\n"
    code += "       return unmarshal_enum_value(json_value, expected_c_type, (int*)dest);\n"
    code += "    }\n"
    # Add specific checks for known struct pointers if needed (e.g. lv_obj_t *)
    # Usually handled by '@' prefix, but could be passed differently.
    # code += " else if (strcmp(expected_c_type, \"lv_obj_t *\") == 0) { ... } \n"


    code += "    else {\n"
    code += "        // Type not handled or is a generic pointer expected via '@' (which failed above)\n"
    # code += "        LOG_ERR(\"Unmarshal Error: Unsupported expected C type '%s' or invalid value format.\", expected_c_type);\n"
    code += "        LOG_ERR_JSON(json_value, \"Unmarshal Error: Unsupported expected C type '%s' or invalid value format.\", expected_c_type);\n"
    code += "        // Attempt string unmarshal as a last resort? Only if pointer expected.\n"
    code += "        if (strchr(expected_c_type, '*') && cJSON_IsString(json_value)) {\n"
    code += "           LOG_WARN(\"Attempting basic string unmarshal for unexpected type %s\", expected_c_type);\n"
    code += "           return unmarshal_string_ptr(json_value, (char **)dest);\n"
    code += "        }\n"
    code += "        return false;\n"
    code += "    }\n"
    code += "}\n\n"

    return code
