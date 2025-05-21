import logging
import re
import copy
from typing import Any, Dict, List, Optional, Tuple, Set, Union

from ..ui_generator_base import BaseUIGenerator, CJSONObject
# Assuming type_utils will be in a sibling directory or accessible via PYTHONPATH
# For now, let's define a placeholder if it's not immediately available
try:
    from .. import type_utils 
except ImportError:
    # Placeholder type_utils if not found (basic functionality)
    class TypeUtilsPlaceholder:
        def lvgl_type_to_widget_name(self, type_str: str) -> str:
            # Simplified: assumes type_str is already the widget name if not "obj"
            if type_str and type_str not in ["object", "obj", "lv_obj"]:
                return type_str.lower().replace("lv_", "")
            return "obj"

        def c_type_to_lvgl_widget_type(self, c_type_str: str) -> Optional[str]:
            match = re.match(r"lv_(\w+)_t\*", c_type_str)
            if match:
                return match.group(1)
            if c_type_str == "lv_obj_t*":
                return "obj"
            return None
        
        def get_lvgl_obj_type_from_var_name(self, var_name: str, api_info: Dict[str, Any]) -> Optional[str]:
            # This would require more sophisticated tracking or conventions
            if "_btn_" in var_name or var_name.startswith("btn"): return "button"
            if "_label_" in var_name or var_name.startswith("label"): return "label"
            return "obj" # Default

    type_utils = TypeUtilsPlaceholder()

logger = logging.getLogger(__name__)

# LVGL Constants that might be used as string literals in JSON
LVGL_CONSTANTS_MAP = {
    # Alignments
    "ALIGN.DEFAULT": "LV_ALIGN_DEFAULT",
    "ALIGN.TOP_LEFT": "LV_ALIGN_TOP_LEFT",
    "ALIGN.TOP_MID": "LV_ALIGN_TOP_MID",
    "ALIGN.TOP_RIGHT": "LV_ALIGN_TOP_RIGHT",
    "ALIGN.BOTTOM_LEFT": "LV_ALIGN_BOTTOM_LEFT",
    "ALIGN.BOTTOM_MID": "LV_ALIGN_BOTTOM_MID",
    "ALIGN.BOTTOM_RIGHT": "LV_ALIGN_BOTTOM_RIGHT",
    "ALIGN.LEFT_MID": "LV_ALIGN_LEFT_MID",
    "ALIGN.RIGHT_MID": "LV_ALIGN_RIGHT_MID",
    "ALIGN.CENTER": "LV_ALIGN_CENTER",
    "ALIGN.OUT_TOP_LEFT": "LV_ALIGN_OUT_TOP_LEFT",
    "ALIGN.OUT_TOP_MID": "LV_ALIGN_OUT_TOP_MID",
    "ALIGN.OUT_TOP_RIGHT": "LV_ALIGN_OUT_TOP_RIGHT",
    "ALIGN.OUT_BOTTOM_LEFT": "LV_ALIGN_OUT_BOTTOM_LEFT",
    "ALIGN.OUT_BOTTOM_MID": "LV_ALIGN_OUT_BOTTOM_MID",
    "ALIGN.OUT_BOTTOM_RIGHT": "LV_ALIGN_OUT_BOTTOM_RIGHT",
    "ALIGN.OUT_LEFT_TOP": "LV_ALIGN_OUT_LEFT_TOP",
    "ALIGN.OUT_LEFT_MID": "LV_ALIGN_OUT_LEFT_MID",
    "ALIGN.OUT_LEFT_BOTTOM": "LV_ALIGN_OUT_LEFT_BOTTOM",
    "ALIGN.OUT_RIGHT_TOP": "LV_ALIGN_OUT_RIGHT_TOP",
    "ALIGN.OUT_RIGHT_MID": "LV_ALIGN_OUT_RIGHT_MID",
    "ALIGN.OUT_RIGHT_BOTTOM": "LV_ALIGN_OUT_RIGHT_BOTTOM",

    # Flex
    "FLEX_FLOW.ROW": "LV_FLEX_FLOW_ROW",
    "FLEX_FLOW.COLUMN": "LV_FLEX_FLOW_COLUMN",
    "FLEX_FLOW.ROW_WRAP": "LV_FLEX_FLOW_ROW_WRAP",
    "FLEX_FLOW.ROW_REVERSE": "LV_FLEX_FLOW_ROW_REVERSE",
    "FLEX_FLOW.ROW_WRAP_REVERSE": "LV_FLEX_FLOW_ROW_WRAP_REVERSE",
    "FLEX_FLOW.COLUMN_WRAP": "LV_FLEX_FLOW_COLUMN_WRAP",
    "FLEX_FLOW.COLUMN_REVERSE": "LV_FLEX_FLOW_COLUMN_REVERSE",
    "FLEX_FLOW.COLUMN_WRAP_REVERSE": "LV_FLEX_FLOW_COLUMN_WRAP_REVERSE",
    "FLEX_ALIGN.START": "LV_FLEX_ALIGN_START",
    "FLEX_ALIGN.END": "LV_FLEX_ALIGN_END",
    "FLEX_ALIGN.CENTER": "LV_FLEX_ALIGN_CENTER",
    "FLEX_ALIGN.SPACE_EVENLY": "LV_FLEX_ALIGN_SPACE_EVENLY",
    "FLEX_ALIGN.SPACE_AROUND": "LV_FLEX_ALIGN_SPACE_AROUND",
    "FLEX_ALIGN.SPACE_BETWEEN": "LV_FLEX_ALIGN_SPACE_BETWEEN",
    
    # Grid
    "GRID_ALIGN.START": "LV_GRID_ALIGN_START",
    "GRID_ALIGN.END": "LV_GRID_ALIGN_END",
    "GRID_ALIGN.CENTER": "LV_GRID_ALIGN_CENTER",
    "GRID_ALIGN.STRETCH": "LV_GRID_ALIGN_STRETCH",
    "GRID_ALIGN.SPACE_EVENLY": "LV_GRID_ALIGN_SPACE_EVENLY",
    "GRID_ALIGN.SPACE_AROUND": "LV_GRID_ALIGN_SPACE_AROUND",
    "GRID_ALIGN.SPACE_BETWEEN": "LV_GRID_ALIGN_SPACE_BETWEEN",

    # Button matrix control map (example, needs to be comprehensive)
    "LV_BTNMATRIX_CTRL.HIDDEN": "LV_BTNMATRIX_CTRL_HIDDEN",
    "LV_BTNMATRIX_CTRL.NO_REPEAT": "LV_BTNMATRIX_CTRL_NO_REPEAT",
    # ... and many more LVGL enums/defines

    # States
    "STATE.DEFAULT": "LV_STATE_DEFAULT",
    "STATE.CHECKED": "LV_STATE_CHECKED",
    "STATE.FOCUSED": "LV_STATE_FOCUSED",
    # ... etc.

    # Parts
    "PART.MAIN": "LV_PART_MAIN",
    "PART.SCROLLBAR": "LV_PART_SCROLLBAR",
    "PART.INDICATOR": "LV_PART_INDICATOR",
    "PART.KNOB": "LV_PART_KNOB",
    "PART.SELECTED": "LV_PART_SELECTED",
    "PART.ITEMS": "LV_PART_ITEMS",
    "PART.CURSOR": "LV_PART_CURSOR",
    "PART.CUSTOM_FIRST": "LV_PART_CUSTOM_FIRST",

    # Misc
    "SIZE.CONTENT": "LV_SIZE_CONTENT",
    "OPA.COVER": "LV_OPA_COVER",
    "OPA.TRANSP": "LV_OPA_TRANSP",
    # ... and so on for common defines
}


