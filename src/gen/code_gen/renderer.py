# code_gen/renderer.py
import logging
from type_utils import lvgl_type_to_widget_name

logger = logging.getLogger(__name__)

def generate_renderer(custom_creators_map):
    """Generates the C code for parsing the JSON UI and rendering it."""
    # custom_creators_map: {'style': 'lv_style_create_managed', ...}

    c_code = "// --- JSON UI Renderer ---\n\n"
    c_code += "#include <stdio.h> // For debug prints\n"
    c_code += "#include \"data_binding.h\" // For action/observes attributes\n\n"
    c_code += "extern data_binding_registry_t* REGISTRY; // Global registry for actions and data bindings\n\n"
    c_code += "// Forward declarations\n"
    c_code += "static char* render_json_node_internal(cJSON *node, lv_obj_t *parent_render_mode, const char *named_path_prefix, FILE *out_c, FILE *out_h, const char *current_parent_c_var_name);\n"
    c_code += "static bool apply_setters_and_attributes_internal(cJSON *attributes_json_obj, void *target_entity_render_mode, const char *target_entity_c_var_name, const char *target_actual_type_str, const char *target_create_type_str, bool target_is_widget, lv_obj_t *explicit_parent_for_children_attr_render_mode, const char *parent_c_var_name_for_children_transpile_mode, const char *path_prefix_for_named_and_children, const char *default_type_name_for_registry_if_named);\n"
    c_code += "static bool process_json_ui_internal(cJSON *root_json, lv_obj_t *implicit_root_parent_render_mode, FILE *out_c, FILE *out_h, const char *initial_parent_c_var_name);\n"
    c_code += "static const invoke_table_entry_t* find_invoke_entry(const char *name);\n"
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent);\n"
    c_code += "extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);\n"
    c_code += "extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);\n"
    c_code += "static void set_current_context(cJSON* new_context);\n"
    c_code += "static cJSON* get_current_context(void); // Already declared in source template, but good for clarity\n"

    c_code += """
// Helper function to sanitize a base filename for C identifiers and header guards
static void sanitize_output_base_name_for_c_ids(
    const char* output_c_filename_base,
    char* sanitized_name_out, size_t sanitized_name_out_len,
    char* sanitized_name_upper_out, size_t sanitized_name_upper_out_len
) {
    if (!output_c_filename_base || !sanitized_name_out || !sanitized_name_upper_out || sanitized_name_out_len == 0 || sanitized_name_upper_out_len == 0) {
        if (sanitized_name_out && sanitized_name_out_len > 0) { strncpy(sanitized_name_out, "default_ui_name", sanitized_name_out_len -1); sanitized_name_out[sanitized_name_out_len-1] = '\\0'; }
        if (sanitized_name_upper_out && sanitized_name_upper_out_len > 0) { strncpy(sanitized_name_upper_out, "DEFAULT_UI_NAME", sanitized_name_upper_out_len -1); sanitized_name_upper_out[sanitized_name_upper_out_len-1] = '\\0';}
        return;
    }

    char base_name_no_suffix[256];
    const char *last_slash = strrchr(output_c_filename_base, '/');
    const char *last_backslash = strrchr(output_c_filename_base, '\\\\'); // Escaped for C
    const char *base_ptr = output_c_filename_base;

    if (last_slash && (last_backslash == NULL || last_slash > last_backslash)) {
        base_ptr = last_slash + 1;
    } else if (last_backslash) {
        base_ptr = last_backslash + 1;
    }

    strncpy(base_name_no_suffix, base_ptr, sizeof(base_name_no_suffix) - 1);
    base_name_no_suffix[sizeof(base_name_no_suffix) - 1] = '\\0';

    char *dot = strrchr(base_name_no_suffix, '.');
    // More robust suffix stripping for common extensions
    if (dot) {
        const char* known_extensions[] = {".json", ".JSON", ".yaml", ".YAML", ".yml", ".YML", ".c", ".C", ".h", ".H", NULL};
        for (int k=0; known_extensions[k] != NULL; ++k) {
            if (strcmp(dot, known_extensions[k]) == 0) {
                *dot = '\\0';
                break;
            }
        }
    }
    base_ptr = base_name_no_suffix; // Use the (potentially) suffix-stripped name

    int j = 0;
    if (base_ptr[0] && isdigit((unsigned char)base_ptr[0]) && j < sanitized_name_out_len -1) {
        sanitized_name_out[j++] = '_';
    }
    for (int i = 0; base_ptr[i] && j < sanitized_name_out_len - 1; ++i) {
        if (isalnum((unsigned char)base_ptr[i]) || base_ptr[i] == '_') { // Allow underscore
            sanitized_name_out[j++] = base_ptr[i];
        } else {
            sanitized_name_out[j++] = '_'; // Replace others with underscore
        }
    }
    sanitized_name_out[j] = '\\0';

    if (j == 0 || (j == 1 && sanitized_name_out[0] == '_')) { // Handle empty or all-invalid-char names
        strncpy(sanitized_name_out, "default_ui_name", sanitized_name_out_len -1);
        sanitized_name_out[sanitized_name_out_len-1] = '\\0';
        j = strlen(sanitized_name_out); // Update j to new length
    }

    // Create uppercase version for header guard
    int k_upper = 0;
    for (int i_upper = 0; sanitized_name_out[i_upper] && k_upper < sanitized_name_upper_out_len - 1; ++i_upper) {
        sanitized_name_upper_out[k_upper++] = toupper((unsigned char)sanitized_name_out[i_upper]);
    }
    sanitized_name_upper_out[k_upper] = '\\0';
}
"""

    # Include custom creator function prototypes
    for type_name, creator_func in custom_creators_map.items():
         c_type = f"lv_{type_name}_t"
         c_code += f"extern {c_type}* {creator_func}(const char *name);\n"
    c_code += "\n"

    # apply_setters_and_attributes_internal function
    c_code += """
static bool apply_setters_and_attributes_internal(
    cJSON *attributes_json_obj,
    void *target_entity_render_mode,
    const char *target_entity_c_var_name, // Used in TRANSPILE_MODE
    const char *target_actual_type_str,
    const char *target_create_type_str,
    bool target_is_widget,
    lv_obj_t *explicit_parent_for_children_attr_render_mode,
    const char *parent_c_var_name_for_children_transpile_mode, // Used in TRANSPILE_MODE
    const char *path_prefix_for_named_and_children,
    const char *default_type_name_for_registry_if_named
) {
    if (!attributes_json_obj || !cJSON_IsObject(attributes_json_obj)) {
        LOG_ERR("apply_setters_and_attributes_internal: Invalid attributes_json_obj.");
        return false;
    }
    if (g_current_operation_mode == RENDER_MODE && !target_entity_render_mode) {
        LOG_ERR("apply_setters_and_attributes_internal: RENDER_MODE but target_entity_render_mode is NULL.");
        return false;
    }
    if (g_current_operation_mode == TRANSPILE_MODE && !target_entity_c_var_name) {
        // Though, for "with" blocks, target_entity_c_var_name might be resolved dynamically.
        // This check might need refinement if target_entity_c_var_name can be legitimately NULL at entry for some cases.
        // LOG_DEBUG("apply_setters_and_attributes_internal: TRANSPILE_MODE but target_entity_c_var_name is NULL. This might be okay for 'with' obj resolution.");
    }


    LOG_DEBUG("Applying attributes with path_prefix: %s to entity (R:%p T:%s) of type %s (create_type %s)",
        path_prefix_for_named_and_children ? path_prefix_for_named_and_children : "NULL",
        target_entity_render_mode, target_entity_c_var_name ? target_entity_c_var_name : "N/A",
        target_actual_type_str, target_create_type_str);

    char current_children_base_path[256];
    if (path_prefix_for_named_and_children) {
        strncpy(current_children_base_path, path_prefix_for_named_and_children, sizeof(current_children_base_path) - 1);
        current_children_base_path[sizeof(current_children_base_path) - 1] = '\\0';
    } else {
        current_children_base_path[0] = '\\0';
    }

    cJSON *prop_item = NULL;
    for (prop_item = attributes_json_obj->child; prop_item != NULL; prop_item = prop_item->next) {
        const char *prop_name = prop_item->string;
        if (!prop_name) continue;

        // Attributes handled by render_json_node's main logic or specific setup
        if (strcmp(prop_name, "type") == 0 || strcmp(prop_name, "id") == 0 || strcmp(prop_name, "context") == 0) {
            continue;
        }
        if (strcmp(target_actual_type_str, "grid") == 0 && (strcmp(prop_name, "cols") == 0 || strcmp(prop_name, "rows") == 0)) {
            continue;
        }
        // "do" is a structural element, not a direct attribute to be set here unless it's for "with"
        // This function is called FOR the contents of a "do" block, it shouldn't process a "do" key itself in most cases.
        // The "with" block below is an exception where "do" is part of its structure.
        if (strcmp(prop_name, "do") == 0 && strcmp(target_actual_type_str, "with") !=0) { // "with" type is a pseudo-type
             // If we are applying attributes for a "use-view"'s "do" block, target_actual_type_str is the component root's type.
             // If we are applying attributes for a regular node, target_actual_type_str is the node's type.
             // In these cases, a "do" key here is unexpected or a nested "do", which is not standard.
            LOG_WARN_JSON(prop_item, "Skipping unexpected 'do' attribute key '%s' during general attribute application for type '%s'.", prop_name, target_actual_type_str);
            continue;
        }

        // Handle "action" attribute
        if (strcmp(prop_name, "action") == 0) {
            if (g_current_operation_mode == RENDER_MODE) {
                if (target_is_widget) {
                    if (cJSON_IsString(prop_item)) {
                        char* action_val_str = NULL;
                        if (unmarshal_value(prop_item, "char *", &action_val_str, target_entity_render_mode)) {
                            if (REGISTRY) {
                                lv_event_cb_t evt_cb = action_registry_get_handler_s(REGISTRY, action_val_str);
                                if (evt_cb) {
                                    lv_obj_add_event_cb((lv_obj_t*)target_entity_render_mode, evt_cb, LV_EVENT_ALL, NULL);
                                } else {
                                    LOG_WARN_JSON(prop_item, "Action '%s' not found in registry.", action_val_str);
                                }
                            } else {
                                LOG_ERR_JSON(prop_item, "REGISTRY is NULL, cannot process 'action': %s", action_val_str);
                            }
                        } else {
                            LOG_ERR_JSON(prop_item, "Failed to unmarshal 'action' string value.");
                        }
                    } else {
                        LOG_WARN_JSON(prop_item, "'action' property must be a string.");
                    }
                } else {
                    LOG_WARN_JSON(prop_item, "'action' property can only be applied to widgets.");
                }
            } else { // TRANSPILE_MODE
                // TODO: Transpile 'action' attribute.
                LOG_DEBUG("TRANSPILE_MODE: 'action' attribute found for %s. Needs transpilation.", target_entity_c_var_name);
            }
            continue;
        }

        // Handle "observes" attribute
        if (strcmp(prop_name, "observes") == 0) {
            if (g_current_operation_mode == RENDER_MODE) {
                if (target_is_widget) {
                    if (cJSON_IsObject(prop_item)) {
                        cJSON* obs_value_item = cJSON_GetObjectItemCaseSensitive(prop_item, "value");
                        cJSON* obs_format_item = cJSON_GetObjectItemCaseSensitive(prop_item, "format");
                        if (obs_value_item && cJSON_IsString(obs_value_item) && obs_format_item && cJSON_IsString(obs_format_item)) {
                            char* obs_value_str = NULL;
                            char* obs_format_str = NULL;
                            bool val_ok = unmarshal_value(obs_value_item, "char *", &obs_value_str, target_entity_render_mode);
                            bool fmt_ok = unmarshal_value(obs_format_item, "char *", &obs_format_str, target_entity_render_mode);
                            if (val_ok && fmt_ok && obs_value_str && obs_format_str) {
                                if (REGISTRY) {
                                    data_binding_register_widget_s(REGISTRY, obs_value_str, (lv_obj_t*)target_entity_render_mode, obs_format_str);
                                } else {
                                    LOG_ERR_JSON(prop_item, "REGISTRY is NULL, cannot process 'observes' for value: %s", obs_value_str);
                                }
                            } else {
                                LOG_ERR_JSON(prop_item, "Failed to unmarshal 'value' or 'format' for 'observes'. val_ok:%d fmt_ok:%d", val_ok, fmt_ok);
                            }
                        } else {
                            LOG_WARN_JSON(prop_item, "'observes' object must have string properties 'value' and 'format'.");
                        }
                    } else {
                        LOG_WARN_JSON(prop_item, "'observes' property must be an object.");
                    }
                } else {
                    LOG_WARN_JSON(prop_item, "'observes' property can only be applied to widgets.");
                }
            } else { // TRANSPILE_MODE
                // TODO: Transpile 'observes' attribute.
                LOG_DEBUG("TRANSPILE_MODE: 'observes' attribute found for %s. Needs transpilation.", target_entity_c_var_name);
            }
            continue;
        }

        if (strcmp(prop_name, "named") == 0 && cJSON_IsString(prop_item)) {
            char *named_value_str = NULL;
            // In both modes, unmarshal_value might depend on context (which should be set correctly before this call)
            // For RENDER_MODE, target_entity_render_mode is the implicit parent for context.
            // For TRANSPILE_MODE, target_entity_c_var_name is not an entity, so pass NULL for implicit_parent for now,
            // or consider if context should be handled differently for transpiled 'named'.
            void* unmarshal_context_parent = (g_current_operation_mode == RENDER_MODE) ? target_entity_render_mode : NULL;
            if (unmarshal_value(prop_item, "char *", &named_value_str, unmarshal_context_parent)) {
                char full_named_path_buf[256] = {0};
                if (path_prefix_for_named_and_children && path_prefix_for_named_and_children[0] != '\\0') {
                    snprintf(full_named_path_buf, sizeof(full_named_path_buf) - 1, "%s:%s", path_prefix_for_named_and_children, named_value_str);
                } else {
                    strncpy(full_named_path_buf, named_value_str, sizeof(full_named_path_buf) - 1);
                }
                
                if (default_type_name_for_registry_if_named && default_type_name_for_registry_if_named[0]) {
                    if (g_current_operation_mode == RENDER_MODE) {
                        lvgl_json_register_ptr(full_named_path_buf, default_type_name_for_registry_if_named, target_entity_render_mode);
                        LOG_INFO("RENDER_MODE: Registered entity %p as '%s' (type %s) via 'named' attribute.", target_entity_render_mode, full_named_path_buf, default_type_name_for_registry_if_named);
                    } else { // TRANSPILE_MODE
                        // In transpile mode, we register the C variable name string.
                        // The `target_entity_c_var_name` is the name of the variable we want to associate with `full_named_path_buf`.
                        if (target_entity_c_var_name && target_entity_c_var_name[0]) {
                            // Store the C variable name. The "type" is "c_var_name" to distinguish from actual LVGL types.
                            // Note: lv_strdup is important here if target_entity_c_var_name is transient or needs to persist in registry.
                            lvgl_json_register_ptr(full_named_path_buf, "c_var_name", (void*)lv_strdup(target_entity_c_var_name));
                            LOG_INFO("TRANSPILE_MODE: Registered C var '%s' as path '%s' via 'named' attribute.", target_entity_c_var_name, full_named_path_buf);
                        } else {
                            LOG_WARN_JSON(prop_item, "TRANSPILE_MODE: 'named' attribute found, but target_entity_c_var_name is empty for path '%s'. Cannot register.", full_named_path_buf);
                        }
                        // TODO: Handle 'named' attribute for transpilation if target_entity_c_var_name needs to be globally mapped in generated C code beyond registry.
                        // This usually means ensuring the C variable is accessible, possibly by making it global or passing it around.
                        // For now, the registry handles mapping the path to the C variable name string.
                    }
                    // Update current_children_base_path for subsequent children within this attribute set
                    strncpy(current_children_base_path, full_named_path_buf, sizeof(current_children_base_path) - 1);
                    current_children_base_path[sizeof(current_children_base_path) - 1] = '\\0';
                } else {
                    LOG_WARN_JSON(prop_item, "'named' attribute used, but no valid type_name_for_registry provided for '%s'. Entity (R:%p T:%s) not registered by this 'named' attribute.", named_value_str, target_entity_render_mode, target_entity_c_var_name);
                }
            } else {
                LOG_ERR_JSON(prop_item, "Failed to resolve 'named' property value string: '%s'", prop_item->valuestring);
            }
            continue;
        }

        if (strcmp(prop_name, "children") == 0) {
            if (!cJSON_IsArray(prop_item)) {
                LOG_ERR_JSON(prop_item, "'children' property must be an array.");
                continue;
            }
            if (g_current_operation_mode == RENDER_MODE) {
                if (!target_is_widget || !explicit_parent_for_children_attr_render_mode) {
                    LOG_ERR_JSON(prop_item, "RENDER_MODE: 'children' attribute found, but target entity is not a widget or explicit_parent_for_children_attr_render_mode is NULL. Cannot add children.");
                    continue;
                }
                LOG_DEBUG("RENDER_MODE: Processing 'children' for entity %p under path prefix '%s'", target_entity_render_mode, current_children_base_path);
                cJSON *child_node_json = NULL;
                cJSON_ArrayForEach(child_node_json, prop_item) {
                    // For render mode, out_c, out_h, and current_parent_c_var_name are NULL
                    char* child_render_result = render_json_node_internal(child_node_json, explicit_parent_for_children_attr_render_mode, current_children_base_path, NULL, NULL, NULL);
                    if (child_render_result == NULL) { // In render mode, result is direct pointer or special non-NULL like (char*)1
                        LOG_ERR_JSON(child_node_json, "RENDER_MODE: Failed to render child node from 'children' attribute. Aborting siblings for this 'children' array.");
                        break;
                    }
                    // In render mode, the returned pointer is the lv_obj_t* or style, not a string to be freed.
                }
            } else { // TRANSPILE_MODE
                 if (!target_is_widget || !parent_c_var_name_for_children_transpile_mode) {
                    LOG_ERR_JSON(prop_item, "TRANSPILE_MODE: 'children' attribute found, but target entity is not a widget or parent_c_var_name_for_children_transpile_mode is NULL. Cannot add children.");
                    continue;
                }
                LOG_DEBUG("TRANSPILE_MODE: Processing 'children' for C var '%s' (parent for children: '%s') under path prefix '%s'", target_entity_c_var_name, parent_c_var_name_for_children_transpile_mode, current_children_base_path);
                cJSON *child_node_json = NULL;
                cJSON_ArrayForEach(child_node_json, prop_item) {
                    // For transpile mode, parent_render_mode is NULL. Pass g_output_c_file, g_output_h_file.
                    char* child_c_var_name = render_json_node_internal(child_node_json, NULL, current_children_base_path, g_output_c_file, g_output_h_file, parent_c_var_name_for_children_transpile_mode);
                    if (child_c_var_name == NULL) {
                        LOG_ERR_JSON(child_node_json, "TRANSPILE_MODE: Failed to transpile child node from 'children' attribute. Aborting siblings for this 'children' array.");
                        break;
                    }
                    lv_free(child_c_var_name); // Free the C variable name string returned by render_json_node_internal
                }
            }
            continue;
        }
        
        if (strcmp(prop_name, "with") == 0 && cJSON_IsObject(prop_item)) {
            cJSON *obj_to_run_with_json = cJSON_GetObjectItemCaseSensitive(prop_item, "obj");
            cJSON *do_block_for_with_json = cJSON_GetObjectItemCaseSensitive(prop_item, "do");

            if (!obj_to_run_with_json) {
                LOG_ERR_JSON(prop_item, "'with' block is missing 'obj' attribute. Skipping.");
                continue;
            }
            if (!do_block_for_with_json || !cJSON_IsObject(do_block_for_with_json)) { // Changed to cJSON_IsObject from cJSON_IsArray
                LOG_ERR_JSON(prop_item, "'with' block is missing 'do' object or it's not an object. Skipping.");
                continue;
            }
            
            if (g_current_operation_mode == RENDER_MODE) {
                lv_obj_t *with_target_obj_render_mode = NULL;
                // Pass current target_entity_render_mode as implicit_parent for context in unmarshal
                // For render_json_node_internal, the entity itself is passed as implicit_parent for resolving context like $parent.width etc.
                // Here, target_entity_render_mode is the 'current_obj_ptr' that is having its attributes applied.
                // If 'with.obj' refers to something via context like "$sibling", that context should be relative to target_entity_render_mode.
                if (!unmarshal_value(obj_to_run_with_json, "lv_obj_t *", &with_target_obj_render_mode, target_entity_render_mode)) {
                    LOG_ERR_JSON(obj_to_run_with_json, "RENDER_MODE: Failed to unmarshal 'obj' for 'with' block. Skipping 'with'.");
                    continue;
                }
                if (!with_target_obj_render_mode) {
                    LOG_ERR_JSON(obj_to_run_with_json, "RENDER_MODE: 'obj' for 'with' block resolved to NULL. Skipping 'with'.");
                    continue;
                }
                LOG_INFO("RENDER_MODE: Applying 'with.do' attributes to target %p (resolved from 'with.obj')", with_target_obj_render_mode);
                // Determine actual type of the resolved 'with_target_obj_render_mode' for accurate attribute application.
                // This is a simplification; ideally, we'd know its actual LVGL type (e.g., "button", "label").
                // For now, assume it's a generic "obj" for attribute application.
                // The path_prefix is inherited. The type_name_for_registry is "lv_obj_t" as we don't know more.
                if (!apply_setters_and_attributes_internal(
                        do_block_for_with_json,            // The attributes to apply
                        with_target_obj_render_mode,       // The target entity
                        NULL,                              // No C var name in render mode
                        "obj",                             // Assume actual type is "obj" for safety
                        "obj",                             // Assume create type is "obj"
                        true,                              // Assume it's a widget
                        with_target_obj_render_mode,       // Parent for its own children if 'do' has 'children'
                        NULL,                              // No parent C var name in render mode
                        path_prefix_for_named_and_children,// Inherit path prefix
                        "lv_obj_t"                         // Type for named registration if 'do' has 'named'
                    )) {
                    LOG_ERR_JSON(do_block_for_with_json, "RENDER_MODE: Failed to apply attributes in 'with.do' block.");
                }
            } else { // TRANSPILE_MODE
                transpile_write_line(g_output_c_file, "    // Handling 'with' attribute for %s", target_entity_c_var_name ? target_entity_c_var_name : "unknown_target");
                char* target_with_obj_c_name = NULL;

                // Determine the C variable name for the target of 'with'
                // The 'implicit_parent_for_context' for unmarshal_value_to_c_string should be related to 'target_entity_c_var_name's context if needed.
                // However, for "@ref" or direct calls, context isn't usually from the object having the "with" attribute, but global or from call structure.
                // Passing NULL for context parent, assuming 'obj' value is self-contained or globally resolvable.
                if (cJSON_IsString(obj_to_run_with_json)) { // It's a reference like "@name" or a direct variable name
                    if (!unmarshal_value_to_c_string(obj_to_run_with_json, "void *", &target_with_obj_c_name, NULL)) {
                        LOG_ERR_JSON(obj_to_run_with_json, "TRANSPILE_MODE: Failed to transpile 'with.obj' reference/string to C variable name: %s.", obj_to_run_with_json->valuestring);
                    }
                } else if (cJSON_IsObject(obj_to_run_with_json)) { // It's a nested call for object creation or a function call returning an object
                    cJSON* call_item = cJSON_GetObjectItemCaseSensitive(obj_to_run_with_json, "call");
                    cJSON* args_item = cJSON_GetObjectItemCaseSensitive(obj_to_run_with_json, "args");
                    if (call_item && cJSON_IsString(call_item) && args_item && cJSON_IsArray(args_item)) {
                        const char* func_name = call_item->valuestring;
                        const invoke_table_entry_t* entry = find_invoke_entry(func_name);
                        if (entry) {
                            // Invoke in transpile mode.
                            // target_obj_c_name_transpile (5th arg for invoke) is the C var name of the object the function is called ON.
                            // For creation (e.g. lv_btn_create), this is NULL or the parent var name.
                            // Let's pass current_parent_c_var_name_for_children_transpile_mode, which is the parent of the object having the "with" attribute.
                            // This might be relevant if the 'with.obj' call is like parent->get_child(...)
                            // The result_c_name_transpile (6th arg) will capture the C name of the *result* of the call.
                            // Corrected 6-argument invoke call:
                            if (!entry->invoke(entry,
                                             target_entity_render_mode,       /* target_obj_ptr_render_mode (context for nested args) */
                                             NULL,                            /* target_obj_c_name_transpile (target C name of nested call itself, if method) */
                                             NULL,                            /* result_dest_render_mode */
                                             &target_with_obj_c_name,         /* result_c_name_transpile (char** for result C var name) */
                                             args_item                        /* json_args_array for nested call */
                                         )) {
                                LOG_ERR_JSON(obj_to_run_with_json, "TRANSPILE_MODE: Failed to transpile 'with.obj' nested call for '%s'.", func_name);
                                // target_with_obj_c_name might be NULL or contain partial data, ensure it's handled before potential free
                            } else {
                                LOG_DEBUG("TRANSPILE_MODE: Nested call for 'with.obj' yielded C var name '%s'", target_with_obj_c_name ? target_with_obj_c_name : "NULL");
                            }
                            // target_with_obj_c_name is now (hopefully) the C variable name of the created/returned object.
                        } else {
                            LOG_ERR_JSON(obj_to_run_with_json, "TRANSPILE_MODE: 'with.obj' call: function '%s' not found in invoke table.", func_name);
                        }
                    } else {
                        LOG_ERR_JSON(obj_to_run_with_json, "TRANSPILE_MODE: 'with.obj' is an object but not a valid 'call' structure.");
                    }
                } else {
                    LOG_ERR_JSON(obj_to_run_with_json, "TRANSPILE_MODE: 'with.obj' must be a string (reference/var name) or an object (call).");
                }

                if (target_with_obj_c_name) {
                    transpile_write_line(g_output_c_file, "    // 'with' actions will target C variable: %s (resolved from 'with.obj')", target_with_obj_c_name);
                    // Similar to render mode, assume "obj" type for simplicity for now.
                    // The parent_c_var_name_for_children_transpile_mode for children inside the 'do' block
                    // should be target_with_obj_c_name itself.
                    if (!apply_setters_and_attributes_internal(
                            do_block_for_with_json,       // The attributes to apply
                            NULL,                         // No render mode entity
                            target_with_obj_c_name,       // The C variable name of the target
                            "obj",                        // Assumed actual type
                            "obj",                        // Assumed create type
                            true,                         // Assumed widget
                            NULL,                         // No render mode parent for children
                            target_with_obj_c_name,       // Parent C var for children in 'do' block
                            path_prefix_for_named_and_children, // Inherit path prefix
                            "lv_obj_t"                    // Type for named registration
                        )) {
                        LOG_ERR_JSON(do_block_for_with_json, "TRANSPILE_MODE: Failed to apply attributes in 'with.do' block for C var '%s'.", target_with_obj_c_name);
                    }
                    lv_free(target_with_obj_c_name); // Free the C name string after use (it's always strdup'd by unmarshal or invoke)
                } else {
                    transpile_write_line(g_output_c_file, "    // ERROR: Could not determine target C variable for 'with' block under %s.", target_entity_c_var_name ? target_entity_c_var_name : "unknown_target");
                }
            }
            continue;
        }


        // --- Standard Setter Logic ---
        cJSON *prop_args_array = prop_item;
        bool temp_array_created = false;
        if (!cJSON_IsArray(prop_item)) {
            prop_args_array = cJSON_CreateArray();
            if (prop_args_array) {
                cJSON_AddItemToArray(prop_args_array, cJSON_Duplicate(prop_item, true));
                temp_array_created = true;
            } else {
                LOG_ERR_JSON(prop_item, "Failed to create temporary array for property '%s'", prop_name);
                continue;
            }
        }

        char setter_name_buf[128];
        const invoke_table_entry_t* setter_entry = NULL;

        // 1. Try specific type: lv_<target_actual_type_str>_set_<prop_name>
        if (target_actual_type_str && target_actual_type_str[0]) {
            snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_%s_set_%s", target_actual_type_str, prop_name);
            setter_entry = find_invoke_entry(setter_name_buf);
        }

        // 2. Try specific type (short form): lv_<target_actual_type_str>_<prop_name>
        if (!setter_entry && target_actual_type_str && target_actual_type_str[0]) {
            snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_%s_%s", target_actual_type_str, prop_name);
            setter_entry = find_invoke_entry(setter_name_buf);
        }
        
        // Fallbacks for widgets (using target_create_type_str for consistency, e.g. "obj" for "grid")
        if (target_is_widget) {
            // 3. Try generic obj: lv_obj_set_<prop_name>
            if (!setter_entry) {
                snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_obj_set_%s", prop_name);
                setter_entry = find_invoke_entry(setter_name_buf);
            }
            // 4. Try generic obj (short form): lv_obj_<prop_name>
            if (!setter_entry) {
                snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_obj_%s", prop_name);
                setter_entry = find_invoke_entry(setter_name_buf);
            }
            // 5. Try style property: lv_obj_set_style_<prop_name>
            if (!setter_entry) {
                snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_obj_set_style_%s", prop_name);
                setter_entry = find_invoke_entry(setter_name_buf);
                size_t arg_count = 0; for (arg_count = 0; setter_entry && setter_entry->arg_types[arg_count] != NULL; ++arg_count);
                if (setter_entry && 
                    arg_count > 2 && // obj, value, selector
                    setter_entry->arg_types[2] != NULL &&
                    (strcmp(setter_entry->arg_types[2], "lv_style_selector_t") == 0 || strcmp(setter_entry->arg_types[2], "int") == 0 || strcmp(setter_entry->arg_types[2], "uint32_t") == 0) &&
                    cJSON_GetArraySize(prop_args_array) == 1) {
                    LOG_DEBUG("Adding default selector LV_PART_MAIN (0) for style property '%s' on %s", prop_name, target_actual_type_str);
                    cJSON_AddItemToArray(prop_args_array, cJSON_CreateNumber(LV_PART_MAIN));
                }
            }
        } else if (strcmp(target_actual_type_str, "style") == 0) { // Fallbacks for styles
            // 3b. Try lv_style_set_<prop_name>
            if (!setter_entry) {
                snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_style_set_%s", prop_name);
                setter_entry = find_invoke_entry(setter_name_buf);
            }
            // 4b. Try lv_style_<prop_name>
            if (!setter_entry) {
                snprintf(setter_name_buf, sizeof(setter_name_buf), "lv_style_%s", prop_name);
                setter_entry = find_invoke_entry(setter_name_buf);
            }
        }

        // 6. Try verbatim name (for direct function calls, less common for setters)
        if (!setter_entry) {
            setter_entry = find_invoke_entry(prop_name);
        }

        if (!setter_entry) {
            LOG_WARN_JSON(prop_args_array, "No setter/invokable found for property '%s' on type '%s' (create type '%s').", prop_name, target_actual_type_str, target_create_type_str);
        } else {
            if (g_current_operation_mode == RENDER_MODE) {
                if (!setter_entry->invoke(setter_entry, target_entity_render_mode, NULL /*target_obj_c_name_transpile*/, NULL /*result_dest_render_mode for void setters*/, NULL /*result_c_name_transpile*/, prop_args_array)) {
                    LOG_ERR_JSON(prop_args_array, "RENDER_MODE: Failed to set property '%s' using '%s' on entity %p.", prop_name, setter_entry->name, target_entity_render_mode);
                } else {
                    LOG_DEBUG("RENDER_MODE: Successfully applied property '%s' using '%s' to entity %p", prop_name, setter_entry->name, target_entity_render_mode);
                }
            } else { // TRANSPILE_MODE
                // TODO: Transpile setter_entry->name for target_entity_c_var_name using prop_args_array
                // This will involve generating C code like: lv_obj_set_width(my_obj_0, 100);
                // Requires transpile_format_arg_as_c_string for each argument in prop_args_array.
                LOG_DEBUG("TRANSPILE_MODE: Setter '%s' found for C var '%s'. Args need transpilation.", setter_entry->name, target_entity_c_var_name);
                // Placeholder:
                // char* arg_str_array[MAX_ARGS]; // MAX_ARGS should be defined
                // int arg_count = 0;
                // cJSON* current_arg_json = NULL;
                // for (int i = 0; (current_arg_json = cJSON_GetArrayItem(prop_args_array, i)) != NULL && i < MAX_ARGS; ++i) {
                //    arg_str_array[i] = transpile_format_arg_as_c_string(current_arg_json, setter_entry->arg_types[i+1], NULL); // arg_types[0] is return type
                //    if (!arg_str_array[i]) { /* error */ }
                //    arg_count++;
                // }
                // transpile_write_line(g_output_c_file, "%s(%s, ...);", setter_entry->name, target_entity_c_var_name, ... join arg_str_array ...);
                // for (int i=0; i<arg_count; ++i) lv_free(arg_str_array[i]);
                transpile_write_line(g_output_c_file, "// TODO: TRANSPILE: %s(%s, ...args...); // Property: %s", setter_entry->name, target_entity_c_var_name, prop_name);

            }
        }

        if (temp_array_created) {
            cJSON_Delete(prop_args_array);
        }
    } // End for loop over attributes

    return true; // Indicate success
}
"""

    # Main recursive rendering function (now internal)
    c_code += "static char* render_json_node_internal(cJSON *node, lv_obj_t *parent_render_mode, const char *named_path_prefix, FILE *out_c, FILE *out_h, const char *current_parent_c_var_name) {\n"
    c_code += "    if (!cJSON_IsObject(node)) {\n"
    c_code += "        LOG_ERR(\"Render Error: Expected JSON object for UI node.\");\n"
    c_code += "        return NULL;\n"
    c_code += "    }\n\n"

    c_code += """
    // --- Context management: Save context active at the start of this node's processing ---
    cJSON* original_context_for_this_node_call = get_current_context();
    bool context_was_locally_changed_by_this_node = false;

    // Transpile mode variable name for the created entity
    char* created_entity_c_var_name_transpile = NULL;

    // --- Handle special node types: component definition, use-view, context wrapper ---
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(node, "type");
    const char *type_str = "obj"; // Default type
    if (type_item && cJSON_IsString(type_item)) {
        type_str = type_item->valuestring;
    }


    if (strcmp(type_str, "component") == 0) {
        cJSON *id_item_comp = cJSON_GetObjectItemCaseSensitive(node, "id");
        cJSON *root_item_comp = cJSON_GetObjectItemCaseSensitive(node, "root");
        if (id_item_comp && cJSON_IsString(id_item_comp) && id_item_comp->valuestring && id_item_comp->valuestring[0] == '@' &&
            root_item_comp && cJSON_IsObject(root_item_comp)) {
            
            const char *comp_id_str = id_item_comp->valuestring + 1; // Skip '@'
            if (g_current_operation_mode == RENDER_MODE) {
                cJSON *duplicated_root = cJSON_Duplicate(root_item_comp, true);
                if (duplicated_root) {
                    lvgl_json_register_ptr(comp_id_str, "component_json_node", (void*)duplicated_root);
                    return (char*)1; // Success, non-NULL arbitrary pointer
                } else {
                    LOG_ERR_JSON(node, "Component Error: Failed to duplicate root for component '%s'", comp_id_str);
                    return NULL;
                }
            } else { // TRANSPILE_MODE
                // TODO: How to handle component definitions in transpile mode?
                // Option 1: Store them in a global C structure (complex).
                // Option 2: Assume components are pre-transpiled into linkable C functions.
                // For now, log and return a placeholder.
                LOG_INFO("TRANSPILE_MODE: Component definition '%s' encountered. Transpilation of components is not yet fully supported.", comp_id_str);
                return lv_strdup("transpile_placeholder_component_def");
            }
        } else {
            LOG_ERR_JSON(node, "Component Error: Invalid 'component' definition. Requires 'id' (string starting with '@') and 'root' (object).");
            return NULL;
        }
    } else if (strcmp(type_str, "use-view") == 0) {
        bool view_context_set_locally = false;
        cJSON *id_item_use_view = cJSON_GetObjectItemCaseSensitive(node, "id");
        if (id_item_use_view && cJSON_IsString(id_item_use_view) && id_item_use_view->valuestring && id_item_use_view->valuestring[0] == '@') {
            const char *view_id_str = id_item_use_view->valuestring + 1; // Skip '@'

            if (g_current_operation_mode == RENDER_MODE) {
                cJSON *component_root_json_node = (cJSON*)lvgl_json_get_registered_ptr(view_id_str, "component_json_node");
                if (component_root_json_node) {
                    LOG_INFO("RENDER_MODE: Using view '%s', named_path_prefix for component render: '%s'", view_id_str, named_path_prefix ? named_path_prefix : "ROOT");
                    cJSON* context_for_view_item = cJSON_GetObjectItemCaseSensitive(node, "context");
                    if (context_for_view_item && cJSON_IsObject(context_for_view_item)) {
                        set_current_context(context_for_view_item);
                        view_context_set_locally = true;
                    }
                    
                    char* component_root_entity = render_json_node_internal(component_root_json_node, parent_render_mode, named_path_prefix, NULL, NULL, NULL);

                    cJSON *do_attrs_json = cJSON_GetObjectItemCaseSensitive(node, "do");
                    if (component_root_entity && do_attrs_json && cJSON_IsObject(do_attrs_json)) {
                        LOG_INFO("RENDER_MODE: Applying 'do' attributes to component '%s' root %p", view_id_str, component_root_entity);
                        const char *comp_root_actual_type_str = "obj";
                        cJSON *comp_root_type_item = cJSON_GetObjectItemCaseSensitive(component_root_json_node, "type");
                        if (comp_root_type_item && cJSON_IsString(comp_root_type_item)) comp_root_actual_type_str = comp_root_type_item->valuestring;
                        const char *comp_root_create_type_str = (strcmp(comp_root_actual_type_str, "grid") == 0) ? "obj" : comp_root_actual_type_str;
                        bool is_comp_root_widget = (strcmp(comp_root_actual_type_str, "style") != 0); // Basic check
                        char comp_root_registry_type_name[64];
                        snprintf(comp_root_registry_type_name, sizeof(comp_root_registry_type_name), "lv_%s_t", is_comp_root_widget ? comp_root_create_type_str : comp_root_actual_type_str);

                        char path_for_do_block_context[256] = {0}; // Calculate as before
                        // ... (path calculation logic from original code) ...
                        const char* base_for_do_path = named_path_prefix;
                        cJSON* comp_root_id_item = cJSON_GetObjectItemCaseSensitive(component_root_json_node, "id");
                        if (comp_root_id_item && cJSON_IsString(comp_root_id_item) && comp_root_id_item->valuestring[0] == '@') {
                            const char* comp_root_id_val = comp_root_id_item->valuestring + 1;
                            if (base_for_do_path && base_for_do_path[0]) snprintf(path_for_do_block_context, sizeof(path_for_do_block_context)-1, "%s:%s", base_for_do_path, comp_root_id_val);
                            else strncpy(path_for_do_block_context, comp_root_id_val, sizeof(path_for_do_block_context)-1);
                        } else if (base_for_do_path) strncpy(path_for_do_block_context, base_for_do_path, sizeof(path_for_do_block_context)-1);


                        apply_setters_and_attributes_internal(
                            do_attrs_json, (void*)component_root_entity, NULL,
                            comp_root_actual_type_str, comp_root_create_type_str, is_comp_root_widget,
                            is_comp_root_widget ? (lv_obj_t*)component_root_entity : NULL, NULL,
                            path_for_do_block_context, comp_root_registry_type_name);
                    }
                    if (view_context_set_locally) set_current_context(original_context_for_this_node_call);
                    return component_root_entity;
                } else {
                    LOG_WARN_JSON(node, "RENDER_MODE: Use-View Error: Component '%s' not found. Creating fallback.", view_id_str);
                    lv_obj_t *fallback_label = lv_label_create(parent_render_mode);
                    if(fallback_label) { /* set text */ }
                    if (view_context_set_locally) set_current_context(original_context_for_this_node_call);
                    return (char*)fallback_label;
                }
            } else { // TRANSPILE_MODE
                LOG_INFO("TRANSPILE_MODE: 'use-view' for '%s' encountered. Full transpilation requires pre-transpiled components or advanced handling.", view_id_str);
                // TODO: Transpile 'use-view'. This might involve calling a C function that represents the component.
                // For now, return a placeholder string.
                if (view_context_set_locally) set_current_context(original_context_for_this_node_call); // Should not be needed if no real work
                return lv_strdup("transpile_placeholder_use_view");
            }
        } else { 
            LOG_ERR_JSON(node, "Use-View Error: Invalid 'use-view' definition. Requires 'id' (string starting with '@').");
            return NULL;
        }
    } else if (strcmp(type_str, "context") == 0) {
        cJSON *values_item = cJSON_GetObjectItemCaseSensitive(node, "values");
        cJSON *for_item = cJSON_GetObjectItemCaseSensitive(node, "for");

        if (values_item && cJSON_IsObject(values_item) && for_item && cJSON_IsObject(for_item)) {
            set_current_context(values_item); 
            // Pass through relevant parameters for the current mode
            char* result_entity_str = render_json_node_internal(for_item, parent_render_mode, named_path_prefix, out_c, out_h, current_parent_c_var_name);
            set_current_context(original_context_for_this_node_call); 
            return result_entity_str;
        } else {
            LOG_ERR_JSON(node, "'context' type node Error: Requires 'values' (object) and 'for' (object) properties.");
            return NULL;
        }
    }

    // --- If not a special type handled above, proceed with generic node processing ---
    cJSON* context_property_on_this_node = cJSON_GetObjectItemCaseSensitive(node, "context");
    if (context_property_on_this_node && cJSON_IsObject(context_property_on_this_node)) {
        set_current_context(context_property_on_this_node);
        context_was_locally_changed_by_this_node = true;
    }
"""

    c_code += "    // 1. Determine Object ID for registration and path construction\n"
    c_code += "    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(node, \"id\");\n"
    c_code += "    const char *id_str_val = NULL; // Used in both modes for path construction and naming\n"
    c_code += "    char current_node_path_segment[128] = {0}; // Segment this node adds to the path\n"
    c_code += "    if (id_item && cJSON_IsString(id_item) && id_item->valuestring && id_item->valuestring[0] == '@') {\n"
    c_code += "        id_str_val = id_item->valuestring + 1;\n"
    c_code += "        strncpy(current_node_path_segment, id_str_val, sizeof(current_node_path_segment) -1);\n"
    c_code += "    } else if (id_item && cJSON_IsString(id_item)) {\n"
    c_code += "        LOG_WARN_JSON(id_item, \"Render Warning: 'id' property '%s' should start with '@' for registration/path construction. Treating as non-identifying.\", id_item->valuestring);\n"
    c_code += "    }\n\n"

    c_code += "    // Construct full path for this node, to be used for its registration and as prefix for its children/named attributes\n"
    c_code += "    char effective_path_for_node_and_children[256];\n"
    c_code += "    if (named_path_prefix && named_path_prefix[0] != '\\0') {\n"
    c_code += "        if (current_node_path_segment[0] != '\\0') {\n"
    c_code += "            snprintf(effective_path_for_node_and_children, sizeof(effective_path_for_node_and_children)-1, \"%s:%s\", named_path_prefix, current_node_path_segment);\n"
    c_code += "        } else {\n"
    c_code += "            strncpy(effective_path_for_node_and_children, named_path_prefix, sizeof(effective_path_for_node_and_children)-1);\n"
    c_code += "        }\n"
    c_code += "    } else {\n"
    c_code += "        strncpy(effective_path_for_node_and_children, current_node_path_segment, sizeof(effective_path_for_node_and_children)-1);\n"
    c_code += "    }\n"
    c_code += "    effective_path_for_node_and_children[sizeof(effective_path_for_node_and_children)-1] = '\\0';\n\n"


    c_code += "    // 2. Create the LVGL Object / Resource\n"
    c_code += "    void *created_entity_render_mode = NULL; // For RENDER_MODE\n"
    c_code += "    // created_entity_c_var_name_transpile is already declared for TRANSPILE_MODE\n"
    c_code += "    bool is_widget = true;\n"
    c_code += "    char type_name_for_registry_buf[64] = \"lv_obj_t\"; // Default, will be updated\n"
    c_code += "    const char *actual_type_str_for_node = type_str; //This is the 'type_str' from JSON\n"
    c_code += "    const char *create_type_str_for_node = type_str; //This might become 'obj' if type_str is 'grid'\n\n"

    c_code += "    if (strcmp(actual_type_str_for_node, \"grid\") == 0) {\n"
    c_code += "        create_type_str_for_node = \"obj\"; // Grids are created as generic lv_obj\n"
    c_code += "        LOG_DEBUG(\"Processing 'grid' type. Will create as 'lv_obj' and apply grid layout properties.\");\n"
    c_code += "    }\n\n"

    c_code += "    if (g_current_operation_mode == RENDER_MODE) {\n"
    c_code += "        // --- RENDER_MODE: Create actual LVGL entities ---\n"
    c_code += "        bool custom_creator_called = false;\n"
    first_custom_type = True
    # Iterate over custom creators first
    for obj_type_py, creator_func_py in custom_creators_map.items():
        if first_custom_type:
            c_code += f"        if (strcmp(actual_type_str_for_node, \"{obj_type_py}\") == 0) {{\n"
            first_custom_type = False
        else:
            c_code += f"        else if (strcmp(actual_type_str_for_node, \"{obj_type_py}\") == 0) {{\n"
        c_code +=  "            const char* name_for_custom_creator = (id_str_val && id_str_val[0]) ? id_str_val : effective_path_for_node_and_children;\n"
        c_code += f"            if (!name_for_custom_creator || !name_for_custom_creator[0]) {{ name_for_custom_creator = \"anonymous_{obj_type_py}\"; }}\n"
        c_code += f"            created_entity_render_mode = (void*){creator_func_py}(name_for_custom_creator);\n"
        c_code += f"            if (!created_entity_render_mode) {{ \n"
        c_code += f"                 LOG_ERR_JSON(node, \"RENDER_MODE: Custom creator {creator_func_py} for name '%s' (type {obj_type_py}) returned NULL.\", name_for_custom_creator);\n"
        c_code +=  "                 if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
        c_code +=  "                 return NULL; \n"
        c_code +=  "            }}\n"
        # Most custom creators are for non-widget types like styles, fonts.
        # If a custom creator *does* make a widget, this logic might need adjustment or the creator map needs to specify this.
        c_code +=  "            is_widget = false; // Assume custom creators are for non-widgets unless specified otherwise\n"
        c_code += f"            snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), \"lv_{obj_type_py}_t\");\n"
        c_code +=  "            custom_creator_called = true;\n"
        c_code +=  "        }\n" # Corrected closing brace

    # Handle 'with' type if it's not in custom_creators_map (it shouldn't be)
    # Ensure 'with' is properly chained with 'else if' or 'if' depending on whether custom_creators_map was empty
    with_keyword = "if" if first_custom_type else "else if" # first_custom_type is true if map was empty
    c_code += f"        {with_keyword} (strcmp(actual_type_str_for_node, \"with\") == 0) {{\n"
    c_code += "            created_entity_render_mode = (void*)parent_render_mode; /* 'with' acts on parent or specified obj */\n"
    c_code += "            is_widget = true; /* Operates on widgets */ \n"
    c_code += "            snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), \"lv_obj_t\"); \n"
    c_code += "            actual_type_str_for_node = \"obj\"; /* Treat as obj for attribute application */ \n"
    c_code += "            create_type_str_for_node = \"obj\"; \n"
    c_code += "            LOG_DEBUG(\"RENDER_MODE: Processing 'with' node. Target entity will be resolved by apply_setters_and_attributes.\");\n"
    c_code += "            custom_creator_called = true; /* Mark as handled to skip standard widget creation */ \n"
    c_code += "        }}\n"

    # Standard widget creation path if not handled by custom creators or 'with'
    c_code += "        if (!custom_creator_called) {\n"
    c_code += "            // This is the path for standard widgets not covered by custom_creators_map\n"
    c_code += "            char create_func_name[64];\n"
    c_code += "            snprintf(create_func_name, sizeof(create_func_name), \"lv_%s_create\", create_type_str_for_node);\n"
    c_code += "            const invoke_table_entry_t* create_entry = find_invoke_entry(create_func_name);\n"
    c_code += "            if (!create_entry) {\n"
    c_code += "                 LOG_ERR_JSON(node, \"RENDER_MODE: Create function '%s' not found for type '%s'.\", create_func_name, actual_type_str_for_node);\n"
    c_code += "                 if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
    c_code += "                 return NULL;\n"
    c_code += "            }\n"
    c_code += "            lv_obj_t* new_widget = NULL;\n"
    c_code += "            // Using the 6-argument invoke signature for RENDER_MODE widget creation:\n"
    c_code += "            if (!create_entry->invoke(create_entry, \n"
    c_code += "                                     (void*)parent_render_mode,      /* target_obj_ptr_render_mode */ \n"
    c_code += "                                     NULL,                           /* target_obj_c_name_transpile */ \n"
    c_code += "                                     &new_widget,                    /* result_dest_render_mode */ \n"
    c_code += "                                     NULL,                           /* result_c_name_transpile */ \n"
    c_code += "                                     NULL                            /* json_args_array - create functions don't use this directly */\n"
    c_code += "                                     )) { \n"
    c_code += "                 LOG_ERR_JSON(node, \"RENDER_MODE: Failed to invoke %s.\", create_func_name);\n"
    c_code += "                 if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
    c_code += "                 return NULL;\n"
    c_code += "            }\n"
    c_code += "            if (!new_widget) { \n"
    c_code += "                 LOG_ERR_JSON(node, \"RENDER_MODE: %s returned NULL.\", create_func_name); \n"
    c_code += "                 if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call); \n"
    c_code += "                 return NULL; \n"
    c_code += "            }\n"
    c_code += "            created_entity_render_mode = (void*)new_widget;\n"
    c_code += "            is_widget = true;\n"
    c_code += "            snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), \"lv_%s_t\", create_type_str_for_node);\n"
    c_code += "            // Register if ID is present and path is valid\n"
    c_code += "            if (id_str_val && id_str_val[0] && effective_path_for_node_and_children[0] != '\\0') {\n"
    c_code += "                lvgl_json_register_ptr(effective_path_for_node_and_children, type_name_for_registry_buf, created_entity_render_mode);\n"
    c_code += "                LOG_INFO(\"RENDER_MODE: Registered widget %p as '%s' (type %s)\", created_entity_render_mode, effective_path_for_node_and_children, type_name_for_registry_buf);\n"
    c_code += "            } else if (id_str_val && id_str_val[0]) {\n"
    c_code += "                LOG_WARN_JSON(node, \"RENDER_MODE: Widget with id '%s' created, but path is empty. Not registered by id path.\", id_str_val);\n"
    c_code += "            }\n"
    c_code += "        } // End if (!custom_creator_called)\n"
    c_code += "    } else { // TRANSPILE_MODE\n"
    c_code += "        g_transpile_var_counter++;\n"
    c_code += "        created_entity_c_var_name_transpile = transpile_generate_c_var_name(id_str_val, actual_type_str_for_node, g_transpile_var_counter);\n"
    c_code += "        if (!created_entity_c_var_name_transpile) { LOG_ERR(\"TRANSPILE_MODE: Failed to generate C variable name.\"); if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call); return NULL; }\n"
    c_code += "\n"
    c_code += "        // Determine C type for declaration (e.g., lv_obj_t*, lv_style_t*)\n"
    c_code += "        char c_type_for_declaration[64];\n"
    c_code += "        if (strcmp(actual_type_str_for_node, \"style\") == 0) { // TODO: expand for other non-widget types\n"
    c_code += "            is_widget = false;\n"
    c_code += "            snprintf(c_type_for_declaration, sizeof(c_type_for_declaration), \"lv_style_t\");\n"
    c_code += "        } else {\n"
    c_code += "            is_widget = true; // All others assumed widgets for now\n"
    c_code += "            // For widgets, use lv_obj_t* for declaration, actual type is for functions like lv_button_set_...\n"
    c_code += "            // Or use specific types like lv_button_t* if preferred and known.\n"
    c_code += "            snprintf(c_type_for_declaration, sizeof(c_type_for_declaration), \"lv_obj_t\"); // Default to lv_obj_t for widgets\n"
    c_code += "            // If you want specific types: snprintf(c_type_for_declaration, sizeof(c_type_for_declaration), \"lv_%s_t\", create_type_str_for_node);\n"
    c_code += "        }\n"
    c_code += "        snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), \"%s *\", c_type_for_declaration); // For registry, store as "lv_obj_t *" etc.\n"
    c_code += "\n"
    c_code += "        // Write declaration to header file (if it's a global or needs to be known across C files)\n"
    c_code += "        // For now, let's assume they are declared locally within the create_ui function.\n"
    c_code += "        // transpile_write_line(out_h, \"extern %s *%s; // JSON id: %s, type: %s\", c_type_for_declaration, created_entity_c_var_name_transpile, id_str_val ? id_str_val : \"N/A\", actual_type_str_for_node);\n"
    c_code += "\n"
    c_code += "        // Write creation/initialization to C file\n"
    c_code += "        const char* parent_var_for_transpile = current_parent_c_var_name ? current_parent_c_var_name : \"NULL\";\n"
    c_code += "        if (strcmp(actual_type_str_for_node, \"style\") == 0) { // Handle style creation\n"
    c_code += "             transpile_write_line(out_c, \"    %s %s; // JSON id: %s\", c_type_for_declaration, created_entity_c_var_name_transpile, id_str_val ? id_str_val : \"N/A\");\n"
    c_code += "             transpile_write_line(out_c, \"    lv_style_init(&%s);\", created_entity_c_var_name_transpile);\n"
    c_code += "             // Styles are registered with their C variable name (which is a struct, not pointer) but registry takes void*\n"
    c_code += "             // This specific registration needs care. For now, let's assume 'named' attribute handles it by path to var name string.\n"
    c_code += "        } else if (strcmp(actual_type_str_for_node, \"with\") == 0) {\n"
    c_code += "            // 'with' doesn't create a new variable; it operates on an existing one.\n"
    c_code += "            // The target C variable name will be resolved within apply_setters_and_attributes_internal.\n"
    c_code += "            // So, created_entity_c_var_name_transpile might be replaced there or this one ignored.\n"
    c_code += "            LOG_DEBUG(\"TRANSPILE_MODE: 'with' node, C var %s is placeholder.\", created_entity_c_var_name_transpile);\n"
    c_code += "            actual_type_str_for_node = \"obj\"; create_type_str_for_node = \"obj\"; // Treat as obj for attribute application\n"
    c_code += "        } else { // Handle widget creation (lv_obj_t *, lv_button_t *, etc.)\n"
    c_code += "            transpile_write_line(out_c, \"    %s *%s = lv_%s_create(%s); // JSON id: %s\", c_type_for_declaration, created_entity_c_var_name_transpile, create_type_str_for_node, parent_var_for_transpile, id_str_val ? id_str_val : \"N/A\");\n"
    c_code += "        }\n"
    c_code += "\n"
    c_code += "        // Register C variable name string if node has an ID path\n"
    c_code += "        if (id_str_val && id_str_val[0] && effective_path_for_node_and_children[0] != '\\0') {\n"
    c_code += "            // Store the C variable name string itself (lv_strdup'd by 'named' handler later or here if not using 'named')\n"
    c_code += "            // This is for runtime lookup if needed, or if 'named' attribute isn't used for this node.\n"
    c_code += "            // If 'named' attribute is present, it will call lvgl_json_register_ptr with 'c_var_name' type.\n"
    c_code += "            // For now, let 'named' attribute handle the registration of the C var name string.\n"
    c_code += "            LOG_INFO(\"TRANSPILE_MODE: C variable '%s' (type %s) generated for path '%s'. Registration via 'named' attr if present.\", created_entity_c_var_name_transpile, type_name_for_registry_buf, effective_path_for_node_and_children);\n"
    c_code += "        } else if (id_str_val && id_str_val[0]) {\n"
    c_code += "            LOG_WARN(\"TRANSPILE_MODE: C variable for id '%s' created, but path is empty. Not registered by id path.\", id_str_val);\n"
    c_code += "        }\n"
    c_code += "    }\n"
    c_code += "\n"

    grid_setup_c_code = """
    if (g_current_operation_mode == RENDER_MODE && created_entity_render_mode && is_widget && strcmp(actual_type_str_for_node, "grid") == 0) {
        lv_obj_t* grid_obj = (lv_obj_t*)created_entity;
        cJSON *cols_item_json = cJSON_GetObjectItemCaseSensitive(node, "cols");
        cJSON *rows_item_json = cJSON_GetObjectItemCaseSensitive(node, "rows");

        int32_t* col_dsc_array = NULL;
        int32_t* row_dsc_array = NULL;
        bool grid_setup_ok = true;

        if (cols_item_json && cJSON_IsArray(cols_item_json)) {
            int num_cols = cJSON_GetArraySize(cols_item_json);
            col_dsc_array = (int32_t*)LV_MALLOC(sizeof(int32_t) * (num_cols + 1));
            if (col_dsc_array) {
                for (int i = 0; i < num_cols; i++) {
                    cJSON *val_item = cJSON_GetArrayItem(cols_item_json, i);
                    if (!unmarshal_value(val_item, "int32_t", &col_dsc_array[i], created_entity)) { 
                        LOG_ERR_JSON(val_item, "Grid Error: Failed to parse 'cols' array item %d as int32_t.", i);
                        LV_FREE(col_dsc_array); col_dsc_array = NULL; grid_setup_ok = false;
                        break;
                    }
                }
                if (col_dsc_array) col_dsc_array[num_cols] = LV_GRID_TEMPLATE_LAST;
            } else {
                LOG_ERR("Grid Error: Failed to allocate memory for column descriptors.");
                grid_setup_ok = false;
            }
        } else { 
            LOG_ERR_JSON(node, "Grid Error: 'cols' array is missing or not an array for grid type.");
            grid_setup_ok = false;
        }

        if (grid_setup_ok && rows_item_json && cJSON_IsArray(rows_item_json)) {
            int num_rows = cJSON_GetArraySize(rows_item_json);
            row_dsc_array = (int32_t*)LV_MALLOC(sizeof(int32_t) * (num_rows + 1));
            if (row_dsc_array) {
                for (int i = 0; i < num_rows; i++) {
                    cJSON *val_item = cJSON_GetArrayItem(rows_item_json, i);
                    if (!unmarshal_value(val_item, "int32_t", &row_dsc_array[i], created_entity)) { 
                        LOG_ERR_JSON(val_item, "Grid Error: Failed to parse 'rows' array item %d as int32_t.", i);
                        LV_FREE(row_dsc_array); row_dsc_array = NULL; grid_setup_ok = false;
                        break;
                    }
                }
                if (row_dsc_array) row_dsc_array[num_rows] = LV_GRID_TEMPLATE_LAST;
            } else {
                LOG_ERR("Grid Error: Failed to allocate memory for row descriptors.");
                grid_setup_ok = false;
            }
        } else if (grid_setup_ok) { 
             LOG_ERR_JSON(node, "Grid Error: 'rows' array is missing or not an array for grid type.");
             grid_setup_ok = false;
        }

        if (grid_setup_ok && col_dsc_array && row_dsc_array) {
            char temp_name_buf[64]; 
            snprintf(temp_name_buf, sizeof(temp_name_buf), "grid_col_dsc_%p", (void*)grid_obj);
            lvgl_json_register_ptr(temp_name_buf, "lv_coord_array_temp", (void*)col_dsc_array); 
            snprintf(temp_name_buf, sizeof(temp_name_buf), "grid_row_dsc_%p", (void*)grid_obj);
            lvgl_json_register_ptr(temp_name_buf, "lv_coord_array_temp", (void*)row_dsc_array); 
            
            lv_obj_set_grid_dsc_array(grid_obj, col_dsc_array, row_dsc_array);
        } else {
            if (col_dsc_array) { LV_FREE(col_dsc_array); } 
            if (row_dsc_array) { LV_FREE(row_dsc_array); } 
            LOG_ERR_JSON(node, "Grid Error: Failed to set up complete grid descriptors. Grid layout will not apply.");
        }
    } else if (g_current_operation_mode == TRANSPILE_MODE && created_entity_c_var_name_transpile && is_widget && strcmp(actual_type_str_for_node, "grid") == 0) {
        // TODO: Transpile grid setup. This involves generating C code to define the col/row descriptor arrays
        // and calling lv_obj_set_grid_dsc_array. This is complex due to array definitions.
        transpile_write_line(out_c, "    // TODO: TRANSPILE_MODE: Grid layout for '%s' (type %s). Requires generating C arrays for col/row descriptors.", created_entity_c_var_name_transpile, actual_type_str_for_node);
        // Placeholder:
        // transpile_write_line(out_c, "    // static const lv_coord_t col_dsc_%s[] = { ... LV_GRID_TEMPLATE_LAST };", created_entity_c_var_name_transpile);
        // transpile_write_line(out_c, "    // static const lv_coord_t row_dsc_%s[] = { ... LV_GRID_TEMPLATE_LAST };", created_entity_c_var_name_transpile);
        // transpile_write_line(out_c, "    // lv_obj_set_grid_dsc_array(%s, col_dsc_%s, row_dsc_%s);", created_entity_c_var_name_transpile, created_entity_c_var_name_transpile, created_entity_c_var_name_transpile);

    }
"""
    c_code += grid_setup_c_code

    c_code += "    // 3. Set Properties, handle children, etc., using the new function\n"
    c_code += "    if (g_current_operation_mode == RENDER_MODE && created_entity_render_mode) {\n"
    c_code += "        if (!apply_setters_and_attributes(\n"
    c_code += "                node,                                   // Attributes are from the node itself\n"
    c_code += "                created_entity,                         // Target is the newly created entity\n"
    c_code += "                actual_type_str_for_node,               // Actual type (e.g. \"button\", \"grid\", \"style\")\n"
    c_code += "                create_type_str_for_node,               // Create type (e.g. \"button\", \"obj\" for grid)\n"
    c_code += "                is_widget,                              // Is it a widget?\n"
    c_code += "                is_widget ? (lv_obj_t*)created_entity : NULL, // Parent for 'children' attribute\n"
    c_code += "                effective_path_for_node_and_children,   // Path prefix for 'named' and 'children' in node\n"
    c_code += "                type_name_for_registry_buf              // Type name for 'named' registration (e.g. lv_button_t)\n"
    c_code += "            )) {\n"
    c_code += "            LOG_ERR_JSON(node, \"Failed to apply attributes or process children for node type '%s'.\", actual_type_str_for_node);\n"
    c_code += "            // Decide if this should be fatal and return NULL. For now, log and continue.\n"
    c_code += "            // If created_entity is a widget, it might need to be deleted if this is considered a hard failure.\n"
    c_code += "        }\n"
    c_code += "    } else if (strcmp(actual_type_str_for_node, \"with\") != 0) {\n"
    c_code += "         // If created_entity is NULL and it wasn't a 'with' node (which doesn't create), it's an error from creation phase.\n"
    c_code += "         LOG_ERR_JSON(node, \"Entity creation failed for type '%s', cannot apply attributes.\", actual_type_str_for_node);\n"
    c_code += "         // Fall through to context restoration and return NULL.\n"
    c_code += "    }\n\n"

    c_code += "    // Context restoration\n"
    c_code += "    if (context_was_locally_changed_by_this_node) {\n"
    c_code += "        set_current_context(original_context_for_this_node_call);\n"
    c_code += "    }\n\n"
    c_code += "    return created_entity;\n"
    c_code += "}\n\n"

    # Main entry point function
    c_code += "// --- Public API --- \n\n"
    c_code += "bool lvgl_json_render_ui(cJSON *root_json, lv_obj_t *implicit_root_parent) {\n"
    c_code += "    if (!root_json) {\n"
    c_code += "        LOG_ERR(\"Render Error: root_json is NULL.\");\n"
    c_code += "        return false;\n"
    c_code += "    }\n"
    c_code += "    lv_obj_t* effective_parent = implicit_root_parent;\n"
    c_code += "    if (!effective_parent) {\n"
    c_code += "        LOG_WARN(\"Render Warning: implicit_root_parent is NULL. Using lv_screen_active().\");\n"
    c_code += "        effective_parent = lv_screen_active();\n"
    c_code += "        if (!effective_parent) {\n"
    c_code += "             LOG_ERR(\"Render Error: Cannot get active screen.\");\n"
    c_code += "             return false;\n"
    c_code += "        }\n"
    c_code += "    }\n\n"

    c_code += "    bool overall_success = true;\n"
    c_code += "    if (cJSON_IsArray(root_json)) {\n"
    c_code += "        cJSON *node_in_array = NULL;\n"
    c_code += "        cJSON_ArrayForEach(node_in_array, root_json) {\n"
    c_code += "            if (render_json_node(node_in_array, effective_parent, NULL) == NULL) { \n"
    c_code += "                // Check if the node was a component definition, which returns non-NULL on success (like (void*)1)\n"
    c_code += "                // but is not an actual LVGL object. Only true failures (NULL) should stop processing here.\n"
    c_code += "                cJSON *type_item_check = cJSON_GetObjectItemCaseSensitive(node_in_array, \"type\");\n"
    c_code += "                if (type_item_check && cJSON_IsString(type_item_check) && strcmp(type_item_check->valuestring, \"component\") == 0) {\n"
    c_code += "                     // If component registration failed, render_json_node returns NULL. This is an error.\n"
    c_code += "                     LOG_ERR_JSON(node_in_array, \"Render Error: Failed to process top-level 'component' definition. Aborting.\");\n"
    c_code += "                     overall_success = false; break;\n"
    c_code += "                } else {\n"
    c_code += "                     LOG_ERR_JSON(node_in_array, \"Render Error: Failed to render top-level node. Aborting.\");\n"
    c_code += "                     overall_success = false; break;\n"
    c_code += "                }\n"
    c_code += "            }\n"
    c_code += "        }\n"
    c_code += "    } else if (cJSON_IsObject(root_json)) {\n"
    c_code += "        if (render_json_node(root_json, effective_parent, NULL) == NULL) {\n"
    c_code += "            cJSON *type_item_check = cJSON_GetObjectItemCaseSensitive(root_json, \"type\");\n"
    c_code += "            if (!(type_item_check && cJSON_IsString(type_item_check) && strcmp(type_item_check->valuestring, \"component\") == 0)) {\n"
    c_code += "                 overall_success = false;\n"
    c_code += "            } else {\n"
    c_code += "                 // Component definition failed at root level\n"
    c_code += "                 LOG_ERR_JSON(root_json, \"Render Error: Failed to process root 'component' definition.\");\n"
    c_code += "                 overall_success = false;\n"
    c_code += "            }\n"
    c_code += "        }\n"
    c_code += "    } else {\n"
    c_code += "        LOG_ERR_JSON(root_json, \"Render Error: root_json must be a JSON object or array.\");\n"
    c_code += "        overall_success = false;\n"
    c_code += "    }\n\n"
    c_code += "    if (!overall_success) {\n"
    c_code += "         LOG_ERR(\"UI Rendering failed.\");\n"
    c_code += "    } else {\n"
    c_code += "         LOG_INFO(\"UI Rendering completed successfully.\");\n"
    c_code += "    }\n"
    c_code += "    return overall_success;\n"
    c_code += "}\n\n"

    return c_code