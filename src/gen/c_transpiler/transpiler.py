# gen/c_transpiler/transpiler.py
import logging
import json
import re
import copy # For deepcopy
import type_utils # For lvgl_type_to_widget_name, etc.
# import api_parser # To access api_info for enums etc. # Not directly used in this class, but api_info is passed
from generator import C_COMMON_DEFINES

logger = logging.getLogger(__name__)

# --- New CJSONObject class and hook for handling duplicate keys ---
class CJSONObject:
    def __init__(self, ordered_pairs):
        # Stores the original [(key, value), (key, value), ...] list
        self.kv_pairs = ordered_pairs

    def get(self, key_to_find, default=None):
        """Returns the value for the first occurrence of key_to_find."""
        for k, v in self.kv_pairs:
            if k == key_to_find:
                return v
        return default

    def __contains__(self, key_to_find):
        """Checks if key_to_find exists (first occurrence)."""
        for k, _ in self.kv_pairs:
            if k == key_to_find:
                return True
        return False

    def __iter__(self):
        """Allows iteration over (key, value) pairs, respecting order and duplicates."""
        return iter(self.kv_pairs)

    def __repr__(self):
        return f"CJSONObject({self.kv_pairs!r})"

def cjson_object_hook(pairs):
    """Hook for json.load to use CJSONObject."""
    return CJSONObject(pairs)
# --- End of CJSONObject section ---