class StaticCTranspiler(BaseUIGenerator):
    def __init__(self, api_info: Dict[str, Any], ui_spec_data: Any): # ui_spec_data is CJSONObject
        super().__init__(api_info, ui_spec_data)
        
        self.c_code: Dict[str, List[str]] = {
            "predecl": [], # For #includes, externs, static const arrays for grid
            "declarations": [], # For C variable declarations within the function
            "implementation": [], # For the main body of C code
            "style_definitions": [], # For static style definitions at global scope (if needed) or top of function
        }
        
        self.declared_c_vars_in_func: Set[str] = set()
        # Maps JSON ID (e.g., from "id": "@my_button") to its C info 
        # {c_var_name, c_type_str, is_ptr, lvgl_setter_type, original_json_type}
        self.local_registered_entities: Dict[str, Dict[str, Any]] = {} 
        self.var_counter: int = 0
        self.current_c_indent_level: int = 1 # Start inside the function body
        self.style_counter: int = 0
        self.grid_array_counter: int = 0

        self._add_initial_c_code()

    def _c_indent(self) -> str:
        return "    " * self.current_c_indent_level

    def _add_c_line(self, section: str, line: str, indent: bool = True, add_semicolon: bool = False):
        code_line = (self._c_indent() if indent else "") + line + (";" if add_semicolon else "")
        self.c_code[section].append(code_line)

    def _new_c_var_name_base(self, base_name: str = "var") -> str:
        # Sanitize base_name: replace invalid C chars with underscore
        # Remove leading '@' if it's from an ID string
        sanitized_base = re.sub(r'[^a-zA-Z0-9_]', '_', base_name.lstrip('@'))
        # Ensure it doesn't start with a digit
        if sanitized_base and sanitized_base[0].isdigit():
            sanitized_base = "_" + sanitized_base
        
        self.var_counter += 1
        return f"{sanitized_base}_{self.var_counter}"

    def _declare_c_var_if_needed(self, c_type: str, c_var_name: str, is_static: bool = False, initial_value: Optional[str] = None) -> None:
        if c_var_name not in self.declared_c_vars_in_func:
            declaration = f"{'static ' if is_static else ''}{c_type} {c_var_name}"
            if initial_value:
                declaration += f" = {initial_value}"
            self._add_c_line("declarations", declaration, indent=False, add_semicolon=True) # Declarations are at top, no indent
            self.declared_c_vars_in_func.add(c_var_name)

    def _add_initial_c_code(self):
        self._add_c_line("predecl", "#include \"lvgl.h\"", indent=False)
        # Common LVGL utility macros/defines (not all are #defines, some are enums, handled by format_value)
        # These are more for reference or if a specific #define is directly needed.
        # Most LVGL constants like LV_ALIGN_CENTER are resolved by format_value.
        # self._add_c_line("predecl", "// Common defines: LV_GRID_FR, LV_SIZE_CONTENT, LV_PART_MAIN etc. are typically used directly.", indent=False)
        self._add_c_line("predecl", "", indent=False)
        # Optional: extern declarations if interacting with a runtime registry
        self._add_c_line("predecl", "// extern void lvgl_json_register_ptr(const char *name, const char *type, void *ptr);", indent=False)
        self._add_c_line("predecl", "// extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type);", indent=False)
        self._add_c_line("predecl", "", indent=False)


    # --- Abstract Method Implementations (Core Logic) ---

    def create_entity(self, py_node_data: CJSONObject, type_str: str, parent_lvgl_c_var: Optional[str], named_path_prefix: Optional[str]) -> Dict[str, Any]:
        obj_c_var_base = self.get_node_id(py_node_data) or type_str # Use ID if present, else type
        obj_c_var_name = self._new_c_var_name_base(obj_c_var_base)
        
        # Default info structure
        info = {
            'c_var_name': obj_c_var_name,
            'c_type_str': "lv_obj_t*", # Default, will be overridden
            'is_ptr': True,
            'lvgl_setter_type': type_str, # Type for finding setters (e.g., "button")
            'original_json_type': type_str # Original "type" from JSON
        }

        if type_str == "style":
            self.style_counter += 1
            style_c_var_name = f"style_{self.style_counter}"
            info.update({
                'c_var_name': style_c_var_name,
                'c_type_str': "lv_style_t",
                'is_ptr': False, # Styles are typically stack or static variables, not pointers from create
                'lvgl_setter_type': "style",
            })
            # Declaration is static at the top of the function or global.
            # For simplicity, let's make them static within the function for now.
            self._add_c_line("style_definitions", f"static lv_style_t {style_c_var_name};", indent=False)
            self._add_c_line("implementation", f"lv_style_init(&{style_c_var_name});")
        
        elif type_str == "with":
            # "with" does not create a new C entity. It targets an existing one.
            # The target is resolved by format_value on py_node_data.get("obj")
            # and then apply_attributes is called on that target.
            # For the purpose of create_entity, we return info about the *expected* target.
            # If "obj" is a string (ID), we look it up. If not, it's an error or implies current parent.
            with_target_spec = py_node_data.get("obj")
            if isinstance(with_target_spec, str):
                resolved_target_info = self.get_entity_reference_for_id(with_target_spec)
                if resolved_target_info:
                    return copy.deepcopy(resolved_target_info) # Return copy of the target's info
                else: # Target not found by ID, this is an issue.
                    logger.error(f"'with' block targets unresolved ID: {with_target_spec}")
                    # Fallback: return info for a dummy lv_obj_t* that won't be used.
                    # Or, this could indicate the 'with' applies to its direct parent_lvgl_c_var if 'obj' is missing.
                    # The logic in 'apply_attributes' for 'with' will need to handle this carefully.
                    info['c_var_name'] = f"error_unknown_with_target_{self.var_counter}"
                    info['c_type_str'] = "lv_obj_t*" 
                    self._declare_c_var_if_needed(info['c_type_str'], info['c_var_name'])
                    self._add_c_line("implementation", f"{info['c_var_name']} = NULL; // Error: 'with' target '{with_target_spec}' not found")

            elif parent_lvgl_c_var: # If 'obj' is not specified, assume 'with' operates on parent
                 parent_info = self.get_type_information(parent_lvgl_c_var) # Try to get info of parent
                 if parent_info: return copy.deepcopy(parent_info)
                 # Fallback if parent_info is somehow not found (shouldn't happen)
                 info['c_var_name'] = parent_lvgl_c_var 
                 info['c_type_str'] = "lv_obj_t*" # Assume parent is lv_obj_t* if info missing
            else:
                logger.error("'with' block has no 'obj' and no parent_lvgl_c_var to implicitly target.")
                info['c_var_name'] = f"error_no_with_target_{self.var_counter}"
                info['c_type_str'] = "lv_obj_t*"
                self._declare_c_var_if_needed(info['c_type_str'], info['c_var_name'])
                self._add_c_line("implementation", f"{info['c_var_name']} = NULL; // Error: 'with' has no target")

        elif type_str == "grid": # "grid" itself is just an lv_obj that will have grid properties applied
            info['c_type_str'] = "lv_obj_t*"
            info['lvgl_setter_type'] = "obj" # Setters are for lv_obj
            self._declare_c_var_if_needed(info['c_type_str'], obj_c_var_name)
            self._add_c_line("implementation", f"{obj_c_var_name} = lv_obj_create({parent_lvgl_c_var if parent_lvgl_c_var else 'NULL'});")
            
        else: # Standard LVGL widgets
            lvgl_widget_name = type_utils.lvgl_type_to_widget_name(type_str) # e.g. "button", "label"
            
            # Determine C type, e.g. lv_button_t*, lv_label_t*
            # Some widgets like 'canvas' might have specific types not just lv_obj_t*
            # self.api_info might have this mapping if type_str is a key there.
            specific_c_type = f"lv_{lvgl_widget_name}_t*"
            # Fallback to lv_obj_t* if type is generic like "obj" or if specific type is not clear
            info['c_type_str'] = specific_c_type if lvgl_widget_name != "obj" else "lv_obj_t*"

            # Determine create function, e.g., lv_button_create
            create_func_name = f"lv_{type_str}_create" # Assumes type_str is like "button", "label"
            
            # Check if create_func_name exists in self.api_info (as a function)
            # This is a simplified check. A real api_info would have a list of functions.
            if not self.api_info.get(create_func_name) and not hasattr(type_utils, create_func_name): # Simplified check
                logger.warning(f"Create function '{create_func_name}' not found in api_info for type '{type_str}'. Falling back to 'lv_obj_create'.")
                create_func_name = "lv_obj_create"
                info['c_type_str'] = "lv_obj_t*" # If fallback, type is definitely lv_obj_t*
                info['lvgl_setter_type'] = "obj" # Setters will be for lv_obj

            self._declare_c_var_if_needed(info['c_type_str'], obj_c_var_name)
            self._add_c_line("implementation", f"{obj_c_var_name} = {create_func_name}({parent_lvgl_c_var if parent_lvgl_c_var else 'NULL'});")

        # Common registration for non-"with" types if they have an ID
        node_id = self.get_node_id(py_node_data)
        if node_id and type_str != "with": # "with" uses existing, doesn't define a new named entity itself
            self.local_registered_entities[node_id] = copy.deepcopy(info)
            # Also add mapping from c_var_name to info for easier lookup by var name in get_type_information
            self.local_registered_entities[obj_c_var_name] = copy.deepcopy(info) 

        return info

    def register_entity_internally(self, id_str: str, entity_info: Dict[str, Any], path_for_registration: str) -> None:
        # This method is primarily for transpiler's internal tracking via self.local_registered_entities
        # which is already handled by create_entity when an "id" is present.
        # It can also generate lvgl_json_register_ptr for runtime interop if needed.
        
        # Re-confirm registration if not done by ID directly in create_entity
        if id_str not in self.local_registered_entities:
             self.local_registered_entities[id_str] = copy.deepcopy(entity_info)
        if entity_info['c_var_name'] not in self.local_registered_entities:
             self.local_registered_entities[entity_info['c_var_name']] = copy.deepcopy(entity_info)


        # Example for runtime registration (if enabled/needed)
        # c_type_for_registry = entity_info.get('c_type_str', 'lv_obj_t*').replace('*', '') # e.g. "lv_button_t"
        # self._add_c_line("implementation", 
        #    f"// lvgl_json_register_ptr(\"{path_for_registration}\", \"{c_type_for_registry}\", (void*){entity_info['c_var_name']}); // Optional runtime registration")
        pass # Mostly handled by create_entity or direct use of local_registered_entities


    def call_setter(self, target_entity_info: Dict[str, Any], setter_fn_name: str, 
                    py_value_args: List[Any], # Python representations
                    py_current_context: CJSONObject) -> None:
        
        target_c_var = target_entity_info['c_var_name']
        if not target_c_var or target_c_var.startswith("error_"):
            logger.warning(f"Skipping setter '{setter_fn_name}' due to invalid target C variable: {target_c_var}")
            return

        # py_value_args are Python values. Need to format them into C literals/expressions.
        # The setter_fn_name should be the actual C function, e.g., "lv_obj_set_width".
        # We need to know the expected C types for each argument of setter_fn_name from api_info.
        
        setter_info = self.api_info.get(setter_fn_name, {})
        expected_arg_c_types = setter_info.get("args", []) # E.g., ["lv_obj_t*", "lv_coord_t"]
        
        formatted_c_args = []

        # First arg is usually the target object itself
        if expected_arg_c_types: # if we have type info
            formatted_c_args.append(target_c_var) # Already a C variable name
            arg_offset = 1 
        else: # No type info, assume first arg in py_value_args is not the target
            formatted_c_args.append(target_c_var) # Target is implicit first arg
            arg_offset = 0


        for i, py_arg_val in enumerate(py_value_args):
            expected_c_type = "unknown"
            if expected_arg_c_types and (i + arg_offset) < len(expected_arg_c_types):
                expected_c_type = expected_arg_c_types[i + arg_offset]
            
            # Special handling for style properties that might need a default selector
            is_style_prop = "style" in setter_fn_name.lower() and "prop" in setter_fn_name.lower()
            # lv_obj_set_style_prop(obj, prop, value, selector) -> 4 args
            # lv_style_set_prop(&style, prop, value) -> 3 args
            # Our py_value_args usually has [prop, value] for styles. Selector is implicit or default.
            
            # This logic is simplified. A full solution needs to know if setter_fn_name
            # is like lv_obj_set_style_... which takes selector, or lv_style_set_... which doesn't.
            # For now, format_value will handle resolving enums like "PROP.WIDTH".
            
            c_val_str = self.format_value(py_arg_val, expected_c_type, target_entity_info, py_current_context)
            formatted_c_args.append(c_val_str)

        # Handle implicit LV_PART_MAIN or LV_STATE_DEFAULT for some setters if not enough args provided
        # e.g. lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN)
        # py_value_args = [color_value]. Need to add LV_PART_MAIN.
        # This requires knowing the setter's signature from api_info.
        num_explicit_args_provided = len(py_value_args) + 1 # +1 for target_entity_info
        
        if expected_arg_c_types and num_explicit_args_provided < len(expected_arg_c_types):
            # Potentially add default selector/state
            last_expected = expected_arg_c_types[-1]
            if "lv_state_t" in last_expected.lower() and "state" not in formatted_c_args[-1].lower():
                 if num_explicit_args_provided == len(expected_arg_c_types) -1:
                    formatted_c_args.append("LV_STATE_DEFAULT")
            elif "lv_selector_t" in last_expected.lower() and "part" not in formatted_c_args[-1].lower(): # Simplified check
                 if num_explicit_args_provided == len(expected_arg_c_types) -1:
                    formatted_c_args.append("LV_PART_MAIN")
            # This might need to be more robust, checking types of already added args.

        args_str = ", ".join(formatted_c_args)
        self._add_c_line("implementation", f"{setter_fn_name}({args_str});")

    def format_value(self, py_json_value: Any, expected_c_type: str, 
                     context_entity_info: Optional[Dict[str, Any]], # Info of the entity this value is for
                     py_current_context: Any) -> str: # Python CJSONObject for current context scope
        
        if isinstance(py_json_value, str):
            # 1. Context variable lookup: $variable_name
            if py_json_value.startswith("$"):
                var_name = py_json_value[1:]
                # Assume context is flat for now. Nested contexts need more complex lookup.
                # py_current_context is a CJSONObject.
                context_val = py_current_context.get(var_name)
                if context_val is not None:
                    # Recursively format the value found in context
                    return self.format_value(context_val, expected_c_type, context_entity_info, py_current_context)
                else:
                    logger.warning(f"Context variable '{var_name}' not found. Rendering as string literal.")
                    return f"\"UNRESOLVED_CONTEXT_VAR_{var_name}\"" # Or some error indicator

            # 2. ID reference lookup: @id_name
            if py_json_value.startswith("@"):
                entity_id = py_json_value # ID includes '@'
                ref_info = self.get_entity_reference_for_id(entity_id)
                if ref_info and 'c_var_name' in ref_info:
                    # Check if the expected type is compatible (e.g., lv_obj_t* vs specific widget type)
                    # This is a simplified check.
                    if "style" in expected_c_type.lower() and "style" in ref_info['c_type_str'].lower() and not ref_info['is_ptr']:
                         return f"&{ref_info['c_var_name']}" # Return address of style
                    return ref_info['c_var_name'] # Return C variable name
                else:
                    # Could be a runtime registered pointer if interacting with dynamic parts
                    # For pure static, this is an error or unfulfilled forward reference.
                    logger.warning(f"Referenced ID '{entity_id}' not found in local registry. Assuming runtime lookup.")
                    return f"(({expected_c_type if expected_c_type != 'unknown' else 'void*'})lvgl_json_get_registered_ptr(\"{entity_id}\", \"{expected_c_type.replace('*','') if expected_c_type != 'unknown' else ''}\"))"


            # 3. LVGL Constant/Enum lookup (e.g., "ALIGN.CENTER", "COLOR.RED")
            if py_json_value in LVGL_CONSTANTS_MAP:
                return LVGL_CONSTANTS_MAP[py_json_value]
            
            # Check against api_info for enums (e.g. those not in LVGL_CONSTANTS_MAP)
            # This requires api_info to be structured to allow easy enum lookup.
            # Example: self.api_info['enums']['lv_align_t']['LV_ALIGN_CENTER'] = 0
            # Or if py_json_value is "LV_ALIGN_CENTER" directly
            if py_json_value.isupper() and (py_json_value.startswith("LV_") or py_json_value.startswith("SL_")): # Heuristic for LVGL define/enum
                # Check if it's a known enum string in api_info (this part is complex)
                # For now, assume if it looks like an LVGL define, pass it through.
                return py_json_value 

            # 4. Color hex code: "#RRGGBB" or "#AARRGGBB"
            if py_json_value.startswith("#"):
                hex_color = py_json_value[1:]
                alpha = "FF"
                if len(hex_color) == 8: # AARRGGBB
                    alpha = hex_color[:2]
                    hex_color = hex_color[2:]
                elif len(hex_color) == 3: # RGB
                    hex_color = "".join([c*2 for c in hex_color])
                elif len(hex_color) != 6: # RRGGBB
                    logger.warning(f"Invalid color hex format: {py_json_value}. Using black.")
                    return "lv_color_hex(0x000000)"

                if expected_c_type == "lv_color_t":
                    if alpha != "FF": # LVGL uses lv_color_hexXX and then lv_obj_set_opa for alpha
                         # This should ideally translate to lv_color_hex and a separate opacity setting.
                         # For now, just use color part. Opacity might be set by separate "opa" property.
                        logger.info(f"Color {py_json_value} has alpha, but lv_color_hex doesn't directly use it. Set 'opa' separately.")
                    return f"lv_color_hex(0x{hex_color.upper()})"
                else: # If not explicitly lv_color_t, could be uint32_t for image colorization etc.
                    return f"0x{alpha.upper()}{hex_color.upper()}"


            # 5. Percentage: "100%" -> LV_PCT(100) (only for lv_coord_t currently)
            if py_json_value.endswith("%") and expected_c_type == "lv_coord_t":
                try:
                    val = int(py_json_value[:-1])
                    return f"LV_PCT({val})"
                except ValueError:
                    logger.warning(f"Invalid percentage: {py_json_value}. Using 0.")
                    return "0"
            
            # Default: treat as a C string literal
            # First, escape backslashes, then escape double quotes
            c_escaped_value = py_json_value.replace('\\', '\\\\').replace('"', '\\"')
            return f'"{c_escaped_value}"' # Use single quotes for f-string robustness

        elif isinstance(py_json_value, bool):
            return "true" if py_json_value else "false"

        elif isinstance(py_json_value, (int, float)):
            if expected_c_type.lower() == "char*" or expected_c_type.lower() == "const char*":
                return f"\"{str(py_json_value)}\"" # If number needs to be string arg
            return str(py_json_value)

        elif isinstance(py_json_value, list):
            # Used for array parameters, e.g. points for lv_line_set_points
            # Or for grid descriptors if not handled by handle_grid_layout directly creating static const.
            # This implies the C function takes a pointer to an array.
            # The array needs to be declared and initialized in C.
            self.grid_array_counter +=1
            arr_c_name = f"static_array_{self.grid_array_counter}"
            
            # Determine element type based on expected_c_type (e.g. "lv_coord_t[]" -> "lv_coord_t")
            # This is a simplification. Real mapping would be more robust.
            element_type = expected_c_type.replace("[]","").replace("const ","").strip()
            if not element_type: element_type = "int" # fallback

            formatted_elements = [self.format_value(v, element_type, context_entity_info, py_current_context) for v in py_json_value]
            
            # For some types like grid descriptors, LV_GRID_TEMPLATE_LAST is needed.
            # This logic should ideally be in handle_grid_layout.
            # if "grid_dsc_array" in expected_c_type: # Heuristic
            #    if "LV_GRID_TEMPLATE_LAST" not in formatted_elements:
            #        formatted_elements.append("LV_GRID_TEMPLATE_LAST")

            array_literal = f"{{{', '.join(formatted_elements)}}}"
            
            # Declare as static const array at the "predecl" or "declarations" level
            # For simplicity, put in "predecl" to make it file-static (if function is called multiple times)
            # or "declarations" if it's unique per call.
            # Let's try "predecl" to ensure it's const and static.
            self._add_c_line("predecl", f"static const {element_type} {arr_c_name}[{len(formatted_elements)}] = {array_literal};", indent=False)
            return arr_c_name # Return the C variable name of the array

        elif isinstance(py_json_value, (dict, CJSONObject)):
            # Could be a nested function call like {"call": "my_helper_func", "args": [...]}
            # Or a structure that needs to be initialized (more complex, not handled here yet)
            if "call" in py_json_value:
                func_to_call = py_json_value["call"]
                raw_args = py_json_value.get("args", [])
                # Need to guess expected C types for these args or get from api_info for func_to_call
                formatted_c_args = [self.format_value(arg, "unknown", context_entity_info, py_current_context) for arg in raw_args]
                return f"{func_to_call}({', '.join(formatted_c_args)})"
            
            logger.warning(f"Cannot format complex dictionary/object to C value directly: {py_json_value}")
            return "{0}" # Placeholder for empty struct or error

        return str(py_json_value) # Fallback for unknown types


    def handle_children_attribute(self, py_children_data: List[CJSONObject], 
                                  parent_entity_info: Dict[str, Any], 
                                  parent_named_path_prefix: Optional[str], 
                                  py_current_context: CJSONObject) -> None:
        # The BaseUIGenerator.process_node iterates children and calls self.process_node for each.
        # This method in StaticCTranspiler itself doesn't need to do much beyond perhaps adding a comment.
        # parent_lvgl_c_var for children will be parent_entity_info['c_var_name'].
        self._add_c_line("implementation", f"// Processing children of {parent_entity_info.get('c_var_name', 'unknown_parent')}", indent=True)
        # Actual child processing is driven by BaseUIGenerator calling process_node for each child.

    def handle_named_attribute(self, py_named_value: str, 
                               target_entity_info: Dict[str, Any], 
                               path_prefix: Optional[str], 
                               type_for_registry_str: str # e.g. "lv_button_t" from api_info
                              ) -> Optional[str]:
        # `type_for_registry_str` would be the C type string like "lv_button_t" (no pointer).
        # This is useful if runtime registration is mixed with transpiled code.
        full_path = f"{path_prefix}.{py_named_value}" if path_prefix else py_named_value
        
        # Optional: Generate runtime registration if this feature is enabled/needed
        # self._add_c_line("implementation", 
        #    f"// lvgl_json_register_ptr(\"{full_path}\", \"{type_for_registry_str}\", (void*){target_entity_info['c_var_name']});")
        
        # The primary role here is to update the path prefix for subsequent children/named items if they are nested
        # under this named entity. However, BaseUIGenerator usually handles the path construction.
        # This method can confirm the new effective path.
        return full_path


    def handle_with_attribute(self, py_with_node_data: CJSONObject, # This is the value of "with": {"obj": "@id", "do": {...}}
                              current_entity_info: Dict[str, Any], # Entity this "with" attribute is part of
                              path_prefix: Optional[str], 
                              py_current_context: CJSONObject) -> None:
        # current_entity_info is the object that has the "with" attribute.
        # The "do" block inside py_with_node_data will be applied to a *target* object.
        # The target object is specified by py_with_node_data.get("obj").
        
        target_spec = py_with_node_data.get("obj")
        if not target_spec:
            logger.error("'with' attribute is missing 'obj' field to specify target.")
            self._add_c_line("implementation", "// Error: 'with' attribute missing 'obj' field.")
            return

        # Format the target_spec to get the C variable name of the target.
        # Expected C type for target_spec would typically be an object pointer like "lv_obj_t*".
        # context_entity_info for format_value would be current_entity_info.
        target_c_var_expr = self.format_value(target_spec, "lv_obj_t*", current_entity_info, py_current_context)

        # The BaseUIGenerator's apply_attributes method will take care of processing the "do" block.
        # It needs to be told that the target for the "do" block is now `target_c_var_expr`.
        # This is a bit tricky with the current BaseUIGenerator flow.
        # One way: store this resolved target_c_var_expr temporarily in self, for apply_attributes to pick up.
        # Or, modify apply_attributes to handle a "resolved_target_override" parameter.
        
        # For now, let's assume the "do" block processing in BaseUIGenerator
        # will correctly use `target_c_var_expr` if `current_entity_info` is updated or
        # if `apply_attributes` is called with a new target.
        
        # This method's main job here is to prepare for the "do" block application.
        # We can add a comment or an if-guard.
        self._add_c_line("implementation", f"// Applying 'with' block to target: {target_c_var_expr}")
        
        # If target_c_var_expr could be NULL at runtime (e.g., from lvgl_json_get_registered_ptr)
        if "lvgl_json_get_registered_ptr" in target_c_var_expr:
            self._add_c_line("implementation", f"if ({target_c_var_expr}) {{")
            self.current_c_indent_level +=1
            # The BaseUIGenerator.apply_attributes for the "do" block should happen within this if.
            # This requires BaseUIGenerator to be aware of such conditional blocks.
            # This part of the flow needs careful review with BaseUIGenerator's apply_attributes.
            # For now, the "do" block will be processed by BaseUIGenerator after this returns.
            # The solution is that BaseUIGenerator.process_node for "with" type (not attribute)
            # should call apply_attributes with the resolved target.
            # If "with" is an *attribute*, BaseUIGenerator.apply_attributes needs to handle it.
            # It will call this `handle_with_attribute`, then it needs to get the resolved target
            # to apply the "do" block.
            
            # Let's assume BaseUIGenerator's apply_attributes will handle the "do" part by calling
            # itself recursively with the new target information derived from target_c_var_expr.
            # This function doesn't directly process "do"; it sets up for it.
            # The current_entity_info for the "do" block should effectively become the resolved target.
            do_block = py_with_node_data.get("do")
            if do_block and isinstance(do_block, (dict,CJSONObject)):
                # We need type info for target_c_var_expr. Try to look it up if it's a known var.
                resolved_target_info = self.get_type_information(target_c_var_expr)
                if not resolved_target_info: # If format_value returned a function call or complex expression
                    resolved_target_info = {'c_var_name': target_c_var_expr, 'c_type_str': 'lv_obj_t*', 'is_ptr': True, 'lvgl_setter_type': 'obj', 'original_json_type': 'obj'} # Fallback
                
                # Temporarily modify indent for the "do" block if inside an if
                original_indent = self.current_c_indent_level
                if "lvgl_json_get_registered_ptr" in target_c_var_expr:
                    pass # Indent already increased for the if block
                else: # No if block, apply directly
                    self.current_c_indent_level = original_indent 

                # Call the generic apply_attributes from base class for the "do" block
                # This assumes BaseUIGenerator.apply_attributes can be called like this
                super().apply_attributes(
                    attributes_data=do_block,
                    target_entity_ref=resolved_target_info, # Python dict with C var name and type
                    target_type_info=resolved_target_info, # same as above
                    parent_for_children_ref=resolved_target_info, # Children go under the 'with' target
                    path_prefix_for_named_and_children=path_prefix, # Path context remains same
                    default_type_for_registry_if_named=resolved_target_info.get('lvgl_setter_type','obj'),
                    current_context_data=py_current_context
                )
                self.current_c_indent_level = original_indent # Restore indent

            if "lvgl_json_get_registered_ptr" in target_c_var_expr:
                self.current_c_indent_level -=1
                self._add_c_line("implementation", f"}} // End if ({target_c_var_expr}) for 'with'")


    def handle_grid_layout(self, target_entity_info: Dict[str, Any], 
                           py_cols_data: Optional[List[Any]], py_rows_data: Optional[List[Any]], 
                           py_current_context: CJSONObject) -> None:
        
        target_c_var = target_entity_info['c_var_name']
        col_arr_c_name = "NULL"
        row_arr_c_name = "NULL"

        if py_cols_data:
            self.grid_array_counter += 1
            arr_var_name = f"grid_cols_{self.grid_array_counter}"
            # Add LV_GRID_TEMPLATE_LAST
            actual_col_data = list(py_cols_data) + ["LV_GRID_TEMPLATE_LAST"]
            # Declare static const lv_coord_t col_arr_X[] = {....};
            # format_value for a list will create this static array and return its name
            col_arr_c_name = self.format_value(actual_col_data, "lv_coord_t[]", target_entity_info, py_current_context)

        if py_rows_data:
            self.grid_array_counter += 1
            arr_var_name = f"grid_rows_{self.grid_array_counter}"
            actual_row_data = list(py_rows_data) + ["LV_GRID_TEMPLATE_LAST"]
            row_arr_c_name = self.format_value(actual_row_data, "lv_coord_t[]", target_entity_info, py_current_context)
            
        self._add_c_line("implementation", f"lv_obj_set_grid_dsc_array({target_c_var}, {col_arr_c_name}, {row_arr_c_name});")

    def register_component_definition(self, component_id: str, py_comp_root_data: CJSONObject) -> None:
        # Store component definition in Python dict for transpile-time expansion by process_use_view_node
        self.component_definitions[component_id] = py_comp_root_data
        # No C code is generated here; components are expanded directly into the C code.

    def get_component_definition(self, component_id: str) -> Optional[CJSONObject]:
        return self.component_definitions.get(component_id)

    def get_entity_reference_for_id(self, id_str: str) -> Optional[Dict[str, Any]]:
        # id_str includes '@' prefix if it was from JSON like "id": "@my_button"
        return self.local_registered_entities.get(id_str)

    def get_type_information(self, entity_info_or_var_name: Union[str, Dict[str, Any]]) -> Optional[Dict[str, Any]]:
        if isinstance(entity_info_or_var_name, str): # It's a C variable name
            # Check if this var_name is a key in local_registered_entities (used after ID registration)
            if entity_info_or_var_name in self.local_registered_entities:
                 return self.local_registered_entities[entity_info_or_var_name]
            
            # Fallback: try to infer from var_name or assume generic lv_obj_t*
            # This part is tricky; create_entity should register all known entities.
            # If we are here for a var_name not in map, it's likely an issue or an external var.
            logger.warning(f"Type info requested for unknown C var '{entity_info_or_var_name}'. Assuming generic lv_obj_t*.")
            return {
                'c_var_name': entity_info_or_var_name,
                'c_type_str': "lv_obj_t*",
                'is_ptr': True,
                'lvgl_setter_type': "obj", # Best guess
                'original_json_type': "obj" # Best guess
            }
        elif isinstance(entity_info_or_var_name, dict) and 'c_var_name' in entity_info_or_var_name:
            return entity_info_or_var_name # It's already an entity_info dict
        return None
        
    def get_node_id(self, node_data: CJSONObject) -> Optional[str]:
        return node_data.get("id") if isinstance(node_data, (dict, CJSONObject)) else None

    def get_node_type(self, node_data: CJSONObject) -> str:
        return node_data.get("type", "obj") if isinstance(node_data, (dict, CJSONObject)) else "obj"
        
    def get_node_context_override(self, node_data: CJSONObject) -> Optional[Any]:
        return node_data.get("context") if isinstance(node_data, (dict, CJSONObject)) else None

    # --- Final Code Assembly Method ---
    def build_c_function_content(self, function_name: str = "create_ui_transpiled", parent_param_name: str = "screen_parent") -> Tuple[str, str]:
        # process_ui (called externally) populates self.c_code sections.
        # This method assembles them into a full C function string and a header content string.
        
        # Add parent object handling at the beginning of implementation
        impl_header = []
        impl_header.append(self._c_indent() + f"lv_obj_t *parent_obj = {parent_param_name} ? {parent_param_name} : lv_screen_active();")
        impl_header.append(self._c_indent() + "if (!parent_obj) {")
        self.current_c_indent_level +=1
        impl_header.append(self._c_indent() + "// LV_LOG_ERROR(\"Parent object is NULL in %s!\", __func__); // Or your logging mechanism")
        impl_header.append(self._c_indent() + "return;")
        self.current_c_indent_level -=1
        impl_header.append(self._c_indent() + "}")
        impl_header.append("") # Newline

        # Styles are defined first (static within function or global)
        # Declarations are for variables local to the function scope
        # Predeclarations are for #includes and externs, file scope static const arrays
        
        c_file_content = []
        c_file_content.extend(self.c_code["predecl"])
        c_file_content.append("")
        c_file_content.append(f"void {function_name}(lv_obj_t *{parent_param_name}) {{")
        
        if self.c_code["style_definitions"]:
            c_file_content.append("    // Style Definitions")
            c_file_content.extend(sdef for sdef in self.c_code["style_definitions"]) # Already indented correctly if added with indent=False
            c_file_content.append("")
            
        if self.c_code["declarations"]:
            c_file_content.append("    // Variable Declarations")
            c_file_content.extend(decl for decl in self.c_code["declarations"]) # Already has semicolons, no extra indent
            c_file_content.append("")

        c_file_content.append("    // Implementation")
        c_file_content.extend(impl_header)
        c_file_content.extend(self.c_code["implementation"])
        c_file_content.append("}")
        
        h_file_content = []
        h_file_content.append(f"#ifndef _{function_name.upper()}_H_")
        h_file_content.append(f"#define _{function_name.upper()}_H_")
        h_file_content.append("")
        h_file_content.append("#include \"lvgl.h\" // Or your main project header")
        h_file_content.append("")
        h_file_content.append("#ifdef __cplusplus")
        h_file_content.append("extern \"C\" {")
        h_file_content.append("#endif")
        h_file_content.append("")
        h_file_content.append(f"void {function_name}(lv_obj_t *{parent_param_name});")
        h_file_content.append("")
        h_file_content.append("#ifdef __cplusplus")
        h_file_content.append("} /*extern \"C\"*/")
        h_file_content.append("#endif")
        h_file_content.append("")
        h_file_content.append(f"#endif /* _{function_name.upper()}_H_ */")

        return "\n".join(c_file_content), "\n".join(h_file_content)
