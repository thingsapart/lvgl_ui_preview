import json
import argparse
import os
import re
from collections import defaultdict

# --- Configuration ---

DEFAULT_FUNCTION_WHITELIST = [
    "lv_obj_create", "lv_obj_del", "lv_obj_set_", "lv_obj_add_style",
    "lv_obj_remove_style", "lv_obj_get_state", # state is often needed
    "lv_label_create", "lv_label_set_",
    # Also add these setters near the comment '# --- Section: setter functions generation ---'
    "lv_button_create", "lv_button_set_",
    "lv_bar_create", "lv_bar_set_",
    "lv_scale_create", "lv_scale_set_",
    "lv_image_create", "lv_img_set_",
    "lv_style_init", "lv_style_reset", "lv_style_set_",
    "lv_obj_align", "lv_obj_add_flag", "lv_obj_clear_flag"
    # Add other widget create/set functions as needed
]
DEFAULT_FUNCTION_BLACKLIST = ["lv_obj_get_", "lv_style_get_", "lv_obj_draw_"] # Ignore most getters and drawing funcs
DEFAULT_ENUM_WHITELIST = ["lv_", "LV_"]
DEFAULT_ENUM_BLACKLIST = []
DEFAULT_MACRO_WHITELIST = ["LV_", "lv_btn_create", "lv_img_create"]
DEFAULT_MACRO_BLACKLIST = []
DEFAULT_STRUCT_WHITELIST = ["lv_style_", "lv_scale_section_t",
                            "lv_anim_enable_t"] # Only include structs if absolutely necessary
DEFAULT_STRUCT_BLACKLIST = ["_"] # Exclude private structs

# --- Helper Functions ---

def sanitize_name(name):
    """Remove leading underscores or potential invalid chars for identifiers."""
    if name and name.startswith('_'):
        return name[1:]
    return name if name else "unnamed_arg"

def get_c_type(type_info, structs, enums, typedefs, base_type_only=False):
    """Recursively determine the C type string from JSON type info."""
    json_type = type_info.get('json_type')
    name = type_info.get('name')
    quals = type_info.get('quals', [])
    qual_str = " ".join(quals) + (" " if quals else "")

    # --- Handle specific LVGL types first ---
    if name == "lv_obj_t":
        return f"{qual_str}lv_obj_t" if not base_type_only else "lv_obj_t"
    if name == "lv_style_t":
         return f"{qual_str}lv_style_t" if not base_type_only else "lv_style_t"
    if not 'type' in type_info:
        return name

    # --- Recursive/Complex types ---
    if json_type == 'pointer':
        if base_type_only: # If asking for base type of a pointer, recurse
             return get_c_type(type_info['type'], structs, enums, typedefs, base_type_only=True)
        pointee_type = get_c_type(type_info['type'], structs, enums, typedefs)
        # Heuristic: Identify pointers likely needing named registration
        # Pointers to non-obj/style structs, fonts, images, etc.
        base_pointee = get_c_type(type_info['type'], structs, enums, typedefs, base_type_only=True)
        if not pointee_type.endswith("*") and \
           base_pointee not in ["void", "char", "lv_obj_t", "lv_style_t"] and \
           not base_pointee.startswith("lv_event"): # Exclude event pointers
             # Assume it might need registration - could be refined
             # In C code, we'll treat all void* and char* specially anyway
             pass # Keep as regular pointer for signature

        return f"{pointee_type} *" # Pointer comes after type
    elif json_type == 'array':
        base = get_c_type(type_info['type'], structs, enums, typedefs)
        dim = type_info.get('dim', '')
        return f"{qual_str}{base}" # Array declared with name: type name[] or type name[dim]
    elif json_type == 'lvgl_type' or json_type == 'stdlib_type' or json_type == 'primitive_type':
        # Check if it's a typedef'd pointer (like lv_font_t *)
        if name in typedefs:
             # If the underlying type of the typedef is a pointer, handle it
             underlying_type_info = typedefs[name].get('type', {})
             if underlying_type_info.get('json_type') == 'pointer':
                 # Check if it's obj or style
                  pointee_type_info = underlying_type_info.get('type', {})
                  pointee_name = pointee_type_info.get('name')
                  if pointee_name == "lv_obj_t":
                      return f"{qual_str}lv_obj_t" if not base_type_only else "lv_obj_t"
                  if pointee_name == "lv_style_t":
                      return f"{qual_str}lv_style_t" if not base_type_only else "lv_style_t"
                  # Otherwise, treat as potential named pointer type for signature
                  # Let the C code handle the void* lookup
                  c_type = get_c_type(underlying_type_info, structs, enums, typedefs)
                  return f"{qual_str}{c_type}" if not base_type_only else c_type

        # Handle common typedefs that resolve to basic types
        c_name_map = {
            "lv_color_t": "uint32_t", # Assuming common config, adjust if needed
            "lv_opa_t": "uint8_t",
            "lv_coord_t": "int16_t",
            "uint8_t": "uint8_t",
            "uint16_t": "uint16_t",
            "uint32_t": "uint32_t",
            "int8_t": "int8_t",
            "int16_t": "int16_t",
            "int32_t": "int32_t",
            "bool": "bool", # Requires <stdbool.h>
            "_Bool": "bool",
        }
        # Use original name if not mapped or special type
        c_name = c_name_map.get(name, name)
        return f"{qual_str}{c_name}"
    elif json_type == 'struct' or json_type == 'union':
        return f"{qual_str}struct {name}" if name else f"{qual_str}{type_info.get('type',{}).get('name')}"
    elif json_type == 'enum':
        return f"{qual_str}enum {name}" if name else f"{qual_str}int" # Fallback for anonymous enums
    elif json_type == 'function_pointer':
        ret_type = get_c_type(type_info['type'], structs, enums, typedefs)
        args_str = ", ".join(get_c_type(arg['type'], structs, enums, typedefs) for arg in type_info['args'])
        return f"{qual_str}{ret_type} (*)({args_str})"
    elif json_type == 'typedef':
         # Resolve the typedef to its underlying type for function signatures
         return get_c_type(type_info['type'], structs, enums, typedefs, base_type_only=base_type_only)
    elif json_type == 'ret_type':
        return get_c_type(type_info['type'], structs, enums, typedefs, base_type_only=base_type_only)
    elif name == 'void':
        return f"{qual_str}void"
    elif name == '...':
        return "..."

    # Fallback
    print(f"Warning: Unhandled type: {type_info}")
    return f"{qual_str}{name}" if name else "/* unknown */"

