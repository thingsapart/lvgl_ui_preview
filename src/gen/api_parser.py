# api_parser.py
import json
import logging

# Default lists (can be overridden by command-line args)
DEFAULT_FUNCTION_INCLUDES = [
    "lv_obj_create", "lv_style_init", "lv_color_hex", "lv_color_mix", "lv_pct" # Common examples
    # Add more specific create/init functions if needed, or rely on prefixes
]
DEFAULT_FUNCTION_INCLUDE_PREFIXES = ["lv_obj_set_", "lv_style_set_", "lv_"] # Broad lv_ prefix, more specific setters
DEFAULT_FUNCTION_EXCLUDE_PREFIXES = ["_lv_", "lv_obj_class", "lv_group_",
                                     "lv_theme_", "lv_event_", "lv_timer_",
                                     "lv_anim_", "lv_indev_", "lv_display_",
                                     "lv_font_manager_", "lv_freetype_", "lv_barcode_",
                                     "lv_fragment_", "lv_imgfont", "lv_bmp_", "lv_gif",
                                     "lv_log_register_print_", "lv_log_add",
                                     "lv_calendar_", "lv_gridnav_", "lv_draw_buf_",
                                     "lv_iter_", "lv_keyboard_set_map", "lv_anim",
                                     "lv_color_brightness", "lv_led", "lv_libpng",
                                     "lv_libjpeg", "lv_lodepng",
                                     "lv_obj_get_id", "lv_obj_free_id", "lv_obj_assign_id",
                                     "lv_obj_stringify_id", "lv_obj_find_by_id", "lv_obj_id_compare",
                                     "lv_objid_builtin_destroy",
                                     "lv_qrcode", "lv_rle", "lv_snapshot", "lv_bidi",
                                     "lv_tiny_ttf", "lv_tjpgd",
                                     "lv_draw_", "lv_obj_set_id", "lv_ll_clear_custom"
                                     ] # Exclude internal/complex subsystems by default

DEFAULT_ENUM_INCLUDES = ['lv_align_t', 'lv_opa_enum_t', 'lv_layout_t', 
                         'lv_text_align_t', 'lv_flex_align_t', 'lv_flex_flow_t', 
                         'lv_grid_align_t', 'lv_text_decor_t', 'lv_scrollbar_mode_t', 
                         'lv_style_parts_t', 'lv_arc_mode_t', 'lv_bar_mode_t', 
                         'lv_bar_orientation_t', 'lv_buttonmatrix_ctrl_t', 'lv_scale_mode_t', 
                         'lv_slider_mode_t', 'lv_slider_orientation_t', 'lv_span_overflow_t', 
                         'lv_span_mode_t', 'lv_table_cell_ctrl_t', 'lv_gridnav_ctrl_t', ]
DEFAULT_ENUM_INCLUDE_PREFIXES = ["LV_"]
DEFAULT_ENUM_EXCLUDES = []
DEFAULT_ENUM_EXCLUDE_PREFIXES = ["_LV_"]

logger = logging.getLogger(__name__)

def djb2_hash(s):
    hash_val = 5381
    for char in s:
        hash_val = ((hash_val << 5) + hash_val) + ord(char) # hash * 33 + c
        hash_val &= 0xFFFFFFFF # Ensure it's a 32-bit unsigned integer
    return hash_val

def _get_base_type_name(type_obj):
    """Drills down into nested type objects to find the innermost name."""
    while isinstance(type_obj, dict) and 'type' in type_obj:
        # Handle pointers, arrays, ret_types etc. that wrap other types
        if type_obj.get('json_type') == 'pointer':
            type_obj = type_obj['type']
        elif type_obj.get('json_type') == 'array':
             # Array type name is often directly in 'name'
             return type_obj.get('name', 'unknown_array_type')
        elif type_obj.get('json_type') == 'ret_type':
            type_obj = type_obj['type']
        # Handle typedefs specifically if they don't have a nested 'type'
        elif type_obj.get('json_type') == 'typedef':
            # Sometimes typedefs have the target type directly under 'type'
            if 'type' in type_obj and isinstance(type_obj['type'], dict):
                 type_obj = type_obj['type']
            else:
                 # If not, the typedef name itself might be what we need, or it's complex
                 return type_obj.get('name', 'unknown_typedef')
        elif 'name' in type_obj:
             return type_obj.get('name')
        else:
             # Reached end of nesting or unexpected structure
             break
    # Final attempt if it's a simple type object
    if isinstance(type_obj, dict) and 'name' in type_obj:
        return type_obj.get('name')
    return 'unknown_type'


