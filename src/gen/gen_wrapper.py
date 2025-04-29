# generate_lvgl_json_wrapper.py

import json
import argparse
import os
import re
from collections import defaultdict

# --- Configuration ---

# Heuristics: Types that are almost always pointers managed by LVGL
DEFAULT_OPAQUE_TYPE_PREFIXES = [
    "lv_obj_t", "lv_style_t", "lv_font_t", "lv_theme_t",
    "lv_timer_t", "lv_group_t", "lv_display_t", "lv_indev_t",
    "lv_anim_t", "lv_anim_timeline_t",
    "lv_scale_section_t", # Example of a sub-object
    "lv_fragment_t", "lv_fragment_manager_t",
    "lv_subject_t", "lv_observer_t",
    "lv_draw_buf_t", # Often managed pointers
    "lv_image_decoder_t",
    "lv_image_decoder_dsc_t",
    "lv_fs_file_t", # File handles usually opaque
    "lv_fs_dir_t",
    # Add other known opaque types/prefixes if needed
]

# Functions typically returning new opaque objects (in addition to 'create')
CONSTRUCTOR_NAME_PATTERNS = [
    r'^lv_([a-z0-9_]+)_create$',  # Standard create pattern
    r'^lv_([a-z0-9_]+)_add_.*',  # Functions adding sub-elements like scale sections
    r'^lv_theme_.*_init', # Theme initializers often return the theme
    # Add specific function names if they act as constructors but don't fit patterns
    "lv_group_create",
    "lv_timer_create", # Although has callback, treat return as constructor if included
    "lv_anim_timeline_create",
    "lv_fragment_create",
    "lv_subject_create_with_value", # If applicable
    "lv_image_decoder_create",
    "lv_fs_open",
    "lv_anim_timeline_create",
]

# Functions initializing an object via pointer arg (usually first arg)
INIT_FUNC_NAME_PATTERNS = [
    r'^lv_([a-z0-9_]+)_init$',
    r'^lv_style_init$', # Specific case
    r'^lv_anim_init$',  # Specific case
    r'^lv_subject_init_.*',
]

# Types that should *never* be treated as opaque pointers, even if heuristics match
ALWAYS_CONCRETE_TYPES = [
    "lv_color_t", "lv_opa_t", "lv_area_t", "lv_point_t", "lv_coord_t",
    "lv_style_value_t", "lv_color_hsv_t", "lv_point_precise_t",
    "lv_color16_t", "lv_color32_t", "lv_grad_dsc_t", "lv_base_dsc_t",
    "lv_text_dsc_t", "lv_image_dsc_t", "lv_image_header_t",
    "lv_draw_rect_dsc_t", "lv_draw_label_dsc_t", "lv_draw_image_dsc_t",
    "lv_draw_line_dsc_t", "lv_draw_arc_dsc_t", "lv_draw_triangle_dsc_t",
    "lv_draw_mask_rect_dsc_t", # Add other descriptor structs, simple value structs
    "lv_result_t", # The enum typedef
    "lv_mem_monitor_t",
    "lv_sqrt_res_t",
    "lv_event_code_t", # Enum typedef
]


# --- Helper Functions ---

def sanitize_name(name):
    """Removes leading underscores and potentially other invalid chars for C identifiers."""
    if name is None:
        return "unnamed_arg"
    name = name.lstrip('_')
    name = re.sub(r'[^a-zA-Z0-9_]', '', name)
    if not name or name[0].isdigit():
        name = "arg_" + name
    # Avoid C keywords (add more if needed)
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
        name = name + "_"
    return name

def matches_prefix(name, prefixes):
    """Checks if a name starts with any of the given prefixes."""
    if not name:
        return False
    for prefix in prefixes:
        if name.startswith(prefix):
            return True
    return False

def get_base_type_name(type_info):
    """Recursively finds the base name of a type (handling pointers, arrays)."""
    if not type_info or not isinstance(type_info, dict):
        return None
    current_type = type_info
    while current_type:
        json_type = current_type.get("json_type")
        if json_type in ("pointer", "array", "ret_type"):
            current_type = current_type.get("type")
        elif json_type == "typedef":
            # Return the typedef name itself, not the underlying type's name here
            # unless it's a pointer/array typedef
            if 'type' in current_type and current_type['type'].get('json_type') in ('pointer', 'array'):
                 current_type = current_type.get("type") # Look through pointer/array typedefs
            else:
                return current_type.get("name") # Return the typedef name like lv_coord_t
        else:
            # Found base type (struct, enum, primitive, etc.)
            return current_type.get("name")
    return None # Should not happen if type_info is valid


def get_type_details(type_info, api_data):
    """Gets full definition (like struct fields or enum members) by name lookup."""
    # Find the ultimate base name first (e.g., from lv_obj_t** get lv_obj_t)
    q = [type_info]
    visited_names = set() # Prevent infinite loops with recursive typedefs
    base_name = None
    while q:
        curr = q.pop(0)
        if not curr or not isinstance(curr, dict): continue
        name = curr.get("name")
        json_type = curr.get("json_type")

        if name and name not in visited_names:
             visited_names.add(name)
             if json_type not in ("pointer", "array", "ret_type"):
                 base_name = name # Found a potential base name
                 break # Stop searching deeper for name

        if "type" in curr:
            q.append(curr["type"])

    if not base_name:
        # Try the original name if the loop didn't find a non-pointer base
        base_name = type_info.get("name") if isinstance(type_info, dict) else None
        if not base_name:
            return None

    # Search in all relevant categories for this base name
    for category in ["enums", "structures", "unions", "typedefs", "function_pointers", "forward_decls"]:
        for item in api_data.get(category, []):
            # Use the name from the item, which might be the internal name (_lv_...)
            item_name = item.get("name")
            if item_name == base_name:
                # Don't resolve typedefs further here, return the typedef def itself
                # The caller (e.g., is_opaque, get_c_type_name) will handle resolution if needed
                return item # Return the definition (struct, enum, typedef, etc.)
    return None # Not found (might be primitive/stdlib type)

def find_typedef_for_base(base_name, api_data, config):
    """Finds if a typedef exists in the filtered list for a given base struct/enum/union name."""
    for typedef in config['filtered_typedefs']:
        underlying_type = typedef.get("type")
        if underlying_type:
            # Check if the typedef's direct underlying type name matches
            current_type = underlying_type
            resolved_base_name = None
            # Simple check first for direct mapping (e.g., typedef struct _lv_foo_t lv_foo_t)
            if current_type.get("name") == base_name and current_type.get("json_type") in ("struct", "union", "enum"):
                 resolved_base_name = base_name
            # Need to handle deeper nesting? Probably not needed if get_type_details is robust.
            # Let's keep it simple for now.

            # If the underlying base matches, return the typedef's name
            if resolved_base_name == base_name:
                # Ensure the typedef itself isn't designated opaque separately
                if not is_opaque(typedef, api_data, config):
                    return typedef["name"]
    return None

def is_opaque(type_info, api_data, config):
    """Determines if a type should be treated as opaque."""
    base_name = get_base_type_name(type_info)
    if not base_name:
        return False

    # 0. Check if explicitly designated non-opaque
    if base_name in ALWAYS_CONCRETE_TYPES:
        return False

    # Get the actual definition of the base type
    details = get_type_details({"name": base_name}, api_data) # Use stub to find details by name

    # --- Early Exits for Non-Opaque Categories ---
    if details:
        json_type = details.get("json_type")
        # Enums and Function Pointers are NEVER opaque in this model
        if json_type in ("enum", "function_pointer"):
            return False

    # --- Standard Checks ---
    # 1. Explicitly configured prefixes (strongest indicator)
    if matches_prefix(base_name, config['opaque_type_prefixes']):
        # Double check it's not in the ALWAYS_CONCRETE list
        if base_name not in ALWAYS_CONCRETE_TYPES:
             return True

    # 2. Check typedefs: Some typedefs might point to opaque structs
    if details and details.get("json_type") == "typedef":
         underlying_type = details.get("type")
         if underlying_type:
             # If typedef points directly to void* AND is in default opaque list (like lv_font_t)
             if underlying_type.get("json_type") == "pointer":
                 pointee = underlying_type.get("type")
                 if pointee and pointee.get("name") == "void" and pointee.get("json_type") == "primitive_type":
                     # Only treat typedef void* as opaque if in the explicit list
                     if matches_prefix(base_name, DEFAULT_OPAQUE_TYPE_PREFIXES):
                         # Double check it's not in the ALWAYS_CONCRETE list
                         if base_name not in ALWAYS_CONCRETE_TYPES:
                             return True

             # Recurse on the underlying type of the typedef *unless* it's primitive/stdlib/enum/funcptr or always concrete
             underlying_base_name = get_base_type_name(underlying_type)
             if underlying_base_name not in ALWAYS_CONCRETE_TYPES:
                 underlying_details = get_type_details(underlying_type, api_data)
                 if underlying_details and underlying_details.get("json_type") not in ("stdlib_type", "primitive_type", "enum", "function_pointer"):
                      # Check opacity of the underlying type definition
                      return is_opaque(underlying_type, api_data, config)

    # 3. Forward declaration implies opacity if it's not later defined as a struct/union we include
    if details and details.get("json_type") == "forward_decl":
         # Check if a full struct/union definition exists *and is included*
         defined_struct = next((s for s in config['filtered_structures'] if s.get("name") == base_name), None)
         defined_union = next((u for u in config['filtered_unions'] if u.get("name") == base_name), None)
         # If it's not defined (or defined but excluded), treat as opaque
         if not defined_struct and not defined_union:
              # Double check it's not in the ALWAYS_CONCRETE list
              if base_name not in ALWAYS_CONCRETE_TYPES:
                  return True

    # 4. Structs/Unions returned *by pointer* from constructors
    # Check only if 'details' corresponds to a struct or union definition
    if details and details.get("json_type") in ("struct", "union"):
         for func in config['filtered_functions']:
             ret_type_wrapper = func.get("type", {}) # This is the "ret_type" wrapper
             ret_type_actual = ret_type_wrapper.get("type", {}) # This is the actual return type (e.g., pointer)

             # *** Check if function returns a POINTER to this type ***
             if ret_type_actual and ret_type_actual.get("json_type") == "pointer":
                 pointee_type = ret_type_actual.get("type")
                 pointee_name = get_base_type_name(pointee_type)
                 if pointee_name == base_name:
                     # Check if the function matches constructor patterns
                     for pattern in CONSTRUCTOR_NAME_PATTERNS:
                         if re.match(pattern, func["name"]):
                              # Double check it's not in the ALWAYS_CONCRETE list
                              if base_name not in ALWAYS_CONCRETE_TYPES:
                                 return True # Constructor returning pointer to this type -> Opaque
                     # Break inner loop once match is found for this function
                     break

    # 5. Structs/Unions passed *by pointer* to init functions (usually first arg)
    # Check only if 'details' corresponds to a struct or union definition
    if details and details.get("json_type") in ("struct", "union"):
         for func in config['filtered_functions']:
             args = func.get("args")
             if args and len(args) > 0:
                 first_arg = args[0]
                 arg_type = first_arg.get("type", {})
                 # *** Check if first arg is a POINTER to this type ***
                 if arg_type.get("json_type") == "pointer":
                     pointee_type = arg_type.get("type")
                     pointee_name = get_base_type_name(pointee_type)
                     if pointee_name == base_name:
                         # Check if the function matches init patterns
                         for pattern in INIT_FUNC_NAME_PATTERNS:
                              if re.match(pattern, func["name"]):
                                   # Double check it's not in the ALWAYS_CONCRETE list
                                   if base_name not in ALWAYS_CONCRETE_TYPES:
                                       return True # Init function taking pointer to this type -> Opaque
                         # Break inner loop once match is found for this function
                         break

    # Default to non-opaque
    return False

def split_c_type_name(ty):
    l = ty.split(":**:")
    if len(l) == 1: return [ty, '']
    return l