def get_property_name(func_name):
    """Derive a JSON property name from a setter function name."""
    m = re.match(r"lv_(?:obj|style|label|img|btn|...)_set_(\w+)", func_name)
    if m:
        return m.group(1)
    return None

def matches_filters(name, whitelist, blacklist):
    """Check if a name matches whitelist and not blacklist."""
    if not name: return False
    whitelisted = any(name.startswith(prefix) for prefix in whitelist)
    blacklisted = any(name.startswith(prefix) for prefix in blacklist)
    #print(f"FILTER {name} => yes: {whitelisted} ({name} {whitelist}), no: {blacklisted}")
    return whitelisted and not blacklisted

# --- Code Generation Functions ---

def generate_header(api_data, filters):
    """Generates the content for emul_lvgl.h"""
    h_content = []
    structs = {s['name']: s for s in api_data.get('structures', []) if s.get('name')}
    enums = {e['name']: e for e in api_data.get('enums', []) if e.get('name')}
    typedefs = {t['name']: t for t in api_data.get('typedefs', []) if t.get('name')}

    h_content.append("// Generated by generate_emul_lvgl.py - DO NOT EDIT\n")
    h_content.append("#ifndef EMUL_LVGL_H")
    h_content.append("#define EMUL_LVGL_H\n")
    h_content.append("#include <stdint.h>")
    h_content.append("#include <stdbool.h>")
    h_content.append("#include <stddef.h>") # For size_t etc.
    h_content.append('#include "cJSON.h" // Requires cJSON library\n')
    h_content.append('#include "lv_const.h" // Other LVGL typedes, macro defines and structs\n')

    h_content.append("// --- Opaque types ---")
    h_content.append("typedef void lv_obj_t;")
    h_content.append("typedef void lv_style_t;\n")

    # --- Enums ---
    h_content.append("// --- Enums ---")
    for enum in api_data.get('enums', []):
        enum_name = enum.get('name')
        if not enum_name:
            members = enum.get('members')
            if hasattr(members, '__len__') and len(members) > 0:
                member0 = members[0]
                ty = members[0].get('type')
                if ty.get('json_type') == 'lvgl_type':
                    enum_name = ty.get('name')

        if matches_filters(enum_name, filters['enum_whitelist'], filters['enum_blacklist']):
            h_content.append(f"// Enum: {enum_name}")
            # Use typedef name if available, otherwise anonymous enum name
            type_name = enum_name if enum_name and not enum_name.startswith('_') else None
            if type_name:
                 h_content.append(f"typedef enum {{")
            else:
                 enum_tag = enum_name if enum_name else f"emul_anon_enum_{len(h_content)}"
                 h_content.append(f"enum {enum_tag} {{")

            for member in enum.get('members', []):
                member_name = member.get('name')
                member_value = member.get('value')
                if member_name and member_value is not None:
                    h_content.append(f"    {member_name} = {member_value},")

            if type_name:
                h_content.append(f"}} {type_name};\n")
            else:
                 h_content.append(f"}};\n") # No typedef name
        elif enum_name and any(f.startswith(enum_name.replace("_t","")+"_") for f in filters['function_whitelist']):
             # If an enum is used by a whitelisted function, add a forward decl hint
             pass # Let C compiler handle implicit int types for now if def is missing

    # --- Macros ---
    # Note: Macro values are not available in the JSON, only names.
    # We can define them, but often without a value, or with a placeholder.
    h_content.append("// --- Macros ---")
    for macro in api_data.get('macros', []):
        macro_name = macro.get('name')
        if matches_filters(macro_name, filters['macro_whitelist'], filters['macro_blacklist']):
            if macro.get('params'):
                params = ', '.join(macro.get('params'))
                h_content.append(f"#define {macro_name}({params}) {macro.get('initializer')}")
            else:
                h_content.append(f"#define {macro_name} {macro.get('initializer')}")
            # Heuristic: Guess common macro types if possible
            #if "WIDTH" in macro_name or "HEIGHT" in macro_name:
            #    h_content.append(f"#define {macro_name} 0 // Placeholder value")
            #elif "RADIUS" in macro_name:
            #     h_content.append(f"#define {macro_name} 0 // Placeholder value")
            #elif "COLOR" in macro_name:
            #     h_content.append(f"#define {macro_name} 0 // Placeholder value (needs mapping)")
            #elif "ALIGN" in macro_name:
            #     h_content.append(f"#define {macro_name} 0 // Placeholder value (use enum instead)")
            #elif macro_name.startswith("LV_STATE_"):
            #    h_content.append(f"#define {macro_name} (1 << {len(h_content) % 16}) // Placeholder state bit")
            #elif macro_name.startswith("LV_PART_"):
            #    h_content.append(f"#define {macro_name} {len(h_content) % 16} // Placeholder part ID")
            #else:
            #    h_content.append(f"// #define {macro_name} ? // Value unknown from JSON")
            #    pass # Avoid defining macros with unknown values by default
    h_content.append("")

    # --- Basic Struct Forward Declarations (if needed) ---
    # Usually not needed as we use opaque pointers, but maybe for some args
    # h_content.append("// --- Struct Forward Declarations ---")
    # for struct in api_data.get('structures', []):
    #      s_name = struct.get('name')
    #      if matches_filters(s_name, filters['struct_whitelist'], filters['struct_blacklist']):
    #           h_content.append(f"struct {s_name};")
    # h_content.append("")

    h_content.append('#include "lv_structs.h"')
    h_content.append("")

    h_content.append("// --- Emulator Specific Functions ---")
    h_content.append("void emul_lvgl_init(void);")
    h_content.append("void emul_lvgl_deinit(void);")
    h_content.append("char* emul_lvgl_render_to_json(void); // Caller must free the returned string")
    h_content.append("void emul_lvgl_register_named_pointer(const void *ptr, const char *name);")
    h_content.append("// Helper to get internal obj ID (for debugging/linking)")
    h_content.append("const char* emul_lvgl_get_obj_id(lv_obj_t* obj);")
    h_content.append("const char* emul_lvgl_get_style_id(lv_style_t* style);\n")


    h_content.append("// --- Emulated LVGL Functions ---")
    for func in api_data.get('functions', []):
        func_name = func.get('name')
        if matches_filters(func_name, filters['function_whitelist'], filters['function_blacklist']):
            ret_type_info = func.get('type', {})
            ret_type = get_c_type(ret_type_info, structs, enums, typedefs)

            args = []
            arg_infos = func.get('args', [])
            # Handle void arguments
            if len(arg_infos) == 1 and arg_infos[0].get('type',{}).get('name') == 'void':
                 args_str = "void"
            else:
                arg_list = []
                for i, arg in enumerate(arg_infos):
                    arg_name = sanitize_name(arg.get('name')) if arg.get('name') else f"arg{i}"
                    #print("!!", arg['type'])
                    arg_type = get_c_type(arg['type'], structs, enums, typedefs)
                    # Handle array syntax in args: type name[]
                    if arg['type'].get('json_type') == 'array':
                         dim = arg['type'].get('dim', '') or ''
                         arg_list.append(f"{arg_type} {arg_name}[{dim}]")
                    elif arg_type == "...":
                         arg_list.append("...")
                    elif arg_name == '...':
                         arg_list.append("...")
                    else:
                         arg_list.append(f"{arg_type} {arg_name}")
                args_str = ", ".join(arg_list) if arg_list else "void"

            h_content.append(f"{ret_type} {func_name}({args_str});")

    h_content.append("\n#endif // EMUL_LVGL_H")
    return "\n".join(h_content)

