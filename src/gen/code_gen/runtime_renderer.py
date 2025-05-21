import logging
from typing import Any, Dict, List, Optional, Tuple, Set

from ..ui_generator_base import BaseUIGenerator, CJSONObject

logger = logging.getLogger(__name__)

class DynamicRuntimeRenderer(BaseUIGenerator):
    def __init__(self, api_info: Dict[str, Any], ui_spec_data: Any, custom_creators_map: Dict[str, str]):
        super().__init__(api_info, ui_spec_data) # api_info and ui_spec_data (CJSONObject) stored in base
        self.custom_creators_map: Dict[str, str] = custom_creators_map
        
        self.c_code: Dict[str, List[str]] = {
            "forward_declarations": [],
            "global_definitions": [], # For invoke_table_entry_t, runtime C helpers
            "render_node_runtime_func_body": [],
            "apply_attrs_runtime_func_body": [],
            "main_render_func_body": [],
            "component_definitions_c_code": [], # For C-side component registration
        }
        
        self.entity_info_map: Dict[str, Dict[str, Any]] = {} # Maps C var name of LVGL entity to its type info
        self.var_counter: int = 0
        self.current_c_indent_level: int = 0 # For indenting generated C code lines

        self._add_initial_c_code()

    def _c_indent(self) -> str:
        return "    " * self.current_c_indent_level

    def _add_c_line(self, section: str, line: str, indent: bool = True):
        self.c_code[section].append((self._c_indent() if indent else "") + line)

    def _new_c_var(self, base_name: str = "temp_var") -> str:
        self.var_counter += 1
        return f"{base_name}_{self.var_counter}"

    def _add_initial_c_code(self):
        # Forward Declarations
        self._add_c_line("forward_declarations", "#include <stdio.h>", indent=False)
        self._add_c_line("forward_declarations", "#include <string.h>", indent=False) # For strcmp, strdup etc.
        self._add_c_line("forward_declarations", "#include \"cJSON.h\"", indent=False)
        self._add_c_line("forward_declarations", "#include \"lvgl.h\"", indent=False)
        self._add_c_line("forward_declarations", "#include \"lvgl_json_utils.h\"", indent=False) # Runtime helpers
        self._add_c_line("forward_declarations", "", indent=False)
        self._add_c_line("forward_declarations", "static void* render_json_node_runtime(cJSON *node_json, lv_obj_t *parent_obj, const char *current_path_prefix, cJSON *context_obj);", indent=False)
        self._add_c_line("forward_declarations", "static bool apply_setters_and_attributes_runtime(cJSON *attrs_json, void *target_obj, const char* target_type_str, const char* target_actual_type_str, bool is_widget, const char *path_prefix, cJSON* context_obj);", indent=False)
        self._add_c_line("forward_declarations", "", indent=False)

        # Global Definitions (Runtime C Helpers)
        # invoke_table_entry_t struct
        self._add_c_line("global_definitions", "typedef struct {", indent=False)
        self._add_c_line("global_definitions", "    const char *name;", indent=False)
        self._add_c_line("global_definitions", "    void (*func_ptr)(); // Generic function pointer", indent=False)
        self._add_c_line("global_definitions", "    // Additional metadata, e.g., for argument types (simplified here)", indent=False)
        self._add_c_line("global_definitions", "} invoke_table_entry_t;", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        # find_invoke_entry
        self._add_c_line("global_definitions", "// Example: A STUB invoke table. Real one would be populated by build system or registration.", indent=False)
        self._add_c_line("global_definitions", "static const invoke_table_entry_t global_invoke_table[] = {", indent=False)
        self._add_c_line("global_definitions", "    // LVGL constructors (simplified names, real might be namespaced or mangled)", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_obj_create\", (void (*)())lv_obj_create},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_button_create\", (void (*)())lv_button_create},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_label_create\", (void (*)())lv_label_create},", indent=False)
        self._add_c_line("global_definitions", "    // LVGL setters (simplified)", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_obj_set_width\", (void (*)())lv_obj_set_width},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_obj_set_height\", (void (*)())lv_obj_set_height},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_obj_set_align\", (void (*)())lv_obj_set_align},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_label_set_text\", (void (*)())lv_label_set_text},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_obj_set_parent\", (void (*)())lv_obj_set_parent},", indent=False)
        self._add_c_line("global_definitions", "    {\"lv_obj_set_grid_dsc_array\", (void(*)())lv_obj_set_grid_dsc_array},", indent=False)
        self._add_c_line("global_definitions", "    // ... more functions", indent=False)
        self._add_c_line("global_definitions", "    {NULL, NULL} // Terminator", indent=False)
        self._add_c_line("global_definitions", "};", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        self._add_c_line("global_definitions", "const invoke_table_entry_t* find_invoke_entry(const char *name) {", indent=False)
        self._add_c_line("global_definitions", "    for (int i = 0; global_invoke_table[i].name != NULL; ++i) {", indent=False)
        self._add_c_line("global_definitions", "        if (strcmp(global_invoke_table[i].name, name) == 0) {", indent=False)
        self._add_c_line("global_definitions", "            return &global_invoke_table[i];", indent=False)
        self._add_c_line("global_definitions", "        }", indent=False)
        self._add_c_line("global_definitions", "    }", indent=False)
        self._add_c_line("global_definitions", "    fprintf(stderr, \"Warning: Invoke entry '%s' not found.\\n\", name);", indent=False)
        self._add_c_line("global_definitions", "    return NULL;", indent=False)
        self._add_c_line("global_definitions", "}", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        # unmarshal_value (basic stub, real one is complex)
        self._add_c_line("global_definitions", "// Simplified unmarshal_value. Assumes lvgl_json_utils.h provides a more robust one.", indent=False)
        self._add_c_line("global_definitions", "bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent, cJSON* context_obj) {", indent=False)
        self._add_c_line("global_definitions", "    if (!json_value) return false;", indent=False)
        self._add_c_line("global_definitions", "    if (strcmp(expected_c_type, \"lv_coord_t\") == 0 && cJSON_IsNumber(json_value)) {", indent=False)
        self._add_c_line("global_definitions", "        *(lv_coord_t*)dest = (lv_coord_t)cJSON_GetNumberValue(json_value);", indent=False)
        self._add_c_line("global_definitions", "        return true;", indent=False)
        self._add_c_line("global_definitions", "    } else if (strcmp(expected_c_type, \"const char*\") == 0 && cJSON_IsString(json_value)) {", indent=False)
        self._add_c_line("global_definitions", "        *(const char**)dest = cJSON_GetStringValue(json_value); // Warning: lifetime issues if not handled carefully", indent=False)
        self._add_c_line("global_definitions", "        return true;", indent=False)
        self._add_c_line("global_definitions", "    } else if (strcmp(expected_c_type, \"lv_align_t\") == 0 && cJSON_IsNumber(json_value)) {", indent=False)
        self._add_c_line("global_definitions", "        *(lv_align_t*)dest = (lv_align_t)cJSON_GetNumberValue(json_value); // Assuming enums are numbers", indent=False)
        self._add_c_line("global_definitions", "        return true;", indent=False)
        self._add_c_line("global_definitions", "    }", indent=False)
        self._add_c_line("global_definitions", "    // TODO: Add more type handling, context lookups ($ context.var)", indent=False)
        self._add_c_line("global_definitions", "    fprintf(stderr, \"Warning: unmarshal_value unhandled type '%s' or invalid JSON type.\\n\", expected_c_type);", indent=False)
        self._add_c_line("global_definitions", "    return false;", indent=False)
        self._add_c_line("global_definitions", "}", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        # Registry functions (stubs)
        self._add_c_line("global_definitions", "#define MAX_REGISTERED_ENTITIES 100", indent=False)
        self._add_c_line("global_definitions", "typedef struct { char* name; char* type_name; void* ptr; } lvgl_json_entity_registry_entry_t;", indent=False)
        self._add_c_line("global_definitions", "static lvgl_json_entity_registry_entry_t entity_registry[MAX_REGISTERED_ENTITIES];", indent=False)
        self._add_c_line("global_definitions", "static int entity_registry_count = 0;", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        self._add_c_line("global_definitions", "void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr) {", indent=False)
        self._add_c_line("global_definitions", "    if (entity_registry_count >= MAX_REGISTERED_ENTITIES) { fprintf(stderr, \"Error: Entity registry full.\\n\"); return; }", indent=False)
        self._add_c_line("global_definitions", "    // Check for duplicates (optional, depends on desired behavior)", indent=False)
        self._add_c_line("global_definitions", "    entity_registry[entity_registry_count].name = strdup(name); // Remember to free this if entries can be removed", indent=False)
        self._add_c_line("global_definitions", "    entity_registry[entity_registry_count].type_name = strdup(type_name);", indent=False)
        self._add_c_line("global_definitions", "    entity_registry[entity_registry_count].ptr = ptr;", indent=False)
        self._add_c_line("global_definitions", "    entity_registry_count++;", indent=False)
        self._add_c_line("global_definitions", "}", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        self._add_c_line("global_definitions", "void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name) {", indent=False)
        self._add_c_line("global_definitions", "    for (int i = 0; i < entity_registry_count; ++i) {", indent=False)
        self._add_c_line("global_definitions", "        if (strcmp(entity_registry[i].name, name) == 0) {", indent=False)
        self._add_c_line("global_definitions", "            if (expected_type_name && strcmp(entity_registry[i].type_name, expected_type_name) != 0) {", indent=False)
        self._add_c_line("global_definitions", "                fprintf(stderr, \"Warning: Registered entity '%s' found, but type '%s' does not match expected '%s'.\\n\", name, entity_registry[i].type_name, expected_type_name);", indent=False)
        self._add_c_line("global_definitions", "                return NULL;", indent=False)
        self._add_c_line("global_definitions", "            }", indent=False)
        self._add_c_line("global_definitions", "            return entity_registry[i].ptr;", indent=False)
        self._add_c_line("global_definitions", "        }", indent=False)
        self._add_c_line("global_definitions", "    }", indent=False)
        self._add_c_line("global_definitions", "    fprintf(stderr, \"Warning: Registered entity '%s' not found.\\n\", name);", indent=False)
        self._add_c_line("global_definitions", "    return NULL;", indent=False)
        self._add_c_line("global_definitions", "}", indent=False)
        self._add_c_line("global_definitions", "", indent=False)

        # Context functions (stubs)
        self._add_c_line("global_definitions", "static cJSON* current_ui_context = NULL; // Simple global context", indent=False)
        self._add_c_line("global_definitions", "void set_current_context(cJSON* new_context) {", indent=False)
        self._add_c_line("global_definitions", "    // Note: This simple version doesn't handle context ownership or deep copies.", indent=False)
        self._add_c_line("global_definitions", "    // A real implementation might need to cJSON_Duplicate or manage a stack.", indent=False)
        self._add_c_line("global_definitions", "    current_ui_context = new_context;", indent=False)
        self._add_c_line("global_definitions", "}", indent=False)
        self._add_c_line("global_definitions", "", indent=False)
        self._add_c_line("global_definitions", "cJSON* get_current_context(void) {", indent=False)
        self._add_c_line("global_definitions", "    return current_ui_context;", indent=False)
        self._add_c_line("global_definitions", "}", indent=False)
        self._add_c_line("global_definitions", "", indent=False)


        # Main Render Function Shell
        self._add_c_line("main_render_func_body", "bool lvgl_json_render_ui(cJSON *root_json_c_var, lv_obj_t *implicit_root_parent_c_var, cJSON *initial_context) {", indent=False)
        self.current_c_indent_level += 1
        self._add_c_line("main_render_func_body", "if (!root_json_c_var) { fprintf(stderr, \"Error: Root JSON is NULL.\\n\"); return false; }")
        self._add_c_line("main_render_func_body", "set_current_context(initial_context);")
        self._add_c_line("main_render_func_body", "")
        self._add_c_line("main_render_func_body", "// If root_json_c_var is an array of nodes, iterate and render each.")
        self._add_c_line("main_render_func_body", "if (cJSON_IsArray(root_json_c_var)) {")
        self.current_c_indent_level += 1
        self._add_c_line("main_render_func_body", "cJSON* node_json = NULL;")
        self._add_c_line("main_render_func_body", "cJSON_ArrayForEach(node_json, root_json_c_var) {")
        self.current_c_indent_level += 1
        self._add_c_line("main_render_func_body", "render_json_node_runtime(node_json, implicit_root_parent_c_var, \"\", get_current_context());")
        self.current_c_indent_level -= 1
        self._add_c_line("main_render_func_body", "}")
        self.current_c_indent_level -= 1
        self._add_c_line("main_render_func_body", "} else if (cJSON_IsObject(root_json_c_var)) {")
        self.current_c_indent_level += 1
        self._add_c_line("main_render_func_body", "render_json_node_runtime(root_json_c_var, implicit_root_parent_c_var, \"\", get_current_context());")
        self.current_c_indent_level -= 1
        self._add_c_line("main_render_func_body", "} else {")
        self.current_c_indent_level += 1
        self._add_c_line("main_render_func_body", "fprintf(stderr, \"Error: Root JSON is not an object or array.\\n\");")
        self._add_c_line("main_render_func_body", "return false;")
        self.current_c_indent_level -= 1
        self._add_c_line("main_render_func_body", "}")
        self._add_c_line("main_render_func_body", "return true;")
        self.current_c_indent_level -= 1
        self._add_c_line("main_render_func_body", "}", indent=False)


    # --- Abstract Method Implementations (Generate C code strings) ---

    def create_entity(self, py_node_data: CJSONObject, type_str: str, parent_lvgl_c_var: str, named_path_prefix: Optional[str], cjson_node_var: Optional[str] = None) -> Dict[str, Any]:
        # py_node_data is called node_data in BaseUIGenerator
        # parent_lvgl_c_var is parent_ref in BaseUIGenerator
        # This method's signature is adjusted to match BaseUIGenerator for the first 4 params,
        # and cjson_node_var is added as an optional param for DynamicRuntimeRenderer's specific needs.
        # Ideally, DynamicRuntimeRenderer would override process_node to supply cjson_node_var.
        if cjson_node_var is None:
            logger.warning(f"DynamicRuntimeRenderer.create_entity called without cjson_node_var for type '{type_str}'. C code generation might be incomplete.")
            # Create a dummy cjson_node_var for C code generation to proceed, though it will be incorrect.
            cjson_node_var = "cjson_node_var_missing" # This will likely cause C errors if used.

        # Note: cjson_node_var is the C variable name for the cJSON object representing the node
        # parent_lvgl_c_var is the C variable name for the parent lv_obj_t*
        # This method generates C code that will be part of the 'render_json_node_runtime' C function.

        entity_c_name = self._new_c_var(type_str if type_str != "with" else "obj") # Avoid "with_1"
        c_type_for_registry = "lv_obj_t" # Default, can be more specific
        actual_type_str = type_str 
        is_widget = True # Most created entities are widgets

        if type_str == "with":
            # "with" doesn't create a new LVGL entity itself.
            # It targets an existing entity. The target is resolved in handle_with_attribute.
            # For now, return info about the parent, or a placeholder.
            # The apply_attributes for "with" will handle the logic.
            self._add_c_line("render_node_runtime_func_body", f"// 'with' block: {cjson_node_var} - target will be resolved in apply_attributes")
            # We need to return some valid info. Let's assume it might operate on parent_lvgl_c_var if not specified.
            # This is a bit of a placeholder; handle_with_attribute will do the heavy lifting.
            parent_info = self.entity_info_map.get(parent_lvgl_c_var)
            if parent_info:
                return parent_info.copy() # Return a copy to avoid accidental modification
            else: # Should not happen if parent_lvgl_c_var is valid
                 return {'c_var_name': parent_lvgl_c_var, 'actual_type_str': "lv_obj_t", 'create_type_str': "lv_obj_t", 'is_widget': True, 'c_type_for_registry': "lv_obj_t"}


        # Custom creators
        if type_str in self.custom_creators_map:
            custom_func_name = self.custom_creators_map[type_str]
            self._add_c_line("render_node_runtime_func_body", f"// Custom creator for type '{type_str}' using '{custom_func_name}'")
            self._add_c_line("render_node_runtime_func_body", f"void* {entity_c_name} = NULL;")
            self._add_c_line("render_node_runtime_func_body", f"const invoke_table_entry_t* custom_create_entry = find_invoke_entry(\"{custom_func_name}\");")
            self._add_c_line("render_node_runtime_func_body", f"if (custom_create_entry) {{")
            self.current_c_indent_level += 1
            # Custom creators might take the cJSON node itself, or parent, or both.
            # Assuming a signature: void* custom_creator(lv_obj_t* parent, cJSON* node_json)
            # Or if it needs to return via out-param: void custom_creator(lv_obj_t* parent, cJSON* node_json, void** out_obj)
            # For simplicity, let's assume: void custom_creator(lv_obj_t* parent, void** out_obj); User adds details via 'do'.
            self._add_c_line("render_node_runtime_func_body", f"((void (*)(lv_obj_t*, void**))custom_create_entry->func_ptr)({parent_lvgl_c_var}, &{entity_c_name});")
            self.current_c_indent_level -= 1
            self._add_c_line("render_node_runtime_func_body", f"}} else {{")
            self.current_c_indent_level += 1
            self._add_c_line("render_node_runtime_func_body", f"fprintf(stderr, \"Error: Custom creator '{custom_func_name}' not found for type '{type_str}'.\\n\");")
            self.current_c_indent_level -= 1
            self._add_c_line("render_node_runtime_func_body", f"}}")
            # What is actual_type_str and c_type_for_registry for custom? Assume obj for now.
            actual_type_str = "custom_obj" # Or get from api_info if available
            c_type_for_registry = "lv_obj_t*" # Or a more specific type if known
        
        elif type_str == "grid": # Special case: grid itself is just an lv_obj
            actual_type_str = "obj" # LVGL grids are applied to lv_obj
            c_type_for_registry = "lv_obj_t*"
            self._add_c_line("render_node_runtime_func_body", f"lv_obj_t* {entity_c_name} = NULL;")
            self._add_c_line("render_node_runtime_func_body", f"const invoke_table_entry_t* create_entry_obj = find_invoke_entry(\"lv_obj_create\");")
            self._add_c_line("render_node_runtime_func_body", f"if (create_entry_obj) {{")
            self.current_c_indent_level += 1
            self._add_c_line("render_node_runtime_func_body", f"((lv_obj_t* (*)(lv_obj_t*))create_entry_obj->func_ptr)({parent_lvgl_c_var});") # Direct call for lv_obj_create
            self._add_c_line("render_node_runtime_func_body", f"{entity_c_name} = lv_obj_create({parent_lvgl_c_var});") # Assign to var
            self.current_c_indent_level -=1
            self._add_c_line("render_node_runtime_func_body", f"}} else {{")
            self.current_c_indent_level += 1
            self._add_c_line("render_node_runtime_func_body", f"fprintf(stderr, \"Error: Creator 'lv_obj_create' not found for grid placeholder.\\n\");")
            self.current_c_indent_level -=1
            self._add_c_line("render_node_runtime_func_body", f"}}")

        else: # Standard LVGL widgets
            # type_str is like "button", "label" etc.
            # We need to find "lv_<type_str>_create" in invoke table.
            lvgl_create_func_name = f"lv_{type_str}_create"
            c_type_for_registry = f"lv_{type_str}_t*" # e.g. lv_button_t*
            
            self._add_c_line("render_node_runtime_func_body", f"// Standard widget creation for type '{type_str}'")
            # Determine C type, e.g. lv_obj_t*, lv_label_t* etc.
            # For simplicity, most create functions return lv_obj_t* which can be cast if needed.
            # Or, if api_info has return types for create functions, use that.
            # For now, assume all create functions return lv_obj_t* assignable to lv_obj_t* var.
            self._add_c_line("render_node_runtime_func_body", f"lv_obj_t* {entity_c_name} = NULL;")
            self._add_c_line("render_node_runtime_func_body", f"const invoke_table_entry_t* create_entry_{type_str} = find_invoke_entry(\"{lvgl_create_func_name}\");")
            self._add_c_line("render_node_runtime_func_body", f"if (create_entry_{type_str}) {{")
            self.current_c_indent_level += 1
            # Assumes: lv_obj_t* lv_widget_create(lv_obj_t* parent);
            self._add_c_line("render_node_runtime_func_body", f"{entity_c_name} = ((lv_obj_t* (*)(lv_obj_t*))create_entry_{type_str}->func_ptr)({parent_lvgl_c_var});")
            self.current_c_indent_level -= 1
            self._add_c_line("render_node_runtime_func_body", f"}} else {{")
            self.current_c_indent_level += 1
            self._add_c_line("render_node_runtime_func_body", f"fprintf(stderr, \"Error: Creator '{lvgl_create_func_name}' not found for type '{type_str}'.\\n\");")
            # Fallback to generic lv_obj_create if specific not found? Or just fail? For now, fail.
            self._add_c_line("render_node_runtime_func_body", f"{entity_c_name} = NULL; // Creation failed")
            self.current_c_indent_level -= 1
            self._add_c_line("render_node_runtime_func_body", f"}}")
        
        info = {
            'c_var_name': entity_c_name, 
            'actual_type_str': actual_type_str, # e.g. "button", "obj"
            'create_type_str': type_str, # Original type from JSON, e.g. "grid"
            'is_widget': is_widget, 
            'c_type_for_registry': c_type_for_registry # e.g. "lv_button_t*"
        }
        if entity_c_name and entity_c_name != parent_lvgl_c_var : # Don't map if it's a "with" just reusing parent
             self.entity_info_map[entity_c_name] = info
        return info

    def register_entity_internally(self, id_str: str, entity_info: Dict[str, Any], type_info_from_base: Optional[Any] = None, path_for_registration: Optional[str] = None) -> None:
        # BaseUIGenerator calls this with (id_str, entity_ref, type_info, path_for_registration)
        # For DynamicRuntimeRenderer, entity_info is entity_ref and type_info_from_base is type_info.
        # path_for_registration is the last argument.
        # If path_for_registration was passed in type_info_from_base's spot by mistake due to signature mismatch:
        if path_for_registration is None and isinstance(type_info_from_base, str):
            path_for_registration = type_info_from_base
            logger.warning("path_for_registration was likely passed in type_info_from_base's position. Adjusted.")

        if path_for_registration is None:
            logger.error(f"register_entity_internally called for {id_str} without a path_for_registration.")
            path_for_registration = f"error_missing_path_for_{id_str}"


        # This method generates C code that will be part of the 'render_json_node_runtime' C function,
        # typically called after an entity is created and its ID is known.
        entity_c_var = entity_info['c_var_name']
        c_type_str = entity_info['c_type_for_registry']
        self._add_c_line("render_node_runtime_func_body", f"// Registering entity '{id_str}' with path '{path_for_registration}'")
        self._add_c_line("render_node_runtime_func_body", f"if ({entity_c_var}) {{") # Only if creation succeeded
        self.current_c_indent_level += 1
        self._add_c_line("render_node_runtime_func_body", f"lvgl_json_register_ptr(\"{path_for_registration}\", \"{c_type_str}\", (void*){entity_c_var});")
        self.current_c_indent_level -= 1
        self._add_c_line("render_node_runtime_func_body", f"}}")

    def call_setter(self, target_entity_info: Dict[str, Any], setter_fn_name: str, 
                    py_value_args: List[Any], # Python representation of arguments
                    cjson_containing_obj_var: str, # cJSON* variable name for the object containing the prop that leads to this setter
                    prop_name_in_json: str, # The JSON key for this property
                    cjson_context_var: str) -> None:
        # This method generates C code for the 'apply_attrs_runtime_func_body' C function.
        # setter_fn_name is the C function name, e.g., "lv_obj_set_width".
        # py_value_args are Python values/objects. We need to generate C to get these from cjson_containing_obj_var.
        
        target_c_var = target_entity_info['c_var_name']
        
        self._add_c_line("apply_attrs_runtime_func_body", f"// Calling setter '{setter_fn_name}' for property '{prop_name_in_json}' on {target_c_var}")
        self._add_c_line("apply_attrs_runtime_func_body", f"const invoke_table_entry_t* setter_entry_{prop_name_in_json} = find_invoke_entry(\"{setter_fn_name}\");")
        self._add_c_line("apply_attrs_runtime_func_body", f"if (setter_entry_{prop_name_in_json}) {{")
        self.current_c_indent_level += 1

        # How we get arguments depends on the setter.
        # For simple setters like lv_obj_set_width(obj, value), py_value_args will have one item.
        # The C code needs to get a cJSON for that value from cjson_containing_obj_var using prop_name_in_json.
        cjson_prop_val_var = self._new_c_var(f"cjson_{prop_name_in_json}")
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* {cjson_prop_val_var} = cJSON_GetObjectItemCaseSensitive({cjson_containing_obj_var}, \"{prop_name_in_json}\");")
        
        # This is a MAJOR simplification. Real invoke needs proper arg marshalling.
        # Assuming simple 1-arg or 2-arg setters for now.
        # Example: lv_obj_set_width(obj, width_val)
        # Example: lv_label_set_text(obj, "text_val")
        # Example: lv_obj_set_align(obj, LV_ALIGN_CENTER) (enum might be passed as number or string to be resolved)
        
        # Lookup setter signature from api_info (Simplified: assume first arg is target_obj)
        # Let's assume api_info[setter_fn_name]['args'] gives list of C types like ["lv_obj_t*", "lv_coord_t"]
        arg_types = self.api_info.get(setter_fn_name, {}).get("args", [])
        num_expected_args = len(arg_types) -1 if arg_types else len(py_value_args) # -1 for target_obj itself

        if num_expected_args == 1:
            # Common case: set_property(obj, value)
            c_arg_type = arg_types[1] if len(arg_types) > 1 else "unknown_type" # e.g. "lv_coord_t"
            temp_val_var = self._new_c_var(f"val_{prop_name_in_json}")
            
            self._add_c_line("apply_attrs_runtime_func_body", f"{c_arg_type} {temp_val_var};")
            self._add_c_line("apply_attrs_runtime_func_body", f"if ({cjson_prop_val_var} && unmarshal_value({cjson_prop_val_var}, \"{c_arg_type}\", &{temp_val_var}, (void*){target_c_var}, {cjson_context_var})) {{")
            self.current_c_indent_level += 1
            # Assumes func_ptr is (void (*func)(target_type, arg1_type))
            self._add_c_line("apply_attrs_runtime_func_body", f"( (void (*)({arg_types[0] if arg_types else 'void*'}, {c_arg_type}) ) setter_entry_{prop_name_in_json}->func_ptr)({target_c_var}, {temp_val_var});")
            self.current_c_indent_level -= 1
            self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
            self.current_c_indent_level += 1
            self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Warning: Failed to unmarshal value for '{prop_name_in_json}' or prop not found.\\n\");")
            self.current_c_indent_level -= 1
            self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        elif num_expected_args == 0 : # E.g. lv_obj_clean(obj)
             self._add_c_line("apply_attrs_runtime_func_body", f"( (void (*)({arg_types[0] if arg_types else 'void*'})) setter_entry_{prop_name_in_json}->func_ptr)({target_c_var});")
        else:
            # More complex setters (e.g. multiple args from a list/object in JSON)
            # This would require iterating py_value_args, unmarshalling each, and then calling.
            # For now, log a warning that it's not fully implemented.
            self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Warning: Multi-arg setter '{setter_fn_name}' for '{prop_name_in_json}' not fully implemented in C codegen.\\n\");")
            # Example for a hypothetical 2-arg setter where JSON gives an array:
            # cJSON* arg1_json = cJSON_GetArrayItem(cjson_prop_val_var, 0);
            # cJSON* arg2_json = cJSON_GetArrayItem(cjson_prop_val_var, 1);
            # Type1 val1; unmarshal(arg1_json, ..., &val1, ...);
            # Type2 val2; unmarshal(arg2_json, ..., &val2, ...);
            # func_ptr(target_c_var, val1, val2);
            pass

        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Warning: Setter function '{setter_fn_name}' not found in invoke table.\\n\");")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")


    def format_value(self, py_json_value: Any, cjson_value_var: str, expected_c_type: str, 
                     context_entity_info: Optional[Dict[str, Any]], # Python info of entity this value is for
                     py_current_context: Any, # Python CJSONObject for current context scope
                     cjson_current_context_var: str) -> str: # C var name for cJSON* of current context
        # This method is expected to return a C expression string.
        # In many cases, the runtime `unmarshal_value` C function will handle the conversion.
        # So, this function might often just return the cjson_value_var (name of the C cJSON* variable).
        # However, if we can determine a C literal (e.g. an enum value like "LV_ALIGN_CENTER"),
        # we should return that C literal as a string.

        # Example: if py_json_value is "ALIGN.CENTER", and api_info maps this to "LV_ALIGN_CENTER"
        if isinstance(py_json_value, str):
            # Check if it's an enum or special constant from api_info (simplified lookup)
            # Example: self.api_info['enums']['lv_align_t']['CENTER'] == "LV_ALIGN_CENTER"
            # This is a placeholder for a more robust enum/constant resolution logic.
            if py_json_value.startswith("LV_"): # Simple heuristic for LVGL constants
                return f"\"{py_json_value}\"" # As a C string if it's a known constant name that unmarshal_value can map
                                            # Or directly: return py_json_value; if it's a valid C enum/define
                                            # This needs careful handling based on how unmarshal_value works with enums.

        # Default: return the C variable name of the cJSON object.
        # The C function `unmarshal_value` will be responsible for parsing it.
        # This implies that call_setter needs to generate code to call unmarshal_value.
        # The current `call_setter` does this.
        return cjson_value_var # This is not a C expression, but the name of the cJSON variable.
                               # The caller (like call_setter) will use this in unmarshal_value.
                               # This method seems misnamed or its role is different than typical "format_value"
                               # It's more like "get_c_json_value_source_for_unmarshalling"

    def handle_children_attribute(self, 
                                  py_children_data: List[CJSONObject], # children_data from Base
                                  parent_entity_info: Dict[str, Any],    # parent_entity_ref from Base
                                  parent_named_path_prefix: Optional[str], # parent_named_path_prefix from Base
                                  py_current_context: Any,               # current_context_data from Base
                                  cjson_children_array_var: Optional[str] = None, # Specific to DynamicRuntimeRenderer
                                  cjson_current_context_var: Optional[str] = None # Specific to DynamicRuntimeRenderer
                                 ) -> None:
        # BaseUIGenerator calls: handle_children_attribute(children_data, parent_entity_ref, parent_named_path_prefix, current_context_data)
        # So, py_children_data = children_data, parent_entity_info = parent_entity_ref, etc.
        # cjson_children_array_var and cjson_current_context_var must be optional or sourced internally.
        
        if cjson_children_array_var is None:
            logger.warning(f"handle_children_attribute called without cjson_children_array_var for parent {parent_entity_info.get('c_var_name')}. C-code will be incomplete.")
            cjson_children_array_var = "cjson_children_array_missing" # Placeholder
        if cjson_current_context_var is None:
            logger.warning(f"handle_children_attribute called without cjson_current_context_var for parent {parent_entity_info.get('c_var_name')}. C-code will be incomplete.")
            cjson_current_context_var = "cjson_context_var_missing" # Placeholder


        # Generates C code in 'apply_attrs_runtime_func_body' to iterate and render child nodes.
        parent_c_var = parent_entity_info['c_var_name']
        
        self._add_c_line("apply_attrs_runtime_func_body", f"// Handling 'children' for {parent_c_var} using cJSON array {cjson_children_array_var}")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({cjson_children_array_var} && cJSON_IsArray({cjson_children_array_var})) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* child_node_cjson = NULL;")
        self._add_c_line("apply_attrs_runtime_func_body", f"int child_idx = 0;")
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON_ArrayForEach(child_node_cjson, {cjson_children_array_var}) {{")
        self.current_c_indent_level += 1
        
        # Path prefix for children: parent_path.child_id or parent_path.child_index
        # For runtime, we might not have child_id easily available here without parsing child_node_cjson.
        # Simplification: use index for unnamed children in path, or rely on child's "id" or "named".
        # The `render_json_node_runtime` will handle the path for the child.
        # The path_prefix passed here is for the *parent* of these children.
        # `render_json_node_runtime` will append to it based on the child's own "id" or "named".
        
        # Construct new_path_prefix_str_literal for C code
        # This needs to be a C string literal. If parent_named_path_prefix is None, use empty string.
        c_path_prefix_literal = f'"{parent_named_path_prefix if parent_named_path_prefix else ""}"'
        
        self._add_c_line("apply_attrs_runtime_func_body", f"char child_path_buf[256];") # Buffer for child path
        self._add_c_line("apply_attrs_runtime_func_body", f"// Construct child path prefix. render_json_node_runtime will append child's ID if any.")
        self._add_c_line("apply_attrs_runtime_func_body", f"snprintf(child_path_buf, sizeof(child_path_buf), \"%s\", {c_path_prefix_literal});")


        self._add_c_line("apply_attrs_runtime_func_body", f"render_json_node_runtime(child_node_cjson, {parent_c_var}, child_path_buf, {cjson_current_context_var});")
        self._add_c_line("apply_attrs_runtime_func_body", f"child_idx++;")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else if ({cjson_children_array_var}) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Warning: 'children' attribute for {parent_c_var} is not a cJSON array.\\n\");")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")


    def handle_named_attribute(self, py_named_value: str, # The string value from "named": "my_name"
                               cjson_named_value_var: str, # C var for cJSON string: cJSON_GetObjectItem(attrs, "named")
                               target_entity_info: Dict[str, Any], 
                               path_prefix: Optional[str], # Current path prefix, e.g., "screen1.container2"
                               type_for_registry_str: str # Python string e.g. "lv_button_t*"
                              ) -> Optional[str]: # Returns the new path_prefix if changed
        # Generates C code in 'apply_attrs_runtime_func_body'.
        target_c_var = target_entity_info['c_var_name']
        c_type_for_reg = target_entity_info.get('c_type_for_registry', type_for_registry_str) # Use more specific if available

        self._add_c_line("apply_attrs_runtime_func_body", f"// Handling 'named' attribute for {target_c_var}")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({cjson_named_value_var} && cJSON_IsString({cjson_named_value_var})) {{")
        self.current_c_indent_level += 1
        
        self._add_c_line("apply_attrs_runtime_func_body", f"const char* named_value_str = cJSON_GetStringValue({cjson_named_value_var});")
        self._add_c_line("apply_attrs_runtime_func_body", f"if (named_value_str) {{")
        self.current_c_indent_level += 1
        
        # Construct full_path C string
        # path_prefix is Python string, convert to C literal for snprintf format
        c_path_prefix_literal = f'"{path_prefix if path_prefix else ""}"'
        
        self._add_c_line("apply_attrs_runtime_func_body", f"char full_path_for_named[256];")
        self._add_c_line("apply_attrs_runtime_func_body", f"if (strlen({c_path_prefix_literal}) > 0) {{")
        self.current_c_indent_level +=1
        self._add_c_line("apply_attrs_runtime_func_body", f"snprintf(full_path_for_named, sizeof(full_path_for_named), \"%s.%s\", {c_path_prefix_literal}, named_value_str);")
        self.current_c_indent_level -=1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
        self.current_c_indent_level +=1
        self._add_c_line("apply_attrs_runtime_func_body", f"snprintf(full_path_for_named, sizeof(full_path_for_named), \"%s\", named_value_str);")
        self.current_c_indent_level -=1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        
        self._add_c_line("apply_attrs_runtime_func_body", f"lvgl_json_register_ptr(full_path_for_named, \"{c_type_for_reg}\", (void*){target_c_var});")
        
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")

        # Return Python string for the new path. This is tricky because it's resolved at C runtime.
        # For the generator's tracking, we might form a best-guess path.
        # The actual C registration uses the runtime resolved name.
        new_path_at_gen_time = f"{path_prefix}.{py_named_value}" if path_prefix else py_named_value
        return new_path_at_gen_time


    def handle_with_attribute(self, py_with_node: CJSONObject, cjson_with_node_var: str, # cJSON* for the "with" block value
                              current_entity_info: Dict[str, Any], # Entity this "with" is attached to (often a dummy or parent)
                              path_prefix: Optional[str], 
                              py_current_context: Any, cjson_current_context_var: str) -> None:
        # Generates C code in 'apply_attrs_runtime_func_body'.
        # 'current_entity_info' might be the object the 'with' itself is operating on, if 'with' is a top-level type.
        # Or, if 'with' is an attribute, 'current_entity_info' is the object having this attribute.
        
        self._add_c_line("apply_attrs_runtime_func_body", f"// Handling 'with' attribute using cJSON data from {cjson_with_node_var}")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({cjson_with_node_var} && cJSON_IsObject({cjson_with_node_var})) {{")
        self.current_c_indent_level += 1

        obj_spec_cjson_var = self._new_c_var("with_obj_spec")
        do_block_cjson_var = self._new_c_var("with_do_block")
        target_obj_c_var = self._new_c_var("with_target_obj")

        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* {obj_spec_cjson_var} = cJSON_GetObjectItemCaseSensitive({cjson_with_node_var}, \"obj\");")
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* {do_block_cjson_var} = cJSON_GetObjectItemCaseSensitive({cjson_with_node_var}, \"do\");")
        
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({obj_spec_cjson_var} && {do_block_cjson_var}) {{")
        self.current_c_indent_level += 1
        
        self._add_c_line("apply_attrs_runtime_func_body", f"lv_obj_t* {target_obj_c_var} = NULL;")
        self._add_c_line("apply_attrs_runtime_func_body", f"// Unmarshal the target object for 'with'")
        self._add_c_line("apply_attrs_runtime_func_body", f"// This could be a string ID to lookup, or a direct pointer (not from JSON), or implicit (current_entity_info['c_var_name']).")
        self._add_c_line("apply_attrs_runtime_func_body", f"// Example: if obj_spec_cjson_var is a string ID:")
        self._add_c_line("apply_attrs_runtime_func_body", f"if (cJSON_IsString({obj_spec_cjson_var})) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"const char* target_id_str = cJSON_GetStringValue({obj_spec_cjson_var});")
        self._add_c_line("apply_attrs_runtime_func_body", f"{target_obj_c_var} = lvgl_json_get_registered_ptr(target_id_str, NULL); // NULL for any type, or specify expected")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"// If 'obj' is not a string, it's an error or needs different handling (e.g. implicit current object).")
        self._add_c_line("apply_attrs_runtime_func_body", f"// For now, assume current_entity_info if 'obj' is not a resolvable string.")
        self._add_c_line("apply_attrs_runtime_func_body", f"// This logic depends on how 'with' targets are specified. If 'with' is an attribute, current_entity_info is the target.")
        self._add_c_line("apply_attrs_runtime_func_body", f"{target_obj_c_var} = (lv_obj_t*){current_entity_info['c_var_name']}; // Fallback or primary target")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")

        self._add_c_line("apply_attrs_runtime_func_body", f"if ({target_obj_c_var}) {{")
        self.current_c_indent_level += 1
        # path_prefix for 'do' block should be the path of the target_obj_c_var if resolvable
        # For simplicity, use current path_prefix. This might need refinement.
        c_path_prefix_literal = f'"{path_prefix if path_prefix else ""}"'
        # The type strings here are placeholders; ideally, they'd come from target_obj_c_var's registration info.
        self._add_c_line("apply_attrs_runtime_func_body", f"apply_setters_and_attributes_runtime({do_block_cjson_var}, {target_obj_c_var}, \"lv_obj_t\", \"lv_obj_t\", true, {c_path_prefix_literal}, {cjson_current_context_var});")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Warning: Target object for 'with' block could not be resolved or is NULL.\\n\");")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Warning: 'with' block is missing 'obj' or 'do' specification.\\n\");")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")

        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")


    def handle_grid_layout(self, target_entity_info: Dict[str, Any], 
                           py_cols_data: Optional[List[Any]], py_rows_data: Optional[List[Any]], 
                           cjson_grid_node_var: str, # cJSON* for the node containing "cols"/"rows" (e.g., the grid widget node itself)
                           py_current_context: Any, cjson_current_context_var: str) -> None:
        # Generates C code in 'apply_attrs_runtime_func_body' (as grid is an attribute of an obj).
        target_c_var = target_entity_info['c_var_name']

        self._add_c_line("apply_attrs_runtime_func_body", f"// Handling 'grid' layout for {target_c_var}")

        cols_cjson_var = self._new_c_var("cols_json")
        rows_cjson_var = self._new_c_var("rows_json")

        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* {cols_cjson_var} = cJSON_GetObjectItemCaseSensitive({cjson_grid_node_var}, \"cols\");")
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* {rows_cjson_var} = cJSON_GetObjectItemCaseSensitive({cjson_grid_node_var}, \"rows\");")

        # Determine number of columns and rows at C runtime
        cols_count_var = self._new_c_var("cols_count")
        rows_count_var = self._new_c_var("rows_count")
        
        self._add_c_line("apply_attrs_runtime_func_body", f"int {cols_count_var} = {cols_cjson_var} ? cJSON_GetArraySize({cols_cjson_var}) : 0;")
        self._add_c_line("apply_attrs_runtime_func_body", f"int {rows_count_var} = {rows_cjson_var} ? cJSON_GetArraySize({rows_cjson_var}) : 0;")

        cols_c_array_var = self._new_c_var("cols_dsc_c_array")
        rows_c_array_var = self._new_c_var("rows_dsc_c_array")

        # Allocate and populate C arrays for column descriptors
        self._add_c_line("apply_attrs_runtime_func_body", f"lv_coord_t* {cols_c_array_var} = NULL;")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({cols_count_var} > 0) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"{cols_c_array_var} = (lv_coord_t*)malloc(sizeof(lv_coord_t) * ({cols_count_var} + 1)); // +1 for LV_GRID_TEMPLATE_LAST")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({cols_c_array_var}) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"for (int i = 0; i < {cols_count_var}; ++i) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* col_val_json = cJSON_GetArrayItem({cols_cjson_var}, i);")
        self._add_c_line("apply_attrs_runtime_func_body", f"unmarshal_value(col_val_json, \"lv_coord_t\", &{cols_c_array_var}[i], (void*){target_c_var}, {cjson_current_context_var});")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        self._add_c_line("apply_attrs_runtime_func_body", f"{cols_c_array_var}[{cols_count_var}] = LV_GRID_TEMPLATE_LAST;")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")

        # Allocate and populate C arrays for row descriptors
        self._add_c_line("apply_attrs_runtime_func_body", f"lv_coord_t* {rows_c_array_var} = NULL;")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({rows_count_var} > 0) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"{rows_c_array_var} = (lv_coord_t*)malloc(sizeof(lv_coord_t) * ({rows_count_var} + 1)); // +1 for LV_GRID_TEMPLATE_LAST")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({rows_c_array_var}) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"for (int i = 0; i < {rows_count_var}; ++i) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"cJSON* row_val_json = cJSON_GetArrayItem({rows_cjson_var}, i);")
        self._add_c_line("apply_attrs_runtime_func_body", f"unmarshal_value(row_val_json, \"lv_coord_t\", &{rows_c_array_var}[i], (void*){target_c_var}, {cjson_current_context_var});")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        self._add_c_line("apply_attrs_runtime_func_body", f"{rows_c_array_var}[{rows_count_var}] = LV_GRID_TEMPLATE_LAST;")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")

        # Call lv_obj_set_grid_dsc_array
        self._add_c_line("apply_attrs_runtime_func_body", f"const invoke_table_entry_t* grid_set_entry = find_invoke_entry(\"lv_obj_set_grid_dsc_array\");")
        self._add_c_line("apply_attrs_runtime_func_body", f"if (grid_set_entry) {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"((void (*)(lv_obj_t*, const lv_coord_t[], const lv_coord_t[]))grid_set_entry->func_ptr)({target_c_var}, {cols_c_array_var}, {rows_c_array_var});")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}} else {{")
        self.current_c_indent_level += 1
        self._add_c_line("apply_attrs_runtime_func_body", f"fprintf(stderr, \"Error: 'lv_obj_set_grid_dsc_array' not found in invoke table.\\n\");")
        self.current_c_indent_level -= 1
        self._add_c_line("apply_attrs_runtime_func_body", f"}}")
        
        # Free the allocated arrays (if not NULL)
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({cols_c_array_var}) free({cols_c_array_var});")
        self._add_c_line("apply_attrs_runtime_func_body", f"if ({rows_c_array_var}) free({rows_c_array_var});")


    def register_component_definition(self, component_id: str, py_comp_root: CJSONObject, cjson_comp_root_var: str) -> None:
        # Store Python CJSONObject for use by get_component_definition
        self.component_definitions[component_id] = py_comp_root 
        
        # Add C code to register the cJSON* variable at runtime.
        # This C code is added to a special section, e.g., "component_definitions_c_code",
        # which would be emitted early in the main C rendering function or as global initializers.
        # For now, let's assume it's called when component definition nodes are processed by process_node.
        # So, the C code generated here will be part of 'render_json_node_runtime' when it sees a 'component' type.
        
        self._add_c_line("render_node_runtime_func_body", f"// Registering component definition '{component_id}' with cJSON data from {cjson_comp_root_var}")
        # The type "component_json_node" is a convention for the C registry.
        self._add_c_line("render_node_runtime_func_body", f"lvgl_json_register_ptr(\"{component_id}\", \"component_json_node\", (void*){cjson_comp_root_var});")
        self._add_c_line("render_node_runtime_func_body", f"// Note: {cjson_comp_root_var} must remain valid for the lifetime of its uses.")


    def get_component_definition(self, component_id: str) -> Optional[CJSONObject]:
        # Retrieve from Python-side storage. This is used by BaseUIGenerator's process_use_view_node.
        return self.component_definitions.get(component_id)

    def get_entity_reference_for_id(self, id_str: str) -> Optional[Dict[str, Any]]:
        # This is for generator-time lookups. Check self.entity_info_map.
        # It could be by C variable name (if id_str is that) or by a registered ID.
        # For now, assume id_str is a C variable name.
        # A more robust lookup might iterate entity_info_map values and check 'id' if stored there.
        return self.entity_info_map.get(id_str)

    def get_type_information(self, entity_c_var_or_info: Any) -> Optional[Dict[str, Any]]:
        if isinstance(entity_c_var_or_info, str):
            return self.entity_info_map.get(entity_c_var_or_info)
        elif isinstance(entity_c_var_or_info, dict) and 'c_var_name' in entity_c_var_or_info:
            # It's already an info dict
            return entity_c_var_or_info
        return None
        
    # --- Methods from BaseUIGenerator that are abstract but not directly generating C lines for specific attributes ---
    # These are called by the BaseUIGenerator's process_node/apply_attributes, which then call the C-generating methods above.

    def get_node_id(self, node_data: CJSONObject) -> Optional[str]:
        # This is called by BaseUIGenerator.process_node on a Python CJSONObject.
        return node_data.get("id") if isinstance(node_data, (dict, CJSONObject)) else None

    def get_node_type(self, node_data: CJSONObject) -> str:
        # This is called by BaseUIGenerator.process_node on a Python CJSONObject.
        return node_data.get("type", "obj") if isinstance(node_data, (dict, CJSONObject)) else "obj"
        
    def get_node_context_override(self, node_data: CJSONObject) -> Optional[Any]:
        # This is called by BaseUIGenerator.process_node on a Python CJSONObject.
        return node_data.get("context") if isinstance(node_data, (dict, CJSONObject)) else None


    # --- Final Code Assembly Method ---
    def build_c_file_content(self) -> str:
        all_c_lines = []

        all_c_lines.extend(self.c_code["forward_declarations"])
        all_c_lines.append("\n// --- Global Definitions ---")
        all_c_lines.extend(self.c_code["global_definitions"])
        
        all_c_lines.append("\n// --- Runtime Node Rendering Function ---")
        all_c_lines.append("static void* render_json_node_runtime(cJSON *node_json, lv_obj_t *parent_obj, const char *current_path_prefix, cJSON *context_obj) {")
        all_c_lines.append("    if (!node_json || !cJSON_IsObject(node_json)) { fprintf(stderr, \"Error: Node JSON is not a valid object.\\n\"); return NULL; }")
        all_c_lines.append("    // Set context for this node's scope (if different or overridden)")
        all_c_lines.append("    cJSON* original_context = get_current_context();")
        all_c_lines.append("    cJSON* local_context_override = cJSON_GetObjectItemCaseSensitive(node_json, \"context\");")
        all_c_lines.append("    cJSON* effective_node_context = context_obj; // Default to passed-in context")
        all_c_lines.append("    if (local_context_override) {")
        all_c_lines.append("        // This needs a proper merge/override mechanism if contexts are complex")
        all_c_lines.append("        // For now, simple override for this scope. Caller should restore.")
        all_c_lines.append("        // A robust solution would involve cJSON_Duplicate and cJSON_Delete, or a context stack.")
        all_c_lines.append("        // Also, values in local_context_override might need formatting/unmarshalling based on original_context.")
        all_c_lines.append("        // For now, assume local_context_override is ready to use or simple key-value pairs.")
        all_c_lines.append("        set_current_context(local_context_override); // Potential issue: who owns/deletes local_context_override if duplicated?")
        all_c_lines.append("        effective_node_context = local_context_override;")
        all_c_lines.append("    } else {")
        all_c_lines.append("        set_current_context(context_obj); // Use inherited context")
        all_c_lines.append("    }")
        all_c_lines.append("")
        all_c_lines.append("    // Extract type and id (already done in Python, but C needs them from cJSON)")
        all_c_lines.append("    cJSON* type_json = cJSON_GetObjectItemCaseSensitive(node_json, \"type\");")
        all_c_lines.append("    const char* type_str = type_json && cJSON_IsString(type_json) ? cJSON_GetStringValue(type_json) : \"obj\";")
        all_c_lines.append("    cJSON* id_json = cJSON_GetObjectItemCaseSensitive(node_json, \"id\");")
        all_c_lines.append("    const char* id_str = id_json && cJSON_IsString(id_json) ? cJSON_GetStringValue(id_json) : NULL;")
        all_c_lines.append("")
        all_c_lines.append("    // Path construction for the current node")
        all_c_lines.append("    char node_path[256];")
        all_c_lines.append("    if (id_str) {")
        all_c_lines.append("        if (current_path_prefix && strlen(current_path_prefix) > 0) {")
        all_c_lines.append("            snprintf(node_path, sizeof(node_path), \"%s.%s\", current_path_prefix, id_str);")
        all_c_lines.append("        } else {")
        all_c_lines.append("            snprintf(node_path, sizeof(node_path), \"%s\", id_str);")
        all_c_lines.append("        }")
        all_c_lines.append("    } else {")
        all_c_lines.append("        snprintf(node_path, sizeof(node_path), \"%s\", current_path_prefix ? current_path_prefix : \"\");")
        all_c_lines.append("    }")
        all_c_lines.append("")
        all_c_lines.append("    // --- Generated C code from create_entity and register_entity_internally will be here ---")
        all_c_lines.extend(self.c_code["render_node_runtime_func_body"]) # This is where create_entity etc. add their lines
        all_c_lines.append("")
        all_c_lines.append("    // --- After entity creation, apply attributes ---")
        all_c_lines.append("    // The 'created_entity_c_var' and its info needs to be known here.")
        all_c_lines.append("    // This part is tricky because the C var name is generated in Python. ")
        all_c_lines.append("    // The generated lines in render_node_runtime_func_body must define the correct C variable for the created entity.")
        all_c_lines.append("    // For now, assume the last created entity's C var name is implicitly known or passed to apply_setters.")
        all_c_lines.append("    // A better way: create_entity C code could assign to a known temp var like 'current_created_obj'.")
        all_c_lines.append("    // This current structure assumes apply_setters_and_attributes_runtime is called for the 'node_json' and its 'target_obj'.")
        all_c_lines.append("    // The actual call to apply_setters_and_attributes_runtime should be generated by the process_node logic in Python,")
        all_c_lines.append("    // using the entity_info returned by create_entity.")
        all_c_lines.append("    // The lines from self.c_code[\"render_node_runtime_func_body\"] should contain the call to apply_setters_and_attributes_runtime.")
        all_c_lines.append("")
        all_c_lines.append("    // Restore context if it was overridden locally")
        all_c_lines.append("    if (local_context_override) {")
        all_c_lines.append("        set_current_context(original_context);")
        all_c_lines.append("        // If local_context_override was a duplicate, it should be cJSON_Deleted here.")
        all_c_lines.append("    }")
        all_c_lines.append("    // This function should return the created lv_obj_t* (or void*)")
        all_c_lines.append("    // The generated code in render_node_runtime_func_body should ensure the correct entity is returned.")
        all_c_lines.append("    // This is also tricky. For now, returning NULL as a placeholder for the function structure.")
        all_c_lines.append("    // The generated code should assign to a common variable like 'void* created_obj_for_return = ...;' and then 'return created_obj_for_return;'")
        all_c_lines.append("    return NULL; // Placeholder - generated code must return the actual created object.")
        all_c_lines.append("}")

        all_c_lines.append("\n// --- Runtime Attribute Application Function ---")
        all_c_lines.append("static bool apply_setters_and_attributes_runtime(cJSON *attrs_json, void *target_obj, const char* target_type_str, const char* target_actual_type_str, bool is_widget, const char *path_prefix, cJSON* context_obj) {")
        all_c_lines.append("    if (!attrs_json || !cJSON_IsObject(attrs_json)) { /*fprintf(stderr, \"Warning: Attributes JSON is not a valid object for target %p.\\n\", target_obj);*/ return false; }")
        all_c_lines.append("    if (!target_obj) { fprintf(stderr, \"Error: Target object is NULL for applying attributes.\\n\"); return false; }")
        all_c_lines.append("    // Set context for this attribute application scope")
        all_c_lines.append("    // This might not be needed if attributes don't define their own mini-contexts.")
        all_c_lines.append("    // cJSON* original_context_for_attrs = get_current_context();")
        all_c_lines.append("    // set_current_context(context_obj);")
        all_c_lines.append("")
        all_c_lines.extend(self.c_code["apply_attrs_runtime_func_body"]) # call_setter, handle_children etc. add lines here
        all_c_lines.append("")
        all_c_lines.append("    // set_current_context(original_context_for_attrs);")
        all_c_lines.append("    return true;")
        all_c_lines.append("}")

        all_c_lines.append("\n// --- Main UI Rendering Entry Point ---")
        all_c_lines.extend(self.c_code["main_render_func_body"])
        
        all_c_lines.append("\n// --- Component Definitions C Code (if any, for pre-registration) ---")
        all_c_lines.extend(self.c_code["component_definitions_c_code"])


        return "\n".join(all_c_lines)
