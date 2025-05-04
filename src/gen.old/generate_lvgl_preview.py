# general_lvgl_preview.py

import json
import argparse
import os
import re
import textwrap
from collections import OrderedDict, defaultdict
import errno # For integer parsing error check (used in generated C)

# --- Constants ---
MAX_ARGS_SUPPORTED = 10 # Max args for generated C data structures and stack buffers

# --- C Type Representation ---
C_TYPE_SIZES = {
    'char': 1, 'signed char': 1, 'unsigned char': 1, 'short': 2, 'unsigned short': 2,
    'int': 4, 'unsigned int': 4, 'long': 4, 'unsigned long': 4, 'long long': 8, 'unsigned long long': 8,
    'float': 4, 'double': 8, 'bool': 1, '_Bool': 1,
    'int8_t': 1, 'uint8_t': 1, 'int16_t': 2, 'uint16_t': 2, 'int32_t': 4, 'uint32_t': 4,
    'int64_t': 8, 'uint64_t': 8, 'size_t': 4, 'uintptr_t': 4, 'intptr_t': 4, 'void': 0,
    'lv_opa_t': 1, 'lv_style_prop_t': 1, 'lv_state_t': 2, 'lv_part_t': 4,
    'lv_coord_t': 4, 'lv_res_t': 1, 'lv_log_level_t': 1,
}

def get_struct_size(struct_name, structs_map):
    """Estimate struct size (basic, ignores padding/alignment)."""
    struct_info = structs_map.get(struct_name)
    if not struct_info:
        return C_TYPE_SIZES.get(struct_name, 4) # Default guess if not found

    # Check cache or placeholder to prevent infinite recursion
    if C_TYPE_SIZES.get(struct_name, -1) != -1:
        return C_TYPE_SIZES[struct_name]

    C_TYPE_SIZES[struct_name] = 4 # Placeholder for recursion
    estimated_size = 0
    for field in struct_info.get('fields', []):
        field_type_info = field.get('type')
        base_name, ptr_level, _, _, _ = get_type_name(field_type_info, structs_map)
        bitsize = field.get('bitsize')

        if bitsize:
            # Crude approximation for bitfields
            estimated_size += (int(bitsize) + 7) // 8
        elif ptr_level > 0:
            estimated_size += C_TYPE_SIZES.get('uintptr_t', 4) # Pointer size
        else:
            estimated_size += get_type_size(base_name, structs_map) # Recursive call

    # Ensure non-zero size if fields exist but calculated 0
    final_size = max(estimated_size, 1) if struct_info.get('fields') else 4
    C_TYPE_SIZES[struct_name] = final_size # Update cache
    return final_size

def get_type_size(base_type_name, structs_map):
    """Estimate type size based on name."""
    if base_type_name in C_TYPE_SIZES:
        return C_TYPE_SIZES[base_type_name]
    # Check for LVGL types (structs, enums, etc.)
    if base_type_name.endswith(('_t', '_cb_t')) or base_type_name.startswith(('lv_', '_lv_')):
        if structs_map and base_type_name in structs_map:
            return get_struct_size(base_type_name, structs_map)
        # Assume others (like enums) are int-sized unless known
        return C_TYPE_SIZES.get(base_type_name, 4)
    # Default guess for unknown types
    return C_TYPE_SIZES.get(base_type_name, 4)

def sanitize_c_identifier(name):
    """Makes a name safe for use as a C identifier."""
    if not isinstance(name, str):
        name = 'unknown'
    # Replace common problematic characters/sequences
    name = name.replace('*', '_ptr')
    name = name.replace(' const', '_const')
    name = name.replace('const ', 'const_')
    name = re.sub(r'\s+', '_', name)
    name = name.replace('...', 'ellipsis')
    # Replace any remaining non-alphanumeric characters (excluding underscore)
    name = re.sub(r'[^a-zA-Z0_9_]', '_', name)
    name = name.strip('_')
    if not name:
        return "unknown"
    # Ensure starts with letter or underscore
    if not name[0].isalpha() and name[0] != '_':
        name = '_' + name
    # Avoid C keywords
    keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
        "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
        "_Static_assert", "_Thread_local"
    }
    if name in keywords:
        name = f"_{name}_"
    return name

def get_type_name(type_info, structs_map=None):
    """Extracts base type name, pointer level, const status, C type string, and size."""
    if not type_info:
        return "void", 0, False, "void", 0

    pointer_level = 0
    is_const = False
    quals = []
    current_type = type_info
    base_name = 'unknown'
    size = 4 # Default guess

    # Handle arrays (treated like pointers in C args)
    if current_type.get('json_type') == 'array':
        pointer_level = 1
        quals = current_type.get('quals', [])
        is_const = 'const' in quals
        element_type_info = current_type.get('type')
        if element_type_info and isinstance(element_type_info, dict):
            # Recursively get element type info
            base_name, _, element_is_const, _, _ = get_type_name(element_type_info, structs_map)
            is_const = is_const or element_is_const # Pointer to const if element is const
        elif isinstance(element_type_info, dict) and 'name' in element_type_info:
             # Handle simple { "name": "type" } case
             base_name = element_type_info['name']
        else:
             # Fallback if type info is missing or just a string name
             base_name = element_type_info if isinstance(element_type_info, str) else current_type.get('name', 'unknown_array_element')
        size = C_TYPE_SIZES.get('uintptr_t', 4) # Array decays to pointer, use pointer size
    else:
        # Handle pointers
        while current_type.get('json_type') == 'pointer':
            pointer_level += 1
            quals.extend(current_type.get('quals', []))
            current_type = current_type.get('type')
            if not current_type: # Handle void*
                base_name = "void"
                break

        if base_name != "void": # If not void*
            quals.extend(current_type.get('quals', []))
            base_name = current_type.get('name')
            if not base_name: # Check nested type field if name is missing
                nested_type_info = current_type.get('type')
                if isinstance(nested_type_info, dict):
                    base_name = nested_type_info.get('name', 'unknown_nested')
                else:
                    base_name = 'unknown_base'

        is_const = 'const' in quals
        # Handle special types like ellipsis (...)
        if current_type.get('json_type') == 'special_type' and base_name == 'ellipsis':
             base_name = '...'
             pointer_level = 0
             is_const = False

        # Calculate size
        if pointer_level > 0:
            size = C_TYPE_SIZES.get('uintptr_t', 4)
        elif base_name != '...':
            size = get_type_size(base_name, structs_map)

    # Construct the full C type string
    c_name_part = base_name if base_name != '...' else '...'
    if is_const and pointer_level == 0: # const value
        full_c_name = f"const {c_name_part}"
    elif is_const and pointer_level > 0: # pointer to const
        full_c_name = f"const {c_name_part}*" + "*" * (pointer_level - 1)
    else: # non-const value or pointer to non-const
        full_c_name = c_name_part + "*" * pointer_level

    # Handle void and ellipsis specifically
    if base_name == 'void' and pointer_level == 0:
        full_c_name = "void"
    elif base_name == '...':
        full_c_name = "..."

    # Ensure base_name is a string for consistency
    if not isinstance(base_name, str):
        base_name = str(base_name)

    return base_name, pointer_level, is_const, full_c_name, size

# --- Code Generation Helpers ---

