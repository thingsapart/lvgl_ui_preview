import json
import argparse
import os
import re
from collections import defaultdict

# --- Configuration (Mirroring emul_lvgl where appropriate) ---

# Whitelist should include functions the BUILDER needs to CALL
# This means creation funcs, setters, and lv_obj_add_style
DEFAULT_FUNCTION_WHITELIST = [
    "lv_obj_create", "lv_obj_set_", "lv_obj_add_style",
    "lv_label_create", "lv_label_set_",
    "lv_btn_create", # Add other widget create/set functions as needed
    "lv_img_create", "lv_img_set_",
    "lv_style_init", "lv_style_set_", # Need init for styles, maybe not reset
    "lv_obj_center", # Common positioning helper
    # Add other functions that might be needed to apply state
]
# Blacklist can exclude things the builder definitely won't call
DEFAULT_FUNCTION_BLACKLIST = [
    "lv_obj_get_", "lv_style_get_", "lv_obj_del", "lv_style_reset",
    "lv_obj_draw_", "_lv_", # Exclude getters, deletion, reset, internal funcs
]
# Enums might be needed if emul_lvgl stores names instead of numbers
DEFAULT_ENUM_WHITELIST = ["lv_", "LV_"]
DEFAULT_ENUM_BLACKLIST = []
# Macros are less likely needed by builder unless used in setters it calls
DEFAULT_MACRO_WHITELIST = ["LV_PART_", "LV_STATE_", "LV_ALIGN_"]
DEFAULT_MACRO_BLACKLIST = []

# --- Helper Functions ---

def sanitize_name(name):
    """Remove leading underscores or potential invalid chars for identifiers."""
    if name and name.startswith('_'):
        return name[1:]
    return name if name else "unnamed_arg"

def get_real_c_type(type_info, typedefs_map):
    """Recursively determine the REAL C type string from JSON type info."""
    json_type = type_info.get('json_type')
    name = type_info.get('name')
    quals = type_info.get('quals', [])
    qual_str = " ".join(quals) + (" " if quals else "")

    print('get_real_c_type::', type_info)
    if json_type == 'pointer':
        pointee_type = get_real_c_type(type_info['type'], typedefs_map)
        # Don't add '*' if pointee is already a pointer type (e.g., function pointer)
        if pointee_type.endswith('*') or '(*)' in pointee_type:
             return f"{qual_str}{pointee_type}"
        else:
             # Qualifiers apply to the pointer itself usually
             return f"{pointee_type} *" # Pointer comes after type
    elif json_type == 'array':
        base = get_real_c_type(type_info['type'], typedefs_map) if 'type' in type_info else type_info['name']
        dim = type_info.get('dim', '')
        # Array qualifier handling might be complex, assume base type carries it
        return f"{base}" # Array declared with name: type name[] or type name[dim]
    elif json_type in ['lvgl_type', 'stdlib_type', 'primitive_type', 'enum', 'struct', 'union']:
         # Use the actual name for real LVGL types
         # Check if it's a typedef and resolve if necessary for function signatures
         if name in typedefs_map and typedefs_map[name].get('type',{}).get('json_type'):
             # Let's prefer the typedef name itself if it exists
             return f"{qual_str}{name}"
             # return get_real_c_type(typedefs_map[name]['type'], typedefs_map) # Option: Resolve fully
         else:
             # Handle basic C types correctly
             c_name_map = {
                 "_Bool": "bool",
             }
             c_name = c_name_map.get(name, name)
             # Add struct/enum keyword if necessary based on original json_type?
             # Assume names like lv_color_t, lv_obj_t are defined correctly by including lvgl.h
             return f"{qual_str}{c_name}"
    elif json_type == 'function_pointer':
        ret_type = get_real_c_type(type_info['type'], typedefs_map)
        args = type_info.get('args', [])
        if len(args) == 1 and args[0].get('type',{}).get('name') == 'void':
            args_str = "void"
        else:
            arg_types = [get_real_c_type(arg['type'], typedefs_map) for arg in args]
            args_str = ", ".join(arg_types) if arg_types else "void"
        return f"{qual_str}{ret_type} (*)({args_str})"
    elif json_type == 'typedef':
         # When a typedef itself is the type, use its name
         return f"{qual_str}{name}"
    elif json_type == 'ret_type':
        return get_real_c_type(type_info['type'], typedefs_map)
    elif name == 'void':
        return f"{qual_str}void"
    elif name == '...':
        return "..."

    # Fallback
    print(f"Warning: Unhandled real type: {type_info}")
    return f"{qual_str}{name}" if name else "/* unknown */"


