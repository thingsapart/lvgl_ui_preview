# gen/c_transpiler/transpiler.py
import logging
import json
import re
import copy # For deepcopy
import type_utils # For lvgl_type_to_widget_name, etc.
import api_parser # To access api_info for enums etc.

logger = logging.getLogger(__name__)

class CTranspiler:
    def __init__(self, api_info, ui_spec_data):
        self.api_info = api_info
        self.ui_spec_data = ui_spec_data
        
        # Stores { "json_id_or_generated": "c_var_name", ... }
        # json_id_or_generated: if json has "id": "@myid", use "myid". Otherwise, generate one.
        self.c_obj_vars = {}    # For lv_obj_t*
        self.c_style_vars = {}  # For lv_style_t (name refers to the variable itself, not pointer)
        self.c_other_vars = {}  # For other registered items like lv_font_t*

        self.generated_c_lines_predecl = [] # Code before variable declarations at the top
        self.generated_c_lines_decl = [] # For variable declarations at the top
        self.generated_c_lines_impl = [] # For implementation code
        self.declared_c_vars_in_func = set() # Track C variables declared in the current function scope
        self.var_counter = 0
        self.current_indent_level = 1 # Start with 1 for inside function body

        # For component definitions
        self.component_definitions = {} # { "comp_id_str": python_dict_of_root }


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
        if indent:
            self.generated_c_lines_predecl.append(self._indent() + line)
        else:
            self.generated_c_lines_predecl.append(line)

    def _get_unique_c_var_name_base(self, base_name="var"):
        self.var_counter += 1
        # Sanitize base_name: replace non-alphanum with underscore
        safe_base_name = re.sub(r'\W|^(?=\d)', '_', base_name)
        if not safe_base_name or safe_base_name.startswith('_'): # Ensure it's a valid start
            safe_base_name = f"lv{safe_base_name}"
        return f"c_{safe_base_name}_{self.var_counter}"

    def _declare_c_var_if_needed(self, c_type, c_var_name):
        if c_var_name not in self.declared_c_vars_in_func:
            self._add_decl(f"    {c_type} {c_var_name};")
            self.declared_c_vars_in_func.add(c_var_name)

    def _format_c_value(self, json_value_node, expected_c_type, current_parent_c_var, current_context):
        """
        Converts a JSON node to a C literal string or C variable name.
        expected_c_type is the C type string (e.g., "int", "const char *", "lv_color_t", "lv_align_t").
        current_parent_c_var is the C variable name of the LVGL object being configured,
                                for context like lv_pct.
        json_value_node is a Python primitive (None, int, float, bool, str) or dict/list.
        current_context is the Python dictionary representing the active context.
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
            
            # 1. Handle prefixes: $, #, @, !
            if s_val.startswith("$") and not s_val.startswith("$$"): 
                var_name = s_val[1:]
                if var_name in current_context:
                    context_value = current_context[var_name]
                    logger.debug(f"Context variable ${var_name} resolved to: {json.dumps(context_value)} from context: {json.dumps(current_context)}")
                    return self._format_c_value(context_value, expected_c_type, current_parent_c_var, current_context)
                else:
                    logger.warning(f"Context variable ${var_name} not found in current context {current_context}. Treating as error or literal.")
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
                    return f"\"{s_val}\""

            if s_val.startswith("@"): # Runtime pointer reference from registry
                ref_id = s_val[1:]
                # Generate C code to call lvgl_json_get_registered_ptr at runtime.
                # The expected_c_type is passed as the expected_type_name for the API call.
                # The API's C implementation handles base type extraction if expected_c_type is "lv_obj_t *".
                # The cast `(expected_c_type)` ensures the C compiler type checks the result.
                logger.debug(f"Generating runtime lookup for '@{ref_id}' with expected C type '{expected_c_type}'")
                return f"(({expected_c_type})lvgl_json_get_registered_ptr(\"{ref_id}\", \"{expected_c_type}\"))"

            if s_val.startswith("!"): 
                static_str_val = s_val[1:]
                escaped_s = static_str_val.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                return f"\"{escaped_s}\""

            if s_val.endswith("%") and (expected_c_type == "lv_coord_t" or expected_c_type == "int32_t"):
                try:
                    num_part = s_val[:-1]
                    val = int(num_part)
                    return f"lv_pct({val})"
                except ValueError:
                    logger.warning(f"Invalid percentage string: {s_val}. Treating as literal string.")
                    return f"\"{s_val}\"" 

            is_known_symbol = False
            for member_info in self.api_info.get('hashed_and_sorted_enum_members', []):
                if member_info['name'] == s_val:
                    is_known_symbol = True
                    return s_val 
            
            if is_known_symbol: 
                pass
            else: 
                escaped_s = s_val.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                return f"\"{escaped_s}\""

        if isinstance(json_value_node, list): 
            if expected_c_type == "const lv_coord_t *" or expected_c_type == "lv_coord_t *":
                c_array_elements = []
                for item_node in json_value_node: 
                    c_array_elements.append(self._format_c_value(item_node, "lv_coord_t", current_parent_c_var, current_context))
                
                array_var_name = self._get_unique_c_var_name_base("coord_array")
                self._add_decl(f"    static const lv_coord_t {array_var_name}[] = {{ {', '.join(c_array_elements)} }};")
                return array_var_name 
            else:
                logger.warning(f"Direct C transpilation of JSON array for type '{expected_c_type}' is not fully supported. Treating as error.")
                return "/* JSON array not supported for this type */"

        if isinstance(json_value_node, dict): 
            call_item = json_value_node.get("call") 
            args_item = json_value_node.get("args") 
            if call_item and isinstance(call_item, str) and args_item and isinstance(args_item, list): 
                nested_func_name = call_item 
                
                func_api_def = next((f for f in self.api_info['functions'] if f['name'] == nested_func_name), None)
                if not func_api_def:
                    logger.warning(f"Nested call to unknown function '{nested_func_name}'. Cannot determine argument types.")
                    return f"/* ERROR: Unknown nested func {nested_func_name} */"

                nested_c_args = []
                num_json_args_provided = len(args_item) 
                implicit_parent_for_nested = current_parent_c_var 
                expected_c_func_args = func_api_def.get('_resolved_arg_types', [])
                
                for i in range(num_json_args_provided):
                    json_arg_node = args_item[i] 
                    arg_c_type_str = "unknown_arg_type"
                    if i < len(expected_c_func_args):
                        arg_base_type, arg_ptr_level, _ = expected_c_func_args[i]
                        arg_c_type_str = type_utils.get_c_type_str(arg_base_type, arg_ptr_level)
                    
                    nested_c_args.append(self._format_c_value(json_arg_node, arg_c_type_str, implicit_parent_for_nested, current_context))
                
                return f"{nested_func_name}({', '.join(nested_c_args)})"
            else:
                logger.warning(f"Unsupported JSON object structure for C value formatting: {json.dumps(json_value_node)}. Expected '{{ \"call\": ..., \"args\": ... }}' structure.")
                return "/* Unsupported JSON object value */"

        logger.warning(f"Unhandled JSON value type for C formatting: {type(json_value_node)}")
        return "/* unhandled_json_value */"


    def _transpile_node(self, json_node_data, parent_c_var_name, current_context, current_named_path=None):
        if not isinstance(json_node_data, dict):
            logger.error("Node data is not a JSON object. Skipping.")
            return

        node_type_item = json_node_data.get("type")
        node_type_str = "obj" 
        if node_type_item and isinstance(node_type_item, str):
            node_type_str = node_type_item
        
        node_id_item = json_node_data.get("id")
        json_id_str = None 
        if node_id_item and isinstance(node_id_item, str) and node_id_item.startswith("@"):
            json_id_str = node_id_item[1:]

        context_for_children = current_context
        context_property_on_node = json_node_data.get("context")

        if context_property_on_node is not None:
            if isinstance(context_property_on_node, dict):
                context_for_children = context_property_on_node
            else:
                logger.warning(f"Node {json_node_data.get('id', json_node_data.get('type', 'unknown'))} has 'context' property, but it's not a dictionary. Ignoring. Value: {context_property_on_node}")

        if node_type_str == "component":
            comp_id_item = json_node_data.get("id")
            comp_root_item = json_node_data.get("root")
            if comp_id_item and isinstance(comp_id_item, str) and \
               comp_id_item.startswith("@") and \
               comp_root_item and isinstance(comp_root_item, dict):
                
                comp_id = comp_id_item[1:]
                self.component_definitions[comp_id] = copy.deepcopy(comp_root_item)
                self._add_impl(f"// Component definition '{comp_id}' processed and stored.")
            else:
                self._add_impl(f"// ERROR: Invalid 'component' definition: {json.dumps(json_node_data)}")
            return 

        if node_type_str == "use-view":
            view_id_item = json_node_data.get("id")
            if view_id_item and isinstance(view_id_item, str) and view_id_item.startswith("@"):
                view_id = view_id_item[1:]
                if view_id in self.component_definitions:
                    self._add_impl(f"// Using view component '{view_id}' with context {context_for_children}")
                    component_root_node_data = self.component_definitions[view_id]
                    self.current_indent_level +=1
                    self._transpile_node(component_root_node_data, parent_c_var_name, context_for_children, current_named_path)
                    self.current_indent_level -=1
                else:
                    self._add_impl(f"// ERROR: View component '{view_id}' not defined.")
                    fb_label_var = self._get_unique_c_var_name_base("fallback_label")
                    self._declare_c_var_if_needed("lv_obj_t *", fb_label_var)
                    self._add_impl(f"{fb_label_var} = lv_label_create({parent_c_var_name});")
                    self._add_impl(f"lv_label_set_text_fmt({fb_label_var}, \"Error: View '%s' not found\", \"{view_id}\");") # Fixed fmt string
            else:
                self._add_impl(f"// ERROR: Invalid 'use-view' definition: {json.dumps(json_node_data)}")
            return 

        if node_type_str == "context": 
            context_values_for_block = json_node_data.get("values")
            for_item = json_node_data.get("for")
            if for_item and isinstance(for_item, dict):
                self._add_impl(f"// Processing 'context -> for' block:")
                self.current_indent_level+=1
                
                defined_context_for_for_child = {} 
                if context_values_for_block and isinstance(context_values_for_block, dict):
                    defined_context_for_for_child = context_values_for_block
                else:
                    logger.warning(f"'context' block's 'values' property is missing or not an object. Using empty context for 'for' child. Node: {json.dumps(json_node_data)}")
                self._transpile_node(for_item, parent_c_var_name, defined_context_for_for_child, current_named_path)
                self.current_indent_level-=1
            else:
                 self._add_impl(f"// WARNING: 'context' block without a valid 'for' object. Skipping. {json.dumps(json_node_data)}")
            return
            
        created_c_entity_var = None 
        is_widget_node = True 
        actual_lvgl_type_for_setters = node_type_str 
        
        c_var_to_register_ptr = None # Will hold the C variable name or &variable_name for registration
        type_name_for_registry = ""  # Will hold the string like "lv_obj_t", "lv_style_t"

        if node_type_str == "style":
            is_widget_node = False
            style_c_var_name = json_id_str if json_id_str else self._get_unique_c_var_name_base("style")
            if json_id_str: 
                self.c_style_vars[json_id_str] = style_c_var_name
            
            self._declare_c_var_if_needed("lv_style_t", style_c_var_name)
            self._add_impl(f"lv_style_init(&{style_c_var_name});")
            created_c_entity_var = f"&{style_c_var_name}" 
            
            c_var_to_register_ptr = f"&{style_c_var_name}"
            type_name_for_registry = "lv_style_t"
            actual_lvgl_type_for_setters = "style" 

        elif node_type_str == "grid": 
            obj_c_var_name = json_id_str if json_id_str else self._get_unique_c_var_name_base("grid_obj")
            if json_id_str: self.c_obj_vars[json_id_str] = obj_c_var_name
            
            self._declare_c_var_if_needed("lv_obj_t *", obj_c_var_name)
            self._add_impl(f"{obj_c_var_name} = lv_obj_create({parent_c_var_name});")
            created_c_entity_var = obj_c_var_name
            
            c_var_to_register_ptr = obj_c_var_name
            type_name_for_registry = "lv_obj_t" # Grids are lv_obj_t
            actual_lvgl_type_for_setters = "obj" 
            
        elif node_type_str == "with": 
            created_c_entity_var = parent_c_var_name
            actual_lvgl_type_for_setters = "obj"
            is_widget_node = True 
            # For "with" blocks, if they have a "named" property, they refer to the parent.
            # Determine parent's type for registration if possible (complex, default to lv_obj_t for now)
            # This part needs parent type info to be accurate for type_name_for_registry.
            # For simplicity, if "with" has "named", assume it registers the parent as "lv_obj_t" unless more info is available.
            parent_obj_type_str_for_setters = "obj" # Default assumption for parent type
            # TODO: Enhance this by trying to get actual parent type for 'with' named registration
            c_var_to_register_ptr = parent_c_var_name 
            type_name_for_registry = f"lv_{parent_obj_type_str_for_setters}_t"

        else: 
            obj_c_var_name = json_id_str if json_id_str else self._get_unique_c_var_name_base(node_type_str)
            if json_id_str: self.c_obj_vars[json_id_str] = obj_c_var_name

            self._declare_c_var_if_needed("lv_obj_t *", obj_c_var_name)
            create_func_name = f"lv_{node_type_str}_create"
            if not any(f['name'] == create_func_name for f in self.api_info['functions']):
                self._add_impl(f"// ERROR: Create function '{create_func_name}' not found for type '{node_type_str}'. Creating as lv_obj_create.")
                create_func_name = "lv_obj_create"
                actual_lvgl_type_for_setters = "obj"
                type_name_for_registry = "lv_obj_t"
            else:
                type_name_for_registry = f"lv_{node_type_str}_t"


            self._add_impl(f"{obj_c_var_name} = {create_func_name}({parent_c_var_name});")
            created_c_entity_var = obj_c_var_name
            c_var_to_register_ptr = obj_c_var_name


        current_named_path_for_children = current_named_path # Inherit by default

        if created_c_entity_var: 
            for prop_json_name, prop_value_node in json_node_data.items():
                if prop_json_name in ["type", "id", "children", "context"]: 
                    continue
                    
                if node_type_str == "grid" and prop_json_name in ["cols", "rows"]:
                    if isinstance(prop_value_node, list): 
                        c_array_literal = self._format_c_value(prop_value_node, "const lv_coord_t *", created_c_entity_var, current_context)
                        if prop_json_name == "cols":
                            self.grid_col_dsc_var = c_array_literal
                        else: 
                            self.grid_row_dsc_var = c_array_literal
                    else:
                        self._add_impl(f"// ERROR: Grid '{prop_json_name}' must be an array.")
                    continue 
                    
                if prop_json_name == "named" and isinstance(prop_value_node, str) and c_var_to_register_ptr:
                    named_suffix = prop_value_node
                    full_named_path = f"{current_named_path}:{named_suffix}" if current_named_path else named_suffix
                    
                    self._add_impl(f"lvgl_json_register_ptr(\"{full_named_path}\", \"{type_name_for_registry}\", (void*){c_var_to_register_ptr});")
                    # Update current_named_path_for_children for children of this "named" node
                    current_named_path_for_children = full_named_path 
                    continue

                setter_func_name = None
                potential_setters = []
                if is_widget_node:
                    potential_setters.append(f"lv_{actual_lvgl_type_for_setters}_set_{prop_json_name}")
                    potential_setters.append(f"lv_obj_set_{prop_json_name}")
                    potential_setters.append(f"lv_obj_{prop_json_name}") 
                    potential_setters.append(f"lv_obj_set_style_{prop_json_name}") 
                elif actual_lvgl_type_for_setters == "style":
                    potential_setters.append(f"lv_style_set_{prop_json_name}")
                    potential_setters.append(f"lv_style_{prop_json_name}")
                potential_setters.append(prop_json_name) 

                found_setter_api_def = None
                for pot_setter in potential_setters:
                    setter_api_def = next((f for f in self.api_info['functions'] if f['name'] == pot_setter), None)
                    if setter_api_def:
                        setter_func_name = pot_setter
                        found_setter_api_def = setter_api_def
                        break
                
                if not setter_func_name or not found_setter_api_def:
                    self._add_impl(f"// WARNING: No setter function found for property '{prop_json_name}' on type '{actual_lvgl_type_for_setters}'. Skipping.")
                    continue

                setter_args_c_types = found_setter_api_def.get('_resolved_arg_types', [])
                c_call_args = [created_c_entity_var] 

                prop_value_json_array_for_setter_args = prop_value_node
                if not isinstance(prop_value_json_array_for_setter_args, list):
                    prop_value_json_array_for_setter_args = [copy.deepcopy(prop_value_node)]

                num_json_setter_args = len(prop_value_json_array_for_setter_args)
                
                for i in range(num_json_setter_args):
                    json_arg_node = prop_value_json_array_for_setter_args[i]
                    expected_arg_c_type = "unknown"
                    if (i + 1) < len(setter_args_c_types): 
                        arg_base, arg_ptr, _ = setter_args_c_types[i+1]
                        expected_arg_c_type = type_utils.get_c_type_str(arg_base, arg_ptr)
                    
                    c_call_args.append(self._format_c_value(json_arg_node, expected_arg_c_type, created_c_entity_var, current_context))

                if setter_func_name.startswith("lv_obj_set_style_") and \
                   len(c_call_args) == 2 and len(setter_args_c_types) == 3: 
                       self._add_impl(f"// INFO: Adding default selector LV_PART_MAIN (0) for {setter_func_name}")
                       c_call_args.append("0") 

                self._add_impl(f"{setter_func_name}({', '.join(c_call_args)});")                    
            
            if node_type_str == "grid" and hasattr(self, 'grid_col_dsc_var') and hasattr(self, 'grid_row_dsc_var'):
                self._add_impl(f"lv_obj_set_grid_dsc_array({created_c_entity_var}, {self.grid_col_dsc_var}, {self.grid_row_dsc_var});")
                del self.grid_col_dsc_var
                del self.grid_row_dsc_var

        if is_widget_node and created_c_entity_var and node_type_str != "with": 
            children_item_list = json_node_data.get("children") 
            if children_item_list and isinstance(children_item_list, list):
                self._add_impl(f"// Children of {created_c_entity_var} ({node_type_str})")
                self.current_indent_level += 1
                for child_node_dict in children_item_list: 
                    path_for_child = current_named_path_for_children
                    self._transpile_node(child_node_dict, created_c_entity_var, context_for_children, path_for_child)
                self.current_indent_level -= 1
        
    def generate_c_function(self, function_name="create_ui_from_spec"):
        self._add_predecl(f"#include \"lvgl.h\"", indent=False)
        self._add_predecl(f"", indent=False)
        self._add_predecl(f"extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);", indent=False)
        self._add_predecl(f"extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);", indent=False)
        self._add_predecl(f"", indent=False)

        self._add_predecl(f"#ifndef LV_GRID_FR", indent=False)
        self._add_predecl(f"#define LV_GRID_FR(x) (lv_coord_t)(LV_COORD_FRACT_MAX / (x))", indent=False)
        self._add_predecl(f"#endif", indent=False)
        self._add_predecl(f"#ifndef LV_SIZE_CONTENT", indent=False)
        self._add_predecl(f"#define LV_SIZE_CONTENT LV_COORD_MAX", indent=False)
        self._add_predecl(f"#endif", indent=False)
        self._add_predecl(f"", indent=False)

        self._add_predecl(f"void {function_name}(lv_obj_t *screen_parent) {{", indent=False)
        
        declaration_section_marker = "// --- VARIABLE DECLARATIONS ---"
        self._add_predecl(declaration_section_marker) 

        self._add_impl(f"lv_obj_t *parent_obj = screen_parent ? screen_parent : lv_screen_active();")
        self._add_impl(f"if (!parent_obj) {{")
        self._add_impl(f"    LV_LOG_ERROR(\"Cannot render UI: No parent screen available.\");") # LV_LOG_ERROR assumes LVGL logging
        self._add_impl(f"    return;")
        self._add_impl(f"}}")
        self._add_impl("") 

        initial_context = {} 
        if isinstance(self.ui_spec_data, list): 
            for child_node_data in self.ui_spec_data: 
                self._transpile_node(child_node_data, "parent_obj", initial_context, None) # Pass None for initial named_path
        elif isinstance(self.ui_spec_data, dict): 
            self._transpile_node(self.ui_spec_data, "parent_obj", initial_context, None) # Pass None for initial named_path
        else:
            self._add_impl("    // UI specification data is not a list or object.");

        self._add_impl("}", indent=False) 
        
        return "\n".join(self.generated_c_lines_predecl) + "\n" + "\n".join(self.generated_c_lines_decl) + "\n\n" + "\n".join(self.generated_c_lines_impl)

def generate_c_transpiled_ui(api_info, ui_spec_path, output_dir, output_filename_base="ui_transpiled"):
    logger.info(f"Starting C transpilation from UI spec: {ui_spec_path}")    

    try:
        with open(ui_spec_path, 'r', encoding='utf-8') as f:
            ui_spec_data = json.load(f) 
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
        f"// Forward declarations for registry API (ensure these are available in your build)",
        f"// void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);",
        f"// void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);",
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
        with open(h_file_path, "w", encoding='utf-8') as f:
            f.write("\n".join(header_content_list))
    except IOError as e:
        logger.error(f"Failed to write C header file {h_file_path}: {e}")
        return

    logger.info(f"C transpilation for '{ui_spec_path}' complete. Output base: '{output_filename_base}'.")