def generate_header(functions, types_details, enums, typedefs, structs):
    """Generates the lvgl_ui_preview.h content using cJSON."""

    # Header content using textwrap.dedent for clean formatting
    header_content = textwrap.dedent(f"""\
        #ifndef LVGL_UI_PREVIEW_H
        #define LVGL_UI_PREVIEW_H

        // !! THIS FILE IS AUTO-GENERATED by general_lvgl_preview.py !!
        // !! DO NOT EDIT MANUALLY !!

        #ifdef __cplusplus
        extern "C" {{
        #endif

        #include "lvgl.h"
        #include "cJSON.h"      // Requires linking cJSON library
        #include <stdbool.h>
        #include <stdint.h>
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <errno.h>

        // --- Logging Configuration ---
        // Define LVGL_UI_PREVIEW_LOG_LEVEL before including this header to override,
        // otherwise it defaults based on LV_USE_LOG.
        #if LV_USE_LOG && !defined(LVGL_UI_PREVIEW_LOG_LEVEL)
            #define LVGL_UI_PREVIEW_LOG_LEVEL LV_LOG_LEVEL_WARN // Default level if LVGL logging is on
        #endif
        #ifndef LVGL_UI_PREVIEW_LOG_LEVEL
            #define LVGL_UI_PREVIEW_LOG_LEVEL LV_LOG_LEVEL_NONE // Disable if LVGL logging is off or level not set
        #endif

        // --- Logging Macros ---
        #if LVGL_UI_PREVIEW_LOG_LEVEL <= LV_LOG_LEVEL_TRACE
            #define LUP_LOG_TRACE(...) LV_LOG_TRACE(__VA_ARGS__)
        #else
            #define LUP_LOG_TRACE(...) do{{}}while(0)
        #endif
        #if LVGL_UI_PREVIEW_LOG_LEVEL <= LV_LOG_LEVEL_INFO
            #define LUP_LOG_INFO(...) LV_LOG_INFO(__VA_ARGS__)
        #else
            #define LUP_LOG_INFO(...) do{{}}while(0)
        #endif
        #if LVGL_UI_PREVIEW_LOG_LEVEL <= LV_LOG_LEVEL_WARN
            #define LUP_LOG_WARN(...) LV_LOG_WARN(__VA_ARGS__)
        #else
            #define LUP_LOG_WARN(...) do{{}}while(0)
        #endif
        #if LVGL_UI_PREVIEW_LOG_LEVEL <= LV_LOG_LEVEL_ERROR
            #define LUP_LOG_ERROR(...) LV_LOG_ERROR(__VA_ARGS__)
        #else
            // Fallback basic error logging if LVGL logging is disabled or level is too high
            #define LUP_LOG_ERROR(...) do {{ \\
                fprintf(stderr, "ERROR [LUP]: "); \\
                fprintf(stderr, __VA_ARGS__); \\
                fprintf(stderr, "\\n"); \\
            }} while(0)
        #endif

        // --- ID Registry API ---
        // Registers a pointer (e.g., an lv_obj_t* or lv_style_t*) with a string ID.
        // Replaces existing entry with the same ID.
        void lvgl_ui_preview_register(const char *id, void *ptr);

        // Retrieves a previously registered pointer by ID. Returns NULL if not found.
        void* lvgl_ui_preview_get(const char *id);

        // Clears the entire ID registry (frees ID strings, does not free the pointers).
        void lvgl_ui_preview_clear_registry(void);

        // --- Custom Unmarshaler API ---
        // Callback function type for custom string-based unmarshalers.
        // input_str: The string value from JSON, *after* the registered prefix.
        // dest_ptr: Pointer to the memory location to store the unmarshaled value.
        // Returns true on success, false on failure.
        typedef bool (*lvgl_ui_unmarshaler_func_t)(const char *input_str, void *dest_ptr);

        // Registers a custom unmarshaler function associated with a string prefix.
        // Example: register("img:", handle_image_source);
        void lvgl_ui_preview_register_unmarshaler(const char *prefix, lvgl_ui_unmarshaler_func_t func);

        // --- Core Rendering API ---
        // Renders a UI defined by a single cJSON object node.
        // Recursively processes children defined in a "children" array within the node.
        // node: The cJSON object representing the UI element.
        // parent: The parent lv_obj_t to render onto. If NULL, uses lv_screen_active().
        // Returns the created lv_obj_t* if the node represented an lv_obj_t, otherwise NULL
        // (e.g., if it was a style definition). Logs errors on failure.
        lv_obj_t* lvgl_ui_preview_render_json_node(cJSON *node, lv_obj_t *parent);

        // --- JSON Loading Helpers ---
        // Parse a JSON string containing an array of UI element objects at the root.
        // Calls lvgl_ui_preview_render_json_node for each element in the root array.
        // json_string: The JSON string to parse.
        // parent_screen: The parent object (usually a screen) for top-level elements.
        // Returns the number of top-level elements successfully processed, or -1 on error.
        int lvgl_ui_preview_process_json_str(const char *json_string, lv_obj_t *parent_screen);

        // Reads a JSON file, parses it, and renders the UI elements defined in the root array.
        // json_filepath: Path to the JSON file.
        // parent_screen: The parent object for top-level elements.
        // Returns the number of top-level elements successfully processed, or -1 on file/parse error.
        int lvgl_ui_preview_process_json_file(const char *json_filepath, lv_obj_t *parent_screen);

        // --- Initialization ---
        // Initializes the UI preview system (e.g., registers default "@" handler).
        // Should be called once before using other functions.
        void lvgl_ui_preview_init(void);

        #ifdef __cplusplus
        }} /* extern "C" */
        #endif

        #endif /* LVGL_UI_PREVIEW_H */
        """)
    return header_content

def generate_c_api_data_code(types_details, functions, struct_map):
    """Generates C code for static API lookup tables."""
    code = "// --- Generated API Data (for internal lookup) ---\n"
    code += f"#define MAX_ARGS_SUPPORTED {MAX_ARGS_SUPPORTED}\n\n"

    # Type Details Table
    code += "typedef struct {\n"
    code += "    const char* base_name;\n"
    code += "    const char* c_type_str; // Base C type name\n"
    code += "    const char* safe_name;  // Sanitized name for unmarshaler lookup\n"
    code += "    size_t size;           // Estimated size\n"
    code += "    bool is_enum;\n"
    code += "    bool is_struct;\n"
    code += "} lup_type_detail_t;\n\n"
    code += "static const lup_type_detail_t lup_type_details[] = {\n"
    type_detail_lookup = {}
    type_idx = 0
    for base_name, details in sorted(types_details.items()):
        type_detail_lookup[base_name] = type_idx
        code += (
            f'    {{"{base_name}", "{details["c_type"]}", "{details["safe_name"]}", '
            f'{details["size"]}, {"true" if details["is_enum"] else "false"}, '
            f'{"true" if details["is_struct"] else "false"} }},\n'
        )
        type_idx += 1
    code += "    {NULL, NULL, NULL, 0, false, false} // Sentinel\n"
    code += "};\n"
    code += f"#define LUP_TYPE_DETAIL_COUNT {type_idx}\n\n"
    code += textwrap.dedent("""\
        // Helper to find type details by base name
        static const lup_type_detail_t* find_type_detail(const char* base_name) {
            if (!base_name) return NULL;
            // TODO: Replace linear scan with hash map or binary search for large APIs
            for (int i = 0; i < LUP_TYPE_DETAIL_COUNT; ++i) {
                // Make sure base_name is not NULL before comparing
                if (lup_type_details[i].base_name && strcmp(lup_type_details[i].base_name, base_name) == 0) {
                    return &lup_type_details[i];
                }
            }
            return NULL;
        }

    """)

    # Function Details Table
    code += "typedef struct {\n"
    code += "    const char* base_name;      // Arg type base name\n"
    code += "    int type_detail_index; // Index into lup_type_details (-1 if not found)\n"
    code += "    uint8_t pointer_level;\n"
    code += "} lup_arg_detail_t;\n\n"

    code += "typedef struct {\n"
    code += "    const char* func_name;\n"
    code += "    int return_type_detail_index; // Index for return type base name\n"
    code += "    uint8_t return_pointer_level;\n"
    code += "    uint8_t arg_count;\n"
    code += "    bool is_varargs;\n"
    code += f"    lup_arg_detail_t args[{MAX_ARGS_SUPPORTED}];\n" # Fixed size array
    code += "} lup_func_detail_t;\n\n"

    code += "static const lup_func_detail_t lup_func_details[] = {\n"
    func_detail_lookup = {}
    func_idx = 0
    for func in functions: # Use filtered functions
        func_name = func['name']
        func_detail_lookup[func_name] = func_idx

        ret_type_info = func.get('type', {}).get('type', {})
        base_ret_name, ret_ptr, _, _, _ = get_type_name(ret_type_info, struct_map)
        ret_type_idx = type_detail_lookup.get(base_ret_name, -1)

        args = func.get('args', [])
        c_args_entries = []
        arg_count = 0
        is_varargs = False

        # Handle void argument case
        temp_args = args
        if len(args) == 1:
            arg0_type_info = args[0].get('type', {})
            arg0_base, arg0_ptr, _, _, _ = get_type_name(arg0_type_info, struct_map)
            if arg0_base == 'void' and arg0_ptr == 0:
                temp_args = [] # Treat as no arguments

        for i, arg in enumerate(temp_args):
            if i >= MAX_ARGS_SUPPORTED:
                print(f"Warning: Function {func_name} has more than {MAX_ARGS_SUPPORTED} args, truncating in generated table.")
                break
            arg_type_info = arg.get('type', {})
            base_arg_name, arg_ptr_level, _, _, _ = get_type_name(arg_type_info, struct_map)
            arg_type_idx = type_detail_lookup.get(base_arg_name, -1)
            # Format each argument detail struct initializer
            c_args_entries.append(f'{{"{base_arg_name}", {arg_type_idx}, {arg_ptr_level}}}')
            arg_count += 1
            if base_arg_name == '...':
                is_varargs = True
                break # Stop processing after ellipsis

        # Format the args array initializer string
        args_c_code = ", ".join(c_args_entries)
        # Pad remaining args with null initializer if needed
        if arg_count < MAX_ARGS_SUPPORTED:
            if arg_count > 0:
                 args_c_code += ", " # Add comma if needed
            padding_arg = "{NULL, -1, 0}"
            args_c_code += ", ".join([padding_arg] * (MAX_ARGS_SUPPORTED - arg_count))

        # Add the function entry to the main table
        # Use textwrap for better formatting of long lines
        entry = textwrap.dedent(f"""\
            {{ // {func_name}
                /* .func_name = */ "{func_name}",
                /* .return_type_detail_index = */ {ret_type_idx},
                /* .return_pointer_level = */ {ret_ptr},
                /* .arg_count = */ {arg_count},
                /* .is_varargs = */ {"true" if is_varargs else "false"},
                /* .args = */ {{{args_c_code}}}
            }},""")
        code += textwrap.indent(entry, '    ') + '\n'
        func_idx += 1

    code += "    {NULL, -1, 0, 0, false, {{NULL, -1, 0}}} // Sentinel\n"
    code += "};\n"
    code += f"#define LUP_FUNC_DETAIL_COUNT {func_idx}\n\n"
    code += textwrap.dedent("""\
        // Helper to find function details by name
        static const lup_func_detail_t* find_func_detail(const char* func_name) {
            if (!func_name) return NULL;
            // TODO: Replace linear scan with hash map or binary search for large APIs
            for (int i = 0; i < LUP_FUNC_DETAIL_COUNT; ++i) {
                // Make sure func_name in table is not NULL before comparing
                if (lup_func_details[i].func_name && strcmp(lup_func_details[i].func_name, func_name) == 0) {
                    return &lup_func_details[i];
                }
            }
            return NULL;
        }

    """)
    return code