def get_full_type_info(type_obj, api_data):
    """Resolves typedefs and pointers to get C type and pointer level."""
    c_type = "unknown"
    pointer_level = 0
    is_array = False
    typedef_map = {t['name']: t for t in api_data.get('typedefs', [])}

    current_type = type_obj
    visited_typedefs = set() # Prevent infinite loops

    while isinstance(current_type, dict):
        json_type = current_type.get('json_type')

        if json_type == 'pointer':
            pointer_level += 1
            if 'type' not in current_type: break
            current_type = current_type['type']
        elif json_type == 'array':
            is_array = True
            # Arrays often point to the element type name directly
            c_type = current_type.get('name', 'unknown')
            # Treat array as a pointer for simplicity in C generation? Depends on usage.
            # For function args, C treats arrays as pointers.
            pointer_level += 1
            break # Stop further resolution for arrays for now
        elif json_type == 'ret_type':
            if 'type' not in current_type: break
            current_type = current_type['type']
        elif json_type == 'typedef':
            typedef_name = current_type.get('name')
            if not typedef_name or typedef_name in visited_typedefs:
                c_type = typedef_name or "unknown_recursive_typedef"
                break # Bail out
            visited_typedefs.add(typedef_name)
            if typedef_name in typedef_map:
                 # Follow the typedef's definition
                 current_type = typedef_map[typedef_name]['type']
            else:
                 # Typedef not found or points to a basic type name directly
                 c_type = typedef_name
                 break
        elif json_type in ['primitive_type', 'stdlib_type', 'lvgl_type', 'enum', 'struct', 'union', 'forward_decl']:
            c_type = current_type.get('name')
            break # Found the base type
        elif 'type' in current_type: # Generic fallback for wrapped types
             current_type = current_type['type']
        else:
            c_type = current_type.get('name', 'unknown_final')
            break # Cannot resolve further

    if c_type is None and isinstance(current_type, dict):
        c_type = current_type.get('name', 'unknown_fallback')

    # Simple type name directly in the object (e.g., for args)
    if c_type == "unknown" and isinstance(type_obj, dict) and 'name' in type_obj:
         c_type = type_obj['name']
         # Re-resolve if this name is a typedef
         if c_type in typedef_map and c_type not in visited_typedefs:
             return get_full_type_info(typedef_map[c_type]['type'], api_data)


    # Clean up potential base types like 'struct _lv_obj_t' -> 'lv_obj_t'
    # This depends heavily on how types are named in the JSON. Let's assume names are clean.
    # We might need specific handling for anonymous structs/unions if they appear.

    if c_type == 'unknown':
        c_type = type_obj['name']
    # Drop leading "_" for types like "_lv_obj_t".
    if c_type[0] == '_':
        c_type = c_type[1:]

    return c_type, pointer_level, is_array


def _should_include(name, include_list, exclude_list, include_prefixes, exclude_prefixes):
    """Applies the include/exclude logic."""
    if not name:
        return False
    if name in include_list:
        return True
    if name in exclude_list:
        return False

    excluded = any(name.startswith(prefix) for prefix in exclude_prefixes)
    if excluded:
        return False

    included = any(name.startswith(prefix) for prefix in include_prefixes)
    return included