def generate_c_source(api_data, filters):
    """Generates the content for emul_lvgl.c"""
    c_content = []
    structs = {s['name']: s for s in api_data.get('structures', []) if s.get('name')}
    enums = {e['name']: e for e in api_data.get('enums', []) if e.get('name')}
    typedefs = {t['name']: t for t in api_data.get('typedefs', []) if t.get('name')}


    c_content.append("// Generated by generate_emul_lvgl.py - DO NOT EDIT\n")
    c_content.append('#include "emul_lvgl.h"')
    c_content.append('#include <stdio.h>')
    c_content.append('#include <stdlib.h>')
    c_content.append('#include <string.h>')
    c_content.append('#include "cJSON.h"\n')

    c_content.append("// --- Global State ---")
    c_content.append("static cJSON *g_emul_root = NULL;")
    c_content.append("static cJSON *g_objects = NULL;")
    c_content.append("static cJSON *g_styles = NULL;")
    c_content.append("static long g_obj_counter = 0;")
    c_content.append("static long g_style_counter = 0;\n")

    c_content.append("// --- Internal Object/Style Representation ---")
    c_content.append("typedef struct {")
    c_content.append("    char id[32]; // Sufficient for obj_ID or style_ID")
    c_content.append("} emul_internal_obj_t;")
    c_content.append("typedef emul_internal_obj_t emul_internal_style_t;\n")

    c_content.append("// --- Named Pointer Registry (Simple Linked List) ---")
    c_content.append("typedef struct NamedPointer {")
    c_content.append("    const void *ptr;")
    c_content.append("    const char *name;")
    c_content.append("    struct NamedPointer *next;")
    c_content.append("} NamedPointer_t;")
    c_content.append("static NamedPointer_t *g_named_pointers_head = NULL;\n")

    c_content.append("void emul_lvgl_register_named_pointer(const void *ptr, const char *name) {")
    c_content.append("    NamedPointer_t *new_node = (NamedPointer_t*)malloc(sizeof(NamedPointer_t));")
    c_content.append("    if (!new_node) return; // Handle allocation failure")
    c_content.append("    // Store copies of the name string")
    c_content.append("    char* name_copy = strdup(name);")
    c_content.append("    if (!name_copy) { free(new_node); return; } // Handle alloc failure")
    c_content.append("    new_node->ptr = ptr;")
    c_content.append("    new_node->name = name_copy;")
    c_content.append("    new_node->next = g_named_pointers_head;")
    c_content.append("    g_named_pointers_head = new_node;")
    c_content.append("}")
    c_content.append("")

    c_content.append("static const char* find_registered_name(const void *ptr) {")
    c_content.append("    if (!ptr) return NULL;")
    c_content.append("    NamedPointer_t *current = g_named_pointers_head;")
    c_content.append("    while (current) {")
    c_content.append("        if (current->ptr == ptr) {")
    c_content.append("            return current->name;")
    c_content.append("        }")
    c_content.append("        current = current->next;")
    c_content.append("    }")
    c_content.append("    // If not found, return address as string for debugging")
    c_content.append("    static char addr_str[20];")
    c_content.append("    snprintf(addr_str, sizeof(addr_str), \"%p\", ptr);")
    c_content.append("    return addr_str;")
    c_content.append("}\n")

    c_content.append("static void free_named_pointers() {")
    c_content.append("    NamedPointer_t *current = g_named_pointers_head;")
    c_content.append("    while (current) {")
    c_content.append("        NamedPointer_t *next = current->next;")
    c_content.append("        free((void*)current->name); // Free the copied name string")
    c_content.append("        free(current);")
    c_content.append("        current = next;")
    c_content.append("    }")
    c_content.append("    g_named_pointers_head = NULL;")
    c_content.append("}\n")


    c_content.append("// --- Helper Functions ---")
    c_content.append("static cJSON* get_obj_json(lv_obj_t* obj) {")
    c_content.append("    if (!obj || !g_objects) return NULL;")
    c_content.append("    return cJSON_GetObjectItemCaseSensitive(g_objects, ((emul_internal_obj_t*)obj)->id);")
    c_content.append("}")
    c_content.append("")
    c_content.append("static cJSON* get_style_json(lv_style_t* style) {")
    c_content.append("    if (!style || !g_styles) return NULL;")
    c_content.append("    return cJSON_GetObjectItemCaseSensitive(g_styles, ((emul_internal_style_t*)style)->id);")
    c_content.append("}")
    c_content.append("")
    c_content.append("static cJSON* get_or_create_object_property_node(cJSON* obj_node, const char* prop_key) {")
    c_content.append("    if (!obj_node) return NULL;")
    c_content.append("    cJSON* props = cJSON_GetObjectItemCaseSensitive(obj_node, \"properties\");")
    c_content.append("    if (!props) {")
    c_content.append("        props = cJSON_AddObjectToObject(obj_node, \"properties\");")
    c_content.append("        if (!props) return NULL; // Allocation failed")
    c_content.append("    }")
    c_content.append("    return props;")
    c_content.append("}")
    c_content.append("")
    c_content.append("static cJSON* get_or_create_style_property_node(cJSON* style_node, const char* prop_key) {")
    c_content.append("    if (!style_node) return NULL;")
    c_content.append("    cJSON* props = cJSON_GetObjectItemCaseSensitive(style_node, \"properties\");")
    c_content.append("    if (!props) {")
    c_content.append("        props = cJSON_AddObjectToObject(style_node, \"properties\");")
    c_content.append("        if (!props) return NULL; // Allocation failed")
    c_content.append("    }")
    c_content.append("    return props;")
    c_content.append("}")
    c_content.append("")

    # --- Emulator Specific Functions ---
    c_content.append("// --- Emulator Specific Functions ---")
    c_content.append("void emul_lvgl_init(void) {")
    c_content.append("    if (g_emul_root) {")
    c_content.append("        cJSON_Delete(g_emul_root);")
    c_content.append("        free_named_pointers();")
    c_content.append("    }")
    c_content.append("    g_emul_root = cJSON_CreateObject();")
    c_content.append("    g_objects = cJSON_AddObjectToObject(g_emul_root, \"objects\");")
    c_content.append("    g_styles = cJSON_AddObjectToObject(g_emul_root, \"styles\");")
    c_content.append("    // Add other top-level nodes if needed, e.g., registered pointers info")
    c_content.append("    cJSON_AddObjectToObject(g_emul_root, \"registered_pointers_debug\"); ")
    c_content.append("    g_obj_counter = 0;")
    c_content.append("    g_style_counter = 0;")
    c_content.append("    g_named_pointers_head = NULL;")
    c_content.append("}")
    c_content.append("")

    c_content.append("void emul_lvgl_deinit(void) {")
    c_content.append("    if (g_emul_root) {")
    c_content.append("        cJSON_Delete(g_emul_root);")
    c_content.append("        g_emul_root = NULL;")
    c_content.append("        g_objects = NULL;")
    c_content.append("        g_styles = NULL;")
    c_content.append("    }")
    c_content.append("    free_named_pointers();")
    c_content.append("}")
    c_content.append("")

    c_content.append("char* emul_lvgl_render_to_json(void) {")
    c_content.append("    if (!g_emul_root) {")
    c_content.append("        emul_lvgl_init(); // Ensure initialized if called early")
    c_content.append("    }")
    c_content.append("    // Optional: Update registered pointers debug info just before rendering")
    c_content.append("    cJSON* reg_ptr_node = cJSON_GetObjectItemCaseSensitive(g_emul_root, \"registered_pointers_debug\");")
    c_content.append("    if (reg_ptr_node) {")
    c_content.append("        // Clear previous debug info")
    c_content.append("        cJSON *child = reg_ptr_node->child;")
    c_content.append("        while(child) {")
    c_content.append("           cJSON* next = child->next;")
    c_content.append("           cJSON_DeleteItemFromObject(reg_ptr_node, child->string); // Does delete internally")
    c_content.append("           child = next;")
    c_content.append("        }")
    c_content.append("        // Add current debug info")
    c_content.append("        NamedPointer_t *current = g_named_pointers_head;")
    c_content.append("        char addr_str[20];")
    c_content.append("        while (current) {")
    c_content.append("           snprintf(addr_str, sizeof(addr_str), \"%p\", current->ptr);")
    c_content.append("           cJSON_AddStringToObject(reg_ptr_node, addr_str, current->name);")
    c_content.append("           current = current->next;")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("    return cJSON_Print(g_emul_root); // Use cJSON_PrintUnformatted for smaller output")
    c_content.append("}")
    c_content.append("")

    c_content.append("const char* emul_lvgl_get_obj_id(lv_obj_t* obj) {")
    c_content.append("    return obj ? ((emul_internal_obj_t*)obj)->id : NULL;")
    c_content.append("}")
    c_content.append("")

    c_content.append("const char* emul_lvgl_get_style_id(lv_style_t* style) {")
    c_content.append("    return style ? ((emul_internal_style_t*)style)->id : NULL;")
    c_content.append("}")
    c_content.append("")

    c_content.append("// --- Emulated LVGL Functions ---")
    for func in api_data.get('functions', []):
        func_name = func.get('name')
        if matches_filters(func_name, filters['function_whitelist'], filters['function_blacklist']):
            ret_type_info = func.get('type', {})
            ret_type = get_c_type(ret_type_info, structs, enums, typedefs)

            args = []
            arg_infos = func.get('args', [])
            # Handle void arguments
            if len(arg_infos) == 1 and arg_infos[0].get('type', {}).get('name') == 'void':
                args_str = "void"
                arg_list = []
            else:
                arg_list = []
                for i, arg in enumerate(arg_infos):
                    arg_name = sanitize_name(arg.get('name')) if arg.get('name') else f"arg{i}"
                    arg_type = get_c_type(arg['type'], structs, enums, typedefs)
                    if arg['type'].get('json_type') == 'array':
                         dim = arg['type'].get('dim', '') or ''
                         arg_list.append(f"{arg_type} {arg_name}[{dim}]")
                         args.append({"name": arg_name, "type": arg_type, "is_ptr": False, "is_obj": False, "is_style": False, "is_array": True, "info": arg['type']})
                    elif arg_type == "..." or arg_name == '...':
                         arg_list.append("...")
                         args.append({"name": "...", "type": "...", "is_ptr": False, "is_obj": False, "is_style": False, "is_array": False, "info": arg['type']})
                    else:
                         arg_list.append(f"{arg_type} {arg_name}")
                         is_obj = arg_type.startswith("lv_obj_t")
                         is_style = arg_type.startswith("lv_style_t")
                         is_ptr = "*" in arg_type
                         args.append({"name": arg_name, "type": arg_type, "is_ptr": is_ptr, "is_obj": is_obj, "is_style": is_style, "is_array": False, "info": arg['type']})

                args_str = ", ".join(arg_list) if arg_list else "void"

            c_content.append(f"{ret_type} {func_name}({args_str}) {{")

            # Function Body Generation
            obj_arg = next((a for a in args if a["is_obj"]), None)
            style_arg = next((a for a in args if a["is_style"]), None)
            target_arg = obj_arg if obj_arg else style_arg
            target_json_func = "get_obj_json" if obj_arg else "get_style_json"
            target_prop_func = "get_or_create_object_property_node" if obj_arg else "get_or_create_style_property_node"

            # --- Object/Style Creation ---
            #print(f"GEN: {func_name}")
            if func_name.endswith("_create"):
                #print(f"CREATE:: {func_name}")
                widget_type = func_name # e.g., "lv_obj_create"
                parent_arg = next((a for a in args if a["is_obj"]), None) # First obj arg is parent

                c_content.append("    if (!g_emul_root) emul_lvgl_init(); // Auto-initialize")
                c_content.append("    emul_internal_obj_t* new_emul_obj = (emul_internal_obj_t*)malloc(sizeof(emul_internal_obj_t));")
                c_content.append("    if (!new_emul_obj) return NULL; // Allocation failed")
                c_content.append(f"    snprintf(new_emul_obj->id, sizeof(new_emul_obj->id), \"obj_%ld\", ++g_obj_counter);")
                c_content.append(f"    cJSON *obj_node = cJSON_CreateObject();")
                c_content.append(f"    if (!obj_node) {{ free(new_emul_obj); return NULL; }}")
                c_content.append(f"    cJSON_AddItemToObject(g_objects, new_emul_obj->id, obj_node);")
                c_content.append(f"    cJSON_AddStringToObject(obj_node, \"type\", \"{widget_type}\");")
                if parent_arg:
                    c_content.append(f"    const char* parent_id = {parent_arg['name']} ? ((emul_internal_obj_t*){parent_arg['name']})->id : NULL;")
                    c_content.append(f"    if (parent_id) {{")
                    c_content.append(f"        cJSON_AddStringToObject(obj_node, \"parent\", parent_id);")
                    c_content.append(f"    }} else {{")
                    c_content.append(f"        cJSON_AddNullToObject(obj_node, \"parent\");")
                    c_content.append(f"    }}")
                else:
                     c_content.append(f"    cJSON_AddNullToObject(obj_node, \"parent\");")
                c_content.append(f"    cJSON_AddObjectToObject(obj_node, \"properties\"); // Initialize properties node")
                c_content.append(f"    cJSON_AddObjectToObject(obj_node, \"styles\"); // Initialize styles node")
                c_content.append(f"    return (lv_obj_t*)new_emul_obj;")

            # --- Style Initialization ---
            elif func_name == "lv_style_init":
                style_arg_init = next((a for a in args if a["is_style"]), None)
                if style_arg_init:
                    c_content.append("    if (!g_emul_root) emul_lvgl_init(); // Auto-initialize")
                    c_content.append(f"    // NOTE: LVGL style_init expects a pointer to an existing lv_style_t.")
                    c_content.append(f"    //       Emulator needs to allocate its internal representation.")
                    c_content.append(f"    //       This deviates slightly but captures the intent.")
                    c_content.append(f"    //       Assume {style_arg_init['name']} is just an output pointer here.")
                    c_content.append("    emul_internal_style_t* emul_style = (emul_internal_style_t*)malloc(sizeof(emul_internal_style_t));")
                    c_content.append("    if (!emul_style) return; // Or handle error appropriately")
                    c_content.append(f"    snprintf(emul_style->id, sizeof(emul_style->id), \"style_%ld\", ++g_style_counter);")
                    c_content.append(f"    cJSON *style_node = cJSON_CreateObject();")
                    c_content.append(f"    if (!style_node) {{ free(emul_style); return; }}")
                    c_content.append(f"    cJSON_AddItemToObject(g_styles, emul_style->id, style_node);")
                    c_content.append(f"    cJSON_AddObjectToObject(style_node, \"properties\"); // Initialize properties")
                    c_content.append(f"    // We can't easily assign back to the original pointer ({style_arg_init['name']})")
                    c_content.append(f"    // Caller should use a dedicated function if needed or manage style pointers.")
                    c_content.append(f"    // Consider adding an emul_lv_style_create() function instead.")
                    c_content.append(f"    // For now, we create the JSON entry but don't return the pointer via the arg.")
                    c_content.append(f"    // TODO: RETHINK lv_style_init emulation strategy.")

            elif func_name == "lv_style_reset":
                 if style_arg:
                      c_content.append(f"    cJSON *style_node = {target_json_func}(({target_arg['type']}){target_arg['name']});")
                      c_content.append(f"    if (style_node) {{")
                      c_content.append(f"        cJSON* props = cJSON_GetObjectItemCaseSensitive(style_node, \"properties\");")
                      c_content.append(f"        if (props) {{")
                      c_content.append(f"            cJSON *current_prop = props->child;")
                      c_content.append(f"            while (current_prop) {{")
                      c_content.append(f"                cJSON *next_prop = current_prop->next;")
                      c_content.append(f"                cJSON_DeleteItemFromObject(props, current_prop->string);")
                      c_content.append(f"                current_prop = next_prop;")
                      c_content.append(f"            }}")
                      c_content.append(f"        }}")
                      c_content.append(f"    }}")

            # --- Object Deletion ---
            elif func_name == "lv_obj_del" and obj_arg:
                 c_content.append(f"    if (!{obj_arg['name']} || !g_objects) return;")
                 c_content.append(f"    emul_internal_obj_t* internal_obj = (emul_internal_obj_t*){obj_arg['name']};")
                 c_content.append(f"    // TODO: Implement recursive deletion of children?")
                 c_content.append(f"    cJSON_DeleteItemFromObject(g_objects, internal_obj->id);")
                 c_content.append(f"    free(internal_obj);")

            # --- Add Style to Object ---
            elif func_name == "lv_obj_add_style" and obj_arg:
                 style_arg_add = next((a for a in args if a["is_style"]), None)
                 part_arg = next((a for a in args if a["name"] == "part" or a["name"] == "selector"), None) # LVGL uses part or selector
                 if style_arg_add and part_arg:
                     c_content.append(f"    cJSON *obj_node = {target_json_func}(({target_arg['type']}){target_arg['name']});")
                     c_content.append(f"    emul_internal_style_t* internal_style = (emul_internal_style_t*){style_arg_add['name']};")
                     c_content.append(f"    if (obj_node && internal_style) {{")
                     c_content.append(f"        cJSON* styles_map = cJSON_GetObjectItemCaseSensitive(obj_node, \"styles\");")
                     c_content.append(f"        if (!styles_map) {{")
                     c_content.append(f"            styles_map = cJSON_AddObjectToObject(obj_node, \"styles\");")
                     c_content.append(f"        }}")
                     c_content.append(f"        if (styles_map) {{")
                     c_content.append(f"            char part_str[16];")
                     c_content.append(f"            snprintf(part_str, sizeof(part_str), \"part_%u\", (unsigned int){part_arg['name']}); // Use part ID as key")
                     c_content.append(f"            cJSON* part_styles_array = cJSON_GetObjectItemCaseSensitive(styles_map, part_str);")
                     c_content.append(f"            if (!part_styles_array) {{")
                     c_content.append(f"                part_styles_array = cJSON_AddArrayToObject(styles_map, part_str);")
                     c_content.append(f"            }}")
                     c_content.append(f"            if (part_styles_array) {{")
                     c_content.append(f"                // Avoid adding duplicates (optional)")
                     c_content.append(f"                bool found = false;")
                     c_content.append(f"                cJSON* current_style_id_node;")
                     c_content.append(f"                cJSON_ArrayForEach(current_style_id_node, part_styles_array) {{")
                     c_content.append(f"                    if (cJSON_IsString(current_style_id_node) && strcmp(current_style_id_node->valuestring, internal_style->id) == 0) {{")
                     c_content.append(f"                        found = true;")
                     c_content.append(f"                        break;")
                     c_content.append(f"                    }}")
                     c_content.append(f"                }}")
                     c_content.append(f"                if (!found) {{")
                     c_content.append(f"                    cJSON_AddItemToArray(part_styles_array, cJSON_CreateString(internal_style->id));")
                     c_content.append(f"                }}")
                     c_content.append(f"            }}")
                     c_content.append(f"        }}")
                     c_content.append(f"    }}")
                 else:
                      c_content.append("    // Missing obj, style, or part argument")


            # --- Setters (Generic) ---
            # --- Section: setter functions generation ---
            elif target_arg and func_name.startswith(("lv_obj_set_", "lv_style_set_", "lv_label_set_", "lv_scale_set_", "lv_bar_set_")):
                prop_name = get_property_name(func_name)
                value_arg = args[-1] # Assume the last argument is the value being set

                if prop_name and func_name == 'lv_obj_set_grid_dsc_array':
                    c_content.append(f"    cJSON *target_node = {target_json_func}(({target_arg['type']}){target_arg['name']});")
                    c_content.append(f"    if (target_node) {{")
                    c_content.append(f"        cJSON *props_node = {target_prop_func}(target_node, \"{prop_name}\");")
                    c_content.append(f"        if (props_node) {{")
                    c_content.append(f"             cJSON * dscNode =  cJSON_AddArrayToObject(props_node, \"row_dsc\");")
                    c_content.append(f"              if (dscNode) {{");
                    c_content.append(f"                  for (size_t i = 0; row_dsc[i] != LV_GRID_TEMPLATE_LAST; ++i) {{ cJSON_AddItemToArray(dscNode, cJSON_CreateNumber(row_dsc[i])); }}")
                    c_content.append(f"                  cJSON_AddItemToArray(dscNode, cJSON_CreateNumber(LV_GRID_TEMPLATE_LAST));")
                    c_content.append(f"              }}");
                    c_content.append(f"             cJSON * cdscNode =  cJSON_AddArrayToObject(props_node, \"col_dsc\");")
                    c_content.append(f"              if (cdscNode) {{");
                    c_content.append(f"                  for (size_t i = 0; col_dsc[i] != LV_GRID_TEMPLATE_LAST; ++i) {{ cJSON_AddItemToArray(cdscNode, cJSON_CreateNumber(col_dsc[i])); }}")
                    c_content.append(f"                  cJSON_AddItemToArray(cdscNode, cJSON_CreateNumber(LV_GRID_TEMPLATE_LAST));")
                    c_content.append(f"              }}");
                    c_content.append(f"         }}");
                    c_content.append(f"    }}")

                elif prop_name and value_arg and value_arg['name'] != "...":
                    c_content.append(f"    cJSON *target_node = {target_json_func}(({target_arg['type']}){target_arg['name']});")
                    c_content.append(f"    if (target_node) {{")
                    c_content.append(f"        cJSON *props_node = {target_prop_func}(target_node, \"{prop_name}\");")
                    c_content.append(f"        if (props_node) {{")

                    val_name = value_arg['name']
                    val_type = value_arg['type']
                    val_info = value_arg['info']

                    # Determine how to add the value to JSON based on type
                    if "char *" in val_type:
                        c_content.append(f"            cJSON_AddStringToObject(props_node, \"{prop_name}\", {val_name} ? {val_name} : \"\");")
                    elif "bool" in val_type:
                        c_content.append(f"            cJSON_AddBoolToObject(props_node, \"{prop_name}\", {val_name});")
                    elif any(t in val_type for t in ["int", "short", "long", "size_t", "lv_coord_t", "lv_opa_t"]) and "*" not in val_type:
                         # Includes enums which are typically int-based
                        c_content.append(f"            cJSON_AddNumberToObject(props_node, \"{prop_name}\", (double){val_name});")
                    elif any(t in val_type for t in ["float", "double"]) and "*" not in val_type:
                         c_content.append(f"            cJSON_AddNumberToObject(props_node, \"{prop_name}\", (double){val_name});")
                    elif "lv_color_t" in val_type and "*" not in val_type:
                         # Represent color as hex string #RRGGBB (assuming 24-bit color)
                         # This might need adjustment based on LV_COLOR_DEPTH
                         c_content.append(f"            char color_str[8];")
                         c_content.append(f"            color_to_str(color_str, value);")
                         c_content.append(f"            cJSON_AddStringToObject(props_node, \"{prop_name}\", color_str);")
                    elif value_arg["is_ptr"]:
                         # Handle pointers: Check if obj*, style*, or needs named lookup
                         if value_arg["is_obj"]:
                              c_content.append(f"            const char* obj_id = emul_lvgl_get_obj_id(({value_arg['type']}){val_name});")
                              c_content.append(f"            if(obj_id) cJSON_AddStringToObject(props_node, \"{prop_name}\", obj_id); else cJSON_AddNullToObject(props_node, \"{prop_name}\");")
                         elif value_arg["is_style"]:
                              c_content.append(f"            const char* style_id = emul_lvgl_get_style_id(({value_arg['type']}){val_name});")
                              c_content.append(f"            if(style_id) cJSON_AddStringToObject(props_node, \"{prop_name}\", style_id); else cJSON_AddNullToObject(props_node, \"{prop_name}\");")
                         else: # Assume potentially named pointer (font, image, etc.)
                              c_content.append(f"            const char* registered_name = find_registered_name((const void*){val_name});")
                              c_content.append(f"            if(registered_name) cJSON_AddStringToObject(props_node, \"{prop_name}\", registered_name); else cJSON_AddNullToObject(props_node, \"{prop_name}\");")
                    else:
                        c_content.append(f"            // TODO: Handle unsupported value type '{val_type}' for property '{prop_name}'")
                        c_content.append(f"            cJSON_AddNullToObject(props_node, \"{prop_name}\"); // Add null placeholder")

                    c_content.append(f"        }}")
                    c_content.append(f"    }}")
                else:
                     c_content.append(f"    // Could not determine property name or value argument for function {func_name}")

            # --- Default/Placeholder for other functions ---
            else:
                c_content.append(f"    // Emulation logic for {func_name} not implemented.")
                # Add default return value if function is not void
                if ret_type != "void":
                    default_ret = "NULL" if "*" in ret_type else "0"
                    c_content.append(f"    return ({ret_type}){default_ret};")


            c_content.append("}")
            c_content.append("")

    return "\n".join(c_content)