def generate_unmarshalers(types_details, enums, enum_map):
    """Generates C code for the static unmarshaler functions."""
    unmarshaler_funcs = []
    processed_unmarshalers = set()
    unmarshaler_call_switch_cases = [] # For the main dispatch switch

    # Default handler
    unmarshaler_funcs.append(textwrap.dedent("""\
        // Default unmarshaler for unknown types
        static bool unmarshal_unknown(cJSON* node, void* dest, const char* type_name) {
            LUP_LOG_ERROR("Cannot unmarshal unknown or unsupported type '%s' from JSON type %d",
                          type_name ? type_name : "NULL", node ? node->type : -1);
            return false;
        }
    """))

    # Generate specific unmarshalers
    for base_type_name, details in sorted(types_details.items()):
        # Skip types that don't need direct unmarshaling from JSON values
        if base_type_name in ['void', '...'] or details['is_pointer_base']:
            continue

        unmarshaler_name = f"unmarshal_{details['safe_name']}"
        if unmarshaler_name in processed_unmarshalers:
            continue
        processed_unmarshalers.add(unmarshaler_name)

        c_type = details['c_type'] # Base C type name
        func_body = ""

        # --- Generate specific unmarshaler logic using cJSON ---
        if details['is_string_ptr']:
            # Handles char*, assuming const char* is acceptable
            func_body = textwrap.dedent(f"""\
                if (!cJSON_IsString(node)) {{
                    LUP_LOG_ERROR("Expected JSON string for {c_type}*");
                    return false;
                }}
                // NOTE: Returns pointer into cJSON obj memory. Caller must not modify.
                // If LVGL function needs non-const, this requires a copy.
                *(const char**)dest = cJSON_GetStringValue(node);
                return true; // Allow empty strings
            """)
        elif base_type_name == "bool":
            func_body = textwrap.dedent(f"""\
                bool val = false;
                if (cJSON_IsTrue(node)) {{
                    val = true;
                }} else if (cJSON_IsFalse(node)) {{
                    val = false;
                }} else if (cJSON_IsString(node)) {{
                    // Allow string versions like "true", "1", "yes" (case-insensitive?)
                    const char* str = cJSON_GetStringValue(node);
                    // Simple case-sensitive check for now
                    if (str && (strcmp(str, "true")==0 || strcmp(str, "1")==0 || strcmp(str, "yes")==0)) {{
                        val = true;
                    }} else if (str && (strcmp(str, "false")==0 || strcmp(str, "0")==0 || strcmp(str, "no")==0)) {{
                         val = false;
                    }} else {{
                         LUP_LOG_ERROR("Invalid string for bool: '%s'", str ? str : "NULL");
                         return false; // Unrecognized string
                    }}
                }} else {{
                    // Also allow integer 1 or 0?
                    if (cJSON_IsNumber(node)) {{
                        double num_val = cJSON_GetNumberValue(node);
                        if (num_val == 1.0) val = true;
                        else if (num_val == 0.0) val = false;
                        else {{
                             LUP_LOG_ERROR("Expected JSON boolean (true/false), string, or number 0/1 for bool type, got number %g", num_val);
                             return false;
                        }}
                    }} else {{
                         LUP_LOG_ERROR("Expected JSON boolean (true/false), string, or number 0/1 for bool type, got type %d", node ? node->type : -1);
                         return false;
                    }}
                }}
                 *(bool*)dest = val;
                 return true;
            """)
        elif details['is_enum']:
            # Handle enums from string name or number
            enum_checks = ""
            # Find all possible C names for members of this enum type
            matching_enums = []
            for enum_data in enums:
                 type_match = False
                 if enum_data.get('name') == base_type_name: type_match = True
                 elif not enum_data.get('name') and enum_data.get('members') and \
                      enum_data['members'][0].get('type',{}).get('name') == base_type_name: type_match = True # Anon enum heuristic
                 if type_match:
                     for member in enum_data.get('members', []):
                         if member.get('name'): matching_enums.append(member['name'])

            if matching_enums:
                 # Create 'else if' chain for string comparison
                 for val_name in sorted(list(set(matching_enums))):
                    c_enum_val = enum_map.get(val_name, val_name) # Get C identifier
                    enum_checks += f' else if (strcmp(scalar, "{val_name}") == 0) {{ *(({c_type}*)dest) = {c_enum_val}; ok = true; }}'
                 if enum_checks.startswith(" else "): enum_checks = enum_checks[6:] # Remove leading ' else '
                 enum_checks += f' else {{ LUP_LOG_TRACE("Enum value \'%s\' not matched by name for type {base_type_name}", scalar); }}'
            else:
                 enum_checks = f' LUP_LOG_WARN("No registered enum members found for type {base_type_name}");'

            func_body = textwrap.dedent(f"""\
                bool ok = false;
                if (cJSON_IsString(node)) {{
                    const char* scalar = cJSON_GetStringValue(node);
                    if (scalar) {{ // Check if string value exists
                        if (0) {{}} // Dummy for else-if chain
                        {textwrap.indent(enum_checks, "                        ")}
                    }}
                }}

                // Allow numeric value as fallback if string didn't match
                if (!ok && cJSON_IsNumber(node)) {{
                    double num_val = cJSON_GetNumberValue(node);
                    // TODO: Add range check for the specific enum type if possible?
                    *(({c_type}*)dest) = ({c_type})num_val;
                    ok = true;
                    LUP_LOG_TRACE("Interpreted enum for {base_type_name} as number %g", num_val);
                }}

                if (!ok) {{
                    LUP_LOG_ERROR("Invalid JSON value for enum {base_type_name}");
                }}
                return ok;
            """)
        elif base_type_name == 'lv_color_t':
             # Handle color from "#RRGGBB", "#AARRGGBB", or name string
             func_body = textwrap.dedent(f"""\
                if (!cJSON_IsString(node)) {{
                    LUP_LOG_ERROR("Expected JSON string for lv_color_t");
                    return false;
                }}
                const char* scalar = cJSON_GetStringValue(node);
                if (!scalar) return false; // Should not happen

                lv_color_t color;
                bool ok = true;
                errno = 0; // Reset errno for strtol

                if (scalar[0] == '#') {{
                    size_t len = strlen(scalar);
                    char* endptr = NULL;
                    // Use strtoul for unsigned hex parsing
                    unsigned long val = strtoul(scalar + 1, &endptr, 16);

                    if (errno != 0 || endptr == scalar + 1 || (endptr && *endptr != '\\0')) {{
                         LUP_LOG_ERROR("Invalid hex color format or characters: %s", scalar);
                         ok = false;
                    }} else if (len == 7) {{ // #RRGGBB
                        color = lv_color_hex((uint32_t)val);
                    }} else if (len == 9) {{ // #AARRGGBB
                         // Extract RGB, ignore alpha for lv_color_t
                         uint8_t r = (val >> 16) & 0xFF;
                         uint8_t g = (val >> 8) & 0xFF;
                         uint8_t b = val & 0xFF;
                         color = lv_color_make(r, g, b);
                         LUP_LOG_TRACE("Alpha component ignored for lv_color_t from %s", scalar);
                    }} else {{
                        LUP_LOG_ERROR("Invalid hex color length (%zu, expected 7 or 9): %s", len, scalar);
                        ok = false;
                    }}
                }} else {{
                    // Check named colors (add more as needed)
                    if (strcmp(scalar, "black") == 0) color = lv_color_black();
                    else if (strcmp(scalar, "white") == 0) color = lv_color_white();
                    else if (strcmp(scalar, "red") == 0) color = lv_color_hex(0xFF0000);
                    else if (strcmp(scalar, "lime") == 0) color = lv_color_hex(0x00FF00);
                    else if (strcmp(scalar, "blue") == 0) color = lv_color_hex(0x0000FF);
                    else if (strcmp(scalar, "yellow") == 0) color = lv_color_hex(0xFFFF00);
                    else if (strcmp(scalar, "cyan") == 0) color = lv_color_hex(0x00FFFF);
                    else if (strcmp(scalar, "magenta") == 0) color = lv_color_hex(0xFF00FF);
                    else if (strcmp(scalar, "gray") == 0) color = lv_color_hex(0x808080);
                    else if (strcmp(scalar, "silver") == 0) color = lv_color_hex(0xC0C0C0);
                    else {{
                        LUP_LOG_ERROR("Unknown color name: %s", scalar);
                        ok = false;
                    }}
                }}
                if (ok) {{
                    *(lv_color_t*)dest = color;
                }}
                return ok;
            """)
        elif details['is_primitive']:
             # Generic handler for C primitive numeric types
             if base_type_name in ['float', 'double']:
                 # Floating point types
                 func_body = textwrap.dedent(f"""\
                    if (!cJSON_IsNumber(node)) {{
                        LUP_LOG_ERROR("Expected JSON number for {c_type}");
                        return false;
                    }}
                    // Note: cJSON stores all numbers as doubles
                    *(({c_type}*)dest) = ({c_type})cJSON_GetNumberValue(node);
                    return true;
                 """)
             else:
                 # Integer types
                 func_body = textwrap.dedent(f"""\
                    if (!cJSON_IsNumber(node)) {{
                        LUP_LOG_ERROR("Expected JSON number for {c_type}");
                        return false;
                    }}
                    double num_val = cJSON_GetNumberValue(node);
                    // TODO: Add strict range checks for {c_type}?
                    // Check for potential truncation from double to integer
                    if (num_val != floor(num_val)) {{
                        LUP_LOG_WARN("Possible loss of precision converting float %g to integer type {c_type}", num_val);
                    }}
                    *(({c_type}*)dest) = ({c_type})num_val;
                    return true;
                 """)
        elif details['is_struct']:
             func_body = textwrap.dedent(f"""\
                 LUP_LOG_ERROR("Cannot unmarshal struct '{base_type_name}' directly from simple JSON value. Use 'call:' or '@id' reference.");
                 return false;
             """)
        else: # Fallback for other LVGL types or unhandled cases
             func_body = textwrap.dedent(f"""\
                 LUP_LOG_WARN("Direct unmarshaling for type '{base_type_name}' (C type: {c_type}) is not specifically implemented.");
                 return unmarshal_unknown(node, dest, "{base_type_name}");
             """)

        # Assemble the static function definition
        unmarshaler_funcs.append(textwrap.dedent(f"""\
            // Unmarshaler for type: {c_type} ({base_type_name})
            static bool {unmarshaler_name}(cJSON* node, void* dest) {{
            {textwrap.indent(func_body, ' ' * 4)}
            }}
        """))
        # Add case for the dispatch switch
        unmarshaler_call_switch_cases.append(
            f' else if (strcmp(safe_type_name, "{details["safe_name"]}") == 0) {{\n'
            f'        return {unmarshaler_name}(value_node, dest);\n'
            f'    }}'
        )

    unmarshalers_code = "\n\n".join(unmarshaler_funcs)
    # Assemble the final switch code used in unmarshal_node_value
    unmarshaler_call_switch_code = "".join(unmarshaler_call_switch_cases)
    if not unmarshaler_call_switch_code:
        unmarshaler_call_switch_code = ' LUP_LOG_ERROR("No unmarshaler generated for safe type: %s", safe_type_name); return false;'
    else:
        # Format the final switch block
        unmarshaler_call_switch_code = (
            f' if (0) {{ // Dummy for else-if chain\n'
            f'    {unmarshaler_call_switch_code}\n'
            f'    else {{\n'
            f'        LUP_LOG_ERROR("Internal Error: No unmarshaler function found for safe type: %s", safe_type_name);\n'
            f'        return false;\n'
            f'    }}'
        )

    return unmarshalers_code, unmarshaler_call_switch_code