def get_property_name_from_setter(func_name):
    """Derive a JSON property name from a setter function name."""
    # Match lv_obj_set_prop, lv_widget_set_prop, lv_style_set_prop
    m = re.match(r"lv_(?:obj|style|[a-z]+)_set_(\w+)", func_name)
    if m:
        prop = m.group(1)
        # Handle special cases like text_font -> text_font
        return prop
    return None

def matches_filters(name, whitelist, blacklist):
    """Check if a name matches whitelist and not blacklist."""
    if not name: return False
    whitelisted = any(name.startswith(prefix) for prefix in whitelist)
    blacklisted = any(name.startswith(prefix) for prefix in blacklist)
    # Special case: allow lv_style_init even if it doesn't start with lv_style_set_
    if name == "lv_style_init":
        return True
    # Special case: allow lv_obj_add_style
    if name == "lv_obj_add_style":
        return True
    return whitelisted and not blacklisted


# --- Code Generation Functions ---

def generate_builder_header(filters):
    """Generates the content for ui_builder.h"""
    h_content = []
    h_content.append("// Generated by generate_ui_builder.py - DO NOT EDIT\n")
    h_content.append("#ifndef UI_BUILDER_H")
    h_content.append("#define UI_BUILDER_H\n")
    h_content.append("#include \"lvgl.h\" // Include real LVGL header\n")

    h_content.append("// --- Callback Function Types ---")
    h_content.append("/**")
    h_content.append(" * @brief Callback function to look up a registered pointer by name.")
    h_content.append(" * @param name The name used in the JSON (e.g., \"font_roboto_16\", \"icon_settings\").")
    h_content.append(" * @param user_data Custom user data pointer passed to ui_builder_load_json.")
    h_content.append(" * @return The corresponding void* pointer (e.g., lv_font_t*, lv_img_dsc_t*), or NULL if not found.")
    h_content.append(" */")
    h_content.append("typedef const void* (*ui_builder_registry_lookup_cb)(const char *name, void *user_data);\n")

    h_content.append("// --- Main Builder Function ---")
    h_content.append("/**")
    h_content.append(" * @brief Parses a JSON string and builds the described LVGL UI.")
    h_content.append(" *")
    h_content.append(" * @param parent The LVGL object to attach the root of the built UI to.")
    h_content.append(" * @param json_string The JSON string generated by the emul_lvgl library.")
    h_content.append(" * @param lookup_func Callback function to resolve named pointers (fonts, images, etc.).")
    h_content.append(" * @param user_data Custom data to pass to the lookup_func.")
    h_content.append(" * @return true on success, false on failure (e.g., JSON parse error, memory allocation failure).")
    h_content.append(" */")
    h_content.append("bool ui_builder_load_json(lv_obj_t *parent, const char *json_string, ui_builder_registry_lookup_cb lookup_func, void *user_data);")
    h_content.append("\n")

    # --- Optional: Include Enums/Macros if needed for function calls ---
    # This is less critical if emul_lvgl passes numbers, but can be added for clarity/completeness
    # h_content.append("// --- Relevant Enums/Macros (from LVGL) ---")
    # ... Add filtered enum/macro definitions here if necessary ...

    h_content.append("#endif // UI_BUILDER_H")
    return "\n".join(h_content)