def get_c_type_name(type_info, api_data, config, use_opaque_typedef=True,
                    remove_const=False, split_separator=False):
    """Gets the C type string for a type_info object, handling const, pointers, opaque types."""
    if not type_info or not isinstance(type_info, dict):
        #return "void /* unknown */"
        return None

    json_type = type_info.get("json_type")
    quals = type_info.get("quals", [])
    const_prefix = "const " if "const" in quals and not remove_const else ""

    # Handle ellipsis early
    if json_type == "special_type" and type_info.get("name") == "ellipsis":
        return "..."

    # Handle pointers recursively
    if json_type == "pointer":
        pointee_type = type_info.get("type")
        # Special case: const char * -> const char * (treat as string)
        pointee_base_name = get_base_type_name(pointee_type)
        pointee_details = get_type_details(pointee_type, api_data)
        pointee_json_type = pointee_details.get("json_type") if pointee_details else (pointee_type.get("json_type") if pointee_type else '')


        if const_prefix and pointee_base_name == 'char' and pointee_json_type == "primitive_type":
             # Pass remove_const=False to keep "const char" if originally present
             pointee_str = get_c_type_name(pointee_type, api_data, config, use_opaque_typedef, remove_const=False)
             return f"{pointee_str}*" # Const is handled by pointee_str
        else:
             # Remove const from the pointee type string since it applies to the pointer itself
             pointee_str = get_c_type_name(pointee_type, api_data, config, use_opaque_typedef, remove_const=True)
             return f"{const_prefix}{pointee_str}*" # Add const for the pointer if present

    # Handle arrays
    elif json_type == "array":
        # Note: LVGL JSON uses 'array' for types like 'const int32_t []' which are really pointers in function args.
        # The 'pointer' logic above often catches these via typedefs, but handle explicitly if needed.
        base_type_info = type_info.get("type")
        base_type_str = get_c_type_name(base_type_info, api_data, config, use_opaque_typedef, remove_const=True) # Const applies to elements
        if not base_type_str: base_type_str = type_info.get("name")
        dim = type_info.get("dim")
        # Heuristic: If dim is null (unknown) and it's a basic type, treat as pointer
        if dim is None:
            base_name = get_base_type_name(base_type_info)
            details = get_type_details(base_type_info, api_data)
            json_type_detail = details.get("json_type") if details else (base_type_info.get("json_type") if base_type_info else '')

            if json_type_detail in ("primitive_type", "stdlib_type"):
                 return f"{const_prefix}{base_type_str}*" # Treat as pointer

        array_suffix = f"[{dim}]" if dim else "[]" # Use [] for unknown size C arrays
        sep = ":**:" if split_separator else ''
        print(f"{const_prefix}{base_type_str}{sep}{array_suffix}", type_info)
        return f"{const_prefix}{base_type_str}{sep}{array_suffix}" # Const applies to the array elements


    # --- Base Types (non-pointer, non-array) ---
    base_name = type_info.get("name")
    if not base_name:
         if type_info.get("name") == "void" and type_info.get("json_type") == "primitive_type":
             # Check if it's the only arg for a void param list (handled later in function gen)
             # Here, just return "void"
             return "void"
         else:
             return "/* unknown_base */"


    # Check for enum/func_ptr *before* checking opaque
    details = get_type_details(type_info, api_data)
    if details:
         json_type_detail = details.get("json_type")
         if json_type_detail == "enum":
             # Find the best name: use typedef if available and included, else the enum's own name
             typedef_name = find_typedef_for_base(details['name'], api_data, config)
             enum_name_to_use = typedef_name if typedef_name else details['name']
             return f"{const_prefix}{enum_name_to_use}"
         elif json_type_detail == "function_pointer":
              # Use the function pointer type name directly
              fp_name = details.get("name")
              if fp_name:
                   return f"{const_prefix}{fp_name}"


    # Check if opaque *after* ruling out enum/func_ptr
    if use_opaque_typedef and is_opaque(type_info, api_data, config):
        opaque_name = f"{sanitize_name(base_name)}" # Use original name (sanitized)
        return f"{const_prefix}{opaque_name}"

    # Resolve typedefs if not opaque
    if details and details.get("json_type") == "typedef":
        # We already checked if the typedef *itself* should be opaque or is enum/func_ptr.
        # Now, resolve to the underlying type if it's not a simple primitive/stdlib mapping.
        underlying_type_info = details.get("type")
        if underlying_type_info:
             underlying_details = get_type_details(underlying_type_info, api_data)
             underlying_json_type = underlying_details.get("json_type") if underlying_details else (underlying_type_info.get("json_type") if underlying_type_info else '')


             # If typedef maps to a primitive/stdlib type, use the typedef name (e.g., lv_coord_t)
             if underlying_json_type in ("stdlib_type", "primitive_type"):
                 return f"{const_prefix}{base_name}"
             # If it maps to a non-opaque struct/union that we are generating, use typedef name
             elif underlying_json_type in ("struct", "union"):
                  if not is_opaque(underlying_type_info, api_data, config):
                      return f"{const_prefix}{base_name}" # Use typedef name like lv_point_t
             # Otherwise, resolve the underlying type's name string
             else:
                  # Pass 'remove_const' from the original call? Or respect typedef's const?
                  # Let's respect the original call's const requirement.
                  return get_c_type_name(underlying_type_info, api_data, config, use_opaque_typedef, remove_const=remove_const)


    # Handle concrete structs/unions explicitly if not handled by typedef
    if details and details.get("json_type") == "struct":
        # Prefer typedef name if available and this struct isn't opaque
        typedef_name = find_typedef_for_base(details['name'], api_data, config)
        if typedef_name:
            return f"{const_prefix}{typedef_name}"
        else:
            # Fallback to "struct <name>"
            return f"{const_prefix}struct {details['name']}"

    if details and details.get("json_type") == "union":
        typedef_name = find_typedef_for_base(details['name'], api_data, config)
        if typedef_name:
            return f"{const_prefix}{typedef_name}"
        else:
            return f"{const_prefix}union {details['name']}"


    # Standard types: primitives, stdlib types, or lvgl_type that weren't opaque/typedef'd/struct/union/enum
    if base_name:
        return f"{const_prefix}{base_name}"

    # Fallback for anything unhandled
    return f"/* unhandled_type: {json.dumps(type_info)} */"


def get_function_type(func_info, api_data, config):
    """Classifies function as constructor, init, setter, or other."""
    name = func_info["name"]
    ret_type_wrapper = func_info.get("type", {})
    ret_type_actual = ret_type_wrapper.get("type", {}) # Actual return type info
    is_ret_ptr = ret_type_actual.get("json_type") == "pointer"
    ret_pointee_type = ret_type_actual.get("type") if is_ret_ptr else None

    # Check for init functions first
    for pattern in INIT_FUNC_NAME_PATTERNS:
        if re.match(pattern, name):
            # Verify first arg is a pointer to a type we determined is opaque
            if func_info.get("args") and len(func_info["args"]) > 0:
                first_arg = func_info["args"][0]
                first_arg_type = first_arg.get("type")
                if first_arg_type and first_arg_type.get("json_type") == "pointer":
                     pointee_type = first_arg_type.get("type")
                     if is_opaque(pointee_type, api_data, config):
                         return "init"

    # Check for constructors
    for pattern in CONSTRUCTOR_NAME_PATTERNS:
        if re.match(pattern, name):
             # Check if return type is a pointer to an opaque type
             if is_ret_ptr and ret_pointee_type and is_opaque(ret_pointee_type, api_data, config):
                 return "constructor"

    # Check for setters
    # Heuristic 1: Name contains "set", "add", "remove", "enable/disable", etc. AND First arg is opaque ptr
    setter_keywords = ["_set_", "_add_", "_remove_", "_enable", "_disable", "_update"]
    is_potential_setter_name = any(kw in name for kw in setter_keywords) or name.startswith("lv_style_set_") or name.startswith("lv_obj_set_style_")

    if is_potential_setter_name:
        if func_info.get("args") and len(func_info["args"]) > 0:
             first_arg = func_info["args"][0]
             first_arg_type = first_arg.get("type")
             # Check if first arg is a pointer to an opaque type
             if first_arg_type and first_arg_type.get("json_type") == "pointer":
                  pointee_type = first_arg_type.get("type")
                  if is_opaque(pointee_type, api_data, config):
                      # Typically void return, but allow others for now
                      return "setter"

    # Heuristic 2: Function takes an opaque pointer as first arg and modifies state (might catch more)
    # This is harder to detect reliably without more semantics. Stick to name/arg pattern.

    return "other" # Could be getters, utilities, etc.

# --- Code Generation Functions ---

def generate_header(api_data, config):
    """Generates the C header file content."""
    h_content = []
    h_content.append("// Generated by generate_emul_lvgl_wrapper.py. DO NOT EDIT.")
    h_content.append("#ifndef EMUL_LVGL_H")
    h_content.append("#define EMUL_LVGL_H")
    h_content.append("\n#ifdef __cplusplus")
    h_content.append('extern "C" {')
    h_content.append("#endif\n")

    h_content.append("#include <stdint.h>")
    h_content.append("#include <stdbool.h>")
    h_content.append("#include <stddef.h>")
    h_content.append("typedef struct cJSON cJSON;")
    h_content.append("")

    # --- Refined Type Generation Order ---
    processed_typedef_names = set()
    processed_struct_union_enum_names = set() # Tracks internal names (_lv_...)
    forward_declared_concrete_structs = set()
    forward_declared_concrete_unions = set()
    forward_declared_items = set() # Tracks names that got a forward declaration

    # 1. Fundamental Primitive/Stdlib Typedefs
    h_content.append("// --- Base Typedefs ---")
    # fundamental_typedefs = ["lv_coord_t", "lv_opa_t", "lv_state_t", "lv_part_t", "lv_style_prop_t", "lv_color_t", "lv_res_t"]
    fundamental_typedefs = []
    for name in fundamental_typedefs:
        typedef = next((t for t in config['filtered_typedefs'] if t['name'] == name), None)
        if typedef and name not in processed_typedef_names:
             if not is_opaque(typedef, api_data, config):
                underlying_type_info = typedef["type"]
                c_type_str = get_c_type_name(underlying_type_info, api_data, config, use_opaque_typedef=False)
                h_content.append(f"typedef {c_type_str} {name};")
                processed_typedef_names.add(name)
    h_content.append("")

    # 2. Enum Definitions (and their potential typedefs)
    h_content.append("// --- Enum Definitions ---")
    for enum in sorted(config['filtered_enums'], key=lambda x: x['name']):
        enum_internal_name = enum["name"]
        typedef_name = find_typedef_for_base(enum_internal_name, api_data, config)
        c_enum_name_to_use = typedef_name if typedef_name else enum_internal_name

        if c_enum_name_to_use in processed_typedef_names or enum_internal_name in processed_struct_union_enum_names:
            continue # Skip if already handled

        h_content.append(f"typedef enum {{ // Internal name: {enum_internal_name}")
        for member in enum.get("members", []):
            h_content.append(f"    {member['name']} = {member.get('value', '/*?*/')},")
        h_content.append(f"}} {c_enum_name_to_use};")

        processed_struct_union_enum_names.add(enum_internal_name)
        if typedef_name:
            processed_typedef_names.add(typedef_name)
        h_content.append("")
    h_content.append("")

    # 3. Struct/Union Forward Declarations (Concrete only needed for non-typedef'd)
    h_content.append("// --- Struct/Union Forward Declarations ---")
    all_structs_unions = sorted(config['filtered_structures'] + config['filtered_unions'], key=lambda x: x['name'])
    for item in all_structs_unions:
        internal_name = item['name'] # e.g., _lv_point_t or lv_area_t
        item_type = item['type']['name'] # "struct" or "union"

        if internal_name in processed_typedef_names or internal_name in processed_struct_union_enum_names or internal_name in forward_declared_items:
            continue # Skip if opaque, already defined, or forward declared

        is_item_opaque = is_opaque(item, api_data, config)
        typedef_name = find_typedef_for_base(internal_name, api_data, config)

        if not is_item_opaque:
            # Concrete Item
            if typedef_name and typedef_name not in processed_typedef_names:
                 # Define the typedef forward declaration
                 h_content.append(f"typedef {item_type} {internal_name} {typedef_name};")
                 processed_typedef_names.add(typedef_name)
                 forward_declared_items.add(internal_name) # Mark internal name as handled
                 if item_type == "struct": forward_declared_concrete_structs.add(internal_name)
                 else: forward_declared_concrete_unions.add(internal_name)
            elif internal_name not in forward_declared_items:
                 # No typedef or typedef already processed, just forward declare the struct/union
                 h_content.append(f"{item_type} {internal_name};")
                 forward_declared_items.add(internal_name)
                 if item_type == "struct": forward_declared_concrete_structs.add(internal_name)
                 else: forward_declared_concrete_unions.add(internal_name)
        # else: Opaque items don't need forward declarations here, handled in step 4
    h_content.append("")


    # 4. Opaque Type Definitions (typedef cJSON* ...)
    h_content.append("// --- Opaque Type Definitions (as cJSON*) ---")
    for type_name in sorted(list(config["opaque_types"])):
        if type_name not in processed_typedef_names and type_name not in processed_struct_union_enum_names:
            sanitized = sanitize_name(type_name)
            # Use the sanitized original name for the typedef
            h_content.append(f"typedef cJSON* {sanitized};")
            processed_typedef_names.add(type_name) # Mark the original LVGL name as handled by this typedef

            # Check if this opaque type name corresponds to a struct/union internal name
            # If so, mark the internal name as processed too, to prevent concrete definition later
            details = get_type_details({"name": type_name}, api_data)
            if details and details.get("json_type") in ("struct", "union"):
                processed_struct_union_enum_names.add(type_name)
    h_content.append("")

    # 6. Remaining Typedefs (Function Pointers, etc.)
    h_content.append("// --- Remaining Typedef Definitions (e.g., Function Pointers) ---")
    typedef_list = sorted(config['filtered_typedefs'], key=lambda x: x['name'])
    for typedef in typedef_list:
        name = typedef["name"]
        if name not in processed_typedef_names: # Check if typedef name itself is already processed
             if is_opaque(typedef, api_data, config): continue # Should have been handled

             underlying_type_info = typedef.get("type")
             details = None
             if underlying_type_info:
                 # Check if it points *directly* to a function pointer definition or via name
                 if underlying_type_info.get("json_type") == "function_pointer":
                     details = underlying_type_info
                 else:
                     details = get_type_details(underlying_type_info, api_data)

             if details and details.get("json_type") == "function_pointer":
                 ret_type_info = details.get("type",{})
                 ret_type = get_c_type_name(ret_type_info.get("type",{}), api_data, config, use_opaque_typedef=True)
                 args_info = details.get("args", [])
                 arg_strs = []
                 is_void_fnc = (not args_info or (len(args_info) == 1 and get_base_type_name(args_info[0].get("type")) == "void"))
                 if is_void_fnc:
                     arg_strs.append("void")
                 else:
                     for i, arg in enumerate(args_info):
                         arg_name_fp = sanitize_name(arg.get("name", f"fp_arg{i}"))
                         arg_type_str_fp = get_c_type_name(arg.get("type",{}), api_data, config, use_opaque_typedef=True)
                         arg_strs.append(f"{arg_type_str_fp}") # Omit name in typedef
                 h_content.append(f"typedef {ret_type} (*{name})({', '.join(arg_strs)});")
                 processed_typedef_names.add(name)
             elif underlying_type_info:
                 # Any other non-opaque, non-processed typedef (should be rare now)
                 c_type_str = get_c_type_name(underlying_type_info, api_data, config, use_opaque_typedef=True)
                 base_name_underlying = get_base_type_name(underlying_type_info)
                 if c_type_str != name and name != base_name_underlying and "unhandled" not in c_type_str and "unknown" not in c_type_str:
                     h_content.append(f"typedef {c_type_str} {name}; // Other typedef")
                     processed_typedef_names.add(name)
    h_content.append("")



    # 5. Concrete Struct/Union Definitions
    h_content.append("// --- Concrete Struct/Union Definitions ---")
    for item in all_structs_unions: # Iterate again through original list
        internal_name = item['name']
        item_type = item['type']['name']

        # Check if it needs definition (is concrete and forward declared) and hasn't been defined yet
        is_concrete_struct = (item_type == "struct" and internal_name in forward_declared_concrete_structs)
        is_concrete_union = (item_type == "union" and internal_name in forward_declared_concrete_unions)

        if (is_concrete_struct or is_concrete_union) and internal_name not in processed_struct_union_enum_names:
            h_content.append(f"typedef {item_type} {internal_name} {{")
            fields = item.get("fields")
            if fields:
                for field in fields:
                    field_name = sanitize_name(field["name"])
                    # IMPORTANT: Pass use_opaque_typedef=True here for field types
                    field_type_str, dim = split_c_type_name(get_c_type_name(field["type"], api_data,
                                                     config,
                                                     use_opaque_typedef=True,
                                                     split_separator=True))
                    bitsize = field.get("bitsize")
                    if "unhandled_type" in field_type_str or "unknown_base" in field_type_str:
                         print(f"  Warning: Skipping field '{field_name}' in {item_type} '{internal_name}' due to unhandled type: {field['type']}")
                         h_content.append(f"    /* Field '{field_name}' skipped due to unknown type */")
                         continue
                    if bitsize:
                        h_content.append(f"    {field_type_str} {field_name}{dim} : {bitsize};")
                    else:
                        h_content.append(f"    {field_type_str} {field_name}{dim};")
            else:
                 h_content.append(f"    /* {item_type} {internal_name} has no fields in json */")
            h_content.append(f"}} {internal_name};")
            processed_struct_union_enum_names.add(internal_name) # Mark internal name as defined
            h_content.append("")
    h_content.append("")


    # 7. Macros
    h_content.append("// --- Macro Definitions ---")
    macro_list = sorted(config['filtered_macros'], key=lambda x: x['name'])
    for macro in macro_list:
        name = macro["name"]
        params = macro.get("params")
        initializer = macro.get("initializer", "")
        if initializer is None: initializer = ""
        initializer = initializer.strip().replace('\n', ' \\\n    ')
        if params:
            param_list = [p for p in params if p is not None]
            param_str = ", ".join(param_list)
            h_content.append(f"#define {name}({param_str}) {initializer}")
        else:
            if initializer:
                 common_consts = {"NULL", "true", "false"}
                 if name not in common_consts:
                     h_content.append(f"#define {name} {initializer}")
                 else:
                     h_content.append(f"// #define {name} {initializer} // Skipped standard define")
            else:
                h_content.append(f"#define {name}")
    h_content.append("")

    # 8. Function Prototypes
    h_content.append("// --- Function Prototypes ---")
    func_list = sorted(config['filtered_functions'], key=lambda x: x['name'])
    for func in func_list:
        func_name = func["name"]
        ret_type_info = func.get("type", {}).get("type", {})
        ret_type_str = get_c_type_name(ret_type_info, api_data, config, use_opaque_typedef=True)
        args = func.get("args", [])
        arg_parts = []
        is_void_func = (not args or (len(args) == 1 and get_base_type_name(args[0].get("type")) == "void"))
        if is_void_func:
            param_str = "void"
        else:
            for i, arg in enumerate(args):
                 arg_name = sanitize_name(arg.get("name", f"arg{i}"))
                 arg_type_str, dim = split_c_type_name(
                         get_c_type_name(arg["type"], api_data,
                                                   config,
                                                   use_opaque_typedef=True,
                                                   split_separator=True))
                 if arg_type_str == "...": arg_parts.append("...")
                 else: arg_parts.append(f"{arg_type_str} {arg_name}{dim}")
            param_str = ", ".join(arg_parts)
        h_content.append(f"{ret_type_str} {func_name}({param_str});")


    # Utility Functions
    h_content.append("\n// --- Utility Functions ---")
    h_content.append("void emul_lvgl_init(void);")
    h_content.append("void emul_lvgl_destroy(void);")
    h_content.append("bool emul_lvgl_export(const char *filename, bool pretty);")
    h_content.append("void emul_lvgl_register_external_ptr(const void *ptr, const char *id, const char* type_hint);")


    h_content.append("\n#ifdef __cplusplus")
    h_content.append("}")
    h_content.append("#endif\n")
    h_content.append("#endif // EMUL_LVGL_H")
    return "\n".join(h_content)