def generate_call_switches(functions, widget_creators, struct_map):
    """Generates C switch code blocks for calling setters and other functions,
       with argument handling inside each block."""
    setter_switch_blocks = []
    function_switch_blocks = []
    processed_switches = set()

    # Helper to format argument passing C code snippet
    def get_c_arg_pass(local_var_name, arg_c_type, arg_ptr_level, arg_size):
        """Generates C code snippet for passing one argument using its local var."""
        # Arguments are passed directly using their correctly typed local variables
        # No complex casting needed here as in the old storage method.
        # The only check needed is for the known issue with large structs by value.
        if arg_ptr_level == 0 and arg_size > 8:
             # FATAL FLAW: Cannot reliably pass structs > 8 bytes by value.
             # Generate dummy value and rely on runtime check before call.
             return f"/* ERR_ARG_TOO_LARGE: ({arg_c_type}) */ ({arg_c_type}){{0}}"
        elif arg_c_type == '...':
             return "/* VARARGS SKIPPED */" # Placeholder
        else:
             # Pass the local variable directly
             return local_var_name

    # Iterate over all *filtered* functions to generate call logic
    for func in functions:
        func_name = func['name']
        if func_name in processed_switches: continue
        processed_switches.add(func_name)

        func_details = func # Use the direct func dict
        args = func_details.get('args', [])
        ret_type_info = func_details.get('type', {}).get('type', {})
        base_ret_name, ret_ptr, _, ret_c_type, ret_size = get_type_name(ret_type_info, struct_map)

        # --- Determine C arguments ---
        c_func_args = [] # Stores details for each C argument
        num_c_args = 0
        is_varargs = False
        arg_check_fail = False # Flag if problematic args found

        temp_args = args
        if len(args) == 1: # Handle C 'void' argument case
             arg0_type_info = args[0].get('type', {})
             arg0_base, arg0_ptr, _, _, _ = get_type_name(arg0_type_info, struct_map)
             if arg0_base == 'void' and arg0_ptr == 0: temp_args = []

        for i, arg_info in enumerate(temp_args):
             if i >= MAX_ARGS_SUPPORTED: break

             arg_type_info = arg_info.get('type', {})
             base_arg_name, arg_ptr_level, _, arg_c_type, arg_size = get_type_name(arg_type_info, struct_map)

             c_func_args.append({
                 'c_index': i, 'base_name': base_arg_name, 'c_type': arg_c_type,
                 'ptr_level': arg_ptr_level, 'size': arg_size,
                 'local_var': f"local_arg_{i}" # Name of the C local variable
             })
             num_c_args += 1

             if base_arg_name == '...': is_varargs = True; break
             if arg_ptr_level == 0 and arg_size > 8: arg_check_fail = True

        # --- Determine if function is a 'setter' ---
        parts = func_name.split('_set_')
        is_setter = (func_name.startswith('lv_obj_set_') or \
                     any(func_name.startswith(f'lv_{wt}_set_') for wt in widget_creators.keys())) \
                    and len(parts) == 2 and num_c_args >= 1

        # --- Generate C code block for this function's case ---
        block_lines = []
        inner_indent = "    " # Indentation inside the if block

        # Start the if/else if block header
        if is_setter:
            block_lines.append(f"}} else if (strcmp(setter_func_name, \"{func_name}\") == 0) {{")
        else: # Regular function call block header
             block_lines.append(f"}} else if (strcmp(func_name_str, \"{func_name}\") == 0"
                                f"{'' if is_varargs else f' && num_json_args == {num_c_args}'}) {{")

        # Check for fatal argument issues upfront
        if arg_check_fail:
            block_lines.append(f"{inner_indent}// FATAL: Cannot handle large pass-by-value arguments reliably.")
            block_lines.append(f"{inner_indent}LUP_LOG_ERROR(\"Call to {func_name} skipped: large pass-by-value args detected.\");")
            if not is_setter: block_lines.append(f"{inner_indent}call_success = false;")
            block_lines.append("}") # Close block early
            if is_setter: setter_switch_blocks.append("\n".join(block_lines))
            else: function_switch_blocks.append("\n".join(block_lines))
            continue

        # Check varargs compatibility early for function calls
        if not is_setter and is_varargs:
            block_lines.append(f"{inner_indent}// Varargs function check")
            # num_c_args here includes fixed args before ...
            block_lines.append(f"{inner_indent}if (num_json_args < {num_c_args}) {{")
            block_lines.append(f'{inner_indent}    LUP_LOG_ERROR("Varargs func {func_name} needs >= {num_c_args} fixed args, got %d", num_json_args);')
            block_lines.append(f"{inner_indent}    call_success = false;")
            block_lines.append(f"{inner_indent}}} else {{ // Varargs arg count OK") # Indent the rest

        # --- Generate Argument Handling inside the block ---
        block_lines.append(f"{inner_indent}// --- Argument Handling for {func_name} ---")
        block_lines.append(f"{inner_indent}bool args_prep_ok = true;")
        arg_pass_list = [] # Store C code snippets for passing args in final call

        start_arg_index = 1 if is_setter else 0 # Index of first C arg needing JSON value
        if is_setter: # Add the implicit object arg for setters
            arg_pass_list.append("(lv_obj_t*)obj_to_configure")

        # Loop through C function arguments that need values from JSON/context
        for arg_details in c_func_args[start_arg_index:]:
            i = arg_details['c_index']
            json_arg_index = i - start_arg_index # Index relative to JSON value(s)

            local_var = arg_details['local_var']
            arg_c_type = arg_details['c_type']
            base_arg_name = arg_details['base_name']
            arg_pass_list.append(local_var) # Use local var name for the final call

            # 1. Declare local variable
            block_lines.append(f"{inner_indent}{arg_c_type} {local_var};")
            # Optional: Zero initialize, especially for structs or pointers
            block_lines.append(f"{inner_indent}memset(&{local_var}, 0, sizeof({arg_c_type}));")

            # 2. Find the source cJSON node
            source_node_var = f"source_node_{i}"
            block_lines.append(f"{inner_indent}cJSON* {source_node_var} = NULL;")
            if is_setter:
                # Get source node from `attr_item` (simple value) or `attr_item` array (style prop)
                block_lines.append(f"{inner_indent}bool is_style_prop_seq_{i} = strncmp(key, \"style_\", 6) == 0 && cJSON_IsArray(attr_item);")
                block_lines.append(f"{inner_indent}if (is_style_prop_seq_{i}) {{")
                block_lines.append(f"{inner_indent}    int json_idx_{i} = {json_arg_index}; // 0 for value, 1 for state/part")
                block_lines.append(f"{inner_indent}    if (json_idx_{i} < num_json_values_available) {{")
                block_lines.append(f"{inner_indent}        {source_node_var} = cJSON_GetArrayItem(attr_item, json_idx_{i});")
                block_lines.append(f"{inner_indent}    }}")
                block_lines.append(f"{inner_indent}}} else if ({json_arg_index} == 0) {{")
                block_lines.append(f"{inner_indent}    {source_node_var} = attr_item;")
                block_lines.append(f"{inner_indent}}}")
            else: # Regular function call - get from `args_item` array
                 block_lines.append(f"{inner_indent}if (cJSON_IsArray(args_item) && {json_arg_index} < num_json_args) {{")
                 block_lines.append(f"{inner_indent}    {source_node_var} = cJSON_GetArrayItem(args_item, {json_arg_index});")
                 block_lines.append(f"{inner_indent}}}")

            # 3. Unmarshal the value into the local variable
            block_lines.append(f"{inner_indent}if (!{source_node_var}) {{")
            block_lines.append(f'{inner_indent}    LUP_LOG_ERROR("Failed to get source JSON node for arg {i} of {func_name}.");')
            block_lines.append(f"{inner_indent}    args_prep_ok = false;")
            block_lines.append(f"{inner_indent}}} else {{")
            block_lines.append(f'{inner_indent}    if (!unmarshal_node_value({source_node_var}, "{base_arg_name}", &{local_var})) {{')
            block_lines.append(f'{inner_indent}        LUP_LOG_ERROR("Failed unmarshal arg {i} (\'{base_arg_name}\') for {func_name}.");')
            block_lines.append(f"{inner_indent}        args_prep_ok = false;")
            block_lines.append(f"{inner_indent}    }}")
            block_lines.append(f"{inner_indent}}}")

        # --- Generate the LVGL Function Call ---
        block_lines.append(f"{inner_indent}// --- Call {func_name} ---")
        block_lines.append(f"{inner_indent}if (args_prep_ok) {{")

        # Format call arguments, potentially splitting long lines
        call_args_formatted = ", ".join(arg_pass_list)
        if len(call_args_formatted) > 70 and len(arg_pass_list) > 1:
            call_args_formatted = f"\n{inner_indent}        " + \
                                  f",\n{inner_indent}        ".join(arg_pass_list) + \
                                  f"\n{inner_indent}    "

        # Generate the call, handling return value
        if ret_c_type != "void":
            # Check return-by-value issue for large structs
            if ret_ptr == 0 and ret_size > 8:
                block_lines.append(f'{inner_indent}    LUP_LOG_ERROR("Return type \'{ret_c_type}\' by value (size > 8) is not handled reliably.");')
                if not is_setter: block_lines.append(f"{inner_indent}    call_success = false;")
            else:
                # Assign result if result_dest is provided (only for non-setters)
                if not is_setter:
                    block_lines.append(f"{inner_indent}    if (result_dest) {{")
                    block_lines.append(f"{inner_indent}        *(({ret_c_type}*)result_dest) = {func_name}({call_args_formatted});")
                    block_lines.append(f"{inner_indent}    }} else {{")
                    block_lines.append(f"{inner_indent}        {func_name}({call_args_formatted}); // Call discarding result")
                    block_lines.append(f"{inner_indent}    }}")
                else: # Setters shouldn't store return value
                    block_lines.append(f"{inner_indent}    // Assuming setter {func_name} returns void or its return is ignored.")
                    block_lines.append(f"{inner_indent}    {func_name}({call_args_formatted});")
        else: # Void return type
            block_lines.append(f"{inner_indent}    {func_name}({call_args_formatted});")

        # Add trace log after successful call
        block_lines.append(f'{inner_indent}    LUP_LOG_TRACE("Called {"setter" if is_setter else "function"}: {func_name}");')
        block_lines.append(f"{inner_indent}}} else {{") # Else for args_prep_ok failed
        block_lines.append(f'{inner_indent}    LUP_LOG_ERROR("Skipping call to {func_name} due to arg prep errors.");')
        if not is_setter: block_lines.append(f"{inner_indent}    call_success = false;")
        block_lines.append(f"{inner_indent}}}") # Close args_prep_ok check

        # Close varargs check block if it was opened
        if not is_setter and is_varargs:
            block_lines.append("    }") # Close the 'else' for varargs count check

        # Close the main if/else if block for this function
        block_lines.append("}")

        # Add the complete C code block to the appropriate list
        if is_setter:
            setter_switch_blocks.append("\n".join(block_lines))
        else:
            function_switch_blocks.append("\n".join(block_lines))
        # --- End of loop generating function block ---

    # --- Assemble final switch code ---
    # (Same assembly logic as before)
    setter_switch_code = "".join(setter_switch_blocks)
    if setter_switch_code:
        setter_switch_code = (
            f' if (0) {{ // Dummy for else-if chain\n'
            f'    {setter_switch_code}\n' # Items start with "} else if"
            f'    }} else {{\n'
            f'        LUP_LOG_ERROR("Setter function \'%s\' not handled in generated code.", setter_func_name);\n'
            f'    }}'
        )
    else: setter_switch_code = '// No setters generated.\n(void)setter_func_name;'

    function_switch_code = "".join(function_switch_blocks)
    if function_switch_code:
        function_switch_code = (
            f' if (0) {{ // Dummy for else-if chain\n'
            f'    {function_switch_code}\n' # Items start with "} else if"
            f'    }} else {{\n'
            f'        LUP_LOG_ERROR("Function \'%s\' not handled in generated code.", func_name_str);\n'
            f'        call_success = false;\n'
            f'    }}'
        )
    else: function_switch_code = '// No callable functions generated.\n(void)func_name_str; call_success = false;'


    return setter_switch_code, function_switch_code