# --- Main Execution ---

def main():
    parser = argparse.ArgumentParser(description="Generate LVGL C emulator library from JSON API description.")
    parser.add_argument("json_api_file", help="Path to the LVGL API JSON file.")
    parser.add_argument("-o", "--output-dir", default=".", help="Directory to output generated files (emul_lvgl.h, emul_lvgl.c).")
    parser.add_argument("--func-wl", nargs='+', default=DEFAULT_FUNCTION_WHITELIST, help="Prefixes for functions to include.")
    parser.add_argument("--func-bl", nargs='+', default=DEFAULT_FUNCTION_BLACKLIST, help="Prefixes for functions to exclude.")
    parser.add_argument("--enum-wl", nargs='+', default=DEFAULT_ENUM_WHITELIST, help="Prefixes for enums to include.")
    parser.add_argument("--enum-bl", nargs='+', default=DEFAULT_ENUM_BLACKLIST, help="Prefixes for enums to exclude.")
    parser.add_argument("--macro-wl", nargs='+', default=DEFAULT_MACRO_WHITELIST, help="Prefixes for macros to include.")
    parser.add_argument("--macro-bl", nargs='+', default=DEFAULT_MACRO_BLACKLIST, help="Prefixes for macros to exclude.")
    # Add more filter arguments if needed (structs, typedefs etc.)

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
        "struct_whitelist": DEFAULT_STRUCT_WHITELIST, # Use defaults for now
        "struct_blacklist": DEFAULT_STRUCT_BLACKLIST,
    }

    # Generate Header Content
    print("Generating emul_lvgl.h...")
    header_content = generate_header(api_data, filters)

    # Generate Source Content
    print("Generating emul_lvgl.c...")
    source_content = generate_c_source(api_data, filters)

    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)

    # Write files
    header_path = os.path.join(args.output_dir, "emul_lvgl.h")
    source_path = os.path.join(args.output_dir, "emul_lvgl.c")

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