# --- C Code Generation --- (No changes needed in generate_c_source from previous version)
def generate_c_source(api_data, config):
    """Generates the C source file content."""
    c_content = []
    c_content.append("// Generated by generate_emul_lvgl_wrapper.py. DO NOT EDIT.")
    c_content.append(f"#include \"{config['output_base_name']}.h\"")
    c_content.append("#include <stdio.h>")
    c_content.append("#include <stdlib.h>")
    c_content.append("#include <string.h>")
    c_content.append("#include <inttypes.h>") # For PRIxPTR, PRIu64
    c_content.append("#include <assert.h>")
    c_content.append("\n// --- Dependencies ---")
    # Assume cJSON.c is compiled separately and linked.
    c_content.append("#include \"cjson/cJSON.h\"")
    # Assume uthash.h is in the include path.
    c_content.append("#include \"uthash.h\"")

    c_content.append("\n// --- Global State ---")
    c_content.append("static cJSON *g_root_json_array = NULL;") # Holds the list of all created objects/externals
    c_content.append("static uint64_t g_id_counters[100]; // Simple counter array for different opaque types")
    c_content.append("static const char* g_id_type_names[100];")
    c_content.append("static int g_id_type_count = 0;")
    c_content.append("#define MAX_ID_TYPES 100")

    # Registry Struct Definition
    c_content.append("\ntypedef struct {")
    c_content.append("    const void *ptr_key; // Key: Original pointer address from user code (use const void* for key safety)")
    c_content.append("    const char *id;      // Value: Unique string ID (e.g., \"label_1\") - owned by entry")
    c_content.append("    cJSON *json_obj;     // Value: Corresponding cJSON object (owned by g_root_json_array or external)")
    c_content.append("    const char *type_name; // Value: Original LVGL type name (e.g. \"lv_obj_t\") - owned by entry")
    c_content.append("    UT_hash_handle hh;   // Makes this structure hashable")
    c_content.append("} lvgl_registry_entry_t;")
    c_content.append("\nstatic lvgl_registry_entry_t *g_object_registry = NULL; // Hash table head")

    # Helper: Get Type Index for ID Counters
    c_content.append("\n// Get internal index for managing ID counters per type")
    c_content.append("static int get_type_index(const char *type_name) {")
    c_content.append("    // Linear search for existing type")
    c_content.append("    for (int i = 0; i < g_id_type_count; ++i) {")
    c_content.append("        if (g_id_type_names[i] != NULL && strcmp(g_id_type_names[i], type_name) == 0) {")
    c_content.append("            return i;")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("    // Add new type if space allows")
    c_content.append("    if (g_id_type_count < MAX_ID_TYPES) {")
    c_content.append("        // Store a copy of the type name string")
    c_content.append("        g_id_type_names[g_id_type_count] = strdup(type_name);")
    c_content.append("        if (!g_id_type_names[g_id_type_count]) {")
    c_content.append("             perror(\"Failed to allocate memory for type name\");")
    c_content.append("             assert(false); // Critical error")
    c_content.append("             return -1;")
    c_content.append("        }")
    c_content.append("        g_id_counters[g_id_type_count] = 0; // Initialize counter")
    c_content.append("        return g_id_type_count++;")
    c_content.append("    } else {")
    c_content.append("        // Error handling if too many unique types")
    c_content.append("        fprintf(stderr, \"Error: Exceeded maximum unique opaque types (%d) for ID generation.\\n\", MAX_ID_TYPES);")
    c_content.append("        fprintf(stderr, \"    Failed type: %s\\n\", type_name);")
    c_content.append("        assert(false); // Critical error")
    c_content.append("        return -1;")
    c_content.append("    }")
    c_content.append("}")

    # Helper: Generate Unique ID String
    c_content.append("\n// Generates a unique ID string (e.g., \"lv_obj_t_1\") - caller owns returned string")
    c_content.append("static char* generate_unique_id(const char *base_type_name) {")
    c_content.append("    int type_index = get_type_index(base_type_name);")
    c_content.append("    if (type_index < 0) return NULL; // Error handled in get_type_index")
    c_content.append("    // Increment counter for this type")
    c_content.append("    uint64_t count = ++g_id_counters[type_index];")
    c_content.append("    // Calculate required buffer size: type_name + '_' + up to 20 digits (for uint64_t) + null terminator")
    c_content.append("    size_t buffer_len = strlen(base_type_name) + 1 + 20 + 1;")
    c_content.append("    char* buffer = (char*)malloc(buffer_len);")
    c_content.append("    if (!buffer) {")
    c_content.append("        perror(\"Failed to allocate buffer for generating ID\");")
    c_content.append("        assert(false); // Critical error")
    c_content.append("        return NULL;")
    c_content.append("    }")
    c_content.append("    // Format the ID string")
    c_content.append("    // Use PRIu64 for platform-independent uint64_t printing")
    c_content.append("    snprintf(buffer, buffer_len, \"%s_%\\\" PRIu64 \\\"\", base_type_name, count);")
    c_content.append("    return buffer; // Ownership transferred to caller")
    c_content.append("}")

    # Helper: Register Opaque Object Pointer
    c_content.append("\n// Registers an opaque pointer with its generated ID and cJSON object")
    c_content.append("static void register_opaque_object(const void *ptr, const char *type_name, char *id, cJSON *json_obj) {")
    c_content.append("    // Check for NULL pointer - cannot register NULL")
    c_content.append("    if (ptr == NULL) {")
    c_content.append("        fprintf(stderr, \"Warning: Attempted to register a NULL pointer for type %s with ID %s. Ignoring.\\n\", type_name, id);")
    c_content.append("        free(id); // Free the generated ID as it won't be used")
    c_content.append("        return;")
    c_content.append("    }")
    c_content.append("    lvgl_registry_entry_t *entry;")
    c_content.append("    // Use HASH_FIND_PTR to check if the pointer address is already a key")
    c_content.append("    HASH_FIND_PTR(g_object_registry, &ptr, entry);")
    c_content.append("    if (entry == NULL) {")
    c_content.append("        // Pointer not found, allocate a new registry entry")
    c_content.append("        entry = (lvgl_registry_entry_t *)malloc(sizeof(lvgl_registry_entry_t));")
    c_content.append("        if (!entry) {")
    c_content.append("             perror(\"Failed to allocate registry entry\"); ")
    c_content.append("             free(id); // Clean up allocated ID")
    c_content.append("             assert(false); return; // Critical error")
    c_content.append("        }")
    c_content.append("        entry->ptr_key = ptr; // Store the const pointer address as key")
    c_content.append("        entry->id = id; // Store the ID string (entry owns it now)")
    c_content.append("        entry->json_obj = json_obj; // Store pointer to the cJSON object")
    c_content.append("        entry->type_name = strdup(type_name); // Store copy of type name (entry owns it)")
    c_content.append("        if (!entry->type_name) {")
    c_content.append("             perror(\"Failed to allocate memory for type name in registry\");")
    c_content.append("             free((void*)entry->id);")
    c_content.append("             free(entry);")
    c_content.append("             assert(false); return; // Critical error")
    c_content.append("        }")
    c_content.append("        // Add the new entry to the hash table")
    c_content.append("        HASH_ADD_PTR(g_object_registry, ptr_key, entry);")
    c_content.append("    } else {")
    c_content.append("        // Pointer already exists in the registry - this might happen with _init functions")
    c_content.append("        // fprintf(stderr, \"Warning: Pointer %p (type %s) already registered with ID '%s'. Updating with new ID '%s'.\\n\", ptr, entry->type_name, entry->id, id);")
    c_content.append("        // Free the old ID and type name associated with the entry")
    c_content.append("        free((void*)entry->id);")
    c_content.append("        free((void*)entry->type_name);")
    c_content.append("        // Update the entry with the new information")
    c_content.append("        entry->id = id; // Entry takes ownership of the new ID")
    c_content.append("        entry->json_obj = json_obj; // Update the cJSON object pointer")
    c_content.append("        entry->type_name = strdup(type_name); // Update the type name (entry owns copy)")
    c_content.append("        if (!entry->type_name) {")
    c_content.append("             perror(\"Failed to allocate memory for updated type name in registry\");")
    c_content.append("             free((void*)entry->id); // Free the new ID as well")
    c_content.append("             // What to do with the entry? Keep old state? Hard to recover.")
    c_content.append("             assert(false); return; // Critical error")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("}")

     # Helper: Find JSON Object by Pointer
    c_content.append("\n// Finds the cJSON object associated with a given opaque pointer")
    c_content.append("static cJSON* find_json_object(const void *ptr) {")
    c_content.append("    if (!ptr) return NULL; // Handle NULL pointers gracefully")
    c_content.append("    lvgl_registry_entry_t *entry;")
    c_content.append("    // Search the hash table using the pointer address as the key")
    c_content.append("    HASH_FIND_PTR(g_object_registry, &ptr, entry);")
    c_content.append("    if (entry) {")
    c_content.append("        return entry->json_obj; // Return the associated cJSON object")
    c_content.append("    } else {")
    c_content.append("        // Pointer not found in the registry")
    c_content.append("        // This can happen if an unregistered external pointer (like a font) is used")
    c_content.append("        // Or if trying to use an object before its constructor/init was called.")
    c_content.append("        // fprintf(stderr, \"Warning: Pointer %p not found in registry during lookup.\\n\", ptr);")
    c_content.append("        return NULL; // Indicate not found")
    c_content.append("    }")
    c_content.append("}")

    # Helper: Find ID by Pointer
    c_content.append("\n// Finds the string ID associated with a given opaque pointer")
    c_content.append("static const char* find_id_by_ptr(const void *ptr) {")
    c_content.append("    if (!ptr) return NULL;")
    c_content.append("    lvgl_registry_entry_t *entry;")
    c_content.append("    HASH_FIND_PTR(g_object_registry, &ptr, entry);")
    c_content.append("    return entry ? entry->id : NULL; // Return ID string or NULL if not found")
    c_content.append("}")

    # Marshaling Functions (Forward Declarations)
    c_content.append("\n// --- Marshaling Function Prototypes ---")
    c_content.append("// Marshals various C types into cJSON values.")
    c_content.append("static cJSON* marshal_value(const void* value_ptr, const char* c_type_str, const char* base_type_name, bool is_pointer, bool is_enum, bool is_struct, bool is_union, bool is_opaque, const char* arg_name);")

    # Specific marshalers for enums and concrete structs/unions
    c_content.append("\n// Forward declarations for specific marshalers")
    for enum in config['filtered_enums']:
        enum_name = enum["name"]
        typedef_name = find_typedef_for_base(enum_name, api_data, config)
        c_enum_name = typedef_name if typedef_name else enum_name
        func_name_suffix = sanitize_name(c_enum_name)
        c_content.append(f"static cJSON* marshal_enum_{func_name_suffix}({c_enum_name} value);")

    for struct in config['filtered_structures']:
        struct_name = struct['name']
        if not is_opaque({"name": struct_name}, api_data, config): # Check if struct is concrete
             typedef_name = find_typedef_for_base(struct_name, api_data, config)
             c_struct_name = typedef_name if typedef_name else f"struct {struct_name}"
             func_name_suffix = sanitize_name(typedef_name if typedef_name else struct_name)
             c_content.append(f"static cJSON* marshal_struct_{func_name_suffix}(const {c_struct_name}* value);")

    for union_def in config['filtered_unions']:
        union_name = union_def['name']
        if not is_opaque({"name": union_name}, api_data, config): # Check if union is concrete
             typedef_name = find_typedef_for_base(union_name, api_data, config)
             c_union_name = typedef_name if typedef_name else f"union {union_name}"
             func_name_suffix = sanitize_name(typedef_name if typedef_name else union_name)
             c_content.append(f"static cJSON* marshal_union_{func_name_suffix}(const {c_union_name}* value);")
    c_content.append("")


    # Marshaling Function Implementations
    c_content.append("// --- Marshaling Function Implementations ---")

    # Enum to String Marshalers
    for enum in config['filtered_enums']:
        enum_name = enum["name"] # Internal name like _lv_align_t
        typedef_name = find_typedef_for_base(enum_name, api_data, config)
        c_enum_name = typedef_name if typedef_name else enum_name # Prefer lv_align_t
        func_name_suffix = sanitize_name(c_enum_name)
        func_name = f"marshal_enum_{func_name_suffix}"

        c_content.append(f"// Marshal enum {c_enum_name} to JSON string")
        c_content.append(f"static cJSON* {func_name}({c_enum_name} value) {{")
        c_content.append(f"    switch (value) {{")
        members = enum.get("members", [])
        if not members:
             c_content.append(f"        // No members defined for {c_enum_name} in JSON")
        else:
            for member in members:
                # Handle potential C incompatibility in names if necessary, but unlikely for enums
                c_content.append(f"        case {member['name']}: return cJSON_CreateString(\"{member['name']}\");")
        c_content.append(f"        default:")
        # Provide numeric fallback if name not found (e.g., for combined flags)
        c_content.append(f"            char buf[40]; // Increased size for name + value")
        c_content.append(f"            snprintf(buf, sizeof(buf), \"{c_enum_name}_VALUE(%%d)\", (int)value);")
        c_content.append(f"            return cJSON_CreateString(buf);")
        c_content.append(f"    }}")
        c_content.append(f"}}")
        c_content.append("")

    # Concrete Struct to JSON Object Marshalers
    for struct in config['filtered_structures']:
        struct_name = struct['name']
        if not is_opaque({"name": struct_name}, api_data, config): # Generate only for concrete structs
            typedef_name = find_typedef_for_base(struct_name, api_data, config)
            c_struct_name = typedef_name if typedef_name else f"struct {struct_name}"
            func_name_suffix = sanitize_name(typedef_name if typedef_name else struct_name)
            func_name = f"marshal_struct_{func_name_suffix}"

            c_content.append(f"// Marshal struct {c_struct_name} to JSON object")
            c_content.append(f"static cJSON* {func_name}(const {c_struct_name}* value) {{")
            c_content.append("    if (!value) return cJSON_CreateNull();")
            c_content.append("    cJSON *obj = cJSON_CreateObject();")
            c_content.append("    if (!obj) return NULL; // Allocation check")
            # Add a type hint for easier parsing on the receiving end
            c_content.append(f"    cJSON_AddStringToObject(obj, \"_struct_type\", \"{struct_name}\");")
            fields = struct.get("fields")
            if fields:
                 for field in fields:
                    field_name_orig = field.get("name")
                    if not field_name_orig: continue # Skip unnamed fields (if any)
                    field_name_c = sanitize_name(field_name_orig)
                    field_type_info = field["type"]

                    # Determine field properties for marshal_value call
                    field_base_name = get_base_type_name(field_type_info)
                    # Get the C type string *as defined in the struct*
                    field_c_type_str = get_c_type_name(field_type_info, api_data, config, use_opaque_typedef=True)
                    field_is_pointer = field_type_info.get("json_type") == "pointer" # or array treated as pointer
                    if field_type_info.get("json_type") == "array" and field_c_type_str.endswith("*"): field_is_pointer = True

                    field_details = get_type_details(field_type_info, api_data)
                    field_is_enum = (field_details.get("json_type") == "enum") if field_details else False
                    field_is_struct = (field_details.get("json_type") == "struct") if field_details else False
                    field_is_union = (field_details.get("json_type") == "union") if field_details else False
                    # Check opacity of the field's base type
                    field_is_opaque = is_opaque({"name": field_base_name} if field_base_name else field_type_info, api_data, config)

                    # Use the generic marshal_value for the field's value
                    c_content.append(f"    cJSON *field_json = marshal_value(&value->{field_name_c}, \"{field_c_type_str}\", \"{field_base_name if field_base_name else ''}\", {str(field_is_pointer).lower()}, {str(field_is_enum).lower()}, {str(field_is_struct).lower()}, {str(field_is_union).lower()}, {str(field_is_opaque).lower()}, \"{field_name_orig}\");")
                    c_content.append(f"    if (field_json) {{") # Check if marshaling succeeded
                    c_content.append(f"        cJSON_AddItemToObject(obj, \"{field_name_orig}\", field_json);") # Use original name as JSON key
                    c_content.append(f"    }} else {{")
                    c_content.append(f"        fprintf(stderr, \"Warning: Failed to marshal field '{field_name_orig}' in struct {struct_name}\\n\");")
                    c_content.append(f"        cJSON_AddNullToObject(obj, \"{field_name_orig}\");") # Add null placeholder
                    c_content.append(f"    }}")
            else:
                c_content.append(f"     // Struct {struct_name} has no fields in JSON definition")
            c_content.append("    return obj;")
            c_content.append(f"}}")
            c_content.append("")

    # Concrete Union to JSON Object Marshalers (Simplified: just pick first field?)
    # Proper union marshaling requires knowing which field is active, which we don't.
    # Best effort: Marshal the raw memory? Or just indicate it's a union?
    for union_def in config['filtered_unions']:
        union_name = union_def['name']
        if not is_opaque({"name": union_name}, api_data, config): # Generate only for concrete unions
            typedef_name = find_typedef_for_base(union_name, api_data, config)
            c_union_name = typedef_name if typedef_name else f"union {union_name}"
            func_name_suffix = sanitize_name(typedef_name if typedef_name else union_name)
            func_name = f"marshal_union_{func_name_suffix}"

            c_content.append(f"// Marshal union {c_union_name} to JSON object (simplified)")
            c_content.append(f"static cJSON* {func_name}(const {c_union_name}* value) {{")
            c_content.append("    if (!value) return cJSON_CreateNull();")
            c_content.append("    cJSON *obj = cJSON_CreateObject();")
            c_content.append("    if (!obj) return NULL;")
            c_content.append(f"    cJSON_AddStringToObject(obj, \"_union_type\", \"{union_name}\");")
            # Option 1: Just indicate the union exists
            c_content.append("    cJSON_AddStringToObject(obj, \"_value\", \"<union_data_omitted>\");")
            # Option 2: Try to marshal the first field? Risky.
            # fields = union_def.get("fields")
            # if fields and len(fields) > 0:
            #     first_field = fields[0]
            #     field_name_orig = first_field.get("name")
            #     # ... marshal first_field ...
            #     # cJSON_AddItemToObject(obj, field_name_orig, marshaled_field);
            c_content.append("    return obj;")
            c_content.append(f"}}")
            c_content.append("")


    # Generic Value Marshaler Implementation
    c_content.append("\n// Generic value marshaler - determines type and calls specific marshalers")
    # Takes pointer to value, C type string, base type name, and flags
    c_content.append("static cJSON* marshal_value(const void* value_ptr, const char* c_type_str, const char* base_type_name, bool is_pointer, bool is_enum, bool is_struct, bool is_union, bool is_opaque, const char* arg_name) {")
    c_content.append("    // Handle NULL pointers early (common case)")
    c_content.append("    if (is_pointer && (*(const void**)value_ptr) == NULL) {")
    c_content.append("         return cJSON_CreateNull();")
    c_content.append("    }")
    # Handle varargs ellipsis type
    c_content.append("    if (strcmp(c_type_str, \"...\") == 0) return cJSON_CreateString(\"<varargs>\");")

    # 1. Opaque Pointers (marshal by ID)
    # Check is_opaque flag first, as it represents managed objects
    c_content.append("    if (is_opaque && is_pointer) {")
    c_content.append("        const void* actual_ptr = *(const void**)value_ptr; // Get the actual pointer value")
    c_content.append("        const char *id = find_id_by_ptr(actual_ptr);")
    c_content.append("        if (id) {")
    c_content.append("            return cJSON_CreateString(id); // Found in registry, marshal as ID string")
    c_content.append("        } else {")
    c_content.append("           // Handle case where pointer is not NULL but not registered")
    c_content.append("           // This might be an external resource not registered via emul_lvgl_register_external_ptr")
    c_content.append("           // Or an internal LVGL pointer we don't track.")
    c_content.append("           char ptr_buf[40];")
    c_content.append("           snprintf(ptr_buf, sizeof(ptr_buf), \"<unregistered_opaque:%s:%p>\", base_type_name ? base_type_name : \"?\", actual_ptr);")
    c_content.append("           fprintf(stderr, \"Warning: Opaque pointer %p ('%s', type %s) not found in registry for marshaling.\\n\", actual_ptr, arg_name ? arg_name : \"?\", base_type_name ? base_type_name : \"?\");")
    c_content.append("           return cJSON_CreateString(ptr_buf);")
    c_content.append("        }")
    c_content.append("    }")

    # 2. Enums (passed by value) - Use specific marshalers based on C type string
    c_content.append("    if (is_enum && !is_pointer) {")
    # Dispatch to specific enum marshaler based on c_type_str (which should be the enum's typedef name)
    enum_found = False
    for enum in config['filtered_enums']:
        enum_name = enum["name"]
        typedef_name = find_typedef_for_base(enum_name, api_data, config)
        c_enum_name = typedef_name if typedef_name else enum_name
        func_name_suffix = sanitize_name(c_enum_name)
        c_content.append(f"        if (strcmp(c_type_str, \"{c_enum_name}\") == 0) {{")
        # Cast the void* value_ptr to the correct enum type pointer and dereference
        c_content.append(f"            return marshal_enum_{func_name_suffix}(*({c_enum_name}*)value_ptr);")
        c_content.append(f"        }}")
        enum_found = True # Mark that we have potential handlers
    if enum_found:
         c_content.append("        // Fallback for enum type string not matching generated functions")
    c_content.append("        return cJSON_CreateNumber((double)(*(int*)value_ptr)); // Assume underlying int")
    c_content.append("    }")

    # 3. Concrete Structs (passed by pointer)
    c_content.append("    if (is_struct && is_pointer && !is_opaque) {")
    struct_found = False
    # Dispatch based on base_type_name or c_type_str? base_type_name is safer maybe.
    for struct in config['filtered_structures']:
         struct_name = struct['name']
         if not is_opaque({"name": struct_name}, api_data, config): # Only for concrete structs
            typedef_name = find_typedef_for_base(struct_name, api_data, config)
            # Match against either the struct name or its typedef name
            match_name = typedef_name if typedef_name else struct_name
            func_name_suffix = sanitize_name(match_name)
            c_struct_type = typedef_name if typedef_name else f"struct {struct_name}"
            # Check c_type_str carefully against pointer types
            c_content.append(f"        if (strcmp(base_type_name, \"{struct_name}\") == 0 || strcmp(c_type_str, \"{match_name}*\") == 0 || strcmp(c_type_str, \"const {match_name}*\") == 0 || strcmp(c_type_str, \"struct {struct_name}*\") == 0 || strcmp(c_type_str, \"const struct {struct_name}*\") == 0) {{")
            # Value ptr is const void**, dereference once to get const struct*
            c_content.append(f"            return marshal_struct_{func_name_suffix}(*(const {c_struct_type}**)value_ptr);")
            c_content.append(f"        }}")
            struct_found = True
    if struct_found:
        c_content.append("        // Fallback for unknown concrete struct pointer")
    c_content.append("        char buf_sp[64]; snprintf(buf_sp, sizeof(buf_sp), \"<unhandled_struct_ptr:%s>\", base_type_name ? base_type_name : c_type_str); return cJSON_CreateString(buf_sp);")
    c_content.append("    }")

    # 4. Concrete Structs (passed by value)
    c_content.append("    if (is_struct && !is_pointer && !is_opaque) {")
    struct_val_found = False
    # Special handling for lv_color_t passed by value
    c_content.append(f"        if (strcmp(c_type_str, \"lv_color_t\") == 0) {{")
    c_content.append(f"            return marshal_struct_lv_color_t((const lv_color_t*)value_ptr);")
    c_content.append(f"        }}")
    struct_val_found = True # Assume lv_color_t exists

    # Generic handling for other structs passed by value
    for struct in config['filtered_structures']:
         struct_name = struct['name']
         # Skip lv_color_t as it's handled above
         if struct_name != "lv_color_t" and not is_opaque({"name": struct_name}, api_data, config):
            typedef_name = find_typedef_for_base(struct_name, api_data, config)
            match_name = typedef_name if typedef_name else struct_name
            func_name_suffix = sanitize_name(match_name)
            c_struct_type = typedef_name if typedef_name else f"struct {struct_name}"
            # Match C type string (should be the typedef name if used)
            c_content.append(f"        if (strcmp(c_type_str, \"{match_name}\") == 0 || strcmp(c_type_str, \"const {match_name}\") == 0) {{")
            # Value_ptr points to the struct value on the stack/in memory
            c_content.append(f"            return marshal_struct_{func_name_suffix}((const {c_struct_type}*)value_ptr);")
            c_content.append(f"        }}")
            struct_val_found = True
    if struct_val_found:
        c_content.append("        // Fallback for unknown struct value")
    c_content.append("        char buf_sv[64]; snprintf(buf_sv, sizeof(buf_sv), \"<unhandled_struct_val:%s>\", c_type_str); return cJSON_CreateString(buf_sv);")
    c_content.append("    }")

    # 5. Concrete Unions (passed by pointer or value) - Simplified handling
    c_content.append("    if (is_union && !is_opaque) {")
    union_found = False
    # Check if passed by pointer
    c_content.append("        if (is_pointer) {")
    for union_def in config['filtered_unions']:
        union_name = union_def['name']
        if not is_opaque({"name": union_name}, api_data, config):
            typedef_name = find_typedef_for_base(union_name, api_data, config)
            match_name = typedef_name if typedef_name else union_name
            func_name_suffix = sanitize_name(match_name)
            c_union_type = typedef_name if typedef_name else f"union {union_name}"
            # Adjust type check for pointers
            c_content.append(f"            if (strcmp(base_type_name, \"{union_name}\") == 0 || strcmp(c_type_str, \"{match_name}*\") == 0 || strcmp(c_type_str, \"const {match_name}*\") == 0 || strcmp(c_type_str, \"union {union_name}*\") == 0 || strcmp(c_type_str, \"const union {union_name}*\") == 0) {{")
            c_content.append(f"                return marshal_union_{func_name_suffix}(*(const {c_union_type}**)value_ptr);")
            c_content.append(f"            }}")
            union_found = True
    c_content.append("        } else { // Passed by value")
    for union_def in config['filtered_unions']:
        union_name = union_def['name']
        if not is_opaque({"name": union_name}, api_data, config):
            typedef_name = find_typedef_for_base(union_name, api_data, config)
            match_name = typedef_name if typedef_name else union_name
            func_name_suffix = sanitize_name(match_name)
            c_union_type = typedef_name if typedef_name else f"union {union_name}"
            c_content.append(f"            if (strcmp(c_type_str, \"{match_name}\") == 0 || strcmp(c_type_str, \"const {match_name}\") == 0) {{")
            c_content.append(f"                return marshal_union_{func_name_suffix}((const {c_union_type}*)value_ptr);")
            c_content.append(f"            }}")
            union_found = True
    c_content.append("        }")
    if union_found:
        c_content.append("        // Fallback for unknown union")
    c_content.append("        char buf_un[64]; snprintf(buf_un, sizeof(buf_un), \"<unhandled_union:%s>\", c_type_str); return cJSON_CreateString(buf_un);")
    c_content.append("    }")


    # 6. Basic Types (int, float, bool, char*, char, void*)
    c_content.append("    // Check specific C standard types passed by value or pointer")
    c_content.append("    // Note: value_ptr always points to the value, even if passed by value.")

    c_content.append("    // String (const char* or char*)")
    c_content.append("    if (strcmp(c_type_str, \"const char*\") == 0 || strcmp(c_type_str, \"char*\") == 0) {")
    c_content.append("        // value_ptr is const void**, dereference to get the char*")
    c_content.append("        const char* str_val = *(const char**)value_ptr;")
    c_content.append("        return str_val ? cJSON_CreateString(str_val) : cJSON_CreateNull();")
    c_content.append("    }")

    c_content.append("    // Boolean (bool)")
    c_content.append("    if (strcmp(c_type_str, \"bool\") == 0) {")
    c_content.append("        return cJSON_CreateBool(*(bool*)value_ptr);")
    c_content.append("    }")

    c_content.append("    // Floating point (float, double)")
    c_content.append("    if (strcmp(c_type_str, \"float\") == 0 || strcmp(c_type_str, \"double\") == 0 || strcmp(c_type_str, \"const float\") == 0 || strcmp(c_type_str, \"const double\") == 0) {")
    c_content.append("        // Promote float to double for cJSON")
    c_content.append("        if (strstr(c_type_str, \"float\")) return cJSON_CreateNumber((double)(*(float*)value_ptr));")
    c_content.append("        else return cJSON_CreateNumber(*(double*)value_ptr);")
    c_content.append("    }")

    c_content.append("    // Integer types (char, short, int, long, size_t, stdint types)")
    c_content.append("    // Use string comparisons based on the C type name")
    c_content.append("    if (strcmp(c_type_str, \"char\") == 0 || strcmp(c_type_str, \"signed char\") == 0 || strcmp(c_type_str, \"const char\") == 0 || strcmp(c_type_str, \"const signed char\") == 0) {")
    c_content.append("        return cJSON_CreateNumber((double)(*(signed char*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"unsigned char\") == 0 || strcmp(c_type_str, \"uint8_t\") == 0 || strcmp(c_type_str, \"const unsigned char\") == 0 || strcmp(c_type_str, \"const uint8_t\") == 0 || strcmp(c_type_str, \"lv_opa_t\") == 0) { // Include lv_opa_t here")
    c_content.append("        return cJSON_CreateNumber((double)(*(unsigned char*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"short\") == 0 || strcmp(c_type_str, \"signed short\") == 0 || strcmp(c_type_str, \"int16_t\") == 0 || strcmp(c_type_str, \"const short\") == 0 || strcmp(c_type_str, \"const signed short\") == 0 || strcmp(c_type_str, \"const int16_t\") == 0 || strcmp(c_type_str, \"lv_state_t\") == 0) { // Include lv_state_t")
    c_content.append("        return cJSON_CreateNumber((double)(*(short*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"unsigned short\") == 0 || strcmp(c_type_str, \"uint16_t\") == 0 || strcmp(c_type_str, \"const unsigned short\") == 0 || strcmp(c_type_str, \"const uint16_t\") == 0) {")
    c_content.append("        return cJSON_CreateNumber((double)(*(unsigned short*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"int\") == 0 || strcmp(c_type_str, \"signed int\") == 0 || strcmp(c_type_str, \"int32_t\") == 0 || strcmp(c_type_str, \"const int\") == 0 || strcmp(c_type_str, \"const signed int\") == 0 || strcmp(c_type_str, \"const int32_t\") == 0 || strcmp(c_type_str, \"lv_coord_t\") == 0 || strcmp(c_type_str, \"lv_res_t\") == 0) { // Include lv_coord_t, lv_res_t")
    c_content.append("        return cJSON_CreateNumber((double)(*(int*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"unsigned int\") == 0 || strcmp(c_type_str, \"uint32_t\") == 0 || strcmp(c_type_str, \"const unsigned int\") == 0 || strcmp(c_type_str, \"const uint32_t\") == 0 || strcmp(c_type_str, \"lv_part_t\") == 0 || strcmp(c_type_str, \"lv_style_selector_t\") == 0) { // Include lv_part_t, lv_style_selector_t")
    c_content.append("        // Use unsigned long for intermediate to potentially hold uint32_t correctly before double conversion")
    c_content.append("        return cJSON_CreateNumber((double)(*(unsigned int*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"long\") == 0 || strcmp(c_type_str, \"signed long\") == 0 || strcmp(c_type_str, \"intptr_t\") == 0 || strcmp(c_type_str, \"int64_t\") == 0 || strcmp(c_type_str, \"const long\") == 0 || strcmp(c_type_str, \"const signed long\") == 0 || strcmp(c_type_str, \"const intptr_t\") == 0 || strcmp(c_type_str, \"const int64_t\") == 0) {")
    c_content.append("        // Use long long for intermediate for portability before double conversion")
    c_content.append("        return cJSON_CreateNumber((double)(*(long long*)value_ptr));")
    c_content.append("    }")
    c_content.append("    if (strcmp(c_type_str, \"unsigned long\") == 0 || strcmp(c_type_str, \"uintptr_t\") == 0 || strcmp(c_type_str, \"size_t\") == 0 || strcmp(c_type_str, \"uint64_t\") == 0 || strcmp(c_type_str, \"const unsigned long\") == 0 || strcmp(c_type_str, \"const uintptr_t\") == 0 || strcmp(c_type_str, \"const size_t\") == 0 || strcmp(c_type_str, \"const uint64_t\") == 0) {")
    c_content.append("        // Use unsigned long long for intermediate for portability before double conversion")
    c_content.append("        return cJSON_CreateNumber((double)(*(unsigned long long*)value_ptr));")
    c_content.append("    }")


    # 7. Non-Opaque Pointers (void*, function pointers, pointers to primitives/concrete structs)
    c_content.append("    if (is_pointer && !is_opaque && !is_struct && !is_union) {")
    c_content.append("        const void* actual_ptr = *(const void**)value_ptr;")
    c_content.append("        // Handle void* pointers (e.g., user_data) - represent as string address?")
    c_content.append("        if (base_type_name && strcmp(base_type_name, \"void\") == 0) {") # Check base_type_name exists
    c_content.append("            char ptr_buf[32]; snprintf(ptr_buf, sizeof(ptr_buf), \"<void*:%p>\", actual_ptr);")
    c_content.append("            return cJSON_CreateString(ptr_buf);")
    c_content.append("        }")
    c_content.append("        // Handle function pointers - represent as string name?")
    c_content.append("        // Base type name might be the function pointer typedef name")
    c_content.append("        bool is_func_ptr = false;")
    c_content.append("        if (base_type_name) {")
    c_content.append("             // Check if base_type_name corresponds to a function_pointer definition")
    c_content.append("             lvgl_registry_entry_t* details_entry = NULL; // Placeholder")
    c_content.append("             // Quick check on name convention")
    c_content.append("             if (strstr(base_type_name, \"_cb_t\") || strstr(base_type_name, \"_f_t\")) is_func_ptr = true;")
    c_content.append("             // Add lookup in api_data if needed for more accuracy")
    c_content.append("             // details = get_type_details({\"name\": base_type_name}, api_data);")
    c_content.append("             // if (details && details.get(\"json_type\") == \"function_pointer\") is_func_ptr = true;")
    c_content.append("        }")
    c_content.append("        if (is_func_ptr) {")
    c_content.append("            char func_buf[64]; snprintf(func_buf, sizeof(func_buf), \"<func_ptr:%s:%p>\", base_type_name ? base_type_name : \"?\", actual_ptr);")
    c_content.append("            return cJSON_CreateString(func_buf);")
    c_content.append("        }")

    c_content.append("        // Other pointers (e.g., pointer to primitive, like int*) - Marshal the value pointed to?")
    c_content.append("        // This requires knowing the pointee type again. Let's try.")
    c_content.append("        if (base_type_name) {")
    c_content.append("             // Marshal the value pointed to by actual_ptr, treating it as non-pointer value")
    c_content.append("             // Need to determine properties of the *pointee* type")
    c_content.append("             bool pointee_is_enum = false, pointee_is_struct = false, pointee_is_union = false, pointee_is_opaque = false;") # Opaque pointee shouldn't happen here
    c_content.append("             // Lookup details for base_type_name")
    c_content.append("             // Simplified: Assume primitives/stdlib if not struct/union/enum")
    c_content.append("             // Let's fallback to representing address for now to avoid complexity.")
    c_content.append("             char ptr_buf[64]; snprintf(ptr_buf, sizeof(ptr_buf), \"<pointer:%s:%p>\", base_type_name, actual_ptr);")
    c_content.append("             return cJSON_CreateString(ptr_buf);")
    c_content.append("             // Proper implementation would call: return marshal_value(actual_ptr, pointee_c_type_str, base_type_name, false, pointee_is_enum, ...);")
    c_content.append("        }")
    c_content.append("         // Fallback if base_type_name is null or pointee handling fails")
    c_content.append("         char ptr_buf_fallback[64]; snprintf(ptr_buf_fallback, sizeof(ptr_buf_fallback), \"<unhandled_pointer:%p>\", actual_ptr);")
    c_content.append("         return cJSON_CreateString(ptr_buf_fallback);")
    c_content.append("    }")

    c_content.append("    // Fallback for completely unknown types/combinations")
    c_content.append("    fprintf(stderr, \"Warning: Could not marshal value for C type '%s' (base: %s, ptr:%d, enum:%d, struct:%d, union:%d, opaque:%d, arg: %s)\\n\",")
    c_content.append("            c_type_str, base_type_name ? base_type_name : \"N/A\", is_pointer, is_enum, is_struct, is_union, is_opaque, arg_name ? arg_name : \"N/A\");")
    c_content.append("    char unknown_buf[128];")
    c_content.append("    snprintf(unknown_buf, sizeof(unknown_buf), \"<unknown_marshal_type:%s>\", c_type_str);")
    c_content.append("    return cJSON_CreateString(unknown_buf);")
    c_content.append("}")


    # --- Init/Destroy ---
    c_content.append("\n// --- Initialization and Cleanup ---")
    c_content.append("\n// Initializes the wrapper, call once before using any wrapped functions.")
    c_content.append("void emul_lvgl_init(void) {")
    c_content.append("    // Check if already initialized")
    c_content.append("    if (g_root_json_array != NULL) {")
    c_content.append("        // fprintf(stderr, \"Warning: emul_lvgl_init called multiple times. Resetting state.\\n\");")
    c_content.append("        emul_lvgl_destroy(); // Clean up previous state first")
    c_content.append("    }")
    c_content.append("\n    // Create the root JSON array to hold all objects")
    c_content.append("    g_root_json_array = cJSON_CreateArray();")
    c_content.append("    if (!g_root_json_array) {")
    c_content.append("        fprintf(stderr, \"Fatal Error: Failed to create root cJSON array.\\n\");")
    c_content.append("        assert(false); // Cannot proceed")
    c_content.append("        return;")
    c_content.append("    }")
    c_content.append("\n    // Reset ID generation state")
    c_content.append("    memset(g_id_counters, 0, sizeof(g_id_counters));")
    c_content.append("    // Free any existing type name strings from previous runs")
    c_content.append("    for(int i = 0; i < g_id_type_count; ++i) {")
    c_content.append("        free((void*)g_id_type_names[i]);")
    c_content.append("        g_id_type_names[i] = NULL;")
    c_content.append("    }")
    c_content.append("    g_id_type_count = 0;")
    c_content.append("\n    // Reset object registry (should be empty after destroy, but double check)")
    c_content.append("    assert(g_object_registry == NULL);")
    c_content.append("    g_object_registry = NULL; // Ensure it's NULL")
    c_content.append("\n    // printf(\"LVGL JSON Wrapper Initialized.\\n\");")
    c_content.append("}")

    c_content.append("\n// Cleans up resources used by the wrapper.")
    c_content.append("void emul_lvgl_destroy(void) {")
    c_content.append("    // Delete the main JSON structure (owns all created object JSONs)")
    c_content.append("    if (g_root_json_array) {")
    c_content.append("        cJSON_Delete(g_root_json_array);")
    c_content.append("        g_root_json_array = NULL;")
    c_content.append("    }")
    c_content.append("\n    // Clear the hash table and free associated memory owned by the registry")
    c_content.append("    lvgl_registry_entry_t *current_entry, *tmp;")
    c_content.append("    HASH_ITER(hh, g_object_registry, current_entry, tmp) {")
    c_content.append("        HASH_DEL(g_object_registry, current_entry); // Remove from hash table")
    c_content.append("        free((void*)current_entry->id); // Free the ID string")
    c_content.append("        free((void*)current_entry->type_name); // Free the type name string")
    c_content.append("        // NOTE: current_entry->json_obj is owned by g_root_json_array, which was deleted above.")
    c_content.append("        free(current_entry); // Free the registry entry struct itself")
    c_content.append("    }")
    c_content.append("    g_object_registry = NULL; // Ensure head is NULL")
    c_content.append("\n    // Free type name strings used for ID generation")
    c_content.append("    for(int i = 0; i < g_id_type_count; ++i) {")
    c_content.append("        free((void*)g_id_type_names[i]);")
    c_content.append("        g_id_type_names[i] = NULL;")
    c_content.append("    }")
    c_content.append("    g_id_type_count = 0;")
    c_content.append("    memset(g_id_counters, 0, sizeof(g_id_counters));")
    c_content.append("\n    // printf(\"LVGL JSON Wrapper Destroyed.\\n\");")
    c_content.append("}")

    # --- Export ---
    c_content.append("\n// Exports the current UI state to a JSON file.")
    c_content.append("bool emul_lvgl_export(const char *filename, bool pretty) {")
    c_content.append("    if (!g_root_json_array) {")
    c_content.append("        fprintf(stderr, \"Error: emul_lvgl_init() not called or no objects created before export.\\n\");")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("\n    // Create the final output structure { \"lvgl_simulation\": [...] }")
    c_content.append("    cJSON *output_root = cJSON_CreateObject();")
    c_content.append("    if (!output_root) {")
    c_content.append("        fprintf(stderr, \"Error: Failed to create output root cJSON object.\\n\");")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("\n    // Add the array of objects to the root object.")
    c_content.append("    // We add the actual array; its ownership is temporarily transferred.")
    c_content.append("    cJSON_AddItemToObject(output_root, \"lvgl_simulation\", g_root_json_array);")
    c_content.append("\n    // Print the JSON object to a string")
    c_content.append("    char *json_string = NULL;")
    c_content.append("    if (pretty) {")
    c_content.append("        json_string = cJSON_Print(output_root); // Use pretty print")
    c_content.append("    } else {")
    c_content.append("        json_string = cJSON_PrintUnformatted(output_root); // Use compact print")
    c_content.append("    }")
    c_content.append("\n    // Detach the array from the output root *before* checking json_string")
    c_content.append("    // so that g_root_json_array is not deleted if cJSON_Delete(output_root) is called on error.")
    c_content.append("    cJSON_DetachItemViaPointer(output_root, g_root_json_array);")
    c_content.append("\n    if (!json_string) {")
    c_content.append("        fprintf(stderr, \"Error: Failed to print JSON to string (memory allocation failed?).\\n\");")
    c_content.append("        cJSON_Delete(output_root); // Delete the container object")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("\n    // Write the string to the specified file")
    c_content.append("    FILE *fp = fopen(filename, \"w\");")
    c_content.append("    if (!fp) {")
    c_content.append("        perror(\"Error opening output file for writing\");")
    c_content.append("        fprintf(stderr, \"    Filename: %s\\n\", filename);")
    c_content.append("        free(json_string);")
    c_content.append("        cJSON_Delete(output_root);")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("\n    // Write the JSON string and close the file")
    c_content.append("    size_t write_len = strlen(json_string);")
    c_content.append("    if (fwrite(json_string, 1, write_len, fp) != write_len) {")
    c_content.append("        perror(\"Error writing JSON string to file\");")
    c_content.append("        fclose(fp);")
    c_content.append("        free(json_string);")
    c_content.append("        cJSON_Delete(output_root);")
    c_content.append("        // Attempt to remove potentially corrupted file")
    c_content.append("        remove(filename);")
    c_content.append("        return false;")
    c_content.append("    }")
    c_content.append("\n    fclose(fp);")
    c_content.append("    printf(\"LVGL JSON wrapper: Exported UI state to %s\\n\", filename);")
    c_content.append("\n    // Clean up")
    c_content.append("    free(json_string);")
    c_content.append("    cJSON_Delete(output_root); // Delete the container object")
    c_content.append("\n    return true;")
    c_content.append("}")

    # --- External Pointer Registration ---
    c_content.append("\n// Registers an external pointer (e.g., font, image src) with a specific ID.")
    c_content.append("void emul_lvgl_register_external_ptr(const void *ptr, const char *id, const char* type_hint) {")
    c_content.append("    // Basic validation")
    c_content.append("    if (!ptr || !id || !type_hint) {")
    c_content.append("        fprintf(stderr, \"Error: Invalid NULL arguments provided to emul_lvgl_register_external_ptr (ptr=%p, id=%s, type_hint=%s).\\n\", ptr, id ? id : \"NULL\", type_hint ? type_hint : \"NULL\");")
    c_content.append("        assert(false); // Indicate programming error")
    c_content.append("        return;")
    c_content.append("    }")
    c_content.append("    if (strlen(id) == 0) {")
    c_content.append("        fprintf(stderr, \"Error: Empty string provided for ID in emul_lvgl_register_external_ptr.\\n\");")
    c_content.append("        assert(false);")
    c_content.append("        return;")
    c_content.append("    }")
    c_content.append("\n    // Ensure the system is initialized")
    c_content.append("    if (!g_root_json_array) {")
    c_content.append("         fprintf(stderr, \"Warning: emul_lvgl_init() was not called before emul_lvgl_register_external_ptr. Initializing now.\\n\");")
    c_content.append("         emul_lvgl_init();")
    c_content.append("    }")
    c_content.append("\n    // Check if the pointer OR the ID is already registered")
    c_content.append("    lvgl_registry_entry_t *entry_by_ptr = NULL, *entry_by_id = NULL, *tmp = NULL;")
    c_content.append("    HASH_FIND_PTR(g_object_registry, &ptr, entry_by_ptr);")
    c_content.append("    // Need to iterate to check for ID collision")
    c_content.append("    HASH_ITER(hh, g_object_registry, tmp, entry_by_id) {") # entry_by_id gets overwritten here
    c_content.append("        if (strcmp(tmp->id, id) == 0) {")
    c_content.append("            entry_by_id = tmp; // Found entry with the same ID")
    c_content.append("            break;")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("\n    if (entry_by_ptr != NULL) {")
    c_content.append("        // Pointer is already registered. Check if ID matches.")
    c_content.append("        if (strcmp(entry_by_ptr->id, id) == 0) {")
    c_content.append("            // Already registered with the same ID, nothing to do.")
    c_content.append("            // fprintf(stderr, \"Debug: Pointer %p already registered as '%s'. Skipping duplicate registration.\\n\", ptr, id);")
    c_content.append("            return;")
    c_content.append("        } else {")
    c_content.append("            // Pointer registered with a DIFFERENT ID. This indicates a problem.")
    c_content.append("            fprintf(stderr, \"Error: Pointer %p already registered with ID '%s', cannot re-register with ID '%s'.\\n\", ptr, entry_by_ptr->id, id);")
    c_content.append("            assert(false); // Indicate programming error")
    c_content.append("            return;")
    c_content.append("        }")
    c_content.append("    }")
    c_content.append("\n    if (entry_by_id != NULL) {")
    c_content.append("        // ID is already registered, but the pointer was not found. This is an ID collision.")
    c_content.append("        fprintf(stderr, \"Error: ID '%s' is already registered for pointer %p. Cannot register pointer %p with the same ID.\\n\", id, entry_by_id->ptr_key, ptr);")
    c_content.append("        assert(false); // Indicate programming error")
    c_content.append("        return;")
    c_content.append("    }")
    c_content.append("\n    // Create a placeholder JSON object for this external resource")
    c_content.append("    cJSON *ext_obj = cJSON_CreateObject();")
    c_content.append("    if (!ext_obj) {")
    c_content.append("        perror(\"Failed to create cJSON object for external pointer registration\");")
    c_content.append("        assert(false); return; // Critical error")
    c_content.append("    }")
    c_content.append("    // Use copies of id and type_hint for safety")
    c_content.append("    char* id_copy = strdup(id);")
    c_content.append("    if (!id_copy) { perror(\"strdup failed for id\"); cJSON_Delete(ext_obj); assert(false); return; }")
    c_content.append("\n    cJSON_AddStringToObject(ext_obj, \"id\", id_copy); // Add the copy")
    c_content.append("    cJSON_AddStringToObject(ext_obj, \"type\", type_hint);")
    c_content.append("    cJSON_AddStringToObject(ext_obj, \"source\", \"external\");")
    c_content.append("\n    // Add to the global array (g_root_json_array owns ext_obj now)")
    c_content.append("    cJSON_AddItemToArray(g_root_json_array, ext_obj);")
    c_content.append("\n    // Register the pointer -> ID mapping (Registry takes ownership of id_copy)")
    c_content.append("    // Note: We pass ext_obj here, but it's just metadata; the pointer itself isn't a cJSON object.")
    c_content.append("    register_opaque_object(ptr, type_hint, id_copy, ext_obj); // Pass id_copy")
    c_content.append("}")


    # --- Wrapped Function Implementations ---
    c_content.append("\n// --- Wrapped LVGL Function Implementations ---")
    func_list = sorted(config['filtered_functions'], key=lambda x: x['name']) # Sort for consistent order
    for func in func_list:
        func_name = func["name"]
        func_type = get_function_type(func, api_data, config) # Pass api_data and config
        ret_type_wrapper = func.get("type", {})
        ret_type_info = ret_type_wrapper.get("type", {}) # Actual return type info
        ret_type_str = get_c_type_name(ret_type_info, api_data, config, use_opaque_typedef=True)
        ret_is_ptr = ret_type_info.get("json_type") == "pointer"
        ret_pointee_type = ret_type_info.get("type") if ret_is_ptr else None
        ret_is_opaque_ptr = ret_is_ptr and ret_pointee_type and is_opaque(ret_pointee_type, api_data, config)
        ret_base_name = get_base_type_name(ret_pointee_type if ret_is_ptr else ret_type_info)


        args = func.get("args", [])
        arg_parts = []
        arg_names = []
        is_void_func = (not args or (len(args) == 1 and get_base_type_name(args[0].get("type")) == "void"))

        if is_void_func:
            param_str = "void"
        else:
            for i, arg in enumerate(args):
                 arg_name = sanitize_name(arg.get("name", f"arg{i}"))
                 arg_names.append(arg_name)
                 arg_type_str, dim = split_c_type_name(get_c_type_name(arg["type"], api_data,
                                                   config,
                                                   use_opaque_typedef=True,
                                                   split_separator=True))
                 if arg_type_str == "...":
                      arg_parts.append("...")
                 else:
                      arg_parts.append(f"{arg_type_str} {arg_name}{dim}")
            param_str = ", ".join(arg_parts)

        # Function signature
        c_content.append(f"\n// Wrapper for: {func_name}")
        c_content.append(f"{ret_type_str} {func_name}({param_str}) {{")
        # Add comment indicating determined type
        c_content.append(f"    /* Wrapper Type: {func_type} */")

        # Ensure initialized at the start of every wrapped function
        c_content.append("    if (!g_root_json_array) emul_lvgl_init();")

        # --- Constructor Logic ---
        if func_type == "constructor":
            # 1. Create cJSON object for the new LVGL object
            c_content.append("    cJSON *new_obj_json = cJSON_CreateObject();")
            c_content.append("    if (!new_obj_json) { perror(\"cJSON_CreateObject failed in constructor\"); assert(false); return NULL; } // Return NULL on failure") # Constructor must return pointer
            # 2. Generate unique ID based on the *returned* opaque pointer type
            return_pointee_type_name = get_base_type_name(ret_pointee_type) if ret_pointee_type else "unknown_opaque"
            c_content.append(f"    const char* type_name = \"{return_pointee_type_name}\";")
            c_content.append(f"    char* obj_id = generate_unique_id(type_name);") # obj_id is owned by us
            c_content.append("    if (!obj_id) { cJSON_Delete(new_obj_json); assert(false); return NULL; }") # Handle ID generation failure
            c_content.append("    cJSON_AddStringToObject(new_obj_json, \"id\", obj_id);") # Add ID to JSON
            c_content.append("    cJSON_AddStringToObject(new_obj_json, \"type\", type_name);")
            c_content.append(f"    cJSON_AddStringToObject(new_obj_json, \"constructor\", \"{func_name}\");")
            # 3. Add placeholder for properties/calls array
            c_content.append("    cJSON *props_array = cJSON_CreateArray();")
            c_content.append("    if (!props_array) { perror(\"cJSON_CreateArray failed\"); free(obj_id); cJSON_Delete(new_obj_json); assert(false); return NULL; }")
            c_content.append("    cJSON_AddItemToObject(new_obj_json, \"props\", props_array);")
            # 4. Marshal constructor arguments into an args array
            c_content.append("    cJSON *constructor_args_array = cJSON_CreateArray();")
            c_content.append("    if (!constructor_args_array) { perror(\"cJSON_CreateArray failed\"); free(obj_id); cJSON_Delete(new_obj_json); assert(false); return NULL; }")
            c_content.append("    cJSON_AddItemToObject(new_obj_json, \"constructor_args\", constructor_args_array);")
            if not is_void_func:
                for i, arg in enumerate(args):
                    arg_name = arg_names[i]
                    arg_type_info = arg["type"]
                    arg_c_type = get_c_type_name(arg_type_info, api_data, config, use_opaque_typedef=True)
                    arg_base_name = get_base_type_name(arg_type_info)
                    arg_is_pointer = arg_type_info.get("json_type") == "pointer" or arg_c_type.endswith("*")
                    arg_details = get_type_details(arg_type_info, api_data)
                    arg_is_enum = (arg_details.get("json_type") == "enum") if arg_details else False
                    arg_is_struct = (arg_details.get("json_type") == "struct") if arg_details else False
                    arg_is_union = (arg_details.get("json_type") == "union") if arg_details else False
                    arg_is_opaque = is_opaque(arg_type_info, api_data, config)
                    # Marshal the argument's value
                    c_content.append(f"    cJSON* marshaled_arg = marshal_value(&{arg_name}, \"{arg_c_type}\", \"{arg_base_name if arg_base_name else ''}\", {str(arg_is_pointer).lower()}, {str(arg_is_enum).lower()}, {str(arg_is_struct).lower()}, {str(arg_is_union).lower()}, {str(arg_is_opaque).lower()}, \"{arg_name}\");")
                    c_content.append(f"    if (marshaled_arg) {{")
                    c_content.append(f"        cJSON_AddItemToArray(constructor_args_array, marshaled_arg);")
                    c_content.append(f"    }} else {{")
                    c_content.append(f"        cJSON_AddItemToArray(constructor_args_array, cJSON_CreateNull()); // Add null if marshal failed")
                    c_content.append(f"    }}")

            # 5. Add the new JSON object to the global array (root array takes ownership)
            c_content.append("    cJSON_AddItemToArray(g_root_json_array, new_obj_json);")
            # 6. Register the mapping from the returned pointer (which *is* the cJSON* cast) to the ID
            #    The registry takes ownership of obj_id here.
            c_content.append(f"    register_opaque_object((const void*)new_obj_json, type_name, obj_id, new_obj_json);")
            # 7. Return the cJSON pointer cast to the expected opaque C type
            c_content.append(f"    return ({ret_type_str})new_obj_json;")

        # --- Init Logic ---
        elif func_type == "init":
             # Assume first arg is the pointer to the opaque object to initialize
             if is_void_func or not args:
                 c_content.append("    /* Warning: Init function with no arguments? Skipping. */")
             else:
                target_ptr_arg_name = arg_names[0]
                target_arg = args[0]
                target_type_info = target_arg["type"] # This is pointer type
                target_pointee_type_info = target_type_info.get("type") # This is the struct/type being initialized
                target_pointee_type_name = get_base_type_name(target_pointee_type_info)

                # This check should have been done in get_function_type, but double check
                if not (target_pointee_type_info and is_opaque(target_pointee_type_info, api_data, config)):
                     c_content.append(f"    /* Warning: Init function '{func_name}' called on non-opaque type '{target_pointee_type_name}'. Skipping JSON generation. */")
                else:
                    # 1. Create cJSON object for the initialized LVGL object state
                    c_content.append("    cJSON *new_obj_json = cJSON_CreateObject();")
                    c_content.append("    if (!new_obj_json) { perror(\"cJSON_CreateObject failed in init\"); assert(false); /* Cannot return if void */ return; }")
                    # 2. Generate unique ID based on the pointee type name
                    c_content.append(f"    const char* type_name = \"{target_pointee_type_name}\";")
                    c_content.append(f"    char* obj_id = generate_unique_id(type_name);") # obj_id is owned by us
                    c_content.append("    if (!obj_id) { cJSON_Delete(new_obj_json); assert(false); return; }")
                    c_content.append("    cJSON_AddStringToObject(new_obj_json, \"id\", obj_id);")
                    c_content.append("    cJSON_AddStringToObject(new_obj_json, \"type\", type_name);")
                    c_content.append(f"    cJSON_AddStringToObject(new_obj_json, \"initializer_func\", \"{func_name}\");")
                    # 3. Add placeholder for properties/calls array
                    c_content.append("    cJSON *props_array = cJSON_CreateArray();")
                    c_content.append("    if (!props_array) { perror(\"cJSON_CreateArray failed\"); free(obj_id); cJSON_Delete(new_obj_json); assert(false); return; }")
                    c_content.append("    cJSON_AddItemToObject(new_obj_json, \"props\", props_array);")
                    # 4. Marshal init arguments (skipping the first pointer arg) into an args array
                    c_content.append("    cJSON *init_args_array = cJSON_CreateArray();")
                    c_content.append("    if (!init_args_array) { perror(\"cJSON_CreateArray failed\"); free(obj_id); cJSON_Delete(new_obj_json); assert(false); return; }")
                    c_content.append("    cJSON_AddItemToObject(new_obj_json, \"init_args\", init_args_array);")
                    if len(arg_names) > 1:
                        for i, arg in enumerate(args[1:], start=1):
                            arg_name = arg_names[i]
                            arg_type_info = arg["type"]
                            arg_c_type = get_c_type_name(arg_type_info, api_data, config, use_opaque_typedef=True)
                            arg_base_name = get_base_type_name(arg_type_info)
                            arg_is_pointer = arg_type_info.get("json_type") == "pointer" or arg_c_type.endswith("*")
                            arg_details = get_type_details(arg_type_info, api_data)
                            arg_is_enum = (arg_details.get("json_type") == "enum") if arg_details else False
                            arg_is_struct = (arg_details.get("json_type") == "struct") if arg_details else False
                            arg_is_union = (arg_details.get("json_type") == "union") if arg_details else False
                            arg_is_opaque = is_opaque(arg_type_info, api_data, config)
                            # Marshal the argument's value
                            c_content.append(f"    cJSON* marshaled_arg = marshal_value(&{arg_name}, \"{arg_c_type}\", \"{arg_base_name if arg_base_name else ''}\", {str(arg_is_pointer).lower()}, {str(arg_is_enum).lower()}, {str(arg_is_struct).lower()}, {str(arg_is_union).lower()}, {str(arg_is_opaque).lower()}, \"{arg_name}\");")
                            c_content.append(f"    if (marshaled_arg) {{")
                            c_content.append(f"        cJSON_AddItemToArray(init_args_array, marshaled_arg);")
                            c_content.append(f"    }} else {{")
                            c_content.append(f"        cJSON_AddItemToArray(init_args_array, cJSON_CreateNull());")
                            c_content.append(f"    }}")

                    # 5. Add the new JSON object to the global array (root array takes ownership)
                    c_content.append("    cJSON_AddItemToArray(g_root_json_array, new_obj_json);")
                    # 6. Register the *passed C pointer address* (target_ptr_arg_name) mapping to the ID
                    #    Registry takes ownership of obj_id here.
                    c_content.append(f"    register_opaque_object((const void*){target_ptr_arg_name}, type_name, obj_id, new_obj_json);")

             # Handle return value (init functions usually return void)
             if ret_type_str != "void":
                 c_content.append(f"    /* Warning: Init function '{func_name}' has non-void return type '{ret_type_str}'. Returning default. */")
                 if ret_is_opaque_ptr: c_content.append("    return NULL;")
                 elif ret_type_str.endswith("*"): c_content.append("    return NULL;")
                 elif ret_type_str == "bool": c_content.append("    return false;")
                 else: c_content.append("    return 0;") # Default for int-like types

        # --- Setter Logic ---
        elif func_type == "setter":
             # Assume first arg is the target opaque object pointer
             if is_void_func or not args:
                 c_content.append(f"    /* Warning: Setter function '{func_name}' with no arguments? Skipping. */")
             else:
                target_ptr_arg_name = arg_names[0]
                target_arg = args[0]
                target_type_info = target_arg["type"] # Pointer type
                target_pointee_type_info = target_type_info.get("type")

                # This check should be redundant due to get_function_type, but safer
                if not (target_pointee_type_info and is_opaque(target_pointee_type_info, api_data, config)):
                     c_content.append(f"    /* Warning: Setter function '{func_name}' called on non-opaque type '{get_base_type_name(target_pointee_type_info)}'. Skipping JSON generation. */")
                else:
                    # 1. Find the cJSON object associated with the target pointer
                    c_content.append(f"    cJSON *target_obj_json = find_json_object({target_ptr_arg_name});")
                    c_content.append(f"    if (!target_obj_json) {{")
                    # Pointer not found - might be an unregistered external or uninitialized object
                    c_content.append(f"        fprintf(stderr, \"Error: Setter '{func_name}' called on unknown or unregistered opaque pointer %p.\\n\", (void*){target_ptr_arg_name});")
                    c_content.append(f"        // Check if it's maybe NULL")
                    c_content.append(f"        if ({target_ptr_arg_name} == NULL) fprintf(stderr, \"       (Pointer was NULL)\\n\");")
                    c_content.append(f"        // Abort or skip? Let's skip and return default if needed.")
                    if ret_type_str != "void":
                        if ret_is_opaque_ptr: c_content.append("        return NULL;")
                        elif ret_type_str.endswith("*"): c_content.append("        return NULL;")
                        elif ret_type_str == "bool": c_content.append("        return false;")
                        else: c_content.append("        return 0;")
                    else:
                        c_content.append("        return;") # Void return
                    c_content.append(f"    }}") # End if (!target_obj_json)

                    # 2. Get the properties array from the target JSON object
                    c_content.append(f"    cJSON *props_array = cJSON_GetObjectItem(target_obj_json, \"props\");")
                    c_content.append(f"    if (!props_array || !cJSON_IsArray(props_array)) {{")
                    c_content.append(f"       fprintf(stderr, \"Error: 'props' array missing or invalid for object in {func_name} (ID: %s)\\n\", cJSON_GetStringValue(cJSON_GetObjectItem(target_obj_json, \"id\")));")
                    c_content.append(f"       // Attempt to fix by creating the array?")
                    c_content.append(f"       if (props_array) cJSON_DeleteItemFromObject(target_obj_json, \"props\");")
                    c_content.append(f"       props_array = cJSON_CreateArray();")
                    c_content.append(f"       if (props_array) cJSON_AddItemToObject(target_obj_json, \"props\", props_array);")
                    c_content.append(f"       else {{ /* Still failed, skip */ ")
                    if ret_type_str != "void":
                         if ret_is_opaque_ptr: c_content.append(" return NULL;");
                         elif ret_type_str.endswith("*"): c_content.append(" return NULL;");
                         elif ret_type_str == "bool": c_content.append(" return false;");
                         else: c_content.append(" return 0;");
                    else: c_content.append(" return;");
                    c_content.append("        }}") # End else (create failed)
                    c_content.append(f"    }}") # End if (!props_array)

                    # 3. Create a new JSON object for this property setting call
                    c_content.append("    cJSON *prop_entry = cJSON_CreateObject();")
                    c_content.append("    if (!prop_entry) { perror(\"cJSON_CreateObject failed for prop_entry\"); assert(false); /* Skip this call */ return; }")

                    # Determine a readable property name from the function name
                    prop_name = func_name
                    prefixes_to_strip = ["lv_obj_set_", "lv_style_set_", "lv_group_", "lv_timer_", "lv_anim_", "lv_indev_", "lv_display_"]
                    # Find longest matching prefix to strip for widget-specific setters too
                    # e.g., lv_label_set_text -> label_set_text -> text
                    # e.g., lv_scale_add_section -> scale_add_section
                    best_match_len = 0
                    prefix_found = False
                    for prefix in prefixes_to_strip:
                         if prop_name.startswith(prefix) and len(prefix) > best_match_len:
                             best_match_len = len(prefix)
                             prefix_found = True

                    if prefix_found:
                         prop_name = prop_name[best_match_len:]
                    elif prop_name.startswith("lv_"):
                         # Try removing widget name prefix if possible
                         parts = prop_name.split('_', 2) # lv_widget_action
                         if len(parts) == 3 and parts[1] != "obj": # Avoid lv_obj_something -> obj_something
                             potential_prop = parts[2]
                             # Check if second part matches known widget types? Too complex maybe.
                             # Let's just strip lv_ for non-obj/style cases for now
                             prop_name = prop_name[3:]

                    # Add the property name
                    c_content.append(f"    cJSON_AddStringToObject(prop_entry, \"name\", \"{prop_name}\");")

                    # 4. Marshal the arguments passed to the setter (excluding the first object ptr)
                    prop_values_code = [] # Store C code snippets that create the marshaled values
                    if len(arg_names) == 1:
                         # Setter with only the target object (e.g., lv_obj_invalidate) - maybe record just the call?
                         # Let's add a value of true to indicate the action was called.
                         prop_values_code.append(f"cJSON_CreateTrue()")
                    else:
                         # Marshal remaining arguments
                         for i, arg in enumerate(args[1:], start=1):
                             arg_name = arg_names[i]
                             arg_type_info = arg["type"]
                             arg_c_type = get_c_type_name(arg_type_info, api_data, config, use_opaque_typedef=True)
                             arg_base_name = get_base_type_name(arg_type_info)
                             arg_is_pointer = arg_type_info.get("json_type") == "pointer" or arg_c_type.endswith("*")
                             arg_details = get_type_details(arg_type_info, api_data)
                             arg_is_enum = (arg_details.get("json_type") == "enum") if arg_details else False
                             arg_is_struct = (arg_details.get("json_type") == "struct") if arg_details else False
                             arg_is_union = (arg_details.get("json_type") == "union") if arg_details else False
                             arg_is_opaque = is_opaque(arg_type_info, api_data, config)

                             # Marshal the argument's value using the generic marshal function
                             marshaled_val_code = f"marshal_value(&{arg_name}, \"{arg_c_type}\", \"{arg_base_name if arg_base_name else ''}\", {str(arg_is_pointer).lower()}, {str(arg_is_enum).lower()}, {str(arg_is_struct).lower()}, {str(arg_is_union).lower()}, {str(arg_is_opaque).lower()}, \"{arg_name}\")"
                             prop_values_code.append(marshaled_val_code)


                    # Add the marshaled value(s) to the property entry JSON
                    if len(prop_values_code) == 1:
                        c_content.append(f"    cJSON *value_json = {prop_values_code[0]};")
                        c_content.append(f"    if (value_json) cJSON_AddItemToObject(prop_entry, \"value\", value_json); else cJSON_AddNullToObject(prop_entry, \"value\");")
                    elif len(prop_values_code) > 1:
                        # Store multiple arguments in a JSON array associated with the "value" key
                        c_content.append(f"    cJSON *values_array = cJSON_CreateArray();")
                        c_content.append(f"    if (values_array) {{") # Check creation
                        for i, val_code in enumerate(prop_values_code):
                            arg_name_val = arg_names[i+1] # Original name for potential debugging
                            c_content.append(f"        cJSON *val_{i} = {val_code};")
                            c_content.append(f"        if (val_{i}) cJSON_AddItemToArray(values_array, val_{i}); else cJSON_AddItemToArray(values_array, cJSON_CreateNull());")
                        c_content.append(f"        cJSON_AddItemToObject(prop_entry, \"value\", values_array);")
                        c_content.append(f"    }} else {{")
                        c_content.append(f"        perror(\"Failed to create values array for setter\");")
                        c_content.append(f"        cJSON_AddNullToObject(prop_entry, \"value\"); // Indicate missing values")
                        c_content.append(f"    }}")
                    else:
                         # Should have handled len==1 case earlier, but as fallback:
                         c_content.append(f"    cJSON_AddNullToObject(prop_entry, \"value\"); // No value arguments")

                    # 5. Add the completed property entry object to the props array
                    c_content.append("    cJSON_AddItemToArray(props_array, prop_entry);")

             # Handle return value for setters (usually void)
             if ret_type_str != "void":
                 c_content.append(f"    /* Warning: Setter function '{func_name}' has non-void return type '{ret_type_str}'. Returning default. */")
                 if ret_is_opaque_ptr: c_content.append("    return NULL;")
                 elif ret_type_str.endswith("*"): c_content.append("    return NULL;")
                 elif ret_type_str == "bool": c_content.append("    return false;")
                 else: c_content.append("    return 0;") # Default for int-like types


        # --- Other Function Types (Getters, Utilities) ---
        else: # func_type == "other"
            c_content.append(f"    /* Function '{func_name}' classified as 'other'. No JSON generated. */")
            # Provide sensible default return values based on return type string
            if ret_type_str == "void":
                 c_content.append("    return;")
            elif ret_is_opaque_ptr:
                 c_content.append(f"    /* Returning NULL for opaque pointer type {ret_type_str} */")
                 c_content.append(f"    return NULL;")
            elif ret_type_str.endswith("*"): # Other pointers
                c_content.append(f"    /* Returning NULL for pointer type {ret_type_str} */")
                c_content.append("    return NULL;")
            elif ret_type_str == "bool":
                c_content.append(f"    /* Returning false for bool */")
                c_content.append("    return false;")
            elif ret_type_str == "float" or ret_type_str == "double":
                c_content.append(f"    /* Returning 0.0 for float/double */")
                c_content.append("    return 0.0;")
            elif ret_type_str == "lv_color_t": # Special case for color getter maybe?
                 c_content.append("    /* Returning black for lv_color_t */")
                 # Assuming lv_color_t struct is defined (which it should be by the header generator)
                 c_content.append("    lv_color_t black_color = {0}; // Initialize to zero/black")
                 c_content.append("    return black_color;")
            elif ret_type_str == "lv_area_t":
                 c_content.append("    /* Returning zero area for lv_area_t */")
                 c_content.append("    lv_area_t zero_area = {0};")
                 c_content.append("    return zero_area;")
            elif ret_type_str == "lv_point_t":
                 c_content.append("    /* Returning zero point for lv_point_t */")
                 c_content.append("    lv_point_t zero_point = {0};")
                 c_content.append("    return zero_point;")
            else: # Assume integer-like types (int, enum, coord, etc.)
                c_content.append(f"    /* Returning 0 for integer-like type {ret_type_str} */")
                c_content.append("    return 0;")

        c_content.append("}") # End of function implementation wrapper

    return "\n".join(c_content)