def generate_widget_dispatch_logic(widget_creators, widget_initializers):
    """Generates the C if/else if chain for widget creation/initialization."""
    type_handlers = []
    # Sort for consistent output order
    for type_name, creator_func in sorted(widget_creators.items()):
        handler = textwrap.dedent(f"""
            }} else if (strcmp(type_str, "{type_name}") == 0) {{
                // Create widget using {creator_func}
                current_obj = {creator_func}(parent); // parent can be NULL
                if (!current_obj) {{
                    LUP_LOG_ERROR("Failed to create '{type_name}' widget");
                    goto cleanup; // Use goto for cleanup on failure
                }}
                LUP_LOG_TRACE("Created {type_name} obj: %p", (void*)current_obj);
                obj_to_configure = current_obj;    // Setters apply to the lv_obj_t
                created_ptr = current_obj;       // This is the primary created object/pointer
                current_widget_type = "{type_name}"; // Store type for specific setters
        """)
        type_handlers.append(handler)

    init_handlers = []
    # Sort for consistent output order
    for type_name, init_info in sorted(widget_initializers.items()):
         func_name = init_info['func_name']
         struct_type = init_info['struct_type']
         handler = textwrap.dedent(f"""\
             }} else if (strcmp(type_str, "{type_name}") == 0) {{
                 // Type '{type_name}' uses an init function ({func_name}).
                 // Allocate struct on stack. MUST be registered via ID or used immediately.
                 {struct_type} instance;
                 // IMPORTANT: Always zero initialize stack allocated structs
                 memset(&instance, 0, sizeof({struct_type}));
                 {func_name}(&instance); // Initialize the struct
                 LUP_LOG_TRACE("Initialized {type_name} on stack: %p", (void*)&instance);
                 // Setters cannot be applied directly via obj_to_configure for _init types.
                 obj_to_configure = NULL;
                 created_ptr = &instance; // Store pointer to STACK variable
                 is_lv_obj = false;       // Mark as not an lv_obj_t
                 // Track stack allocation details for potential ID registration duplication
                 stack_allocated_struct_ptr = &instance;
                 stack_allocated_struct_size = sizeof({struct_type});
                 // current_widget_type is not set as obj setters don't apply
         """)
         init_handlers.append(handler)

    # Combine handlers into the final if/else if structure
    create_init_logic = "\n".join(type_handlers) + "\n" + "\n".join(init_handlers)
    if not create_init_logic:
        # If no creators/initializers were found
        create_init_logic = ' LUP_LOG_ERROR("Unknown type: %s", type_str); goto cleanup;'
    else:
        # Format the final block
        create_init_logic = (
            f' if (0) {{ // Dummy for else-if chain\n'
            f'    {create_init_logic}\n' # Contains the "} else if..." chains
            f'    }} else {{\n'
            f'        LUP_LOG_ERROR("Unknown or unsupported type string: \\"%s\\"", type_str);\n'
            f'        goto cleanup;\n' # Ensure cleanup path on unknown type
            f'    }}'
        )
    return create_init_logic