def generate_builder_c_source(api_data, filters):
    """Generates the content for ui_builder.c"""
    c_content = []
    typedefs_map = {t['name']: t for t in api_data.get('typedefs', []) if t.get('name')}
    enums_map = {e['name']: e for e in api_data.get('enums', []) if e.get('name')}

    c_content.append("// Generated by generate_ui_builder.py - DO NOT EDIT\n")
    c_content.append('#include "ui_builder.h"')
    c_content.append('#include <string.h>')
    c_content.append('#include <stdlib.h>')
    c_content.append('#include <stdio.h>') # For debug printf, remove later
    c_content.append('#include "cJSON.h"\n')

    c_content.append("// --- Internal ID Mapping (JSON ID -> lv_obj_t* / lv_style_t*) ---")
    c_content.append("typedef struct IdMapEntry {")
    c_content.append("    const char *json_id;")
    c_content.append("    void *lv_ptr;")
    c_content.append("    struct IdMapEntry *next;")
    c_content.append("} IdMapEntry_t;")
    c_content.append("")
    c_content.append("static void id_map_add(IdMapEntry_t **head, const char *json_id, void *lv_ptr) {")
    c_content.append("    if (!json_id || !lv_ptr) return;")
    c_content.append("    IdMapEntry_t *new_entry = (IdMapEntry_t *)malloc(sizeof(IdMapEntry_t));")
    c_content.append("    if (!new_entry) return; // Handle allocation failure")
    c_content.append("    new_entry->json_id = json_id; // cJSON string, owned by cJSON object")
    c_content.append("    new_entry->lv_ptr = lv_ptr;")
    c_content.append("    new_entry->next = *head;")
    c_content.append("    *head = new_entry;")
    c_content.append("}")
    c_content.append("")
    c_content.append("static void* id_map_find(IdMapEntry_t *head, const char *json_id) {")
    c_content.append("    if (!json_id) return NULL;")
    c_content.append("    IdMapEntry_t *current = head;")
    c_content.append("    while (current) {")
    c_content.append("        if (strcmp(current->json_id, json_id) == 0) {")
    c_content.append("            return current->lv_ptr;")
    c_content.append("        }")
    c_content.append("        current = current->next;")
    c_content.append("    }")
    c_content.append("    return NULL;")
    c_content.append("}")
    c_content.append("")
    c_content.append("static void id_map_free(IdMapEntry_t **head) {")
    c_content.append("    IdMapEntry_t *current = *head;")
    c_content.append("    while (current) {")
    c_content.append("        IdMapEntry_t *next = current->next;")
    c_content.append("        // Don't free json_id (owned by cJSON), don't free lv_ptr (owned by LVGL)")
    c_content.append("        free(current);")
    c_content.append("        current = next;")
    c_content.append("    }")
    c_content.append("    *head = NULL;")
    c_content.append("}")
    c_content.append("")

    c_content.append("// --- Helper Functions ---")
    c_content.append("static lv_color_t parse_color_string(const char *color_str) {")
    c_content.append("    if (!color_str || color_str[0] != '#' || strlen(color_str) != 7) {")
    c_content.append("        return lv_color_black(); // Default or error color")
    c_content.append("    }")
    c_content.append("    long color_val = strtol(color_str + 1, NULL, 16);")
    c_content.append("    return lv_color_hex((uint32_t)color_val);")
    c_content.append("}")
    c_content.append("")

    # --- Enum String to Value Mapping (Optional - if emul_lvgl stores names) ---
    # Example: Generate this if needed
    # c_content.append("static int parse_enum_string(const char* enum_str) {")
    # c_content.append("    if (!enum_str) return 0;")
    # enum_map_generated = False
    # for enum in api_data.get('enums', []):
    #      if matches_filters(enum.get('name'), filters['enum_whitelist'], filters['enum_blacklist']):
    #          for member in enum.get('members', []):
    #               m_name = member.get('name')
    #               m_val = member.get('value')
    #               if m_name and m_val is not None:
    #                   c_content.append(f'    if (strcmp(enum_str, "{m_name}") == 0) return {m_val};')
    #                   enum_map_generated = True
    # if not enum_map_generated:
    #      c_content.append("    // No enums matched filters for string mapping")
    # c_content.append("    printf(\"Warning: Unknown enum string: %s\\n\", enum_str);")
    # c_content.append("    return 0; // Default / Error value")
    # c_content.append("}")
    # c_content.append("")
    # ---- NOTE: Assuming emul_lvgl stores numbers for enums to avoid this ----


    # --- Main Builder Function ---
    c_content.append("bool ui_builder_load_json(lv_obj_t *parent, const char *json_string, ui_builder_registry_lookup_cb lookup_func, void *user_data) {")
    c_content.append("    if (!parent || !json_string || !lookup_func) {")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("")
    c_content.append("    cJSON *root = cJSON_Parse(json_string);")
    c_content.append("    if (!root) {")
    c_content.append("        const char *error_ptr = cJSON_GetErrorPtr();")
    c_content.append("        if (error_ptr != NULL) {")
    c_content.append("            fprintf(stderr, \"Error parsing JSON before: %s\\n\", error_ptr);")
    c_content.append("        }")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("")
    c_content.append("    cJSON *j_objects = cJSON_GetObjectItemCaseSensitive(root, \"objects\");")
    c_content.append("    cJSON *j_styles = cJSON_GetObjectItemCaseSensitive(root, \"styles\");")
    c_content.append("    bool success = false; // Assume failure until proven otherwise")
    c_content.append("")
    c_content.append("    IdMapEntry_t *obj_id_map = NULL;")
    c_content.append("    IdMapEntry_t *style_id_map = NULL;")
    c_content.append("")
    c_content.append("    // --- 1. Create Styles ---")
    c_content.append("    if (cJSON_IsObject(j_styles)) {")
    c_content.append("        cJSON *j_style = NULL;")
    c_content.append("        cJSON_ArrayForEach(j_style, j_styles) {")
    c_content.append("            const char *style_id = j_style->string;")
    c_content.append("            if (!style_id) continue;")
    c_content.append("            // Allocate style data on the heap, as LVGL might store pointers to them")
    c_content.append("            lv_style_t *new_style = (lv_style_t*)malloc(sizeof(lv_style_t));")
    c_content.append("            if (!new_style) {")
    c_content.append("                 fprintf(stderr, \"Failed to allocate memory for style %s\\n\", style_id);")
    c_content.append("                 goto cleanup; // Allocation failed")
    c_content.append("            }")
    c_content.append("            lv_style_init(new_style);")
    c_content.append("            id_map_add(&style_id_map, style_id, new_style);")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("")
    c_content.append("    // --- 2. Create Objects (Widgets) ---")
    c_content.append("    if (cJSON_IsObject(j_objects)) {")
    c_content.append("        cJSON *j_obj = NULL;")
    c_content.append("        cJSON_ArrayForEach(j_obj, j_objects) {")
    c_content.append("            const char *obj_id = j_obj->string;")
    c_content.append("            if (!obj_id) continue;")
    c_content.append("")
    c_content.append("            cJSON *j_type = cJSON_GetObjectItemCaseSensitive(j_obj, \"type\");")
    c_content.append("            cJSON *j_parent = cJSON_GetObjectItemCaseSensitive(j_obj, \"parent\");")
    c_content.append("")
    c_content.append("            if (!cJSON_IsString(j_type) || !j_type->valuestring) continue;")
    c_content.append("            const char *type_str = j_type->valuestring;")
    c_content.append("")
    c_content.append("            lv_obj_t *parent_obj = parent; // Default to the main parent")
    c_content.append("            if (cJSON_IsString(j_parent) && j_parent->valuestring) {")
    c_content.append("                parent_obj = (lv_obj_t*)id_map_find(obj_id_map, j_parent->valuestring);")
    c_content.append("                if (!parent_obj) {")
    c_content.append("                    fprintf(stderr, \"Warning: Parent object '%s' not found for object '%s'\\n\", j_parent->valuestring, obj_id);")
    c_content.append("                    parent_obj = parent; // Fallback to main parent")
    c_content.append("                }")
    c_content.append("            }")
    c_content.append("")
    c_content.append("            lv_obj_t *new_obj = NULL;")
    c_content.append("            // --- Widget Creation Dispatcher ---")

    # Generate creation logic
    widget_creators = {}
    for func in api_data.get('functions', []):
        name = func.get('name')
        # Identify creation functions (heuristic: ends with _create, returns lv_obj_t*)
        ret_type_info = func.get('type', {}).get('type', {}).get('type', {}) # ret_type -> pointer -> lvgl_type
        ret_type_name = ret_type_info.get('name')
        is_creator = name and name.endswith('_create') and ret_type_name == 'lv_obj_t'
        if is_creator and matches_filters(name, filters['function_whitelist'], filters['function_blacklist']):
            widget_creators[name] = name # Map JSON type string to C function name

    first_creator = True
    for type_str, func_name in widget_creators.items():
        if_cond = "if" if first_creator else "else if"
        c_content.append(f"            {if_cond} (strcmp(type_str, \"{type_str}\") == 0) {{")
        c_content.append(f"                new_obj = {func_name}(parent_obj);")
        c_content.append(f"            }}")
        first_creator = False
    if not first_creator: # If any creators were added
         c_content.append(f"            else {{")
         c_content.append(f"                fprintf(stderr, \"Warning: Unsupported widget type: %s\\n\", type_str);")
         c_content.append(f"                // Fallback: Create a basic lv_obj_t")
         c_content.append(f"                new_obj = lv_obj_create(parent_obj);")
         c_content.append(f"            }}")
    else:
         c_content.append(f"            // Fallback: Create a basic lv_obj_t (no specific creators matched filters)")
         c_content.append(f"            new_obj = lv_obj_create(parent_obj);")


    c_content.append("")
    c_content.append("            if (new_obj) {")
    c_content.append("                id_map_add(&obj_id_map, obj_id, new_obj);")
    c_content.append("            } else {")
    c_content.append("                 fprintf(stderr, \"Failed to create object %s of type %s\\n\", obj_id, type_str);")
    c_content.append("                 goto cleanup; // Critical failure")
    c_content.append("            }")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("")

    # --- 3. Apply Style Properties ---
    c_content.append("    // --- 3. Apply Style Properties ---")
    c_content.append("    if (cJSON_IsObject(j_styles)) {")
    c_content.append("        cJSON *j_style = NULL;")
    c_content.append("        cJSON_ArrayForEach(j_style, j_styles) {")
    c_content.append("            const char *style_id = j_style->string;")
    c_content.append("            lv_style_t *style_ptr = (lv_style_t*)id_map_find(style_id_map, style_id);")
    c_content.append("            if (!style_ptr) continue;")
    c_content.append("")
    c_content.append("            cJSON *j_props = cJSON_GetObjectItemCaseSensitive(j_style, \"properties\");")
    c_content.append("            if (cJSON_IsObject(j_props)) {")
    c_content.append("                cJSON *j_prop = NULL;")
    c_content.append("                cJSON_ArrayForEach(j_prop, j_props) {")
    c_content.append("                    const char *prop_name = j_prop->string;")
    c_content.append("                    if (!prop_name) continue;")
    c_content.append("")
    c_content.append("                    // --- Style Property Setter Dispatcher ---")

    # Generate style property setting logic
    style_setters = defaultdict(list) # prop_name -> list of funcs (handle potential conflicts later if needed)
    for func in api_data.get('functions', []):
        name = func.get('name')
        args = func.get('args', [])
        if name and name.startswith('lv_style_set_') and len(args) >= 2: # style*, prop_name, value...
            if matches_filters(name, filters['function_whitelist'], filters['function_blacklist']):
                prop_name = get_property_name_from_setter(name)
                if prop_name:
                     # Get the type of the value argument (usually the last one)
                     value_arg_info = args[-1]
                     value_arg_type = get_real_c_type(value_arg_info['type'], typedefs_map)
                     style_setters[prop_name].append({
                         "func_name": name,
                         "value_type": value_arg_type,
                         "value_info": value_arg_info['type'] # Keep original type info
                     })

    first_setter = True
    for prop_name, funcs in style_setters.items():
        if not funcs: continue
        # Simple approach: use the first matching function found
        setter = funcs[0]
        func_name = setter["func_name"]
        value_type = setter["value_type"]
        value_info = setter["value_info"]

        if_cond = "if" if first_setter else "else if"
        c_content.append(f"                    {if_cond} (strcmp(prop_name, \"{prop_name}\") == 0) {{")

        # Generate code to parse j_prop value based on value_type
        if "lv_color_t" in value_type and "*" not in value_type:
            c_content.append(f"                        if (cJSON_IsString(j_prop)) {{")
            c_content.append(f"                            lv_color_t color = parse_color_string(j_prop->valuestring);")
            c_content.append(f"                            {func_name}(style_ptr, color);")
            c_content.append(f"                        }}")
        elif any(t in value_type for t in ["int", "short", "long", "lv_coord_t", "lv_opa_t", "uint", "size_t"]) and "*" not in value_type:
             # Includes enums assumed to be numbers
            c_content.append(f"                        if (cJSON_IsNumber(j_prop)) {{")
            c_content.append(f"                            {func_name}(style_ptr, ({value_type})cJSON_GetNumberValue(j_prop));")
            c_content.append(f"                        }}")
        elif "bool" in value_type and "*" not in value_type:
             c_content.append(f"                        if (cJSON_IsBool(j_prop)) {{")
             c_content.append(f"                            {func_name}(style_ptr, cJSON_IsTrue(j_prop) ? true : false);")
             c_content.append(f"                        }}")
        elif "const char *" in value_type: # Check for string pointer type
             c_content.append(f"                        if (cJSON_IsString(j_prop)) {{")
             c_content.append(f"                            {func_name}(style_ptr, j_prop->valuestring);")
             c_content.append(f"                        }}")
        elif "*" in value_type and ("lv_font_t" in value_type or "lv_img_dsc_t" in value_type or "void" in value_type): # Heuristic for named pointers
             c_content.append(f"                        if (cJSON_IsString(j_prop)) {{")
             c_content.append(f"                            const void* ptr = lookup_func(j_prop->valuestring, user_data);")
             c_content.append(f"                            if (ptr) {{")
             c_content.append(f"                                {func_name}(style_ptr, ({value_type})ptr);")
             c_content.append(f"                            }} else {{")
             c_content.append(f"                                fprintf(stderr, \"Warning: Named pointer '%s' not found for style property '%s'\\n\", j_prop->valuestring, prop_name);")
             c_content.append(f"                            }}")
             c_content.append(f"                        }}")
        else:
             c_content.append(f"                        // TODO: Handle style property type '{value_type}' for '{prop_name}'")


        c_content.append(f"                    }}")
        first_setter = False

    if not first_setter:
         c_content.append(f"                    else {{")
         c_content.append(f"                        fprintf(stderr, \"Warning: Unsupported style property: %s\\n\", prop_name);")
         c_content.append(f"                    }}")


    c_content.append("                }")
    c_content.append("            }")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("")

    # --- 4. Apply Object Properties ---
    c_content.append("    // --- 4. Apply Object Properties ---")
    c_content.append("    if (cJSON_IsObject(j_objects)) {")
    c_content.append("        cJSON *j_obj = NULL;")
    c_content.append("        cJSON_ArrayForEach(j_obj, j_objects) {")
    c_content.append("            const char *obj_id = j_obj->string;")
    c_content.append("            lv_obj_t *obj_ptr = (lv_obj_t*)id_map_find(obj_id_map, obj_id);")
    c_content.append("            if (!obj_ptr) continue;")
    c_content.append("")
    c_content.append("            cJSON *j_props = cJSON_GetObjectItemCaseSensitive(j_obj, \"properties\");")
    c_content.append("            if (cJSON_IsObject(j_props)) {")
    c_content.append("                cJSON *j_prop = NULL;")
    c_content.append("                cJSON_ArrayForEach(j_prop, j_props) {")
    c_content.append("                    const char *prop_name = j_prop->string;")
    c_content.append("                    if (!prop_name) continue;")
    c_content.append("")
    c_content.append("                     // --- Object Property Setter Dispatcher ---")


    # Generate object property setting logic (similar to styles)
    obj_setters = defaultdict(list) # prop_name -> list of funcs
    for func in api_data.get('functions', []):
        name = func.get('name')
        args = func.get('args', [])
        # Match lv_obj_set_*, lv_widget_set_* but NOT lv_style_set_*
        is_obj_setter = name and ("_set_" in name) and not name.startswith('lv_style_set_') and len(args) >= 1
        if is_obj_setter and args[0].get('type', {}).get('type', {}).get('name') == 'lv_obj_t': # First arg must be lv_obj_t*
             if matches_filters(name, filters['function_whitelist'], filters['function_blacklist']):
                prop_name = get_property_name_from_setter(name)
                if prop_name:
                     value_arg_info = args[-1] # Assume value is last arg
                     value_arg_type = get_real_c_type(value_arg_info['type'], typedefs_map)
                     obj_setters[prop_name].append({
                         "func_name": name,
                         "value_type": value_arg_type,
                         "value_info": value_arg_info['type']
                     })

    first_setter = True
    for prop_name, funcs in obj_setters.items():
        if not funcs: continue
        # Simple approach: use the first matching function found
        # TODO: Could potentially use object type hint from JSON to resolve ambiguities
        setter = funcs[0]
        func_name = setter["func_name"]
        value_type = setter["value_type"]
        value_info = setter["value_info"]

        if_cond = "if" if first_setter else "else if"
        c_content.append(f"                    {if_cond} (strcmp(prop_name, \"{prop_name}\") == 0) {{")

        # Generate code to parse j_prop value based on value_type
        if "lv_color_t" in value_type and "*" not in value_type:
            c_content.append(f"                        if (cJSON_IsString(j_prop)) {{")
            c_content.append(f"                            lv_color_t color = parse_color_string(j_prop->valuestring);")
            c_content.append(f"                            {func_name}(obj_ptr, color);") # Assumes func takes obj*, color
            c_content.append(f"                        }}")
        elif any(t in value_type for t in ["int", "short", "long", "lv_coord_t", "lv_opa_t", "uint", "size_t"]) and "*" not in value_type:
             # Includes enums assumed to be numbers
            c_content.append(f"                        if (cJSON_IsNumber(j_prop)) {{")
            c_content.append(f"                            {func_name}(obj_ptr, ({value_type})cJSON_GetNumberValue(j_prop));")
            c_content.append(f"                        }}")
        elif "bool" in value_type and "*" not in value_type:
             c_content.append(f"                        if (cJSON_IsBool(j_prop)) {{")
             c_content.append(f"                            {func_name}(obj_ptr, cJSON_IsTrue(j_prop) ? true : false);")
             c_content.append(f"                        }}")
        elif "const char *" in value_type: # Check for string pointer type
             c_content.append(f"                        if (cJSON_IsString(j_prop)) {{")
             c_content.append(f"                            {func_name}(obj_ptr, j_prop->valuestring);")
             c_content.append(f"                        }}")
        elif "*" in value_type and ("lv_font_t" in value_type or "lv_img_dsc_t" in value_type or "void" in value_type): # Heuristic for named pointers
             c_content.append(f"                        if (cJSON_IsString(j_prop)) {{")
             c_content.append(f"                            const void* ptr = lookup_func(j_prop->valuestring, user_data);")
             c_content.append(f"                            if (ptr) {{")
             c_content.append(f"                                {func_name}(obj_ptr, ({value_type})ptr);")
             c_content.append(f"                            }} else {{")
             c_content.append(f"                                fprintf(stderr, \"Warning: Named pointer '%s' not found for object property '%s'\\n\", j_prop->valuestring, prop_name);")
             c_content.append(f"                            }}")
             c_content.append(f"                        }}")
        else:
             c_content.append(f"                        // TODO: Handle object property type '{value_type}' for '{prop_name}'")

        c_content.append(f"                    }}")
        first_setter = False

    if not first_setter:
         c_content.append(f"                    else {{")
         c_content.append(f"                        fprintf(stderr, \"Warning: Unsupported object property: %s\\n\", prop_name);")
         c_content.append(f"                    }}")


    c_content.append("                }")
    c_content.append("            }")
    c_content.append("")
    c_content.append("            // --- 5. Apply Styles to Object ---")
    c_content.append("            cJSON *j_obj_styles = cJSON_GetObjectItemCaseSensitive(j_obj, \"styles\");")
    c_content.append("            if (cJSON_IsObject(j_obj_styles)) {")
    c_content.append("                cJSON *j_part_styles = NULL;")
    c_content.append("                cJSON_ArrayForEach(j_part_styles, j_obj_styles) {")
    c_content.append("                    const char *part_key = j_part_styles->string; // e.g., \"part_0\"")
    c_content.append("                    if (!part_key || strncmp(part_key, \"part_\", 5) != 0) continue;")
    c_content.append("                    lv_part_t part_id = (lv_part_t)atoi(part_key + 5);")
    c_content.append("")
    c_content.append("                    if (cJSON_IsArray(j_part_styles)) {")
    c_content.append("                        cJSON *j_style_id_item = NULL;")
    c_content.append("                        cJSON_ArrayForEach(j_style_id_item, j_part_styles) {")
    c_content.append("                            if (cJSON_IsString(j_style_id_item) && j_style_id_item->valuestring) {")
    c_content.append("                                const char *style_id_str = j_style_id_item->valuestring;")
    c_content.append("                                lv_style_t *style_ptr = (lv_style_t*)id_map_find(style_id_map, style_id_str);")
    c_content.append("                                if (style_ptr) {")
    c_content.append("                                    lv_obj_add_style(obj_ptr, style_ptr, part_id);")
    c_content.append("                                } else {")
    c_content.append("                                    fprintf(stderr, \"Warning: Style '%s' not found when applying to object '%s' part %d\\n\", style_id_str, obj_id, (int)part_id);")
    c_content.append("                                }")
    c_content.append("                            }")
    c_content.append("                        }")
    c_content.append("                    }")
    c_content.append("                }")
    c_content.append("            }")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("")
    c_content.append("    success = true; // If we reached here without critical errors")
    c_content.append("")
    c_content.append("cleanup:")
    c_content.append("    id_map_free(&obj_id_map);")
    c_content.append("    // IMPORTANT: Styles allocated with malloc need to be freed eventually!")
    c_content.append("    //            The caller needs a strategy for this. We free the map only.")
    c_content.append("    //            Consider adding a function `ui_builder_free_styles(style_map)`?")
    c_content.append("    id_map_free(&style_id_map); ")
    c_content.append("    cJSON_Delete(root);")
    c_content.append("    return success;")
    c_content.append("}")
    c_content.append("")


    return "\n".join(c_content)