# --- Main Script Logic ---

def main():
    parser = argparse.ArgumentParser(
        description="Generate C wrapper for LVGL API to output JSON.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter # Show defaults
    )
    parser.add_argument("json_api_file", help="Path to the LVGL API JSON description file.")
    parser.add_argument("-o", "--output-dir", default="generated_emul_lvgl", help="Directory to output the C header and source files.")
    parser.add_argument("-p", "--output-prefix", default="emul_lvgl", help="Prefix for the output C files (e.g., 'lvgl_wrap' -> lvgl_wrap.h, lvgl_wrap.c).")

    # Include/Exclude Lists
    parser.add_argument("--include-funcs", nargs='+', default=["lv_"], help="Prefixes for functions to include.")
    parser.add_argument("--exclude-funcs", nargs='*', default=[
        "lv_log_", "lv_mem_", "lv_tick_", "lv_timer_handler", "lv_init", "lv_deinit", "lv_is_initialized",
        "lv_debug_", "lv_profiler_", "lv_assert_", "lv_memcpy", "lv_memset", "lv_memscpy",
        "lv_malloc", "lv_free", "lv_realloc", # Exclude memory management
        "lv_event_send", "lv_event_get_", "lv_event_register_id", "lv_event_dsc_get_", # Exclude event system internals (keep add/remove)
        "lv_async_", # Exclude async calls for now
        "lv_anim_custom_get", "lv_anim_get_timer", "lv_anim_get_playtime", "lv_anim_path_", # Exclude anim internals
        "lv_draw_", # Exclude low-level drawing funcs and units
        "lv_display_set_buffers", "lv_display_add_event", "lv_display_get_event", "lv_display_send_event", "lv_display_flush_", "lv_display_wait_", # Exclude display driver setup/internals
        "lv_indev_read", "lv_indev_search", "lv_indev_get_read_timer", "lv_indev_read_timer_cb", # Exclude input driver details
        "lv_obj_class_create_obj", "lv_obj_class_init_obj", "lv_obj_assign_spec_attr", "lv_obj_get_disp", # Exclude class internals
        "lv_obj_invalidate_area", "lv_obj_redraw", # Low level redraw
        "lv_global_get", # Internal global access
        "lv_snapshot_", # Exclude snapshotting funcs
        "lv_theme_apply", # Theme application is complex, maybe exclude?
        "lv_image_decoder_get_", "lv_image_decoder_built_in_", "lv_image_cache_", # Image internals
        "lv_font_get_", "lv_font_load", "lv_font_free", # Font management internals
        "lv_fs_", # File system internals (keep open/close maybe?)
        "lv_task_", "lv_timer_exec", # Task/timer execution
        "lv_style_get_prop", "lv_style_prop_lookup_flags", # Style internals
        "lv_rand", "lv_srand", # Random numbers
        # Functions known to use callbacks or be problematic for simulation
        "lv_timer_create", # Has callback
        "lv_obj_add_event_cb", "lv_obj_remove_event", "lv_obj_remove_event_cb", # Event callbacks
        "lv_anim_set_exec_cb", "lv_anim_set_custom_exec_cb", "lv_anim_set_start_cb", "lv_anim_set_completed_cb", "lv_anim_set_deleted_cb", "lv_anim_set_get_value_cb", # Anim callbacks
        "lv_observer_create_with_handler", # Observer callback
        "lv_subject_add_observer_with_handler", # Subject callback
        "lv_group_set_focus_cb", "lv_group_set_edge_cb", # Group callbacks
        "lv_display_set_flush_cb", "lv_display_set_flush_wait_cb", # Display callbacks
        "lv_indev_set_read_cb", # Input callback
        # Varargs functions (difficult to marshal reliably)
        "lv_label_set_text_fmt", "lv_snprintf", "lv_subject_snprintf",
        # Other
        "lv_line_", "lv_canvas_get_image", "lv_group_get_edge_cb",
        "lv_group_get_focus_cb"
        "lv_image_buf_", "lv_image_get_bitmap_map_src",
        "lv_image_set_bitmap_map_src", "lv_indev_get_read_cb",
        "lv_indev_", "lv_refr_now", "lv_ll_clear_custom",
        "lv_font_glyph_release_draw_data", "lv_group_get_focus_cb",
        "lv_obj_init_draw_image_dsc", "lv_screen_load",
        "lv_obj_init_draw_label_dsc", "lv_obj_init_draw_image_dsc",
        "lv_obj_init_draw_line_dsc", "lv_obj_init_draw_rect_dsc",
        "lv_obj_init_draw_rect_dsc", "lv_obj_init_draw_arc_dsc",
        "lv_point_precise_", "lv_point_from_precise", "lv_tree_node_",
        "lv_utils_bsearch"
        # Deprecated
        "lv_obj_set_style_local_", "lv_obj_get_style_",
        ], help="Prefixes for functions to exclude.")
    parser.add_argument("--include-enums", nargs='+', default=["lv_"], help="Prefixes for enums to include.")
    parser.add_argument("--exclude-enums", nargs='*', default=["_lv_"], help="Prefixes for enums to exclude (e.g., internal ones).")
    parser.add_argument("--include-structs", nargs='+', default=["lv_"], help="Prefixes for structs to include.")
    parser.add_argument("--exclude-structs", nargs='*', default=[
        "_lv_", "lv_global_t", "lv_ll_t", "lv_rb_t", "lv_event_dsc_t", "lv_ts_calibration_t",
        "lv_draw_unit_t", "lv_draw_task_t", "lv_draw_buf_t", # Exclude low-level draw structs unless needed
        "lv_font_fmt_", # Font format internals
        "lv_anim_path_t", # Internal anim path
        "lv_fs_drv_t", "lv_font_glyph_dsc_t",
        "lv_draw_", "lv_yuv_buf_t", "lv_tree_class_t", "lv_yuv_plane_t",
        "lv_point_precise_t", "lv_image_flags_t", "_lvimage_flags_t",
        "lv_tree_node_t"
        ], help="Prefixes for structs to exclude (often internal).")
    parser.add_argument("--include-unions", nargs='+', default=["lv_"], help="Prefixes for unions to include.")
    parser.add_argument("--exclude-unions", nargs='*', default=["_lv_",
    "lv_yuv_buf_t"], help="Prefixes for unions to exclude.")
    parser.add_argument("--include-typedefs", nargs='+', default=["lv_"], help="Prefixes for typedefs to include.")
    parser.add_argument("--exclude-typedefs", nargs='*', default=[], help="Prefixes for typedefs to exclude.")
    parser.add_argument("--include-macros", nargs='+', default=["LV_"], help="Prefixes for macros to include.")
    parser.add_argument("--exclude-macros", nargs='*', default=[
        "LV_UNUSED", "LV_ASSERT", "LV_LOG_", "LV_TRACE_", "LV_ATTRIBUTE_", "LV_DEPRECATED",
        "LV_EXPORT_CONST_INT", "LV_USE_", "LV_CONF_", "LV_ENABLE_", "_LV_", "LV_INDEV_DEF_",
        "LV_VERSION_", "LV_BIG_ENDIAN", "LV_LITTLE_ENDIAN", "LV_ARCH_", "LV_COMPILER_",
        "LV_DRAW_SW_", # Exclude low-level SW draw defines
        "LV_COLOR_DEPTH", "LV_COLOR_16_SWAP", "LV_COLOR_SCREEN_TRANSP", # Config defines
        "LV_MEM_", # Memory config
        "LV_GC_", # Garbage collector config
        "LV_ASSERT_", # Assertion macros
        "LV_DPX", # Screen density - hard to simulate generically
        "LV_GRIDNAV_", # Gridnav internals
        "LV_IMAGE_HEADER_MAGIC", # Internal magic number
        ], help="Prefixes for macros to exclude.")
    parser.add_argument("--opaque-types", nargs='*', default=DEFAULT_OPAQUE_TYPE_PREFIXES, help="Prefixes of types to *always* treat as opaque.")

    args = parser.parse_args()

    # --- Load API Data ---
    try:
        with open(args.json_api_file, 'r', encoding='utf-8') as f:
            api_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: API JSON file not found at {args.json_api_file}")
        exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Could not parse API JSON file {args.json_api_file}: {e}")
        exit(1)
    except Exception as e:
        print(f"An unexpected error occurred reading the JSON file: {e}")
        exit(1)

    # --- Filter API Data ---
    config = {
        "output_base_name": args.output_prefix,
        "opaque_type_prefixes": args.opaque_types,
        "filtered_functions": [],
        "filtered_enums": [],
        "filtered_structures": [],
        "filtered_unions": [],
        "filtered_typedefs": [],
        "filtered_macros": [],
        "opaque_types": set(), # Will be populated based on heuristics AND opaque_type_prefixes
        "all_known_types": set() # All type names encountered from included items
    }

    print("Filtering API elements...")
    # Define categories and keys for filtering
    categories_to_filter = [
        ("functions", "name", args.include_funcs, args.exclude_funcs),
        ("enums", "name", args.include_enums, args.exclude_enums),
        ("structures", "name", args.include_structs, args.exclude_structs),
        ("unions", "name", args.include_unions, args.exclude_unions),
        ("typedefs", "name", args.include_typedefs, args.exclude_typedefs),
        ("macros", "name", args.include_macros, args.exclude_macros),
    ]

    # First pass: Filter based on include/exclude prefixes
    for category, key, inc_list, exc_list in categories_to_filter:
        filtered_items = []
        original_count = len(api_data.get(category, []))
        for item in api_data.get(category, []):
            item_name = item.get(key)
            if item_name and matches_prefix(item_name, inc_list) and not matches_prefix(item_name, exc_list):
                # --- Additional Filtering Logic ---
                skip_item = False
                if category == "functions":
                    # Exclude functions with varargs (...)
                    if any(arg and arg.get("type", {}).get("json_type") == "special_type" and arg["type"].get("name") == "ellipsis" for arg in item.get("args", [])):
                        print(f"  Excluding varargs function: {item_name}")
                        skip_item = True
                    # Exclude functions taking function pointers as arguments (simplification)
                    elif any(arg and get_type_details(arg.get("type"), api_data) and get_type_details(arg.get("type"), api_data).get("json_type") == "function_pointer" for arg in item.get("args", [])):
                        print(f"  Excluding function with function pointer arg: {item_name}")
                        skip_item = True
                    # Exclude functions with common callback typedef names in args
                    elif any( (t_name := arg and get_c_type_name(arg.get("type"), api_data, config, use_opaque_typedef=False)) and ("_cb_t" in t_name or "_f_t" in t_name) for arg in item.get("args",[])):
                        # Check if the callback type itself is excluded
                        base_cb_name = get_base_type_name(arg.get("type")) if arg else None
                        if base_cb_name and not matches_prefix(base_cb_name, exc_list): # Only exclude if the CB type wasn't explicitly excluded already
                            print(f"  Excluding function with callback arg type: {item_name}")
                            skip_item = True


                if not skip_item:
                    filtered_items.append(item)

        config[f"filtered_{category}"] = filtered_items
        print(f"  {category.capitalize()}: Kept {len(filtered_items)} out of {original_count}")


    # Second pass: Collect all type names used by included elements
    print("Collecting used type names...")
    def collect_type_names(type_info, known_types):
        if not type_info or not isinstance(type_info, dict):
            return
        base_name = get_base_type_name(type_info)
        if base_name:
            known_types.add(base_name)
        # Recurse for pointers, arrays, typedefs, struct/union fields
        json_type = type_info.get("json_type")
        if json_type in ("pointer", "array", "ret_type"):
             collect_type_names(type_info.get("type"), known_types)
        elif json_type == "typedef":
             # Also collect the typedef name itself
             td_name = type_info.get("name")
             if td_name: known_types.add(td_name)
             collect_type_names(type_info.get("type"), known_types)
        elif json_type in ("struct", "union"):
             # Need full details to recurse into fields
             details = get_type_details(type_info, api_data)
             # Also add the struct/union name itself
             su_name = type_info.get("name")
             if su_name: known_types.add(su_name)
             if details and "fields" in details:
                 for field in details["fields"]:
                     collect_type_names(field.get("type"), known_types)

    # Collect from functions (return types and args)
    for func in config['filtered_functions']:
        collect_type_names(func.get("type", {}).get("type"), config["all_known_types"])
        for arg in func.get("args", []):
            collect_type_names(arg.get("type"), config["all_known_types"])
    # Collect from struct/union fields
    for struct in config['filtered_structures']:
        if "fields" in struct:
            for field in struct["fields"]:
                collect_type_names(field.get("type"), config["all_known_types"])
    for union_def in config['filtered_unions']:
        if "fields" in union_def:
            for field in union_def["fields"]:
                collect_type_names(field.get("type"), config["all_known_types"])
    # Collect from typedef underlying types
    for typedef in config['filtered_typedefs']:
        collect_type_names(typedef.get("type"), config["all_known_types"])
    # Add names of included structs, enums, unions, typedefs themselves
    for category in ["enums", "structures", "unions", "typedefs"]:
        for item in config[f"filtered_{category}"]:
            name = item.get("name")
            if name: config["all_known_types"].add(name)

    print(f"  Found {len(config['all_known_types'])} unique type names used by included elements.")

    # --- Determine Opaque Types ---
    print("Determining opaque types...")
    # Use the collected type names and heuristics/config lists
    for type_name in list(config["all_known_types"]): # Iterate over a copy
        type_info_stub = {"name": type_name} # Create a stub to pass to is_opaque
        if is_opaque(type_info_stub, api_data, config):
            config["opaque_types"].add(type_name)
            # Add base types of opaque types too if they exist (e.g. struct _lv_obj_t for typedef lv_obj_t)
            details = get_type_details(type_info_stub, api_data)
            if details and details.get("json_type") == "typedef":
                 underlying_type = details.get("type")
                 if underlying_type:
                     base = get_base_type_name(underlying_type)
                     if base and base != type_name:
                         # Check if the underlying base itself should be opaque
                         if is_opaque(underlying_type, api_data, config):
                              config["opaque_types"].add(base)

    print(f"  Identified {len(config['opaque_types'])} opaque types/prefixes based on usage and config.")
    # print(f"  Opaque types: {sorted(list(config['opaque_types']))}") # Debug print

    # --- Generate Code ---
    print("Generating C header file...")
    header_code = generate_header(api_data, config)

    print("Generating C source file...")
    source_code = generate_c_source(api_data, config)

    # --- Write Output Files ---
    os.makedirs(args.output_dir, exist_ok=True)
    header_path = os.path.join(args.output_dir, f"{args.output_prefix}.h")
    source_path = os.path.join(args.output_dir, f"{args.output_prefix}.c")

    try:
        with open(header_path, 'w', encoding='utf-8') as f:
            f.write(header_code)
        print(f"Successfully wrote header file: {header_path}")
    except IOError as e:
        print(f"Error writing header file {header_path}: {e}")
        exit(1)
    except Exception as e:
        print(f"An unexpected error occurred writing the header file: {e}")
        exit(1)


    try:
        with open(source_path, 'w', encoding='utf-8') as f:
            f.write(source_code)
        print(f"Successfully wrote source file: {source_path}")
    except IOError as e:
        print(f"Error writing source file {source_path}: {e}")
        exit(1)
    except Exception as e:
        print(f"An unexpected error occurred writing the source file: {e}")
        exit(1)

    print("\nGeneration complete. Remember to:")
    print(f"1. Add '{os.path.basename(source_path)}' and cJSON source to your build system.")
    print(f"2. Include '{os.path.basename(header_path)}' in your application code.")
    print(f"3. Ensure 'uthash.h' is in your compiler's include path.")
    print(f"4. Call `emul_lvgl_init()` before using any wrapped LVGL functions.")
    print(f"5. Call `emul_lvgl_register_external_ptr()` for any fonts, images, etc., declared outside the wrapper *before* they are used in setters/functions.")
    print(f"6. Call `emul_lvgl_export(\"output.json\", true)` (true for pretty-print) to generate the simulation output.")
    print(f"7. Call `emul_lvgl_destroy()` when your application exits to clean up resources.")

if __name__ == "__main__":
    main()