def generate_source(functions, types_details, enums, typedefs, structs, all_funcs_map, struct_map):
    """Generates the lvgl_ui_preview.c content using cJSON with improved arg handling."""

    enum_map = {}
    for enum_info in enums:
        for member in enum_info.get('members', []):
            if member.get('name'):
                enum_map[member['name']] = member['name'] # C Name -> C Name

    print("  Generating C API data tables...")
    c_api_data_code = generate_c_api_data_code(types_details, functions, struct_map)

    print("  Generating C unmarshaler functions...")
    unmarshalers_code, unmarshaler_call_switch_code = generate_unmarshalers(types_details, enums, enum_map)

    print("  Generating C widget creation logic...")
    widget_creators = {}
    widget_initializers = {}
    # Find creators/initializers
    for func in all_funcs_map.values():
        func_name = func.get('name')
        if not func_name: continue
        args = func.get('args', [])
        ret_type_info = func.get('type', {}).get('type', {})
        base_ret_name, ret_ptr, _, ret_c_type, _ = get_type_name(ret_type_info, struct_map)

        if func_name.endswith('_create') and not func_name.endswith('_style_create') and \
           base_ret_name == 'lv_obj_t' and ret_ptr == 1 and len(args) == 1:
            arg_type_info = args[0].get('type', {})
            _, _, _, arg_c_type, _ = get_type_name(arg_type_info, struct_map)
            if arg_c_type == 'lv_obj_t *' or arg_c_type == 'void *':
                widget_type = func_name.split('_create')[0].replace('lv_', '')
                widget_creators[widget_type] = func_name
        elif func_name.endswith('_init') and not func_name.startswith('lv_draw_') and \
             ret_c_type == 'void' and len(args) == 1:
            arg_type_info = args[0].get('type', {})
            base_arg_name, arg_ptr, _, _, _ = get_type_name(arg_type_info, struct_map)
            # Check if arg type name starts with expected pattern (e.g., lv_style_t for lv_style_init)
            expected_prefix = f"lv_{func_name.split('_init')[0].replace('lv_', '')}_t"
            if arg_ptr == 1 and base_arg_name.startswith(expected_prefix):
                init_type = func_name.split('_init')[0].replace('lv_', '')
                widget_initializers[init_type] = {'func_name': func_name, 'struct_type': base_arg_name }

    create_init_logic = generate_widget_dispatch_logic(widget_creators, widget_initializers)

    print("  Generating C function/setter call switches (with inline arg handling)...")
    # Use the NEW generate_call_switches
    setter_call_switch_code, function_call_switch_code = generate_call_switches(functions, widget_creators, struct_map)

    # --- Assemble the final C source ---
    print("  Assembling final C source file...")
    # ID Registry & Custom Unmarshaler Impls (formatted)
    id_registry_impl = textwrap.dedent("""\
        // --- ID Registry Implementation ---
        typedef struct id_entry_s {
            char *id;
            void *ptr;
            struct id_entry_s *next;
        } id_entry_t;
        static id_entry_t *id_registry_head = NULL;

        void lvgl_ui_preview_register(const char *id, void *ptr) {
            if (!id || !ptr) return;
            LUP_LOG_INFO("Registering ID: %s -> %p", id, ptr);
            id_entry_t *current = id_registry_head;
            id_entry_t *prev = NULL;
            // Check for existing entry and remove it first
            while (current) {
                if (strcmp(current->id, id) == 0) {
                    LUP_LOG_WARN("ID '%s' already registered. Replacing.", id);
                    if (prev) {
                        prev->next = current->next;
                    } else {
                        id_registry_head = current->next; // Removing head
                    }
                    lv_free(current->id);
                    lv_free(current); // Free the old entry container
                    break;
                }
                prev = current;
                current = current->next;
            }
            // Allocate and add the new entry
            id_entry_t *new_entry = (id_entry_t *)lv_malloc(sizeof(id_entry_t));
            if (!new_entry) {
                LUP_LOG_ERROR("Failed to allocate memory for ID registry entry");
                return;
            }
            new_entry->id = lv_malloc(strlen(id) + 1);
            if (!new_entry->id) {
                LUP_LOG_ERROR("Failed to allocate memory for ID string");
                lv_free(new_entry);
                return;
            }
            strcpy(new_entry->id, id);
            new_entry->ptr = ptr;
            new_entry->next = id_registry_head; // Insert at head
            id_registry_head = new_entry;
        }

        void* lvgl_ui_preview_get(const char *id) {
            if (!id) return NULL;
            id_entry_t *current = id_registry_head;
            while (current) {
                if (strcmp(current->id, id) == 0) {
                    LUP_LOG_TRACE("Found ID '%s': %p", id, current->ptr);
                    return current->ptr;
                }
                current = current->next;
            }
            LUP_LOG_WARN("ID not found: %s", id);
            return NULL;
        }

        void lvgl_ui_preview_clear_registry(void) {
             id_entry_t *current = id_registry_head;
             LUP_LOG_INFO("Clearing ID registry.");
             while (current) {
                 id_entry_t *next = current->next;
                 lv_free(current->id);
                 // We ONLY free the registry entry and the ID string.
                 // We DO NOT free the registered ptr itself, as ownership lies elsewhere.
                 lv_free(current);
                 current = next;
             }
             id_registry_head = NULL;
        }
    """)
    custom_unmarshaler_impl = textwrap.dedent("""\
        // --- Custom Unmarshaler Implementation ---
        typedef struct cu_entry_s {
            const char *prefix; // The prefix string (e.g., "@", "img:")
            lvgl_ui_unmarshaler_func_t func; // The handler function
            struct cu_entry_s *next;
        } cu_entry_t;
        static cu_entry_t *cu_head = NULL; // Head of the linked list

        // Registers a custom unmarshaler
        void lvgl_ui_preview_register_unmarshaler(const char *prefix, lvgl_ui_unmarshaler_func_t func) {
             if (!prefix || !func) return;
             // TODO: Check for duplicate prefix?
             cu_entry_t *new_entry = (cu_entry_t *)lv_malloc(sizeof(cu_entry_t));
             if (!new_entry) {
                 LUP_LOG_ERROR("Failed to allocate memory for custom unmarshaler entry");
                 return;
             }
             // Assume prefix is a static string, don't copy it.
             new_entry->prefix = prefix;
             new_entry->func = func;
             new_entry->next = cu_head; // Insert at head
             cu_head = new_entry;
             LUP_LOG_INFO("Registered custom unmarshaler for prefix: %s", prefix);
        }

        // Tries to find and run a custom unmarshaler based on string prefix
        static bool run_custom_unmarshaler(cJSON* node, void* dest) {
            if (!cJSON_IsString(node)) return false; // Only run on strings
            const char* scalar = cJSON_GetStringValue(node);
            if (!scalar) return false; // Should not happen for string node

            cu_entry_t *current = cu_head;
            while (current) {
                 size_t prefix_len = strlen(current->prefix);
                 // Check if the scalar starts with the prefix
                 if (strncmp(scalar, current->prefix, prefix_len) == 0) {
                      LUP_LOG_TRACE("Using custom unmarshaler for prefix '%s'", current->prefix);
                      // Call the handler with the rest of the string
                      return current->func(scalar + prefix_len, dest);
                 }
                 current = current->next;
            }
            return false; // No matching prefix found
        }

        // Default custom unmarshaler for '@' ID references
        static bool unmarshal_id_ref(const char *id_str_without_at, void *dest_ptr) {
            void* ptr = lvgl_ui_preview_get(id_str_without_at);
            if (!ptr) {
                LUP_LOG_ERROR("Referenced ID '@%s' not found in registry.", id_str_without_at);
                return false;
            }
            // Store the retrieved pointer into the destination
            *(void**)dest_ptr = ptr;
            return true;
        }
    """)

    # Forward declare internal functions
    forward_decls = textwrap.dedent("""\
        // --- Forward Declarations of Static Functions ---
        static lv_obj_t* lvgl_ui_preview_render_json_node_internal(cJSON *node, lv_obj_t *parent);
        static bool execute_json_call(cJSON *call_node, void *result_dest, const char* expected_type_name_hint);
        static bool unmarshal_node_value(cJSON* value_node, const char* expected_base_type_name, void* dest);
        // Static unmarshal_... functions are defined before use below.
    """)

    # Main Rendering Function (Updated with new structure)
    render_node_func = textwrap.dedent(f"""\
        // --- Core Rendering Logic ---

        // Internal recursive function to render a single JSON object node
        static lv_obj_t* lvgl_ui_preview_render_json_node_internal(cJSON *node, lv_obj_t *parent) {{
            if (!cJSON_IsObject(node)) {{
                LUP_LOG_ERROR("Render node: Expected JSON object, got type %d. Skipping.", node ? node->type : -1);
                return NULL;
            }}

            // Get common properties first
            cJSON* type_item = cJSON_GetObjectItemCaseSensitive(node, "type");
            cJSON* id_item = cJSON_GetObjectItemCaseSensitive(node, "id");
            cJSON* children_item = cJSON_GetObjectItemCaseSensitive(node, "children");

            // Validate required 'type' property
            if (!cJSON_IsString(type_item) || !type_item->valuestring) {{
                LUP_LOG_ERROR("Render node: Node is missing required 'type' string property.");
                return NULL;
            }}
            const char* type_str = type_item->valuestring;
            LUP_LOG_INFO("Processing type: %s", type_str);

            // --- State Variables for processing this node ---
            lv_obj_t* current_obj = NULL;           // lv_obj_t* if created by _create
            void* obj_to_configure = NULL;      // Pointer that setters operate on (usually current_obj)
            void* created_ptr = NULL;           // Pointer to the object OR stack struct created
            bool is_lv_obj = true;              // Tracks if created_ptr is lv_obj_t*
            const char* current_widget_type = NULL; // Type name for specific setters
            void* stack_allocated_struct_ptr = NULL;// Used if _init created on stack
            size_t stack_allocated_struct_size = 0; // Size if stack allocated

            // --- Create or Initialize Object Based on 'type' ---
            // This block expands the generated if/else if chain for widget types
            {create_init_logic}

            // --- Register ID (if specified) ---
            if (cJSON_IsString(id_item) && id_item->valuestring) {{
                const char* id_str = id_item->valuestring;
                const char* id_to_register = id_str;
                void* ptr_to_register = created_ptr;

                if (id_str[0] == '@') {{
                    id_to_register = id_str + 1; // Use name without '@'
                }} else {{
                    LUP_LOG_WARN("ID '%s' should start with '@'. Registering anyway.", id_str);
                }}

                // If the created object was stack-allocated (e.g., style), duplicate it for the registry
                if (stack_allocated_struct_ptr != NULL && ptr_to_register == stack_allocated_struct_ptr) {{
                    LUP_LOG_TRACE("Duplicating stack struct (size %zu) for ID registration: %s",
                                  stack_allocated_struct_size, id_to_register);
                    ptr_to_register = lv_malloc(stack_allocated_struct_size);
                    if (ptr_to_register) {{
                        memcpy(ptr_to_register, stack_allocated_struct_ptr, stack_allocated_struct_size);
                        // Now ptr_to_register points to heap memory owned by the registry
                    }} else {{
                        LUP_LOG_ERROR("Failed to allocate memory to duplicate stack struct for ID '%s'. ID NOT REGISTERED.",
                                      id_to_register);
                        ptr_to_register = NULL; // Ensure we don't register garbage
                    }}
                }}

                // Register the pointer (either original or duplicated heap copy)
                if (ptr_to_register) {{
                     lvgl_ui_preview_register(id_to_register, ptr_to_register);
                }}
            }} // End ID registration

            // --- Apply Attributes via Setters ---
            if (obj_to_configure) {{ // Only proceed if we have an lv_obj_t* to configure
                cJSON *attr_item = NULL;
                // Iterate through all key-value pairs in the JSON object node
                cJSON_ArrayForEach(attr_item, node) {{
                    const char* key = attr_item->string; // Attribute name (JSON key)
                    if (!key) continue;

                    // Skip keys handled elsewhere
                    if (strcmp(key, "type") == 0 || strcmp(key, "id") == 0 || strcmp(key, "children") == 0) {{
                        continue;
                    }}

                    // 1. Find the setter function name for this attribute key
                    const char* setter_func_name = NULL;
                    char potential_setter[128];
                    if (current_widget_type) {{ // Check widget-specific first
                        snprintf(potential_setter, sizeof(potential_setter), "lv_%s_set_%s", current_widget_type, key);
                        const lup_func_detail_t* details = find_func_detail(potential_setter);
                        if (details) setter_func_name = details->func_name; // Use canonical name
                    }}
                    if (!setter_func_name) {{ // Fallback to generic lv_obj_set_
                        snprintf(potential_setter, sizeof(potential_setter), "lv_obj_set_%s", key);
                        const lup_func_detail_t* details = find_func_detail(potential_setter);
                        if (details) setter_func_name = details->func_name;
                    }}

                    if (!setter_func_name) {{
                        LUP_LOG_WARN("No setter found for attribute '%s' on type '%s'. Skipping.",
                                     key, current_widget_type ? current_widget_type : "lv_obj");
                        continue;
                    }}

                    // 2. Execute the specific setter block (which handles args & call)
                    // Context needed inside the generated switch block:
                    // - setter_func_name (checked by the if/else)
                    // - obj_to_configure (the lv_obj_t* passed directly)
                    // - key (the attribute name, for logging or complex mapping)
                    // - attr_item (the cJSON node for the attribute's value)
                    // - num_json_values_available (calculated from attr_item type)
                    int num_json_values_available = 1;
                    if(cJSON_IsArray(attr_item)) {{
                        num_json_values_available = cJSON_GetArraySize(attr_item);
                    }}

                    // --- Generated Setter Switch (Handles args & call) ---
                    // This switch contains the full logic for each setter, including
                    // declaring local vars, finding source nodes, unmarshaling, and calling.
                    {textwrap.indent(setter_call_switch_code, ' ' * 20)}
                    // --- End Generated Setter Switch ---

                }} // End loop through attributes
            }} // End if(obj_to_configure)

            // --- Process Children ---
            if (cJSON_IsArray(children_item)) {{
                int num_children = cJSON_GetArraySize(children_item);
                LUP_LOG_TRACE("Processing %d children for type %s", num_children, type_str);
                if (is_lv_obj && current_obj) {{ // Children can only be added to lv_obj_t
                    cJSON *child_node = NULL;
                    cJSON_ArrayForEach(child_node, children_item) {{
                        // Recursively render child node onto the current object
                        lvgl_ui_preview_render_json_node_internal(child_node, current_obj);
                    }}
                }} else if (num_children > 0) {{
                    LUP_LOG_WARN("Type '%s' is not an lv_obj_t; cannot add JSON children specified.", type_str);
                }}
            }} // End processing children

        cleanup:
            // Currently no specific cleanup needed here per node.
            // Return the lv_obj_t* if one was created, otherwise NULL.
            return is_lv_obj ? current_obj : NULL;
        }} // end lvgl_ui_preview_render_json_node_internal

        // Helper to unmarshal any value node (checks custom handler, call, standard type)
        // (Unmarshaler dispatch logic remains the same as before)
        static bool unmarshal_node_value(cJSON* value_node, const char* expected_base_type_name, void* dest) {{
            if (!value_node || !dest || !expected_base_type_name) {{
                 LUP_LOG_ERROR("Unmarshal: Invalid arguments (node=%p, dest=%p, type=%s)",
                               (void*)value_node, dest, expected_base_type_name ? expected_base_type_name : "NULL");
                 return false;
            }}

            // 1. Try custom unmarshalers first (e.g., "@id", "img:")
            if (run_custom_unmarshaler(value_node, dest)) {{
                return true; // Handled by custom logic
            }}

            // 2. Check for function call structure: {{ "call": "func_name", "args": [...] }}
            if (cJSON_IsObject(value_node) && cJSON_HasObjectItem(value_node, "call")) {{
                LUP_LOG_TRACE("Unmarshal: Detected 'call' structure for expected type '%s'", expected_base_type_name);
                return execute_json_call(value_node, dest, expected_base_type_name);
            }}

            // 3. Standard unmarshaling based on expected type name
            const lup_type_detail_t* type_detail = find_type_detail(expected_base_type_name);
            if (!type_detail) {{
                 LUP_LOG_ERROR("Unmarshal: Type detail not found for base name: %s", expected_base_type_name);
                 // Use the unknown handler as a last resort attempt
                 return unmarshal_unknown(value_node, dest, expected_base_type_name);
            }}
            const char* safe_type_name = type_detail->safe_name; // Sanitized name used in switch

            // --- Generated Unmarshaler Dispatch Switch ---
            // This switch calls the appropriate static unmarshal_... function
            {textwrap.indent(unmarshaler_call_switch_code, ' ' * 12)}
            // --- End Generated Unmarshaler Dispatch Switch ---
        }}

        // Executes a function specified in a {{ "call": "...", "args": [...] }} structure
        static bool execute_json_call(cJSON *call_node, void *result_dest, const char* expected_type_name_hint) {{
            if (!cJSON_IsObject(call_node)) {{
                LUP_LOG_ERROR("Execute call: Expected JSON object for call structure.");
                return false;
            }}
            cJSON* func_name_item = cJSON_GetObjectItemCaseSensitive(call_node, "call");
            cJSON* args_item = cJSON_GetObjectItemCaseSensitive(call_node, "args"); // Optional cJSON array

            if (!cJSON_IsString(func_name_item) || !func_name_item->valuestring) {{
                LUP_LOG_ERROR("Execute call: Call node missing 'call' string property.");
                return false;
            }}
            const char* func_name_str = func_name_item->valuestring;
            LUP_LOG_TRACE("Executing call: %s", func_name_str);

            // 1. Find Function Details from generated table
            const lup_func_detail_t* func_details = find_func_detail(func_name_str);
            if (!func_details) {{
                LUP_LOG_ERROR("Execute call: Function '%s' not found in generated API details.", func_name_str);
                return false;
            }}

            // 2. Check Argument Counts provided in JSON vs. expected by C function
            int num_json_args = 0;
            if (cJSON_IsArray(args_item)) {{
                 num_json_args = cJSON_GetArraySize(args_item);
            }}

            if (!func_details->is_varargs && func_details->arg_count != num_json_args) {{
                LUP_LOG_ERROR("Execute call: Arg count mismatch for '%s'. C expects %d, JSON provides %d.",
                              func_name_str, func_details->arg_count, num_json_args);
                return false;
            }}
            if (func_details->is_varargs && num_json_args < func_details->arg_count) {{
                 LUP_LOG_ERROR("Execute call: Varargs func '%s' needs at least %d fixed args, JSON provides %d.",
                               func_name_str, func_details->arg_count, num_json_args);
                 return false;
            }}
             if (num_json_args > MAX_ARGS_SUPPORTED) {{
                  LUP_LOG_ERROR("Execute call: Too many JSON args (%d > %d) provided for call '%s'.",
                                num_json_args, MAX_ARGS_SUPPORTED, func_name_str);
                  return false;
             }}

            // 3. Execute the specific function block (handles args & call)
            bool call_success = true; // Assume success unless switch block sets it false

            // --- Generated Function Call Switch ---
            // This switch contains the full logic for each function, including
            // declaring local vars, finding source nodes, unmarshaling, and calling.
            {textwrap.indent(function_call_switch_code, ' ' * 12)}
            // --- End Generated Function Call Switch ---

             return call_success;
        }}

        // --- Public API Functions ---

        // Renders a single JSON node
        lv_obj_t* lvgl_ui_preview_render_json_node(cJSON *node, lv_obj_t *parent) {{
             LUP_LOG_INFO("Render JSON node %p, parent %p", (void*)node, (void*)parent);
             // Use active screen if parent is NULL
             lv_obj_t* effective_parent = parent ? parent : lv_screen_active();
             if (!effective_parent) {{
                 LUP_LOG_ERROR("Cannot render node: No active screen and parent is NULL.");
                 return NULL;
             }}
             // Call the internal recursive renderer
             return lvgl_ui_preview_render_json_node_internal(node, effective_parent);
        }}

        // Processes a JSON string (parses and renders root array)
        int lvgl_ui_preview_process_json_str(const char *json_string, lv_obj_t *parent_screen) {{
            if (!json_string) {{
                LUP_LOG_ERROR("Process JSON string: Input string is NULL.");
                return -1;
            }}
            LUP_LOG_INFO("Processing JSON string...");
            cJSON *root = cJSON_Parse(json_string);
            if (!root) {{
                const char *error_ptr = cJSON_GetErrorPtr();
                LUP_LOG_ERROR("cJSON_Parse failed near: %s", error_ptr ? error_ptr : "(unknown location)");
                return -1; // Parse error
            }}

            int processed_count = 0;
            int error_count = 0;
            // Expect root to be an array of UI elements
            if (!cJSON_IsArray(root)) {{
                LUP_LOG_ERROR("JSON root must be an array of UI element objects.");
                error_count++;
            }} else {{
                cJSON *element_node = NULL;
                // Iterate through the root array and render each element
                cJSON_ArrayForEach(element_node, root) {{
                    if (lvgl_ui_preview_render_json_node(element_node, parent_screen)) {{
                        processed_count++; // Count successful renderings
                    }} else {{
                        error_count++; // Count elements that failed to render
                    }}
                }}
            }}

            cJSON_Delete(root); // Free the memory allocated by cJSON_Parse
            LUP_LOG_INFO("JSON string processing finished. %d elements processed, %d errors.", processed_count, error_count);
            // Return count of successfully processed elements, or -1 if errors occurred
            return error_count > 0 ? -1 : processed_count;
        }}

        // Helper to read entire file into a string buffer (caller must free using lv_free/free)
        static char *read_file_to_string(const char *filename) {{
            FILE *file = fopen(filename, "rb");
            if (!file) {{
                LUP_LOG_ERROR("Failed to open file '%s': %s (errno %d)", filename, strerror(errno), errno);
                return NULL;
            }}

            fseek(file, 0, SEEK_END);
            long length = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (length < 0) {{
                LUP_LOG_ERROR("Failed to determine size of file '%s': %s (errno %d)", filename, strerror(errno), errno);
                fclose(file);
                return NULL;
            }}

            // Use lv_malloc for consistency if available, otherwise standard malloc
            #if LV_USE_MEM_MONITOR
                char *buffer = (char *)lv_malloc(length + 1);
            #else
                char *buffer = (char *)malloc(length + 1);
            #endif

            if (!buffer) {{
                LUP_LOG_ERROR("Failed to allocate %ld bytes to read file: %s", length + 1, filename);
                fclose(file);
                return NULL;
            }}

            size_t read_len = fread(buffer, 1, length, file);
            fclose(file);

            if (read_len != (size_t)length) {{
                LUP_LOG_ERROR("Failed to read entire file (%zu bytes read != %ld bytes expected): %s",
                              read_len, length, filename);
                #if LV_USE_MEM_MONITOR
                    lv_free(buffer);
                #else
                    free(buffer);
                #endif
                return NULL;
            }}

            buffer[length] = '\\0'; // Null-terminate the string
            return buffer;
        }}

        // Processes a JSON file (reads, parses, renders root array)
        int lvgl_ui_preview_process_json_file(const char *json_filepath, lv_obj_t *parent_screen) {{
            if (!json_filepath) {{
                 LUP_LOG_ERROR("Process JSON file: File path is NULL.");
                 return -1;
            }}
            LUP_LOG_INFO("Processing JSON file: %s", json_filepath);

            char *json_string = read_file_to_string(json_filepath);
            if (!json_string) {{
                // read_file_to_string logs errors
                return -1;
            }}

            // Process the string content
            int result = lvgl_ui_preview_process_json_str(json_string, parent_screen);

            // Free the buffer allocated by read_file_to_string
            #if LV_USE_MEM_MONITOR
                lv_free(json_string);
            #else
                free(json_string);
            #endif

            return result;
        }}

        // --- Initialization ---
        void lvgl_ui_preview_init(void) {{
             LUP_LOG_INFO("Initializing LVGL UI Preview (cJSON backend v %s)", cJSON_Version());
             // Register the default handler for "@" id references
             lvgl_ui_preview_register_unmarshaler("@", unmarshal_id_ref);
             // User can register more custom handlers after calling init
        }}

    """) # End render_node_func etc.

    # --- Final Assembly ---
    source_content = textwrap.dedent(f"""\
        // !! THIS FILE IS AUTO-GENERATED by general_lvgl_preview.py !!
        // !! DO NOT EDIT MANUALLY !!

        #include "lvgl_ui_preview.h"

        // Standard C includes
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <errno.h>
        #include <math.h> // For floor()

        // LVGL include (already in header)
        // cJSON include (already in header)

        // Generated API data structures and lookup functions
        {c_api_data_code}

        // Forward declarations of static functions
        {forward_decls}

        // ID Registry implementation
        {id_registry_impl}

        // Custom Unmarshaler implementation
        {custom_unmarshaler_impl}

        // Generated static unmarshaler functions
        {unmarshalers_code}

        // Core rendering functions (render_node_internal, helpers)
        {render_node_func}

    """)
    return source_content