class CTranspiler:
    def __init__(self, api_info, ui_spec_data):
        self.api_info = api_info
        self.ui_spec_data = ui_spec_data # This will be CJSONObject or list of CJSONObjects
        
        self.c_obj_vars = {}    # For lv_obj_t* : { "json_id": "c_var_name", ... }
        self.c_style_vars = {}  # For lv_style_t: { "json_id": "c_var_name_of_struct", ... }
        # self.c_other_vars = {} # Potentially for other types like fonts, not fully used yet

        self.local_registered_entities = {}
        # Key: json_id_str (e.g., "container" from "id": "@container")
        # Value: {'c_var_name': str, 'c_type_str': str, 'is_ptr': bool, 'lvgl_type_str': str }
        #   'c_var_name': C variable name (e.g., "c_style_1" or "c_obj_1")
        #   'c_type_str': C type of the variable itself (e.g., "lv_style_t" or "lv_obj_t *")
        #   'is_ptr': True if c_var_name is a pointer type
        #   'lvgl_type_str': e.g. "style", "button", "obj" (for setter lookup)

        self.generated_c_lines_predecl = [] 
        self.generated_c_lines_decl = [] 
        self.generated_c_lines_impl = [] 
        self.declared_c_vars_in_func = set() 
        self.var_counter = 0
        self.current_indent_level = 1 

        self.component_definitions = {} 

        self._SENTINEL = object()


    def _indent(self):
        return "    " * self.current_indent_level

    def _add_decl(self, line):
        self.generated_c_lines_decl.append(line)

    def _add_impl(self, line, indent=True):
        if indent:
            self.generated_c_lines_impl.append(self._indent() + line)
        else:
            self.generated_c_lines_impl.append(line)

    def _add_predecl(self, line, indent=True):
        # Predecl lines are typically not indented by current_indent_level
        self.generated_c_lines_predecl.append(line)


    def _get_unique_c_var_name_base(self, base_name="var"):
        self.var_counter += 1
        safe_base_name = re.sub(r'\W|^(?=\d)', '_', base_name)
        if not safe_base_name or safe_base_name.startswith('_'): 
            safe_base_name = f"lv{safe_base_name}"
        return f"c_{safe_base_name}_{self.var_counter}"

    def _declare_c_var_if_needed(self, c_type, c_var_name):
        if c_var_name not in self.declared_c_vars_in_func:
            self._add_decl(f"    {c_type} {c_var_name};")
            self.declared_c_vars_in_func.add(c_var_name)

    def _format_c_value(self, json_value_node, expected_c_type, current_entity_c_var, current_context):
        """
        Converts a JSON node to a C literal string or C variable name/expression.
        expected_c_type: C type string (e.g., "int", "const char *", "lv_color_t", "lv_align_t").
        current_entity_c_var: C var name of the LVGL object being configured (for context like lv_pct).
        json_value_node: Python primitive or CJSONObject/list.
        current_context: Python dictionary or CJSONObject for '$' variable resolution.
        """
        if json_value_node is None: 
            if "*" in expected_c_type: 
                return "NULL"
            elif expected_c_type == "bool":
                return "false"
            logger.warning(f"JSON null encountered for non-pointer type {expected_c_type}. Defaulting to 0/NULL representation.")
            return "0"
        
        if isinstance(json_value_node, (int, float)): 
            if "float" in expected_c_type or "double" in expected_c_type:
                return f"{json_value_node}f" if "float" in expected_c_type else str(json_value_node)
            return str(int(json_value_node))

        if isinstance(json_value_node, bool): 
            return "true" if json_value_node is True else "false" 

        if isinstance(json_value_node, str): 
            s_val = json_value_node 

            TOKENS = ['$', '#', '@', '!']
            if len(s_val) > 1 and s_val[0] in TOKENS and s_val[-1] in TOKENS and s_val[0] == s_val[-1]:
                s_val = s_val[1:-1] # Unescape if wrapped, e.g. "@my_ref@" -> "my_ref"

            if s_val.startswith("$") and not s_val.startswith("$$"): 
                var_name = s_val[1:]
                context_value = self._SENTINEL
                if isinstance(current_context, CJSONObject):
                    context_value = current_context.get(var_name, self._SENTINEL)
                elif isinstance(current_context, dict): # Standard dict from initial context
                    context_value = current_context.get(var_name, self._SENTINEL)
                
                if context_value is not self._SENTINEL:
                    return self._format_c_value(context_value, expected_c_type, current_entity_c_var, current_context)
                else:
                    logger.warning(f"Context variable ${var_name} not found. Treating as error or literal.")
                    return f"\"/* CONTEXT_ERROR: ${var_name} not found */\""
            elif s_val.startswith("$$"): 
                s_val = s_val[1:] 

            if s_val.startswith("#"): 
                if expected_c_type == "lv_color_t":
                    hex_code = s_val[1:]
                    if len(hex_code) == 3: 
                        r, g, b = hex_code[0]*2, hex_code[1]*2, hex_code[2]*2
                        hex_code = f"{r}{g}{b}"
                    if len(hex_code) == 6: 
                        return f"lv_color_hex(0x{hex_code.upper()})"
                    elif len(hex_code) == 8: 
                         return f"lv_color_hex32(0x{hex_code.upper()})" 
                    else:
                        logger.warning(f"Invalid color hex format: {s_val}. Defaulting to black.")
                        return "lv_color_hex(0x000000)"
                else: 
                    logger.warning(f"Color string {s_val} used for non-lv_color_t type {expected_c_type}.")
                    return f"\"{s_val}\"" # As a literal string if not for color

            if s_val.startswith("@"):
                ref_id = s_val[1:]
                if ref_id in self.local_registered_entities:
                    entity_info = self.local_registered_entities[ref_id]
                    var_c_name = entity_info['c_var_name']
                    var_c_type = entity_info['c_type_str'] # e.g. "lv_style_t" or "lv_obj_t *"
                    var_is_ptr = entity_info['is_ptr']
                    
                    normalized_expected_c_type = expected_c_type.replace("const ", "").strip()
                    
                    if var_is_ptr: # var is like lv_obj_t* obj_1;
                        if normalized_expected_c_type == var_c_type: # Expecting lv_obj_t*
                            return var_c_name
                    else: # var is like lv_style_t style_1;
                        if normalized_expected_c_type == var_c_type: # Expecting lv_style_t (by value, rare for func args)
                            return var_c_name 
                        elif normalized_expected_c_type == var_c_type + " *" or \
                             normalized_expected_c_type == "const " + var_c_type + " *": # Expecting lv_style_t*
                            return f"&{var_c_name}"
                    
                    logger.warning(f"Local var @{ref_id} ({var_c_name}, type {var_c_type}) incompatible with expected C type '{expected_c_type}'. Falling back to runtime lookup if available.")
                
                # Fallback to runtime lookup (if transpiler is for a system with the registry)
                # This might be for references to things not created by this specific transpiled function.
                logger.debug(f"Generating runtime lookup for '@{ref_id}' with expected C type '{expected_c_type}'")
                return f"(({expected_c_type})lvgl_json_get_registered_ptr(\"{ref_id}\", \"{expected_c_type.replace('const ', '').replace(' *', '')}\"))" # Simplified type name for registry

            if s_val.startswith("!"): 
                static_str_val = s_val[1:]
                escaped_s = static_str_val.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                return f"\"{escaped_s}\""

            if s_val.endswith("%") and (expected_c_type == "lv_coord_t" or expected_c_type == "int32_t"):
                try:
                    num_part = s_val[:-1]
                    val = int(num_part)
                    # LV_PCT needs the parent object for some calculations if it's a child.
                    # current_entity_c_var is the object being set, which is correct for lv_obj_set_width(obj, LV_PCT(50))
                    # If LV_PCT is used for grid tracks, the parent is the grid obj itself.
                    return f"LV_PCT({val})" # LV_PCT does not take parent as arg. It's relative to parent's content area.
                except ValueError:
                    logger.warning(f"Invalid percentage string: {s_val}. Treating as literal string.")
                    return f"\"{s_val}\"" 

            # Check if it's a known LVGL enum/define (symbol)
            # This requires api_info to have a list of all such symbols.
            # For now, assume if it's all caps or LV_ prefixed, it's a symbol.
            # A more robust check would use self.api_info.
            is_known_symbol = False
            if self.api_info and 'hashed_and_sorted_enum_members' in self.api_info:
                 if any(member['name'] == s_val for member in self.api_info['hashed_and_sorted_enum_members']):
                    is_known_symbol = True
            elif re.fullmatch(r"LV_[A-Z0-9_]+", s_val) or re.fullmatch(r"[A-Z_][A-Z0-9_]*", s_val):
                 # Heuristic: if it looks like a C define/enum, pass it through raw.
                 # This is risky if a string literal happens to match this pattern.
                 # logger.debug(f"Assuming '{s_val}' is an LVGL symbol/enum.")
                 is_known_symbol = True # Tentatively

            if is_known_symbol:
                return s_val 
            else: 
                escaped_s = s_val.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                return f"\"{escaped_s}\""

        if isinstance(json_value_node, list): 
            if expected_c_type == "const lv_coord_t *" or expected_c_type == "lv_coord_t *":
                c_array_elements = []
                for item_node in json_value_node: 
                    c_array_elements.append(self._format_c_value(item_node, "lv_coord_t", current_entity_c_var, current_context))
                c_array_elements.append("LV_GRID_TEMPLATE_LAST") # Sentinel for grid arrays
                
                array_var_name = self._get_unique_c_var_name_base("coord_array")
                # Declare as static const because it's usually for grid descriptors
                self._add_decl(f"    static const lv_coord_t {array_var_name}[] = {{ {', '.join(c_array_elements)} }};")
                return array_var_name 
            else: # Generic array - not directly supported for C literals this way
                  # For function args, each element would be formatted.
                  # This path is for when a list IS the value, e.g. for a single arg.
                logger.warning(f"Direct C transpilation of JSON array for type '{expected_c_type}' into a single C value is complex. Assuming it's for multiple arguments to a function, which is handled by arg iteration.")
                return "/* JSON array to C value not fully implemented for this type */"

        if isinstance(json_value_node, CJSONObject): 
            call_item = json_value_node.get("call") 
            args_item = json_value_node.get("args") 
            if call_item and isinstance(call_item, str) and isinstance(args_item, list): 
                nested_func_name = call_item 
                func_api_def = next((f for f in self.api_info['functions'] if f['name'] == nested_func_name), None)
                
                if not func_api_def:
                    logger.warning(f"Nested call to unknown function '{nested_func_name}'. Cannot determine argument types accurately.")
                    # Fallback: try to format args as generic types or strings
                    formatted_args = []
                    for arg_node in args_item:
                        # Guess type or use a very generic one for formatting
                        formatted_args.append(self._format_c_value(arg_node, "void *", current_entity_c_var, current_context))
                    return f"{nested_func_name}({', '.join(formatted_args)})"


                nested_c_args = []
                expected_c_func_args_info = func_api_def.get('_resolved_arg_types', [])
                
                # Implicit first argument for LVGL methods (obj or style)
                # This is tricky: is the 'call' on the current_entity_c_var or something else?
                # The prompt examples suggest args are explicit:
                # { call: lv_tileview_add_tile, args: [tileview_var, 0, 0, LV_DIR_RIGHT] }
                # { call: 'lv_dropdown_get_list', args: [dropdown_var] }
                # So, current_entity_c_var is passed as implicit_parent_for_call context.
                # The first argument of the call should come from args_item[0].

                logger.info(f"{args_item} => { expected_c_func_args_info }")
                num_json_args_provided = len(args_item)
                # TODO: Crude heuristic (match "lv_*_t"), check for prefix "lv_*" and make sure it's a pointer instead.
                if num_json_args_provided < len(expected_c_func_args_info) and len(expected_c_func_args_info) > 0 and expected_c_func_args_info[0][0].startswith("lv_") and expected_c_func_args_info[0][0].endswith("_t"):
                    nested_c_args.append(current_entity_c_var)
                    expected_c_func_args_info = expected_c_func_args_info[1:]

                for i in range(num_json_args_provided):
                    json_arg_node = args_item[i]
                    arg_c_type_str = "void *" # Default if type info missing
                    if i < len(expected_c_func_args_info):
                        arg_base_type, arg_ptr_level, _ = expected_c_func_args_info[i]
                        arg_c_type_str = type_utils.get_c_type_str(arg_base_type, arg_ptr_level)
                    
                    # The `current_entity_c_var` is the context for LV_PCT, not necessarily an arg to the call.
                    # For nested calls, if an arg is "@self" or similar, that would need special handling.
                    # Assume args resolve independently or use the broader current_context.
                    nested_c_args.append(self._format_c_value(json_arg_node, arg_c_type_str, current_entity_c_var, current_context))
                
                return f"{nested_func_name}({', '.join(nested_c_args)})"
            else: # Other CJSONObject structures are not directly formattable to a C value
                logger.warning(f"Unsupported CJSONObject structure for C value formatting: {repr(json_value_node)}.")
                return "/* Unsupported JSON object value */"

        logger.warning(f"Unhandled JSON value type '{type(json_value_node)}' for C formatting. Value: {str(json_value_node)[:50]}")
        return "/* unhandled_json_value */"

    def _apply_properties_and_attributes(self, 
                                         json_attributes_data,  # CJSONObject of attributes
                                         target_c_entity_var_name, # C var name (e.g. "c_obj_1", "&c_style_1")
                                         target_actual_lvgl_type_str, # LVGL type for setters ("button", "style", "obj")
                                         target_original_json_type_str, # Original "type" from JSON ("grid", "button")
                                         target_is_widget,
                                         explicit_parent_c_var_for_children_attr, # C var for children's parent
                                         path_prefix_for_named_and_children_in_attrs, 
                                         default_c_type_for_registry_if_named_in_attrs, # e.g. "lv_button_t"
                                         current_context):
        
        effective_path_prefix = path_prefix_for_named_and_children_in_attrs

        for prop_json_name, prop_value_node in json_attributes_data:
            # These are handled by _transpile_node or are structural for this function
            if prop_json_name in ["type", "id", "context"]:
                continue
            
            # Grid cols/rows are handled by _transpile_node directly before/after this call
            if target_original_json_type_str == "grid" and prop_json_name in ["cols", "rows"]:
                continue

            # 'do' is a structural element, processed if this function is called FOR a 'do' block's content.
            # A 'do' key appearing as a property name here (unless inside 'with') is an error or unsupported.
            # The check for target_actual_lvgl_type_str != "with_pseudo_type_marker" is removed as it's not standard.
            if prop_json_name == "do": 
                # This typically shouldn't be hit if called for a use-view.do or with.do,
                # as 'do' itself isn't a property *applied* but a container.
                logger.debug(f"Skipping '{prop_json_name}' key during general attribute application for {target_c_entity_var_name}.")
                continue


            if prop_json_name == "named":
                if not isinstance(prop_value_node, str):
                    logger.warning(f"'named' property value for {target_c_entity_var_name} is not a string: {repr(prop_value_node)}. Skipping.")
                    continue
                
                # Resolve prop_value_node (can be $context var or literal string)
                # _format_c_value returns a C literal (e.g. "my_name_str") or var. We need the Python string.
                
                resolved_named_value_str = None
                temp_value = prop_value_node # Start with the raw JSON value
                if isinstance(temp_value, str) and temp_value.startswith("$") and not temp_value.startswith("$$"):
                    context_var_name = temp_value[1:]
                    resolved_context_val = self._SENTINEL
                    if isinstance(current_context, CJSONObject):
                        resolved_context_val = current_context.get(context_var_name, self._SENTINEL)
                    elif isinstance(current_context, dict):
                        resolved_context_val = current_context.get(context_var_name, self._SENTINEL)

                    if resolved_context_val is not self._SENTINEL:
                        if isinstance(resolved_context_val, str):
                            resolved_named_value_str = resolved_context_val
                        else:
                            logger.warning(f"Context variable ${context_var_name} for 'named' property is not a string. Got: {repr(resolved_context_val)}. Skipping 'named'.")
                            continue
                    else:
                        logger.warning(f"Context variable ${context_var_name} for 'named' property not found. Skipping 'named'.")
                        continue
                elif isinstance(temp_value, str):
                     resolved_named_value_str = temp_value.lstrip("!") # Handle "!literal" case too for the name
                else:
                    logger.warning(f"'named' value '{repr(temp_value)}' is not a string or simple context var. Skipping.")
                    continue

                if resolved_named_value_str:
                    full_named_path = f"{effective_path_prefix}:{resolved_named_value_str}" if effective_path_prefix else resolved_named_value_str
                    
                    self._add_impl(f"lvgl_json_register_ptr(\"{full_named_path}\", \"{default_c_type_for_registry_if_named_in_attrs}\", (void*){target_c_entity_var_name});")
                    logger.info(f"Registered {target_c_entity_var_name} as '{full_named_path}' (type {default_c_type_for_registry_if_named_in_attrs}) via 'named'.")
                    effective_path_prefix = full_named_path # Update for subsequent children/named in this block
                continue

            if prop_json_name == "children":
                if not target_is_widget or not explicit_parent_c_var_for_children_attr:
                    self._add_impl(f"// WARNING: 'children' attribute found, but target {target_c_entity_var_name} is not a widget or parent_for_children is NULL.")
                    continue
                if not isinstance(prop_value_node, list):
                    self._add_impl(f"// ERROR: 'children' property for {target_c_entity_var_name} must be an array.")
                    continue

                self._add_impl(f"// Children of {target_c_entity_var_name} ({target_actual_lvgl_type_str}) with path prefix '{effective_path_prefix}'")
                self.current_indent_level += 1
                for child_node_item in prop_value_node: 
                    self._transpile_node(child_node_item, explicit_parent_c_var_for_children_attr, current_context, effective_path_prefix)
                self.current_indent_level -= 1
                continue
            
            if prop_json_name == "with" and isinstance(prop_value_node, CJSONObject):
                obj_to_run_with_item = prop_value_node.get("obj")
                do_block_for_with_item = prop_value_node.get("do")

                if not obj_to_run_with_item:
                    self._add_impl(f"// ERROR: 'with' block for {target_c_entity_var_name} is missing 'obj' attribute. Skipping.")
                    continue
                if not do_block_for_with_item or not isinstance(do_block_for_with_item, CJSONObject):
                    self._add_impl(f"// ERROR: 'with' block for {target_c_entity_var_name} is missing 'do' object or it's not an object. Skipping.")
                    continue

                # Resolve 'obj'. Assume lv_obj_t* for now.
                # target_c_entity_var_name is passed as current_entity_c_var for LV_PCT context within obj_to_run_with_item evaluation.
                with_target_obj_expr_str = self._format_c_value(obj_to_run_with_item, "lv_obj_t *", target_c_entity_var_name, current_context)
                
                with_target_c_var = self._get_unique_c_var_name_base("with_target")
                self._declare_c_var_if_needed("lv_obj_t *", with_target_c_var) # Assuming obj resolves to lv_obj_t*
                self._add_impl(f"{with_target_c_var} = {with_target_obj_expr_str};")
                
                self._add_impl(f"if ({with_target_c_var}) {{ // Check if 'with.obj' for {target_c_entity_var_name} resolved")
                self.current_indent_level += 1

                # Recursively call for the 'do' block on the resolved object
                self._apply_properties_and_attributes(
                    json_attributes_data=do_block_for_with_item,
                    target_c_entity_var_name=with_target_c_var,
                    target_actual_lvgl_type_str="obj", # Assume "obj" for the target of 'with.do'
                    target_original_json_type_str="obj", # Original type of the 'with.obj' is unknown here, assume "obj"
                    target_is_widget=True, # Assume it's a widget
                    explicit_parent_c_var_for_children_attr=with_target_c_var, # Children in 'do' are parented to with_target_c_var
                    path_prefix_for_named_and_children_in_attrs=effective_path_prefix, # Path prefix is inherited
                    default_c_type_for_registry_if_named_in_attrs="lv_obj_t", # If 'named' in 'do', register as lv_obj_t
                    current_context=current_context
                )
                self.current_indent_level -= 1
                self._add_impl(f"}} else {{")
                self._add_impl(f"    // WARNING: 'with.obj' for {target_c_entity_var_name} resolved to NULL. 'do' block not applied.")
                self._add_impl(f"}}")
                continue

            # --- Action Attribute ---
            if prop_json_name == "action":
                if target_is_widget:
                    if isinstance(prop_value_node, str):
                        action_val_c_str = self._format_c_value(prop_value_node, "const char *", target_c_entity_var_name, current_context)
                        self._add_impl(f"if (REGISTRY) {{")
                        self.current_indent_level += 1
                        self._add_impl(f"lv_event_cb_t evt_cb = action_registry_get_handler_s(REGISTRY, {action_val_c_str});")
                        self._add_impl(f"if (evt_cb) {{")
                        self.current_indent_level += 1
                        self._add_impl(f"lv_obj_add_event_cb({target_c_entity_var_name}, evt_cb, LV_EVENT_ALL, NULL);")
                        self.current_indent_level -= 1
                        self._add_impl(f"}} else {{")
                        self.current_indent_level += 1
                        self._add_impl(f"LV_LOG_WARN(\"Action '%s' not found in registry.\", {action_val_c_str});")
                        self.current_indent_level -= 1
                        self._add_impl(f"}}")
                        self.current_indent_level -= 1
                        self._add_impl(f"}} else {{")
                        self.current_indent_level += 1
                        self._add_impl(f"LV_LOG_ERROR(\"REGISTRY is NULL, cannot process 'action' for {target_c_entity_var_name}.\");")
                        self.current_indent_level -= 1
                        self._add_impl(f"}}")
                    else:
                        self._add_impl(f"// WARNING: 'action' property for {target_c_entity_var_name} is not a string. Value: {prop_value_node}")
                else:
                    self._add_impl(f"// WARNING: 'action' property can only be applied to widgets. Target: {target_c_entity_var_name}")
                continue

            # --- Observes Attribute ---
            if prop_json_name == "observes":
                if target_is_widget:
                    if isinstance(prop_value_node, CJSONObject):
                        obs_value_item_node = prop_value_node.get("value")
                        obs_format_item_node = prop_value_node.get("format")
                        if obs_value_item_node is not None and obs_format_item_node is not None and \
                           isinstance(obs_value_item_node, str) and isinstance(obs_format_item_node, str):
                            obs_value_c_str = self._format_c_value(obs_value_item_node, "const char *", target_c_entity_var_name, current_context)
                            obs_format_c_str = self._format_c_value(obs_format_item_node, "const char *", target_c_entity_var_name, current_context)
                            self._add_impl(f"if (REGISTRY) {{")
                            self.current_indent_level += 1
                            self._add_impl(f"data_binding_register_widget_s(REGISTRY, {obs_value_c_str}, {target_c_entity_var_name}, {obs_format_c_str});")
                            self.current_indent_level -= 1
                            self._add_impl(f"}} else {{")
                            self.current_indent_level += 1
                            self._add_impl(f"LV_LOG_ERROR(\"REGISTRY is NULL, cannot process 'observes' for {target_c_entity_var_name}.\");")
                            self.current_indent_level -= 1
                            self._add_impl(f"}}")
                        else:
                            self._add_impl(f"// WARNING: 'observes' object for {target_c_entity_var_name} must have string properties 'value' and 'format'. Found value: '{obs_value_item_node}', format: '{obs_format_item_node}'.")
                    else:
                        self._add_impl(f"// WARNING: 'observes' property for {target_c_entity_var_name} is not an object. Value: {prop_value_node}")
                else:
                    self._add_impl(f"// WARNING: 'observes' property can only be applied to widgets. Target: {target_c_entity_var_name}")
                continue

            # --- Standard Setter Logic ---
            setter_func_name = None
            found_setter_api_def = None
            potential_setters = []

            # 1. Try specific type: lv_<target_actual_lvgl_type_str>_set_<prop_json_name>
            if target_actual_lvgl_type_str:
                potential_setters.append(f"lv_{target_actual_lvgl_type_str}_set_{prop_json_name}")
            # 2. Try specific type (short form): lv_<target_actual_lvgl_type_str>_<prop_json_name>
            if target_actual_lvgl_type_str:
                potential_setters.append(f"lv_{target_actual_lvgl_type_str}_{prop_json_name}")
            
            if target_is_widget:
                # 3. Try generic obj: lv_obj_set_<prop_json_name>
                potential_setters.append(f"lv_obj_set_{prop_json_name}")
                # 4. Try generic obj (short form): lv_obj_<prop_json_name>
                potential_setters.append(f"lv_obj_{prop_json_name}")
                # 5. Try style property: lv_obj_set_style_<prop_json_name>
                potential_setters.append(f"lv_obj_set_style_{prop_json_name}")
            elif target_actual_lvgl_type_str == "style": # Fallbacks for styles
                # 3b. Try lv_style_set_<prop_json_name>
                potential_setters.append(f"lv_style_set_{prop_json_name}")
                # 4b. Try lv_style_<prop_json_name>
                potential_setters.append(f"lv_style_{prop_json_name}")

            # 6. Try verbatim name (for direct function calls)
            potential_setters.append(prop_json_name)

            for pot_setter in potential_setters:
                setter_api_def = next((f for f in self.api_info.get('functions', []) if f['name'] == pot_setter), None)
                if setter_api_def:
                    setter_func_name = pot_setter
                    found_setter_api_def = setter_api_def
                    break
            
            if not setter_func_name or not found_setter_api_def:
                self._add_impl(f"// WARNING: No setter function found for property '{prop_json_name}' on type '{target_actual_lvgl_type_str}' for entity {target_c_entity_var_name}. Skipping.")
                continue

            setter_args_c_types_info = found_setter_api_def.get('_resolved_arg_types', [])
            c_call_args = [target_c_entity_var_name] 

            prop_value_as_list_for_args = prop_value_node
            if not isinstance(prop_value_as_list_for_args, list):
                prop_value_as_list_for_args = [copy.deepcopy(prop_value_node)]

            num_json_setter_args = len(prop_value_as_list_for_args)
            
            # Setter API def includes the object itself as arg 0. JSON args map to API args from index 1.
            for i in range(num_json_setter_args):
                json_arg_node = prop_value_as_list_for_args[i]
                expected_arg_c_type = "void *" # Default
                if (i + 1) < len(setter_args_c_types_info): 
                    arg_base, arg_ptr, _ = setter_args_c_types_info[i+1]
                    expected_arg_c_type = type_utils.get_c_type_str(arg_base, arg_ptr)
                
                c_call_args.append(self._format_c_value(json_arg_node, expected_arg_c_type, target_c_entity_var_name, current_context))

            # Handle default selector for lv_obj_set_style_...
            # API: (obj, value, selector). JSON often provides only value.
            # So, c_call_args has [target_obj, formatted_value]. We need to add selector.
            if setter_func_name.startswith("lv_obj_set_style_") and \
               len(c_call_args) == 2 and \
               (len(setter_args_c_types_info) == 3 or (len(setter_args_c_types_info) > 3 and setter_args_c_types_info[3][0] == "...")): # Obj, Value, Selector[, State]
                # Check if the third argument in C API is indeed a selector type
                if len(setter_args_c_types_info) >=3:
                    selector_arg_base, selector_arg_ptr, _ = setter_args_c_types_info[2]
                    selector_c_type = type_utils.get_c_type_str(selector_arg_base, selector_arg_ptr)
                    if selector_c_type == "lv_style_selector_t" or selector_c_type == "uint32_t" or selector_c_type == "int":
                        self._add_impl(f"// INFO: Adding default selector LV_PART_MAIN (0) for {setter_func_name} on {target_c_entity_var_name}")
                        c_call_args.append("LV_PART_MAIN") 
            
            self._add_impl(f"{setter_func_name}({', '.join(c_call_args)});")


    def _transpile_node(self, json_node_data, parent_c_var_name, current_context, current_named_path_prefix):
        if not isinstance(json_node_data, CJSONObject):
            logger.error(f"Node data is not a CJSONObject. Type: {type(json_node_data)}. Value: {repr(json_node_data)}. Skipping.")
            return None, None # Var name, LVGL type for setters

        node_original_json_type_str = json_node_data.get("type", "obj")
        
        node_id_item = json_node_data.get("id")
        json_id_str_val = None 
        if node_id_item and isinstance(node_id_item, str) and node_id_item.startswith("@"):
            json_id_str_val = node_id_item[1:]

        # --- Context Management for this node ---
        # A "context" property on the node itself overrides the inherited current_context for its children and attributes
        effective_context_for_node = current_context
        context_property_on_node = json_node_data.get("context")
        if context_property_on_node is not None:
            if isinstance(context_property_on_node, CJSONObject): 
                effective_context_for_node = context_property_on_node
                logger.debug(f"Node {node_id_item or node_original_json_type_str} defines local context.")
            else:
                logger.warning(f"Node {node_id_item or node_original_json_type_str} 'context' is not a CJSONObject. Ignored.")

        # --- Handle special node types: component, use-view, context ---
        if node_original_json_type_str == "component":
            comp_id_item = json_node_data.get("id")
            comp_root_item = json_node_data.get("root")
            if comp_id_item and isinstance(comp_id_item, str) and \
               comp_id_item.startswith("@") and \
               comp_root_item and isinstance(comp_root_item, CJSONObject): 
                comp_id = comp_id_item[1:]
                self.component_definitions[comp_id] = copy.deepcopy(comp_root_item)
                self._add_impl(f"// Component definition '{comp_id}' processed and stored.", indent=True)
            else:
                self._add_impl(f"// ERROR: Invalid 'component' definition: {repr(json_node_data)}", indent=True)
            return None, None # Components don't generate direct C code here

        if node_original_json_type_str == "use-view":
            view_id_item = json_node_data.get("id") # This is the ID of the component to use
            if view_id_item and isinstance(view_id_item, str) and view_id_item.startswith("@"):
                view_id_to_use = view_id_item[1:]
                if view_id_to_use in self.component_definitions:
                    self._add_impl(f"// Using view component '{view_id_to_use}'", indent=True)
                    component_root_json_data = self.component_definitions[view_id_to_use]
                    
                    # The use-view node might have an "id" itself, for referring to the instantiated component.
                    # This "id" (json_id_str_val) determines the name for local_registered_entities for the component's root.
                    # The current_named_path_prefix is for the use-view node itself.
                    
                    # Transpile the component's root. Parent is parent_c_var_name. Context is effective_context_for_node (from use-view).
                    # Path prefix for component's root children/named attributes comes from the use-view's own path context.
                    comp_root_c_var, comp_root_lvgl_setter_type = self._transpile_node(
                        component_root_json_data, 
                        parent_c_var_name, 
                        effective_context_for_node, # Context provided by "use-view"
                        current_named_path_prefix # Path context of the "use-view" node itself
                    )

                    if comp_root_c_var and comp_root_lvgl_setter_type:
                        # If use-view itself has an "id" (json_id_str_val), register the component's root C var under that id.
                        if json_id_str_val: # This is the ID of the use-view node itself
                             is_comp_root_widget = comp_root_lvgl_setter_type != "style"
                             comp_root_c_type_str = "lv_obj_t *" if is_comp_root_widget else "lv_style_t"
                             self.local_registered_entities[json_id_str_val] = {
                                'c_var_name': comp_root_c_var,
                                'c_type_str': comp_root_c_type_str,
                                'is_ptr': is_comp_root_widget,
                                'lvgl_type_str': comp_root_lvgl_setter_type
                             }
                             logger.debug(f"Locally tracked use-view instance @{json_id_str_val} (component '{view_id_to_use}') -> {self.local_registered_entities[json_id_str_val]}")


                        do_attributes_json = json_node_data.get("do")
                        if do_attributes_json and isinstance(do_attributes_json, CJSONObject):
                            self._add_impl(f"// Applying 'do' attributes to component '{view_id_to_use}' root ({comp_root_c_var})", indent=True)
                            
                            is_comp_root_widget = comp_root_lvgl_setter_type != "style"
                            # Type for registering "named" items *inside* the "do" block.
                            # These items are attributes of the component's root.
                            comp_root_registry_c_type = f"lv_{comp_root_lvgl_setter_type}_t" if comp_root_lvgl_setter_type != "obj" else "lv_obj_t"
                            if not is_comp_root_widget:
                                comp_root_registry_c_type = f"lv_{comp_root_lvgl_setter_type}_t" # e.g. lv_style_t

                            # Path prefix for "named" and "children" *inside the "do" block*.
                            # This should be relative to the component root's own potential ID.
                            path_for_do_block_attrs = current_named_path_prefix
                            comp_root_node_actual_id_item = component_root_json_data.get("id") # ID of the *component's root definition*
                            if comp_root_node_actual_id_item and isinstance(comp_root_node_actual_id_item, str) and comp_root_node_actual_id_item.startswith("@"):
                                comp_root_def_id = comp_root_node_actual_id_item[1:]
                                path_for_do_block_attrs = f"{current_named_path_prefix}:{comp_root_def_id}" if current_named_path_prefix else comp_root_def_id
                            
                            # If the use-view node itself has an "id", that should be the primary path component.
                            if json_id_str_val : # ID of the use-view node
                                path_for_do_block_attrs = f"{current_named_path_prefix}:{json_id_str_val}" if current_named_path_prefix else json_id_str_val


                            self._apply_properties_and_attributes(
                                json_attributes_data=do_attributes_json,
                                target_c_entity_var_name=comp_root_c_var,
                                target_actual_lvgl_type_str=comp_root_lvgl_setter_type,
                                target_original_json_type_str=component_root_json_data.get("type", "obj"), # Original type of comp root
                                target_is_widget=is_comp_root_widget,
                                explicit_parent_c_var_for_children_attr=comp_root_c_var if is_comp_root_widget else None,
                                path_prefix_for_named_and_children_in_attrs=path_for_do_block_attrs,
                                default_c_type_for_registry_if_named_in_attrs=comp_root_registry_c_type,
                                current_context=effective_context_for_node # Context from use-view
                            )
                    # Return the C variable of the instantiated component's root
                    return comp_root_c_var, comp_root_lvgl_setter_type
                else:
                    self._add_impl(f"// ERROR: View component '{view_id_to_use}' not defined.", indent=True)
                    # Create a fallback label
                    fb_label_var = self._get_unique_c_var_name_base("fallback_label")
                    self._declare_c_var_if_needed("lv_obj_t *", fb_label_var)
                    self._add_impl(f"{fb_label_var} = lv_label_create({parent_c_var_name});", indent=True)
                    self._add_impl(f"lv_label_set_text_fmt({fb_label_var}, \"Error: View '%s' not found\", \"{view_id_to_use}\");", indent=True)
                    return fb_label_var, "label"
            else:
                self._add_impl(f"// ERROR: Invalid 'use-view' definition (missing or invalid 'id'): {repr(json_node_data)}", indent=True)
            return None, None

        if node_original_json_type_str == "context": 
            context_values_for_block = json_node_data.get("values") # This is the new context
            for_item_json = json_node_data.get("for")
            if for_item_json and isinstance(for_item_json, CJSONObject): 
                self._add_impl(f"// Processing 'context -> for' block:", indent=True)
                # The new context for the "for" block is context_values_for_block
                # If values is missing, it should ideally use current_context, but renderer implies it's an error.
                # For transpiler, let's allow falling back to current_context if 'values' is not a CJSONObject.
                new_inner_context = effective_context_for_node # Start with node's own context property
                if isinstance(context_values_for_block, CJSONObject):
                    new_inner_context = context_values_for_block
                elif context_values_for_block is not None: # `values` exists but is not a CJSONObject
                     logger.warning(f"'context' block 'values' is not a CJSONObject. Using outer context for 'for' child. Node: {repr(json_node_data)}")
                
                # Pass current_named_path_prefix through to the 'for' item.
                return self._transpile_node(for_item_json, parent_c_var_name, new_inner_context, current_named_path_prefix)
            else:
                 self._add_impl(f"// WARNING: 'context' block without a valid 'for' CJSONObject. Skipping. {repr(json_node_data)}", indent=True)
            return None, None

        # --- Generic Node Processing (Creation) ---
        created_c_entity_var = None # C var name for the created entity (e.g., "c_obj_1", "&c_style_1")
        c_entity_actual_c_type_str = "" # e.g., "lv_obj_t *", "lv_style_t"
        is_entity_pointer = False
        is_widget_node = True
        # LVGL type for finding setters (e.g., "button", "style", "obj" if grid)
        lvgl_setter_type_for_entity = node_original_json_type_str 
        # C type name for runtime registry (e.g. "lv_button_t", "lv_style_t")
        c_type_name_for_runtime_registry = "" 
        
        # Determine path for this node (if it has an ID) and for its children/named attributes
        # This becomes `path_prefix_for_named_and_children_in_attrs` for the call to _apply_properties_and_attributes
        effective_path_for_node_attrs = current_named_path_prefix
        if json_id_str_val: # If this node has an ID
            effective_path_for_node_attrs = f"{current_named_path_prefix}:{json_id_str_val}" if current_named_path_prefix else json_id_str_val


        if node_original_json_type_str == "style":
            is_widget_node = False
            style_c_var_name_base = json_id_str_val if json_id_str_val else "style"
            # For styles, c_style_vars stores the name of the lv_style_t variable itself
            style_actual_c_var_name = self._get_unique_c_var_name_base(style_c_var_name_base)
            if json_id_str_val: self.c_style_vars[json_id_str_val] = style_actual_c_var_name
            
            self._declare_c_var_if_needed("static lv_style_t", style_actual_c_var_name) # Static not needed for local func vars
            self._add_impl(f"lv_style_init(&{style_actual_c_var_name});")
            
            created_c_entity_var = f"&{style_actual_c_var_name}"
            c_entity_actual_c_type_str = "lv_style_t" # The var itself is lv_style_t
            is_entity_pointer = False # The C var is a struct, created_c_entity_var is its address
            lvgl_setter_type_for_entity = "style"
            c_type_name_for_runtime_registry = "lv_style_t"

        elif node_original_json_type_str == "grid": 
            # Grid is created as a generic lv_obj, then layout is set.
            obj_c_var_name_base = json_id_str_val if json_id_str_val else "grid_obj"
            obj_actual_c_var_name = self._get_unique_c_var_name_base(obj_c_var_name_base)
            if json_id_str_val: self.c_obj_vars[json_id_str_val] = obj_actual_c_var_name
            
            self._declare_c_var_if_needed("lv_obj_t *", obj_actual_c_var_name)
            self._add_impl(f"{obj_actual_c_var_name} = lv_obj_create({parent_c_var_name});")
            # Grid-specific setup like lv_obj_set_layout is now preferred via attributes.
            # self._add_impl(f"lv_obj_set_layout({obj_actual_c_var_name}, LV_LAYOUT_GRID);") 
            # cols/rows will be handled via _apply_properties_and_attributes and then set_grid_dsc_array call
            
            created_c_entity_var = obj_actual_c_var_name
            c_entity_actual_c_type_str = "lv_obj_t *"
            is_entity_pointer = True
            lvgl_setter_type_for_entity = "obj" # Setters use lv_obj_... for grid obj
            c_type_name_for_runtime_registry = "lv_obj_t" # Registered as a generic object

        else: # Default widget creation
            obj_c_var_name_base = json_id_str_val if json_id_str_val else node_original_json_type_str
            obj_actual_c_var_name = self._get_unique_c_var_name_base(obj_c_var_name_base)
            if json_id_str_val: self.c_obj_vars[json_id_str_val] = obj_actual_c_var_name

            self._declare_c_var_if_needed("lv_obj_t *", obj_actual_c_var_name)
            create_func_name = f"lv_{node_original_json_type_str}_create"
            
            # Verify create_func_name exists in API
            if not any(f['name'] == create_func_name for f in self.api_info['functions']):
                self._add_impl(f"// WARNING: Create function '{create_func_name}' not found for type '{node_original_json_type_str}'. Using lv_obj_create.", indent=True)
                create_func_name = "lv_obj_create"
                lvgl_setter_type_for_entity = "obj" # Fallback to generic obj setters
                c_type_name_for_runtime_registry = "lv_obj_t"
            else:
                # lvgl_setter_type_for_entity remains node_original_json_type_str
                c_type_name_for_runtime_registry = f"lv_{node_original_json_type_str}_t" 
            
            self._add_impl(f"{obj_actual_c_var_name} = {create_func_name}({parent_c_var_name});")
            created_c_entity_var = obj_actual_c_var_name
            c_entity_actual_c_type_str = "lv_obj_t *"
            is_entity_pointer = True
            # c_type_name_for_runtime_registry is already set above

        # Register with local_registered_entities if it has an ID
        # This happens for initially created entities (not for use-view roots here, handled in use-view block)
        if json_id_str_val and created_c_entity_var:
            # Determine the actual C variable name to store (obj_actual_c_var_name or style_actual_c_var_name)
            var_name_for_local_tracking = obj_actual_c_var_name if is_widget_node else style_actual_c_var_name
            self.local_registered_entities[json_id_str_val] = {
                'c_var_name': var_name_for_local_tracking,
                'c_type_str': c_entity_actual_c_type_str, # "lv_obj_t *" or "lv_style_t"
                'is_ptr': is_entity_pointer, # True for obj*, False for style_t
                'lvgl_type_str': lvgl_setter_type_for_entity # "button", "style", "obj"
            }
            logger.debug(f"Locally tracked entity @{json_id_str_val} -> {self.local_registered_entities[json_id_str_val]}")

        # Register with runtime registry if it has an ID (this is for lvgl_json_register_ptr)
        # This is separate from "named" attribute, this is for the node's own "id".
        if json_id_str_val and created_c_entity_var and effective_path_for_node_attrs:
             # effective_path_for_node_attrs IS the name for C runtime registry here.
             # c_type_name_for_runtime_registry is lv_button_t, lv_style_t, etc.
             # created_c_entity_var is &style_var or obj_var*.
             self._add_impl(f"lvgl_json_register_ptr(\"{effective_path_for_node_attrs}\", \"{c_type_name_for_runtime_registry}\", (void*){created_c_entity_var});")
             logger.info(f"Registered {created_c_entity_var} as '{effective_path_for_node_attrs}' (type {c_type_name_for_runtime_registry}) via its 'id'.")


        # --- Grid specific: Pre-process cols/rows to get array var names ---
        grid_col_dsc_c_var_name = None
        grid_row_dsc_c_var_name = None
        if node_original_json_type_str == "grid" and created_c_entity_var:
            cols_item = json_node_data.get("cols")
            rows_item = json_node_data.get("rows")
            if cols_item and isinstance(cols_item, list):
                # _format_c_value will declare the static array and return its name
                grid_col_dsc_c_var_name = self._format_c_value(cols_item, "const lv_coord_t *", created_c_entity_var, effective_context_for_node)
            if rows_item and isinstance(rows_item, list):
                grid_row_dsc_c_var_name = self._format_c_value(rows_item, "const lv_coord_t *", created_c_entity_var, effective_context_for_node)
            if not grid_col_dsc_c_var_name or not grid_row_dsc_c_var_name:
                self._add_impl(f"// WARNING: Grid '{created_c_entity_var}' is missing 'cols' or 'rows' array, or they are invalid.", indent=True)


        # --- Apply properties, children, and other attributes ---
        if created_c_entity_var:
            self._apply_properties_and_attributes(
                json_attributes_data=json_node_data, # Pass the whole node data
                target_c_entity_var_name=created_c_entity_var,
                target_actual_lvgl_type_str=lvgl_setter_type_for_entity,
                target_original_json_type_str=node_original_json_type_str, # Original "type" for checks like grid
                target_is_widget=is_widget_node,
                explicit_parent_c_var_for_children_attr=created_c_entity_var if is_widget_node else None,
                path_prefix_for_named_and_children_in_attrs=effective_path_for_node_attrs,
                default_c_type_for_registry_if_named_in_attrs=c_type_name_for_runtime_registry,
                current_context=effective_context_for_node
            )
        
        # --- Grid specific: Set descriptor array AFTER other attributes are processed ---
        if node_original_json_type_str == "grid" and created_c_entity_var and \
           grid_col_dsc_c_var_name and grid_row_dsc_c_var_name:
            self._add_impl(f"lv_obj_set_grid_dsc_array({created_c_entity_var}, {grid_col_dsc_c_var_name}, {grid_row_dsc_c_var_name});")
        
        return created_c_entity_var, lvgl_setter_type_for_entity
        
    def generate_c_function(self, function_name="create_ui_from_spec"):
        self._add_predecl(f"#include \"lvgl.h\"", indent=False)
        self._add_predecl(f"", indent=False)
        # Add C_COMMON_DEFINES if it contains necessary macros like LV_LOG_INFO etc.
        # For now, assume it's for runtime renderer. Transpiler might not need all of it.
        self._add_predecl(C_COMMON_DEFINES, indent=False) 
        self._add_predecl(f"// For LV_LOG_INFO, LV_LOG_ERROR, etc. Ensure lv_conf.h enables logging.", indent=False)
        self._add_predecl(f"// And that these macros are available or mapped.", indent=False)
        self._add_predecl(f"#ifndef LV_LOG_INFO", indent=False)
        self._add_predecl(f"#define LV_LOG_INFO(...) // Log stub", indent=False)
        self._add_predecl(f"#endif", indent=False)
        self._add_predecl(f"#ifndef LV_LOG_ERROR", indent=False)
        self._add_predecl(f"#define LV_LOG_ERROR(...) // Log stub", indent=False)
        self._add_predecl(f"#endif", indent=False)

        self._add_predecl(f"#include \"data_binding.h\" // For action/observes attributes", indent=False)
        self._add_predecl(f"extern data_binding_registry_t* REGISTRY; // Global registry for actions and data bindings", indent=False)
        self._add_predecl(f"extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);", indent=False)
        self._add_predecl(f"extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);", indent=False)
        self._add_predecl(f"", indent=False)

        # Ensure common LVGL macros are available if not via lvgl.h (e.g. if lv_conf.h is minimal)
        self._add_predecl(f"#ifndef LV_GRID_FR", indent=False)
        self._add_predecl(f"#define LV_GRID_FR(x) (lv_coord_t)(LV_COORD_FRACT_MAX / (x))", indent=False)
        self._add_predecl(f"#endif", indent=False)
        self._add_predecl(f"#ifndef LV_SIZE_CONTENT", indent=False)
        self._add_predecl(f"#define LV_SIZE_CONTENT LV_COORD_MAX", indent=False)
        self._add_predecl(f"#endif", indent=False)
        self._add_predecl(f"#ifndef LV_PART_MAIN", indent=False)
        self._add_predecl(f"#define LV_PART_MAIN LV_STATE_DEFAULT", indent=False)
        self._add_predecl(f"#endif", indent=False)
        self._add_predecl(f"", indent=False)


        self._add_predecl(f"void {function_name}(lv_obj_t *screen_parent) {{", indent=False)
        
        declaration_section_marker = "    // --- VARIABLE DECLARATIONS ---" # Indented to align
        self._add_predecl(declaration_section_marker) 

        self._add_impl(f"lv_obj_t *parent_obj = screen_parent ? screen_parent : lv_screen_active();")
        self._add_impl(f"if (!parent_obj) {{")
        self.current_indent_level += 1
        self._add_impl(f"LV_LOG_ERROR(\"Cannot render UI: No parent screen available.\");") 
        self._add_impl(f"return;")
        self.current_indent_level -= 1
        self._add_impl(f"}}")
        self._add_impl("") 

        initial_context = {} 
        # Root elements are processed with no prefix for their own ID-based registration.
        # Their children will use the root element's ID as a prefix if applicable.
        if isinstance(self.ui_spec_data, list): 
            for child_node_data in self.ui_spec_data: 
                self._transpile_node(child_node_data, "parent_obj", initial_context, None) # No initial path prefix
        elif isinstance(self.ui_spec_data, CJSONObject): 
            self._transpile_node(self.ui_spec_data, "parent_obj", initial_context, None) # No initial path prefix
        else:
            self._add_impl(f"    // UI specification data is not a list or CJSONObject. Type: {type(self.ui_spec_data)}", indent=True);

        self._add_impl("}", indent=False) 
        
        final_code_lines = []
        inserted_decls = False
        for line in self.generated_c_lines_predecl:
            if line == declaration_section_marker and not inserted_decls:
                final_code_lines.append(line) # Add the marker itself
                final_code_lines.extend(self.generated_c_lines_decl)
                final_code_lines.append("    // --- END VARIABLE DECLARATIONS ---") # Add an end marker
                inserted_decls = True
            else:
                final_code_lines.append(line)
        
        if not inserted_decls: # Should not happen if marker was added
             # Fallback: add declarations at the end of predecls if marker somehow missed
             final_code_lines.extend(self.generated_c_lines_decl)

        return "\n".join(final_code_lines) + "\n" + "\n".join(self.generated_c_lines_impl)