# --- Main Execution ---

def main():
    parser = argparse.ArgumentParser(description="Generate LVGL C UI Builder library from JSON API description.")
    parser.add_argument("json_api_file", help="Path to the LVGL API JSON file.")
    parser.add_argument("-o", "--output-dir", default=".", help="Directory to output generated files (ui_builder.h, ui_builder.c).")
    # Add filter arguments similar to the other script
    parser.add_argument("--func-wl", nargs='+', default=DEFAULT_FUNCTION_WHITELIST, help="Prefixes for LVGL functions the builder might call.")
    parser.add_argument("--func-bl", nargs='+', default=DEFAULT_FUNCTION_BLACKLIST, help="Prefixes for LVGL functions to exclude.")
    parser.add_argument("--enum-wl", nargs='+', default=DEFAULT_ENUM_WHITELIST, help="Prefixes for enums (potentially needed for mapping).")
    parser.add_argument("--enum-bl", nargs='+', default=DEFAULT_ENUM_BLACKLIST, help="Prefixes for enums to exclude.")
    parser.add_argument("--macro-wl", nargs='+', default=DEFAULT_MACRO_WHITELIST, help="Prefixes for macros (potentially needed for mapping).")
    parser.add_argument("--macro-bl", nargs='+', default=DEFAULT_MACRO_BLACKLIST, help="Prefixes for macros to exclude.")

    args = parser.parse_args()

    # Load the JSON API data
    try:
        with open(args.json_api_file, 'r') as f:
            api_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: JSON API file not found at '{args.json_api_file}'")
        return
    except json.JSONDecodeError:
        print(f"Error: Could not parse JSON file '{args.json_api_file}'")
        return

    # Prepare filters
    filters = {
        "function_whitelist": args.func_wl,
        "function_blacklist": args.func_bl,
        "enum_whitelist": args.enum_wl,
        "enum_blacklist": args.enum_bl,
        "macro_whitelist": args.macro_wl,
        "macro_blacklist": args.macro_bl,
    }

    # Generate Header Content
    print("Generating ui_builder.h...")
    header_content = generate_builder_header(filters)

    # Generate Source Content
    print("Generating ui_builder.c...")
    source_content = generate_builder_c_source(api_data, filters)

    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)

    # Write files
    header_path = os.path.join(args.output_dir, "ui_builder.h")
    source_path = os.path.join(args.output_dir, "ui_builder.c")

    try:
        with open(header_path, 'w') as f:
            f.write(header_content)
        print(f"Successfully wrote {header_path}")
    except IOError as e:
        print(f"Error writing header file: {e}")

    try:
        with open(source_path, 'w') as f:
            f.write(source_content)
        print(f"Successfully wrote {source_path}")
    except IOError as e:
        print(f"Error writing source file: {e}")

if __name__ == "__main__":
    main()