# --- Main Script Logic ---
def main():
    parser = argparse.ArgumentParser(
        description='Generate LVGL UI Preview C library (cJSON backend) from LVGL API JSON.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter # Show defaults in help
    )
    parser.add_argument('json_api_path', help='Path to the LVGL JSON API definition file.')
    parser.add_argument('--output-dir', default='.', help='Directory to output lvgl_ui_preview.c/h files.')
    parser.add_argument('--include', action='append', default=[],
                        help='Function name prefix or exact name to include. Can use multiple times.')
    parser.add_argument('--exclude', action='append', default=["lv_freetype_", "lv_tiny_ttf_"],
                        help='Function name prefix or exact name to exclude.')
    parser.add_argument('--default-include', action='store_true',
                        help='Include common lv_obj_*, lv_<widget>_*, lv_style_*, etc. if no --include is given.')

    args = parser.parse_args()

    # --- Load API Definition ---
    print(f"Loading LVGL API definition from: {args.json_api_path}")
    try:
        with open(args.json_api_path, 'r', encoding='utf-8') as f:
            api_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found '{args.json_api_path}'")
        exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: JSON parse failed '{args.json_api_path}': {e}")
        exit(1)
    print("API definition loaded.")

    # --- Prepare Data ---
    all_functions = api_data.get('functions', [])
    enums = api_data.get('enums', [])
    typedefs = api_data.get('typedefs', [])
    structs = api_data.get('structures', [])
    # Build helper maps for faster lookups
    all_funcs_map = {func['name']: func for func in all_functions if func.get('name')}
    struct_map = {s['name']: s for s in structs if s.get('name')}
    # typedef_map = {t['name']: t for t in typedefs if t.get('name')} # Currently unused

    # --- Determine Include/Exclude Lists ---
    include_list = args.include
    exclude_list = args.exclude

    if not include_list and args.default_include:
         print("No specific includes given, using --default-include set.")
         # Populate a default list of common/useful functions
         include_list = [
             'lv_obj_',        # Base object functions
             'lv_group_',      # Group functions often needed
             'lv_style_',      # Style functions
             'lv_color_',      # Color utilities
             'lv_pct',         # Percentage helper
             'lv_theme_',      # Theme functions
             'lvgl_ui_preview_get' # Self-reference for @id
         ]
         # Add widget creators and their setters/initializers
         for func_name in all_funcs_map.keys():
             widget_name = None
             if func_name.endswith('_create') and not func_name.endswith('_style_create'):
                 widget_name = func_name.split('_create')[0]
                 include_list.append(func_name) # Include exact creator
             elif func_name.endswith('_init') and \
                  not func_name.startswith('lv_draw_') and \
                  not func_name.startswith('lv_global') and \
                  not func_name == 'lv_init': # Exclude some specific inits
                 # Include initializers like lv_style_init, lv_anim_init etc.
                 include_list.append(func_name)
                 # Potentially add setters related to the initialized type? Harder to map.

             if widget_name:
                # Add specific setters for this widget type
                include_list.append(f'{widget_name}_set_')

         include_list = sorted(list(set(include_list))) # Remove duplicates and sort

    elif not include_list and not args.default_include:
         print("Warning: No --include patterns specified and --default-include not used.")
         print("Including ALL functions. This might lead to a very large generated library.")
         include_list = ['lv_'] # Include everything as a fallback

    print(f"Effective Include List (patterns/names): {include_list}")
    print(f"Effective Exclude List (patterns/names): {exclude_list}")

    # --- Filter Functions ---
    print("Filtering functions based on include/exclude rules...")
    filtered_functions = []
    skipped_count = 0
    included_count = 0
    for func in all_functions:
        func_name = func.get('name')
        if not func_name: continue

        # Check include rules
        included = func_name in include_list or any(func_name.startswith(p) for p in include_list)
        if not included: continue

        # Check exclude rules
        excluded = func_name in exclude_list or any(func_name.startswith(p) for p in exclude_list)
        if excluded:
            # print(f"  Excluding (matches exclude list): {func_name}") # Optional: verbose logging
            skipped_count += 1
            continue

        # Add additional checks for suitability (e.g., skip callbacks)
        can_handle = True
        if func.get('args'):
             for arg in func['args']:
                 arg_type_info = arg.get('type', {})
                 base_arg_name, _, _, _, _ = get_type_name(arg_type_info, struct_map)
                 # Skip functions taking function pointers (callbacks)
                 if base_arg_name.endswith('_cb_t'):
                      # print(f"  Excluding (callback arg type): {func_name} (arg: {base_arg_name})") # Optional: verbose logging
                      can_handle = False
                      skipped_count += 1
                      break
                 # Add other checks here if needed

        if can_handle:
            filtered_functions.append(func)
            included_count += 1

    print(f"Selected {included_count} functions for generation (skipped {skipped_count}).")
    if not filtered_functions:
        print("Error: No functions selected after filtering. Nothing to generate.")
        exit(1)

    # --- Extract Unique Types Required by Filtered Functions ---
    print("Analyzing required types...")
    types_details = defaultdict(lambda: { # Use defaultdict for easier population
        'count': 0, 'is_primitive': False, 'is_enum': False, 'is_struct': False,
        'is_lvgl_type': False, 'is_pointer_base': False, 'is_string_ptr': False,
        'c_type': 'unknown', 'safe_name': 'unknown', 'size': 0
    })

    def add_type_details(type_info):
        """Helper to populate the types_details dictionary."""
        if not type_info: return
        base_name, ptr_level, is_const, full_c_name, size = get_type_name(type_info, struct_map)

        if base_name in ['void', '...']: return # Skip void and ellipsis

        details = types_details[base_name]
        details['count'] += 1
        details['c_type'] = base_name # Store base C type name
        details['safe_name'] = sanitize_c_identifier(base_name)
        details['size'] = max(details['size'], size) # Store max estimated size seen

        # Set flags (only set to true, never back to false)
        if ptr_level > 0: details['is_pointer_base'] = True
        if base_name == 'char' and ptr_level > 0: details['is_string_ptr'] = True

        if not details['is_enum']:
            details['is_enum'] = any(
                e.get('name') == base_name or \
                (not e.get('name') and e.get('members') and \
                 e['members'][0].get('type',{}).get('name') == base_name) # Anon enum heuristic
                for e in enums
            )
        if not details['is_struct']:
            details['is_struct'] = base_name in struct_map

        details['is_primitive'] = base_name in C_TYPE_SIZES and not details['is_enum'] and not details['is_struct']
        details['is_lvgl_type'] = base_name.startswith('lv_') and base_name.endswith('_t')

    # Populate types_details based on return types and arguments of filtered functions
    for func in filtered_functions:
        add_type_details(func.get('type', {}).get('type', {})) # Return type
        if func.get('args'):
            for arg in func['args']:
                 add_type_details(arg.get('type', {})) # Argument types

    print(f"Identified {len(types_details)} unique base types required.")

    # --- Generate Code ---
    print("Generating C header (cJSON)...")
    header_code = generate_header(filtered_functions, types_details, enums, typedefs, structs)

    print("Generating C source (cJSON)...")
    source_code = generate_source(filtered_functions, types_details, enums, typedefs, structs, all_funcs_map, struct_map)

    # --- Write Output Files ---
    os.makedirs(args.output_dir, exist_ok=True)
    header_path = os.path.join(args.output_dir, 'lvgl_ui_preview.h')
    source_path = os.path.join(args.output_dir, 'lvgl_ui_preview.c')

    try:
        print(f"Writing header file: {header_path}")
        with open(header_path, 'w', encoding='utf-8') as f:
            f.write(header_code)

        print(f"Writing source file: {source_path}")
        with open(source_path, 'w', encoding='utf-8') as f:
            f.write(source_code)

        print("\nGeneration Complete (cJSON backend).")
        print("-" * 40)
        print("NEXT STEPS:")
        print("1. Add generated .c/.h to your project build.")
        print("2. Add cJSON source/headers to your project build.")
        print("3. Link cJSON library during compilation.")
        print("4. Call `lvgl_ui_preview_init()` once at startup.")
        print("5. Use `lvgl_ui_preview_process_json_str/file()` to load UI.")
        print("6. Review generated code warnings/errors (LUP_LOG_...)")
        print("   and potential TODOs or limitations mentioned in comments.")
        print("-" * 40)

    except IOError as e:
        print(f"\nError writing output file: {e}")
        exit(1)

if __name__ == "__main__":
    main()