# code_gen/renderer.py
import logging
from type_utils import lvgl_type_to_widget_name

logger = logging.getLogger(__name__)

def generate_renderer(custom_creators_map):
    """Generates the C code for parsing the JSON UI and rendering it."""
    # custom_creators_map: {'style': 'lv_style_create_managed', ...}

    c_code = "// --- JSON UI Renderer ---\n\n"
    c_code += "#include <stdio.h> // For debug prints\n\n"

    c_code += "// Forward declarations\n"
    c_code += "static void* render_json_node(cJSON *node, lv_obj_t *parent, const char *named_path_prefix);\n" # Return void*
    c_code += "static bool apply_setters_and_attributes(cJSON *attributes_json_obj, void *target_entity, const char *target_actual_type_str, const char *target_create_type_str, bool target_is_widget, lv_obj_t *parent_for_children_attr, const char *path_prefix_for_named_and_children, const char *default_type_name_for_registry_if_named);\n"
    c_code += "static const invoke_table_entry_t* find_invoke_entry(const char *name);\n"
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent);\n"
    c_code += "extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);\n"
    c_code += "extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);\n"
    c_code += "static void set_current_context(cJSON* new_context);\n"
    c_code += "static cJSON* get_current_context(void); // Already declared in source template, but good for clarity\n"


    # Include custom creator function prototypes
    for type_name, creator_func in custom_creators_map.items():
         c_type = f"lv_{type_name}_t"
         c_code += f"extern {c_type}* {creator_func}(const char *name);\n"
    c_code += "\n"

    # apply_setters_and_attributes function
    c_code += """
static bool apply_setters_and_attributes(
    cJSON *attributes_json_obj,
    void *target_entity,
    const char *target_actual_type_str, // e.g., "button", "style", "obj" (real type of target_entity)
    const char *target_create_type_str, // e.g., "obj" if target_actual_type_str is "grid" (type used for lv_obj_set_... fallbacks)
    bool target_is_widget,
    lv_obj_t *explicit_parent_for_children_attr, // Parent if "children" attr is found, usually target_entity if widget
    const char *path_prefix_for_named_and_children, // Path prefix for "named" attr registration and for children within attributes_json_obj
    const char *default_type_name_for_registry_if_named // e.g. "lv_obj_t", "lv_style_t" if "named" is processed
) {
    if (!attributes_json_obj || !cJSON_IsObject(attributes_json_obj) || !target_entity) {
        LOG_ERR("apply_setters_and_attributes: Invalid arguments.");
        return false;
    }

    LOG_DEBUG("Applying attributes with path_prefix: %s to entity %p of type %s (create_type %s)",
        path_prefix_for_named_and_children ? path_prefix_for_named_and_children : "NULL",
        target_entity, target_actual_type_str, target_create_type_str);

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


        if (strcmp(prop_name, "named") == 0 && cJSON_IsString(prop_item)) {
            char *named_value_str = NULL;
            if (unmarshal_value(prop_item, "char *", &named_value_str, target_entity)) {
                char full_named_path_buf[256] = {0};
                if (path_prefix_for_named_and_children && path_prefix_for_named_and_children[0] != '\\0') {
                    snprintf(full_named_path_buf, sizeof(full_named_path_buf) - 1, "%s:%s", path_prefix_for_named_and_children, named_value_str);
                } else {
                    strncpy(full_named_path_buf, named_value_str, sizeof(full_named_path_buf) - 1);
                }
                
                if (default_type_name_for_registry_if_named && default_type_name_for_registry_if_named[0]) {
                     lvgl_json_register_ptr(full_named_path_buf, default_type_name_for_registry_if_named, target_entity);
                     LOG_INFO("Registered entity %p as '%s' (type %s) via 'named' attribute.", target_entity, full_named_path_buf, default_type_name_for_registry_if_named);
                     // Update current_children_base_path for subsequent children within this attribute set
                     strncpy(current_children_base_path, full_named_path_buf, sizeof(current_children_base_path) - 1);
                     current_children_base_path[sizeof(current_children_base_path) - 1] = '\\0';
                } else {
                    LOG_WARN_JSON(prop_item, "'named' attribute used, but no valid type_name_for_registry provided for '%s'. Entity %p not registered by this 'named' attribute.", named_value_str, target_entity);
                }
            } else {
                LOG_ERR_JSON(prop_item, "Failed to resolve 'named' property value string: '%s'", prop_item->valuestring);
            }
            continue;
        }

        if (strcmp(prop_name, "children") == 0) {
            if (!cJSON_IsArray(prop_item)) {
                LOG_ERR_JSON(prop_item, "'children' property must be an array.");
                // Continue processing other attributes, but this is an error.
                continue;
            }
            if (!target_is_widget || !explicit_parent_for_children_attr) {
                LOG_ERR_JSON(prop_item, "'children' attribute found, but target entity is not a widget or parent_for_children_attr is NULL. Cannot add children.");
                continue;
            }
            LOG_DEBUG("Processing 'children' for entity %p under path prefix '%s'", target_entity, current_children_base_path);
            cJSON *child_node_json = NULL;
            cJSON_ArrayForEach(child_node_json, prop_item) {
                if (render_json_node(child_node_json, explicit_parent_for_children_attr, current_children_base_path) == NULL) {
                    LOG_ERR_JSON(child_node_json, "Failed to render child node from 'children' attribute. Aborting siblings for this 'children' array.");
                    // Depending on desired strictness, could return false here or just log and continue.
                    // For now, log and let other attributes/children process.
                    break; 
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
            if (!do_block_for_with_json || !cJSON_IsObject(do_block_for_with_json)) {
                LOG_ERR_JSON(prop_item, "'with' block is missing 'do' object or it's not an object. Skipping.");
                continue;
            }

            lv_obj_t *with_target_obj = NULL;
            if (!unmarshal_value(obj_to_run_with_json, "lv_obj_t *", &with_target_obj, target_entity)) { // Pass current target_entity as implicit_parent for context in unmarshal
                LOG_ERR_JSON(obj_to_run_with_json, "Failed to unmarshal 'obj' for 'with' block. Skipping 'with'.");
                continue;
            }
            if (!with_target_obj) {
                LOG_ERR_JSON(obj_to_run_with_json, "'obj' for 'with' block resolved to NULL. Skipping 'with'.");
                continue;
            }
            
            LOG_INFO("Applying 'with.do' attributes to target %p (resolved from 'with.obj')", with_target_obj);
            // For 'with.do', the target is the resolved 'with_target_obj'. Assume it's an 'obj' type.
            // The named path context for children/named inside this 'do' block should be the same as the 'with' block's context.
            // The 'default_type_name_for_registry_if_named' for 'with_target_obj' is "lv_obj_t".
            if (!apply_setters_and_attributes(
                    do_block_for_with_json,
                    with_target_obj,
                    "obj", // Actual type of with_target_obj is lv_obj_t*, specific type (button, label) is unknown here.
                    "obj", // Create type for fallbacks.
                    true,  // It's an lv_obj_t*.
                    with_target_obj, // Parent for children defined in the 'do' block.
                    path_prefix_for_named_and_children, // Path prefix is inherited from the context of the 'with' attribute itself.
                    "lv_obj_t" // Type for registering if 'named' is in the 'do' block.
                )) {
                LOG_ERR_JSON(do_block_for_with_json, "Failed to apply attributes in 'with.do' block.");
                // Decide if this is a fatal error for the current apply_setters_and_attributes call.
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
            if (!setter_entry->invoke(setter_entry, target_entity, NULL, prop_args_array)) {
                LOG_ERR_JSON(prop_args_array, "Failed to set property '%s' using '%s' on entity %p.", prop_name, setter_entry->name, target_entity);
                // Potentially return false or handle error more strictly
            } else {
                 LOG_DEBUG("Successfully applied property '%s' using '%s' to entity %p", prop_name, setter_entry->name, target_entity);
            }
        }

        if (temp_array_created) {
            cJSON_Delete(prop_args_array);
        }
    } // End for loop over attributes

    return true; // Indicate success
}
"""

    # Main recursive rendering function
    c_code += "static void* render_json_node(cJSON *node, lv_obj_t *parent, const char *named_path_prefix) {\n"
    c_code += "    if (!cJSON_IsObject(node)) {\n"
    c_code += "        LOG_ERR(\"Render Error: Expected JSON object for UI node.\");\n"
    c_code += "        return NULL;\n"
    c_code += "    }\n\n"

    c_code += """
    // --- Context management: Save context active at the start of this node's processing ---
    cJSON* original_context_for_this_node_call = get_current_context();
    bool context_was_locally_changed_by_this_node = false;

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
            cJSON *duplicated_root = cJSON_Duplicate(root_item_comp, true);
            if (duplicated_root) {
                lvgl_json_register_ptr(comp_id_str, "component_json_node", (void*)duplicated_root);
                return (void*)1; // Success, non-NULL arbitrary pointer
            } else {
                LOG_ERR_JSON(node, "Component Error: Failed to duplicate root for component '%s'", comp_id_str);
                return NULL;
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
            cJSON *component_root_json_node = (cJSON*)lvgl_json_get_registered_ptr(view_id_str, "component_json_node");

            if (component_root_json_node) {
                LOG_INFO("Using view '%s', named_path_prefix for component render: '%s'", view_id_str, named_path_prefix ? named_path_prefix : "ROOT");
                
                cJSON* context_for_view_item = cJSON_GetObjectItemCaseSensitive(node, "context");
                if (context_for_view_item && cJSON_IsObject(context_for_view_item)) {
                    set_current_context(context_for_view_item); 
                    view_context_set_locally = true;
                }
                
                // Render the component's root node. It will be parented to `parent` of `use-view`.
                // The `named_path_prefix` for the component instance is the same as the use-view's.
                // If the component_root_json_node has an `id`, it will be registered relative to this `named_path_prefix`.
                void* component_root_entity = render_json_node(component_root_json_node, parent, named_path_prefix);
                
                // After render_json_node returns, context is restored to what it was BEFORE component_root_json_node
                // was processed. This would be the context set by use-view's "context" or inherited.

                cJSON *do_attrs_json = cJSON_GetObjectItemCaseSensitive(node, "do");
                if (component_root_entity && do_attrs_json && cJSON_IsObject(do_attrs_json)) {
                    LOG_INFO("Applying 'do' attributes to component '%s' root %p", view_id_str, component_root_entity);
                    
                    const char *comp_root_actual_type_str = "obj"; // Default
                    cJSON *comp_root_type_item = cJSON_GetObjectItemCaseSensitive(component_root_json_node, "type");
                    if (comp_root_type_item && cJSON_IsString(comp_root_type_item)) {
                        comp_root_actual_type_str = comp_root_type_item->valuestring;
                    }
                    const char *comp_root_create_type_str = (strcmp(comp_root_actual_type_str, "grid") == 0) ? "obj" : comp_root_actual_type_str;
                    
                    bool is_comp_root_widget = true;
                    char comp_root_registry_type_name[64] = "lv_obj_t";

                    if (strcmp(comp_root_actual_type_str, "style") == 0) { // Add other non-widget types if any
                        is_comp_root_widget = false;
                        snprintf(comp_root_registry_type_name, sizeof(comp_root_registry_type_name), "lv_style_t");
                    } else {
                         // For widgets, construct full type name e.g. lv_button_t
                        snprintf(comp_root_registry_type_name, sizeof(comp_root_registry_type_name), "lv_%s_t", comp_root_create_type_str);
                    }

                    // Determine the path prefix for "named" and "children" inside the "do" block.
                    // This path should be relative to the component root's own identity.
                    char path_for_do_block_context[256];
                    path_for_do_block_context[0] = '\\0';
                    const char* base_for_do_path = named_path_prefix; // Path context of the use-view instantiation

                    cJSON* comp_root_id_item = cJSON_GetObjectItemCaseSensitive(component_root_json_node, "id");
                    if (comp_root_id_item && cJSON_IsString(comp_root_id_item) && comp_root_id_item->valuestring[0] == '@') {
                        const char* comp_root_id_val = comp_root_id_item->valuestring + 1;
                        if (base_for_do_path && base_for_do_path[0]) {
                            snprintf(path_for_do_block_context, sizeof(path_for_do_block_context)-1, "%s:%s", base_for_do_path, comp_root_id_val);
                        } else {
                            strncpy(path_for_do_block_context, comp_root_id_val, sizeof(path_for_do_block_context)-1);
                        }
                    } else if (base_for_do_path) {
                         strncpy(path_for_do_block_context, base_for_do_path, sizeof(path_for_do_block_context)-1);
                    }
                    // else path_for_do_block_context remains empty (root)

                    apply_setters_and_attributes(
                        do_attrs_json,
                        component_root_entity,
                        comp_root_actual_type_str,
                        comp_root_create_type_str,
                        is_comp_root_widget,
                        is_comp_root_widget ? (lv_obj_t*)component_root_entity : NULL,
                        path_for_do_block_context, // Path context for "named"/"children" in DO block
                        comp_root_registry_type_name
                    );
                }

                if (view_context_set_locally) {
                    set_current_context(original_context_for_this_node_call); 
                }
                return component_root_entity; 

            } else { // component_root_json_node not found
                LOG_WARN_JSON(node, "Use-View Error: Component '%s' not found or type error. Creating fallback label.", view_id_str);
                lv_obj_t *fallback_label = lv_label_create(parent);
                if(fallback_label) {
                    char fallback_text[128];
                    snprintf(fallback_text, sizeof(fallback_text), "Component '%s' error", view_id_str);
                    lv_label_set_text(fallback_label, fallback_text);
                }
                if (view_context_set_locally) { 
                     set_current_context(original_context_for_this_node_call);
                }
                return fallback_label; 
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
            // The `named_path_prefix` is passed through to the `for_item`.
            void* result_entity = render_json_node(for_item, parent, named_path_prefix); 
            set_current_context(original_context_for_this_node_call); 
            return result_entity; 
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
    c_code += "    const char *id_str_val = NULL;\n"
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
    c_code += "    void *created_entity = NULL;\n"
    c_code += "    bool is_widget = true;\n"
    c_code += "    char type_name_for_registry_buf[64] = \"lv_obj_t\"; // Default, will be updated\n"
    c_code += "    const char *actual_type_str_for_node = type_str; //This is the 'type_str' from JSON\n"
    c_code += "    const char *create_type_str_for_node = type_str; //This might become 'obj' if type_str is 'grid'\n\n"

    c_code += "    if (strcmp(actual_type_str_for_node, \"grid\") == 0) {\n"
    c_code += "        create_type_str_for_node = \"obj\";\n"
    c_code += "        LOG_INFO(\"Encountered 'grid' type, will create as 'lv_obj' and apply grid layout.\");\n"
    c_code += "    }\n\n"

    c_code += "    // Check for custom creators (e.g., for styles)\n"
    first_creator = True
    for obj_type, creator_func in custom_creators_map.items():
        c_code += f"    {'else ' if not first_creator else ''}if (strcmp(actual_type_str_for_node, \"{obj_type}\") == 0) {{\n"
        # Custom creators (like styles) usually require an ID for registration.
        # The path for registration is `effective_path_for_node_and_children` if `id_str_val` (derived from `node->id`) was present.
        # If `id_str_val` is NULL, it means the node itself doesn't have an ID to form the final segment of its path.
        c_code += f"        if (!id_str_val || !id_str_val[0]) {{\n" 
        c_code += "             // If custom type absolutely needs an ID, this is an error. For styles, it's managed by name.\n"
        c_code += "             // The registration happens using effective_path_for_node_and_children which is built from id_str_val.\n"
        c_code += "             // Let creator decide if name is sufficient or log error if id_str_val was expected to be part of effective_path_for_node_and_children\n"
        c_code += f"            LOG_WARN_JSON(node, \"Warning: Type '%s' created without a direct '@id' for path construction. Name for registration: '%s'\", actual_type_str_for_node, effective_path_for_node_and_children);\n"
        c_code += f"        }}\n"
        c_code += f"        // Use effective_path_for_node_and_children as the name if it's non-empty, otherwise NULL.\n"
        c_code += f"        const char* name_for_custom_creator = (effective_path_for_node_and_children[0] != '\\0') ? effective_path_for_node_and_children : NULL;\n"
        c_code += f"        if (!name_for_custom_creator && id_str_val) name_for_custom_creator = id_str_val; // Fallback if path is empty but id_str_val exists (e.g. root node)\n"
        c_code += f"        if (!name_for_custom_creator) {{ \n"
        c_code += "            LOG_ERR_JSON(node, \"Render Error: Type '%s' requires an 'id' to form a registration name, but no id found or path is empty.\", actual_type_str_for_node);\n"
        c_code += "            if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
        c_code += "            return NULL;\n"
        c_code += f"        }}\n"
        c_code += f"        created_entity = (void*){creator_func}(name_for_custom_creator);\n"
        c_code += f"        if (!created_entity) {{ \n"
        c_code += "             LOG_ERR_JSON(node, \"Render Error: Custom creator %s for name '%s' returned NULL.\", \"{creator_func}\", name_for_custom_creator);\n"
        c_code += "             if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
        c_code += "             return NULL; \n"
        c_code += "        }\n"
        c_code += f"        is_widget = false;\n" # Custom creators are typically for non-widget resources like styles
        c_code += f"        snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), \"lv_{obj_type}_t\");\n"
        # Registration for custom types is handled by the custom creator itself using the passed name.
        c_code += f"    }}\n"
        first_creator = False

    # 'with' type is a pseudo-type; it doesn't create a new entity but operates on an existing one (parent or specified obj).
    # Attributes (like 'obj' and 'do') for 'with' are handled by apply_setters_and_attributes.
    # Here, we just acknowledge it. 'created_entity' will remain NULL or parent, and apply_setters_and_attributes will handle 'with' logic.
    c_code += f"    {'else ' if not first_creator else ''}if (strcmp(actual_type_str_for_node, \"with\") == 0) {{\n"
    c_code += """        // 'with' doesn't create its own LVGL object. It's a directive to apply attributes to another object.
        // The 'target_entity' for its attributes will be resolved by apply_setters_and_attributes when it processes the 'with' key.
        // For the purpose of render_json_node, we can say the 'created_entity' is the parent,
        // as 'with' acts as a modifier in the context of its parent or specified object.
        // However, specific 'with.obj' handling is in apply_setters_and_attributes.
        // This block mainly sets up types for the apply_setters_and_attributes call for the 'with' node itself.
        created_entity = (void*)parent; // Placeholder, real target is resolved in apply_setters_and_attributes
        is_widget = true; // Assume it operates on widgets primarily.
        snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), "lv_obj_t"); 
        actual_type_str_for_node = "obj"; // Treat as obj for attribute application purposes
        create_type_str_for_node = "obj";
        LOG_DEBUG(\"Processing 'with' node. Target entity will be determined by 'with.obj' attribute.\");\n"""
    c_code += f"    }}\n"


    c_code += f"    {'else ' if not first_creator else ''} {{\n"
    c_code += "        char create_func_name[64];\n"
    c_code += "        snprintf(create_func_name, sizeof(create_func_name), \"lv_%s_create\", create_type_str_for_node);\n"
    c_code += "        const invoke_table_entry_t* create_entry = find_invoke_entry(create_func_name);\n"
    c_code += "        if (!create_entry) {\n"
    c_code += "            LOG_ERR_JSON(node, \"Render Error: Create function '%s' not found for type '%s' (create type '%s').\", create_func_name, actual_type_str_for_node, create_type_str_for_node);\n"
    c_code += "            if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
    c_code += "            return NULL;\n"
    c_code += "        }\n\n"
    c_code += "        lv_obj_t* new_widget = NULL;\n"
    c_code += "        // First arg to creator is parent. Creator returns void*, but we expect lv_obj_t** for widget creators via invoke. \n"
    c_code += "        if (!create_entry->invoke(create_entry, (void*)parent, &new_widget, NULL)) { \n"
    c_code += "            LOG_ERR_JSON(node, \"Render Error: Failed to invoke %s.\", create_func_name);\n"
    c_code += "            if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
    c_code += "            return NULL;\n"
    c_code += "        }\n"
    c_code += "        if (!new_widget) { \n"
    c_code += "             LOG_ERR_JSON(node, \"Render Error: %s returned NULL.\", create_func_name); \n"
    c_code += "             if (context_was_locally_changed_by_this_node) set_current_context(original_context_for_this_node_call);\n"
    c_code += "             return NULL; \n"
    c_code += "        }\n"
    c_code += "        created_entity = (void*)new_widget;\n"
    c_code += "        is_widget = true;\n\n"
    c_code += "        // Determine type for registry (e.g., lv_button_t, lv_obj_t)\n"
    c_code += "        snprintf(type_name_for_registry_buf, sizeof(type_name_for_registry_buf), \"lv_%s_t\", create_type_str_for_node);\n"
    c_code += "        if (id_str_val && id_str_val[0] && effective_path_for_node_and_children[0] != '\\0') {\n"
    c_code += "            lvgl_json_register_ptr(effective_path_for_node_and_children, type_name_for_registry_buf, created_entity);\n"
    c_code += "            LOG_INFO(\"Registered widget %p as '%s' (type %s)\", created_entity, effective_path_for_node_and_children, type_name_for_registry_buf);\n"
    c_code += "        } else if (id_str_val && id_str_val[0]) {\n"
    c_code += "            LOG_WARN(\"Widget with id '%s' created, but effective_path_for_node_and_children is empty. Not registered by id.\", id_str_val);\n"
    c_code += "        }\n"
    c_code += "    }\n"
    c_code += "\n"

    grid_setup_c_code = """
    if (created_entity && is_widget && strcmp(actual_type_str_for_node, "grid") == 0) { 
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
            // Register arrays for potential later retrieval/management if needed, though LVGL copies them.
            // This registration is mostly for debugging or advanced scenarios.
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
    }
"""
    c_code += grid_setup_c_code

    c_code += "    // 3. Set Properties, handle children, etc., using the new function\n"
    c_code += "    if (created_entity) {\n"
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