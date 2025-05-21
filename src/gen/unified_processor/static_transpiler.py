import json
import logging
import re # For sanitizing names for C variables
from .base_processor import BaseUIProcessor

logger = logging.getLogger(__name__)

class StaticUICodeTranspiler(BaseUIProcessor):
    def __init__(self, api_info, lv_def_json_path, str_vals_json_path):
        super().__init__(api_info, lv_def_json_path, str_vals_json_path)
        
        self.c_code_lines_predecl = []  # For includes, forward declarations, global static styles
        self.c_code_lines_decl = []     # For C variable declarations within the main function
        self.c_code_lines_impl = []     # For C implementation code within the main function
        
        # Maps JSON IDs/paths to C variable info
        # e.g., {"my_button": {"c_var": "c_button_1", "lvgl_type": "lv_button", "is_ptr": True, "json_type": "button"}}
        self.c_entity_vars = {}         
        self.c_var_counter = 0          # For generating unique C variable names
        self.declared_c_vars_in_func = set() # To avoid duplicate C declarations in the current function scope
        self.current_indent_level = 1   # For pretty C code (1 level for inside function)
        self.ui_spec_root_node = None # To store the root of the UI spec for processing

    # --- Helper Methods ---

    def _indent(self, level_offset=0):
        """Returns current indentation string."""
        return "    " * (self.current_indent_level + level_offset)

    def _add_predecl(self, line, indent=False):
        """Adds a line to the predeclarations section."""
        if indent:
            # For predeclarations, indent usually starts from 0 unless specified.
            # Let's assume self.current_indent_level is for impl lines.
            # For global scope, indent is typically 0 or 1.
            # This method might need refinement based on usage.
            self.c_code_lines_predecl.append("    " * (0 + level_offset if indent is True else 0) + line)
        else:
            self.c_code_lines_predecl.append(line)

    def _add_decl(self, line):
        """Adds a line to the C variable declarations section (typically at the start of the function)."""
        # Declarations inside a function are typically indented one level.
        self.c_code_lines_decl.append("    " * self.current_indent_level + line)

    def _add_impl(self, line, indent=True):
        """Adds a line to the C implementation code section."""
        if indent:
            self.c_code_lines_impl.append(self._indent() + line)
        else:
            self.c_code_lines_impl.append(line)

    def _sanitize_name_for_c(self, name):
        """Sanitizes a name to be a valid C variable name component."""
        # Replace non-alphanumeric characters (except underscore) with underscore
        name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
        # If the first character is a digit, prepend with an underscore or a char
        if name and name[0].isdigit():
            name = "c_" + name
        if not name: # Handle empty string case
            return "c_empty_name"
        return name

    def _get_unique_c_var_name_base(self, base_name="var"):
        """Generates unique C variable names."""
        self.c_var_counter += 1
        clean_base = self._sanitize_name_for_c(base_name)
        return f"{clean_base}_{self.c_var_counter}"

    def _declare_c_var_if_needed(self, c_type, c_var_name, is_static=False, is_global=False):
        """
        Adds a C variable declaration if not already declared.
        If is_global is True, adds to predeclarations (e.g. for static styles).
        If is_static is True (and not global), it's a static var within the function.
        """
        # Globals are handled by adding directly to self.c_code_lines_predecl outside this function for now.
        # This function primarily focuses on declarations within the current C function scope.
        if is_global:
            # For global static variables (like styles), they are added to c_code_lines_predecl.
            # We still add to declared_c_vars_in_func to prevent local re-declaration if name clashes.
            if c_var_name not in self.declared_c_vars_in_func: # Check to avoid adding to set multiple times if called directly
                 self.declared_c_vars_in_func.add(c_var_name) # Track it to avoid local re-declaration
                 # The actual line is added via _add_predecl where this is called.
            return # Global declaration is handled by caller using _add_predecl

        if c_var_name in self.declared_c_vars_in_func:
            return

        declaration = f"{c_type} {c_var_name};"
        if is_static: # Static within function
            declaration = f"static {c_type} {c_var_name};"
        
        self._add_decl(declaration) 
        self.declared_c_vars_in_func.add(c_var_name)
        logger.debug(f"Declared C variable in function scope: {declaration}")

    # --- Abstract Method Implementations ---

    def _resolve_value(self, json_value_node, expected_type_hint=None, current_context=None, target_entity_c_var_name=None):
        """
        Translates a JSON value node directly into a C code string representing that value.
        """
        if current_context is None:
            current_context = {}

        if isinstance(json_value_node, str):
            if json_value_node.startswith("$"):
                var_name = json_value_node[1:]
                if var_name in current_context:
                    # Recursively resolve the value from context
                    return self._resolve_value(current_context[var_name], expected_type_hint, current_context, target_entity_c_var_name)
                else:
                    logger.warning(f"Context variable '{var_name}' not found. Using as literal string.")
                    return f"\"{json_value_node}\"" # Fallback to string literal
            elif json_value_node.startswith("@"):
                ptr_name = json_value_node[1:]
                entity_info = self.c_entity_vars.get(ptr_name)
                if entity_info:
                    c_var = entity_info["c_var"]
                    is_ptr = entity_info.get("is_ptr", True) # Assume pointer by default
                    # If a pointer type is expected, or if the entity itself is a pointer (e.g. lv_obj_t*)
                    if (expected_type_hint and expected_type_hint.endswith("*")) or is_ptr:
                        return c_var if is_ptr else f"&{c_var}"
                    else: # Value type expected
                        return f"*{c_var}" if is_ptr else c_var
                else:
                    logger.error(f"Referenced entity '@{ptr_name}' not found in c_entity_vars. Using NULL.")
                    return "NULL"
            elif json_value_node.startswith("#"):
                hex_color = json_value_node[1:]
                return f"lv_color_hex(0x{hex_color})"
            elif json_value_node.endswith("%") and json_value_node[:-1].lstrip('-').isdigit(): # Handles "N%" and "-N%"
                # Note: lv_pct needs target_entity_c_var_name for some use cases (LV_PCT_PARENT), not handled here yet.
                # expected_type_hint == "lv_coord_t" is a good check for when to use lv_pct.
                # For now, we assume if it's N%, it's lv_pct.
                numeric_val = json_value_node[:-1]
                return f"lv_pct({numeric_val})"
            elif json_value_node.startswith("!"): # Explicit string literal
                str_val = json_value_node[1:]
                return f"\"{str_val.replace('"', '\\"')}\""
            else: # Regular string literal
                return f"\"{json_value_node.replace('"', '\\"')}\""
        elif isinstance(json_value_node, bool):
            return "true" if json_value_node else "false"
        elif isinstance(json_value_node, (int, float)):
            return str(json_value_node)
        elif isinstance(json_value_node, dict):
            if "call" in json_value_node:
                func_name_json = json_value_node["call"]
                args_json = json_value_node.get("args", [])
                
                # Resolve func_name if it's a variable, else treat as literal
                c_func_name = self._resolve_value(func_name_json, None, current_context, target_entity_c_var_name)
                # If func_name_json was a variable that resolved to a string literal (e.g. "$funcVar" -> "\"my_c_func\""),
                # we need to strip the quotes for it to be a valid C function name.
                if c_func_name.startswith('"') and c_func_name.endswith('"'):
                    c_func_name = c_func_name[1:-1]

                # TODO: Sophisticated argument type lookup from self.api_info if available
                # For now, resolve args without specific type hints beyond what _resolve_value does.
                resolved_args = [self._resolve_value(arg, None, current_context, target_entity_c_var_name) for arg in args_json]
                return f"{c_func_name}({', '.join(resolved_args)})"
            else:
                logger.warning(f"Cannot resolve complex dict to C value: {json_value_node}")
                return "NULL /* complex dict not resolved */"
        elif isinstance(json_value_node, list):
            # Handle arrays, especially for grid descriptors like `const lv_coord_t arr[] = {...};`
            if expected_type_hint == "const lv_coord_t *" or expected_type_hint == "lv_coord_t[]":
                array_var_name = self._get_unique_c_var_name_base("coord_array")
                resolved_elements = [self._resolve_value(el, "lv_coord_t", current_context, target_entity_c_var_name) for el in json_value_node]
                elements_str = ", ".join(resolved_elements)
                # Declare as static const array in predeclarations
                self._add_predecl(f"static const lv_coord_t {array_var_name}[] = {{ {elements_str} }};")
                self.declared_c_vars_in_func.add(array_var_name) # Track to avoid local redeclaration
                return array_var_name # Return the name of the C array
            else:
                logger.warning(f"Cannot resolve list to C value without specific type hint (e.g., const lv_coord_t *): {json_value_node}")
                return "NULL /* list not resolved */"
        elif json_value_node is None:
            return "NULL"
        
        logger.error(f"Unsupported JSON value type for C transpilation: {type(json_value_node)}, value: {json_value_node}")
        return "NULL /* error: unsupported value type */"

    def _handle_widget_creation(self, json_node, parent_c_var_name, context_at_creation):
        widget_type_json = json_node.get("type", "obj") # e.g., "button", "label"
        # Default to "obj" if type is not specific enough for a concrete lv_ widget type
        lvgl_constructor_base = f"lv_{widget_type_json}" if widget_type_json != "obj" else "lv_obj"
        
        c_var_name = self._get_unique_c_var_name_base(self._sanitize_name_for_c(widget_type_json))
        
        # All LVGL objects are pointers
        self._declare_c_var_if_needed("lv_obj_t *", c_var_name)
        
        # Generate creation code
        # TODO: Handle constructor arguments if json_node has them (e.g. for lv_label_create(parent, text))
        # For now, assume all create functions just take parent.
        # A more robust solution would check self.api_info for constructor signature.
        creation_func = f"{lvgl_constructor_base}_create"
        if widget_type_json == "label" and "text" in json_node: # Special common case for label
            # If label has text property, it's often a second arg to create func
            # This is a heuristic. Proper handling needs API info.
            # text_val = self._resolve_value(json_node["text"], "const char *", context_at_creation, None)
            # self._add_impl(f"{c_var_name} = {creation_func}({parent_c_var_name}, {text_val});")
            # For now, stick to create(parent) and set text via property
            self._add_impl(f"{c_var_name} = {creation_func}({parent_c_var_name});")

        else:
            self._add_impl(f"{c_var_name} = {creation_func}({parent_c_var_name});")
            
        # Return C var name, LVGL type string (e.g., "lv_button" - used for finding setters), original JSON type
        return c_var_name, lvgl_constructor_base, widget_type_json

    def _handle_style_creation(self, json_node, context_at_creation):
        style_json_id = json_node.get("id")
        base_name = "style"
        if style_json_id:
            base_name = self._sanitize_name_for_c(style_json_id)

        c_var_name = self._get_unique_c_var_name_base(base_name)
        
        # Declare static style struct in predeclarations (global scope for the file)
        self._add_predecl(f"static lv_style_t {c_var_name};")
        self.declared_c_vars_in_func.add(c_var_name) # Track to prevent local re-declaration if name clashes (unlikely for unique names)
        
        # Add initialization implementation
        self._add_impl(f"lv_style_init(&{c_var_name});")
        
        # Return C variable name (struct, not pointer) and "style" as its LVGL type string
        # The entity_lvgl_type_str "style" signals that operations need '&' for this C variable.
        return c_var_name, "style", "style" # c_var_name, entity_lvgl_type_str, original_json_type

    def _apply_property(self, entity_c_var_name, entity_lvgl_type_str, prop_name, prop_value_json, current_context):
        # entity_c_var_name is the C variable name. If it's a style, it's "c_style_1". If widget, "c_obj_1".
        # entity_lvgl_type_str is like "lv_button" or "style".

        # 1. Determine the target C variable/reference
        c_target_expr = entity_c_var_name
        if entity_lvgl_type_str == "style": # Styles are structs, setters take pointers
            c_target_expr = f"&{entity_c_var_name}"
        
        # 2. Resolve the property value(s) to C code string(s)
        # Some setters take multiple arguments. If prop_value_json is a list, resolve each.
        # For now, assume simple setters or that _resolve_value handles lists for specific cases (like grid arrays).
        
        # Heuristic for setter function name. This needs to be much more robust using self.api_info.
        # Example: prop_name "width", entity_lvgl_type_str "lv_obj" -> lv_obj_set_width
        # Example: prop_name "bg_color", entity_lvgl_type_str "style" -> lv_style_set_bg_color
        # Example: prop_name "text_font", entity_lvgl_type_str "style" -> lv_style_set_text_font
        # Example: prop_name "grid_column_dsc_array", entity_lvgl_type_str "lv_obj" -> lv_obj_set_grid_dsc_array (special)

        setter_func_name = ""
        num_expected_args = 1 # Most setters take 1 value arg (plus obj/style pointer)
        is_style_prop = (entity_lvgl_type_str == "style")

        # TODO: Replace this with actual API lookup from self.api_info
        # This is a very simplified placeholder for function lookup.
        if is_style_prop:
            setter_func_name = f"lv_style_set_{prop_name}"
            # Special case for lv_style_set_transform_width, etc.
            if prop_name.startswith("transform_"): 
                 setter_func_name = f"lv_style_set_{prop_name}" # e.g. lv_style_set_transform_width
            # elif prop_name == "prop_meta": # Example for a meta property with multiple args
            #    setter_func_name = "lv_style_set_property_meta" 
            #    num_expected_args = 2 # e.g. (style, prop_id, value) - prop_id would be part of prop_name or value
        else: # Widget property
            # Common pattern: lv_obj_set_PROPERTY
            # More specific: lv_button_set_PROPERTY, lv_label_set_PROPERTY
            # Fallback: lv_obj_set_PROPERTY
            # Try specific type first: e.g. lv_button_set_checked
            # If not found, try generic lv_obj_set_prop_name
            setter_func_name = f"{entity_lvgl_type_str}_set_{prop_name}" 
            # A real lookup would be:
            # func_info = self.api_info.find_setter(entity_lvgl_type_str, prop_name)
            # setter_func_name = func_info.name
            # num_expected_args = func_info.num_args 
        
        # Resolve value(s)
        c_value_exprs = []
        if isinstance(prop_value_json, list) and num_expected_args > 1:
            # If setter expects multiple value args and we got a list
            # TODO: Get expected types for each arg from api_info
            for i, val_item in enumerate(prop_value_json):
                # Placeholder for expected_type_hint for multi-arg setters
                arg_type_hint = None # self.api_info.get_arg_type(setter_func_name, i)
                c_value_exprs.append(self._resolve_value(val_item, arg_type_hint, current_context, entity_c_var_name))
        else:
            # Single argument setter, or complex value handled by _resolve_value (like grid array)
            # TODO: Get expected type for the single arg from api_info
            # expected_arg_type = self.api_info.get_arg_type(setter_func_name, 0)
            expected_arg_type = None 
            if prop_name == "grid_column_dsc_array" or prop_name == "grid_row_dsc_array":
                expected_arg_type = "const lv_coord_t *"
            elif prop_name.endswith("_color") and is_style_prop:
                 expected_arg_type = "lv_color_t" # For _resolve_value if it uses hints
            
            c_value_exprs.append(self._resolve_value(prop_value_json, expected_arg_type, current_context, entity_c_var_name))

        # Selector for style properties (default to LV_PART_MAIN or 0 if not applicable)
        # TODO: Get selector from prop_value_json if it's a complex object, or use default.
        # For now, assume default selector for style properties if not specified.
        selector_expr = ""
        if is_style_prop:
            # Check if the LVGL function expects a selector as its first value argument
            # e.g. lv_style_set_bg_color(style, color) vs lv_style_set_text_color(style, selector, color)
            # This needs actual API info. For now, a simple heuristic:
            # Most lv_style_set_xxx take (style, value). Some take (style, selector, value).
            # Let's assume most common (style, value) unless we have specific info.
            # If the setter name is known from api_info, we'd know its signature.
            # For now, we pass only resolved values. If selector is needed, it must be part of prop_value_json
            # and resolved via _resolve_value, or the prop_name implies it.
            # A common pattern is that prop_name itself might include the part, e.g. "text_color_main_part".
            # Or, the JSON could be: "bg_color": {"value": "#FF0000", "selector": "LV_PART_SCROLLBAR"}
            # Current _resolve_value doesn't handle that complex dict for selector.
            # For simplicity, let's assume if a selector is needed, it's the *first* resolved value.
            # This is a guess.
            # A common simplified approach: if it's a style prop, pass 0 as selector if func expects it.
            # The LVGL C functions are overloaded or have default parts. lv_style_set_property(style, prop, value)
            # often uses the default part. If a specific part is needed, then lv_style_set_property_for_part(style, part, prop, value).
            # The direct function calls like lv_style_set_bg_color(style, color) apply to the default part.
            # If prop_name implies a part like "border_width_main", it should be handled.
            # For now, we assume the direct LVGL functions like lv_style_set_bg_color handle the default part.
            # If `prop_value_json` itself leads to a function call that includes a selector, that's fine.
            pass # No explicit selector handling here for now, assume direct mapping or value contains it.


        if setter_func_name:
            all_args_str = ", ".join([c_target_expr] + c_value_exprs)
            self._add_impl(f"{setter_func_name}({all_args_str});")
        else:
            logger.error(f"Could not determine setter function for property '{prop_name}' on type '{entity_lvgl_type_str}'.")


    def _register_entity_by_id(self, entity_c_var_name, id_str, entity_type_str_for_registry, lvgl_type_str):
        raise NotImplementedError("_register_entity_by_id must be implemented by StaticUICodeTranspiler")

    def _register_entity_by_id(self, entity_c_var_name, id_str, entity_type_str_for_registry, lvgl_type_str):
        # entity_lvgl_type_str is "lv_button", "style", etc.
        is_ptr_type = (lvgl_type_str != "style") # Styles are structs, widgets are lv_obj_t* (pointers)
        
        self.c_entity_vars[id_str] = {
            "c_var": entity_c_var_name,
            "lvgl_type": lvgl_type_str, # e.g., "lv_button" or "style"
            "is_ptr": is_ptr_type,
            "json_type": entity_type_str_for_registry # e.g., "button" or "style" (original JSON type)
        }
        logger.debug(f"Stored C var '{entity_c_var_name}' for ID '{id_str}' (LVGL type: {lvgl_type_str}, is_ptr: {is_ptr_type})")
        
        # Optional: C code for runtime registration (if that system is also used with static UI)
        # ref_for_runtime = f"&{entity_c_var_name}" if not is_ptr_type else entity_c_var_name
        # self._add_impl(f"// lvgl_json_register_ptr(\"{id_str}\", \"{entity_type_str_for_registry}\", (void*){ref_for_runtime}); // Optional runtime registration")


    def _register_entity_by_named_attr(self, entity_c_var_name, named_str, entity_type_str_for_registry, lvgl_type_str, current_path_prefix):
        if current_path_prefix and not current_path_prefix.endswith(":"):
            full_name = f"{current_path_prefix}:{named_str}"
        elif current_path_prefix: # Ends with ":"
            full_name = f"{current_path_prefix}{named_str}"
        else:
            full_name = named_str
        
        is_ptr_type = (lvgl_type_str != "style")
        self.c_entity_vars[full_name] = {
            "c_var": entity_c_var_name,
            "lvgl_type": lvgl_type_str,
            "is_ptr": is_ptr_type,
            "json_type": entity_type_str_for_registry
        }
        logger.debug(f"Stored C var '{entity_c_var_name}' for named attribute '{full_name}' (LVGL type: {lvgl_type_str}, is_ptr: {is_ptr_type})")
        # Optional runtime registration:
        # ref_for_runtime = f"&{entity_c_var_name}" if not is_ptr_type else entity_c_var_name
        # self._add_impl(f"// lvgl_json_register_ptr(\"{full_name}\", \"{entity_type_str_for_registry}\", (void*){ref_for_runtime});")


    def _get_entity_ref_from_id(self, id_str, expected_type_hint=None):
        # id_str can be a direct ID or a path like "screen1:button1"
        entity_info = self.c_entity_vars.get(id_str)
        if not entity_info:
            logger.error(f"Entity with ID or path '{id_str}' not found in c_entity_vars.")
            return f"NULL /* Unknown ID/path: {id_str} */"
        
        c_var = entity_info["c_var"]
        is_ptr = entity_info["is_ptr"]
        
        if expected_type_hint and expected_type_hint.endswith("*"): # Pointer type expected
            return c_var if is_ptr else f"&{c_var}"
        elif expected_type_hint and not expected_type_hint.endswith("*"): # Value type expected
             return f"*{c_var}" if is_ptr else c_var # Dereference if it's a pointer but value needed
        else: # No type hint, return direct C variable name or its address if it's a non-pointer (style)
            return c_var if is_ptr else f"&{c_var}"


    def _perform_do_block_on_entity(self, entity_c_var_name, entity_lvgl_type_str, entity_original_json_type, 
                                   do_block_json, context_for_do_block, path_prefix_for_do_block_attrs, 
                                   type_for_named_in_do_block):
        # entity_c_var_name is the C variable name (e.g. "c_obj_1", "c_style_1")
        # entity_lvgl_type_str is the LVGL base type ("lv_button", "style")

        original_context = self.current_context
        self.current_context = context_for_do_block.copy() # Use a copy for the 'do' block
        
        original_path_prefix = self.current_path_prefix
        self.current_path_prefix = path_prefix_for_do_block_attrs

        logger.debug(f"Performing 'do' block on entity '{entity_c_var_name}' (LVGL type: {entity_lvgl_type_str}) with path prefix '{path_prefix_for_do_block_attrs}'")

        for key, value in do_block_json.items():
            if key == "named":
                if isinstance(value, str):
                    self._register_entity_by_named_attr(
                        entity_c_var_name, value, 
                        type_for_named_in_do_block, # original json type like "button"
                        entity_lvgl_type_str, # lvgl type like "lv_button" or "style"
                        path_prefix_for_do_block_attrs # Path prefix for this specific named attribute
                    )
                else:
                    logger.warning(f"'named' attribute in 'do' block expects a string value. Got {type(value)} for entity {entity_c_var_name}.")
                continue
            elif key in ("id", "type", "context", "children", "with", "use-view", "component", "for"): # Meta-keys not processed as properties
                logger.debug(f"Skipping meta-key '{key}' within 'do' block for entity {entity_c_var_name}.")
                continue
            
            # Assume it's a property to apply
            self._apply_property(entity_c_var_name, entity_lvgl_type_str, key, value, self.current_context)
        
        self.current_context = original_context
        self.current_path_prefix = original_path_prefix

    # --- Orchestration ---
    def generate_transpiled_c_files(self, output_dir_path, ui_spec_root_node, c_function_name="create_ui"):
        """
        Generates the transpiled C code files (.c and .h).
        """
        self.ui_spec_root_node = ui_spec_root_node

        # Reset state for a fresh generation
        self.c_code_lines_predecl = [
            f"// Generated by StaticUICodeTranspiler for function {c_function_name}",
            "#include \"lvgl.h\"",
            "// Add any other common includes here (e.g., custom components, fonts)",
            ""
        ]
        self.c_code_lines_decl = []
        self.c_code_lines_impl = [] # Will hold the entire function body including signature
        self.c_entity_vars = {}
        self.c_var_counter = 0
        self.declared_c_vars_in_func = set()
        self.current_indent_level = 1 # Default indent for inside function body

        # Main C function signature. Parent obj is often lv_scr_act() or a passed container.
        func_signature = f"void {c_function_name}(lv_obj_t *parent_obj)"
        
        # Add function signature to implementation lines (will be first line of function body)
        self._add_impl(f"{func_signature} {{", indent=False)

        # Process the UI spec. Initial parent_ref is "parent_obj" from function argument.
        # This populates self.c_code_lines_decl and adds to self.c_code_lines_impl.
        self.process_ui_spec(self.ui_spec_root_node, initial_parent_ref="parent_obj", initial_context={}, initial_path_prefix="")

        self._add_impl("}", indent=False) # Close main C function

        # --- Assemble C File Content ---
        final_c_lines = []
        final_c_lines.extend(self.c_code_lines_predecl) # Includes, static styles
        final_c_lines.append("")
        
        # Full function body: signature, then declarations, then implementation lines, then closing brace.
        # The self.c_code_lines_impl already starts with the function signature and ends with '}'.
        # We need to insert declarations after the opening brace.
        
        # Find index of the opening brace in self.c_code_lines_impl
        open_brace_index = -1
        for i, line in enumerate(self.c_code_lines_impl):
            if line.strip() == f"{func_signature} {{": # Find the exact line
                open_brace_index = i
                break
        
        if open_brace_index != -1:
            # Insert declarations after the opening brace
            # Declarations from self.c_code_lines_decl are already indented
            # Add a blank line before declarations if there are any
            if self.c_code_lines_decl:
                self.c_code_lines_impl.insert(open_brace_index + 1, "") 
            for i, decl_line in enumerate(self.c_code_lines_decl):
                self.c_code_lines_impl.insert(open_brace_index + 1 + i + (1 if self.c_code_lines_decl else 0) , decl_line)
            # Add a blank line after declarations if there are any and if there's implementation code after
            if self.c_code_lines_decl and len(self.c_code_lines_impl) > (open_brace_index + 1 + len(self.c_code_lines_decl) + (1 if self.c_code_lines_decl else 0)):
                 self.c_code_lines_impl.insert(open_brace_index + 1 + len(self.c_code_lines_decl) + (1 if self.c_code_lines_decl else 0), "")
        
        final_c_lines.extend(self.c_code_lines_impl)

        # --- Assemble H File Content ---
        h_file_content_lines = [
            f"#ifndef {self._sanitize_name_for_c(c_function_name).upper()}_H",
            f"#define {self._sanitize_name_for_c(c_function_name).upper()}_H",
            "",
            "#include \"lvgl.h\"",
            "",
            f"{func_signature};", # The function prototype
            "",
            "#endif",
        ]
        
        # Store for potential writing by a tool later (or direct write for testing)
        self._generated_c_content = "\n".join(final_c_lines)
        self._generated_h_content = "\n".join(h_file_content_lines)

        logger.info(f"--- C File Content ({c_function_name}.c) ---")
        logger.info(self._generated_c_content)
        logger.info(f"--- H File Content ({c_function_name}.h) ---")
        logger.info(self._generated_h_content)
            
        # Actual file writing would be done here using appropriate tools or direct I/O.
        # For example:
        # self._write_to_file(f"{output_dir_path}/{c_function_name}.c", self._generated_c_content)
        # self._write_to_file(f"{output_dir_path}/{c_function_name}.h", self._generated_h_content)

        return True # Placeholder for success
    
    # def _write_to_file(self, filepath, content):
    #     import os
    #     os.makedirs(os.path.dirname(filepath), exist_ok=True)
    #     with open(filepath, 'w') as f:
    #         f.write(content)

```