def generate_c_transpiled_ui(api_info, ui_spec_path, output_dir, output_filename_base="ui_transpiled"):
    logger.info(f"Starting C transpilation from UI spec: {ui_spec_path}")    

    try:
        with open(ui_spec_path, 'r', encoding='utf-8') as f:
            ui_spec_data = json.load(f, object_pairs_hook=cjson_object_hook) 
    except FileNotFoundError:
        logger.error(f"UI spec file not found: {ui_spec_path}")
        return
    except json.JSONDecodeError as e: 
        logger.error(f"Failed to parse UI spec file {ui_spec_path} with Python's json module: {e}")
        return
    except Exception as e: 
        logger.error(f"An unexpected error occurred while loading/parsing UI spec file {ui_spec_path}: {e}")
        return

    transpiler = CTranspiler(api_info, ui_spec_data) 
    c_function_name = f"create_ui_{output_filename_base.replace('-', '_').replace('.', '_')}"
    c_code_content = transpiler.generate_c_function(function_name=c_function_name)

    c_file_path = output_dir / f"{output_filename_base}.c"
    logger.info(f"Writing transpiled C source to: {c_file_path}")
    try:
        c_file_path.parent.mkdir(parents=True, exist_ok=True) # Ensure dir exists
        with open(c_file_path, "w", encoding='utf-8') as f:
            f.write(c_code_content)
    except IOError as e:
        logger.error(f"Failed to write C source file {c_file_path}: {e}")
        return

    h_file_path = output_dir / f"{output_filename_base}.h"
    header_content_list = [
        f"#ifndef {output_filename_base.upper().replace('-', '_').replace('.', '_')}_H",
        f"#define {output_filename_base.upper().replace('-', '_').replace('.', '_')}_H",
        "",
        f"#include \"lvgl.h\"", 
        "",
        f"// If using the runtime registry with transpiled code, ensure these are linked:",
        f"// extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);",
        f"// extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);",
        f"",
        f"#ifdef __cplusplus",
        f"extern \"C\" {{",
        f"#endif",
        "",
        f"/**",
        f" * @brief Creates the UI defined in '{ui_spec_path}' onto the given parent.",
        f" * @param screen_parent The parent LVGL object. If NULL, lv_screen_active() will be used.",
        f" */",
        f"void {c_function_name}(lv_obj_t *screen_parent);",
        "",
        f"#ifdef __cplusplus",
        f"}}",
        f"#endif",
        "",
        f"#endif // {output_filename_base.upper().replace('-', '_').replace('.', '_')}_H"
    ]
    logger.info(f"Writing transpiled C header to: {h_file_path}")
    try:
        h_file_path.parent.mkdir(parents=True, exist_ok=True) # Ensure dir exists
        with open(h_file_path, "w", encoding='utf-8') as f:
            f.write("\n".join(header_content_list))
    except IOError as e:
        logger.error(f"Failed to write C header file {h_file_path}: {e}")
        return

    logger.info(f"C transpilation for '{ui_spec_path}' complete. Output base: '{output_filename_base}'.")