def parse_api(api_filepath,
              func_includes=None, func_excludes=None, func_include_prefixes=None, func_exclude_prefixes=None,
              enum_includes=None, enum_excludes=None, enum_include_prefixes=None, enum_exclude_prefixes=None):
    """Loads, parses, and filters the API definition."""

    func_includes = func_includes if func_includes is not None else DEFAULT_FUNCTION_INCLUDES
    func_excludes = func_excludes if func_excludes is not None else []
    func_include_prefixes = func_include_prefixes if func_include_prefixes is not None else DEFAULT_FUNCTION_INCLUDE_PREFIXES
    func_exclude_prefixes = func_exclude_prefixes if func_exclude_prefixes is not None else DEFAULT_FUNCTION_EXCLUDE_PREFIXES

    enum_includes = enum_includes if enum_includes is not None else DEFAULT_ENUM_INCLUDES
    enum_excludes = enum_excludes if enum_excludes is not None else DEFAULT_ENUM_EXCLUDES
    enum_include_prefixes = enum_include_prefixes if enum_include_prefixes is not None else DEFAULT_ENUM_INCLUDE_PREFIXES
    enum_exclude_prefixes = enum_exclude_prefixes if enum_exclude_prefixes is not None else DEFAULT_ENUM_EXCLUDE_PREFIXES

    try:
        with open(api_filepath, 'r') as f:
            api_data = json.load(f)
    except Exception as e:
        logger.error(f"Failed to load or parse API file {api_filepath}: {e}")
        return None

    # Filter Functions
    filtered_functions = []
    for func in api_data.get('functions', []):
        name = func.get('name')
        if _should_include(name, func_includes, func_excludes, func_include_prefixes, func_exclude_prefixes):
            # Basic check for varargs (...) which we likely can't support generically
            is_varargs = any(arg.get('type', {}).get('name') == 'ellipsis' for arg in func.get('args', []))
            if is_varargs:
                logger.warning(f"Skipping variadic function (unsupported): {name}")
                continue

            # Resolve types for args and return value
            try:
                 ret_type_info = get_full_type_info(func.get('type', {}), api_data)
                 arg_type_infos = []
                 # Handle void args explicitly
                 args = func.get('args', [])
                 if len(args) == 1 and _get_base_type_name(args[0].get('type')) == 'void':
                      args = [] # Treat (void) as no arguments

                 for arg in args:
                      arg_type_info = get_full_type_info(arg.get('type', {}), api_data)
                      arg_type_infos.append(arg_type_info)

                 func['_resolved_ret_type'] = ret_type_info
                 func['_resolved_arg_types'] = arg_type_infos
                 filtered_functions.append(func)
            except Exception as e:
                 logger.error(f"Error resolving types for function {name}: {e}")

    # Gather needed typedefs (those used by filtered functions/structs)
    # This is complex. For now, let's just pass all typedefs and let C compiler handle it.
    # A more refined approach would trace dependencies.
    typedefs = api_data.get('typedefs', [])
    structs = api_data.get('structures', []) # Might need structs if we pass them by value (less common in LVGL API)

    # Also collect global variables if needed (e.g., fonts like lv_font_montserrat_14)
    variables = api_data.get('variables', [])

    # Filter Enums (include all members if enum type is included)
    filtered_enums = [] # List of original enum dicts that are included
    # Globally collected, hashed, and sorted enum members for the new unmarshaler
    # Each item: {'name': str, 'value': int, 'hash': int, 'original_type_name': str (for debug/future)}
    hashed_and_sorted_enum_members = [] 
    
    # Keep track of all_enum_members for compatibility or other uses if needed
    all_enum_members_compat_map = {} 

    temp_enum_members_list = [] # Temporary list to gather before sorting

    for enum_original_data in api_data.get('enums', []):
        enum_type_name = enum_original_data.get('name') # e.g., lv_align_t
        first_member_name = enum_original_data.get('members', [{}])[0].get('name')
        
        identifier_for_filtering = enum_type_name if enum_type_name else (first_member_name if first_member_name else None)

        if _should_include(identifier_for_filtering, enum_includes, enum_excludes, enum_include_prefixes, enum_exclude_prefixes):
            if enum_type_name: # Only process enums that have a type name for clarity
                filtered_enums.append(enum_original_data) 
            else:
                # If no type name, we might still include its members if they match LV_ prefix etc.
                # but they won't have an 'original_type_name' associated here.
                # For simplicity, we will primarily focus on enums with type names for the new system.
                # If an enum has no 'name' but its members are LV_ prefixed, they'll still be caught.
                pass # Members will be processed below regardless of top-level enum name presence

            for member in enum_original_data.get('members', []):
                member_name = member.get('name')
                if member_name:
                    # Check if member itself should be included (e.g., starts with LV_)
                    # This ensures we don't include internal _LV_ members if the enum type itself was included by a broader rule
                    if not _should_include(member_name, [], [], enum_include_prefixes, enum_exclude_prefixes):
                        continue

                    try:
                        value_str = member.get('value', '0')
                        value = int(value_str, 0)
                        
                        all_enum_members_compat_map[member_name] = value # Populate old map

                        member_hash = djb2_hash(member_name)
                        temp_enum_members_list.append({
                            'name': member_name,
                            'value': value,
                            'hash': member_hash,
                            'original_type_name': enum_type_name if enum_type_name else "UNKNOWN_TYPE"
                        })
                    except ValueError:
                        logger.warning(f"Could not parse enum value for {member_name}: {value_str}")
    
    # Sort the global list: primarily by hash, secondarily by name (for stable collision groups)
    hashed_and_sorted_enum_members = sorted(temp_enum_members_list, key=lambda x: (x['hash'], x['name']))

    logger.info(f"Filtered {len(filtered_functions)} functions.")
    logger.info(f"Collected {len(hashed_and_sorted_enum_members)} enum members for hashed lookup.")
    logger.info(f"Legacy enum_members map has {len(all_enum_members_compat_map)} entries.")


    return {
        "functions": filtered_functions,
        "enums": filtered_enums, # Original filtered enum structures
        "enum_members": all_enum_members_compat_map, # Legacy map for compatibility
        
        # New structure for optimized enum unmarshaling
        "hashed_and_sorted_enum_members": hashed_and_sorted_enum_members,

        "typedefs": typedefs,
        "structs": structs,
        "variables": variables,
        "_full_api_data": api_data
    }