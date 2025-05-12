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
    c_code += "static bool render_json_node(cJSON *node, lv_obj_t *parent);\n"
    c_code += "static const invoke_table_entry_t* find_invoke_entry(const char *name);\n"
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent);\n"
    c_code += "extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);\n"
    c_code += "extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);\n"
    c_code += "static void set_current_context(cJSON* new_context);\n"

    # Include custom creator function prototypes
    for type_name, creator_func in custom_creators_map.items():
         # Need the return type (e.g., lv_style_t*) - reconstruct from name?
         c_type = f"lv_{type_name}_t"
         c_code += f"extern {c_type}* {creator_func}(const char *name);\n"
    c_code += "\n"


    # Main recursive rendering function
    c_code += "static bool render_json_node(cJSON *node, lv_obj_t *parent) {\n"
    c_code += "    if (!cJSON_IsObject(node)) {\n"
    c_code += "        LOG_ERR(\"Render Error: Expected JSON object for UI node.\");\n"
    c_code += "        return false;\n"
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
                return true; // Component definition processed
            } else {
                LOG_ERR_JSON(node, "Component Error: Failed to duplicate root for component '%s'", comp_id_str);
                return false;
            }
        } else {
            LOG_ERR_JSON(node, "Component Error: Invalid 'component' definition. Requires 'id' (string starting with '@') and 'root' (object).");
            return false;
        }
    } else if (strcmp(type_str, "use-view") == 0) {
        cJSON *id_item_use_view = cJSON_GetObjectItemCaseSensitive(node, "id");
        if (id_item_use_view && cJSON_IsString(id_item_use_view) && id_item_use_view->valuestring && id_item_use_view->valuestring[0] == '@') {
            const char *view_id_str = id_item_use_view->valuestring + 1; // Skip '@'
            cJSON *component_root_node = (cJSON*)lvgl_json_get_registered_ptr(view_id_str, "component_json_node");

            if (component_root_node) {
                LOG_INFO("Using view '%s'", view_id_str);
                // --- Context for use-view ---
                cJSON* context_for_view_item = cJSON_GetObjectItemCaseSensitive(node, "context");
                cJSON* previous_context_before_use_view = get_current_context(); // This is original_context_for_this_node_call
                bool context_set_for_use_view = false;

                if (context_for_view_item && cJSON_IsObject(context_for_view_item)) {
                    set_current_context(context_for_view_item);
                    context_set_for_use_view = true;
                }
                
                bool render_success = render_json_node(component_root_node, parent);
                
                if (context_set_for_use_view) {
                    set_current_context(previous_context_before_use_view); // Restore context
                }
                return render_success;
            } else {
                LOG_WARN_JSON(node, "Use-View Error: Component '%s' not found or type error. Creating fallback label.", view_id_str);
                lv_obj_t *fallback_label = lv_label_create(parent);
                if(fallback_label) {
                    char fallback_text[128];
                    snprintf(fallback_text, sizeof(fallback_text), "Component '%s' error", view_id_str);
                    lv_label_set_text(fallback_label, fallback_text);
                }
                return true; 
            }
        } else {
            LOG_ERR_JSON(node, "Use-View Error: Invalid 'use-view' definition. Requires 'id' (string starting with '@').");
            return false;
        }
    } else if (strcmp(type_str, "context") == 0) {
        cJSON *values_item = cJSON_GetObjectItemCaseSensitive(node, "values");
        cJSON *for_item = cJSON_GetObjectItemCaseSensitive(node, "for");

        if (values_item && cJSON_IsObject(values_item) && for_item && cJSON_IsObject(for_item)) {
            // LOG_DEBUG("Processing 'context' type node.");
            // original_context_for_this_node_call is already saved at the top
            set_current_context(values_item); // Set new context from "values"
            
            bool render_success = render_json_node(for_item, parent); // Render the "for" block
            
            set_current_context(original_context_for_this_node_call); // Restore original context
            return render_success; 
        } else {
            LOG_ERR_JSON(node, "'context' type node Error: Requires 'values' (object) and 'for' (object) properties.");
            return false;
        }
    }

    // --- If not a special type handled above, proceed with generic node processing ---
    // --- Set node-specific context if provided for this generic node ---
    cJSON* context_property_on_this_node = cJSON_GetObjectItemCaseSensitive(node, "context");
    if (context_property_on_this_node && cJSON_IsObject(context_property_on_this_node)) {
        // original_context_for_this_node_call is already saved
        set_current_context(context_property_on_this_node);
        context_was_locally_changed_by_this_node = true;
    }
    
    // --- Continue with standard object/resource determination and creation ---
"""

    c_code += "    // 1. Determine Object Type and ID\n"
    #c_code += "    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(node, \"type\");\n"
    #c_code += "    const char *type_str = \"obj\"; // Default type\n"
    #c_code += "    if (type_item && cJSON_IsString(type_item)) {\n"
    #c_code += "        type_str = type_item->valuestring;\n"
    #c_code += "    }\n"
    c_code += "    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(node, \"id\");\n"
    c_code += "    const char *id_str = NULL;\n"
    c_code += "    if (id_item && cJSON_IsString(id_item)) {\n"
    # ID should start with '@' for registration
    c_code += "        if (id_item->valuestring[0] == '@') {\n"
    c_code += "           id_str = id_item->valuestring + 1; // Get name part after '@'\n"
    c_code += "        } else {\n"
    c_code += "           LOG_WARN(\"Render Warning: 'id' property '%s' should start with '@' for registration. Ignoring registration.\", id_item->valuestring);\n"
    c_code += "           id_str = NULL; // Don't register if format is wrong\n"
    c_code += "        }\n"
    c_code += "    }\n\n"

    c_code += "    // 2. Create the LVGL Object / Resource\n"
    c_code += "    void *created_entity = NULL; // Can be lv_obj_t* or lv_style_t* etc.\n"
    c_code += "    bool is_widget = true; // Is it an lv_obj_t based widget?\n"
    c_code += "    const char *type_str_for_create = type_str;\n\n" # Use a copy for creation func name

    c_code += "    if (strcmp(type_str, \"grid\") == 0) {\n"
    c_code += "        type_str_for_create = \"obj\"; // A 'grid' is a generic obj with grid layout\n"
    c_code += "        LOG_INFO(\"Encountered 'grid' type, will create as 'lv_obj' and apply grid layout.\");\n"
    c_code += "    }\n\n"

    # Check if it's a type using a custom creator (like style)
    c_code += f"    // Check for custom creators (e.g., for styles)\n"

    first_creator = True
    for obj_type, creator_func in custom_creators_map.items():
        c_code += f"    {'else ' if not first_creator else ''}if (strcmp(type_str, \"{obj_type}\") == 0) {{\n"
        c_code += f"        if (!id_str) {{\n"
        c_code += f"            LOG_ERR(\"Render Error: Type '{obj_type}' requires an 'id' property starting with '@'.\");\n"
        c_code += f"            return false;\n"
        c_code += f"        }}\n"
        c_code += f"        // Call custom creator which handles allocation, init, and registration\n"
        c_code += f"        created_entity = (void*){creator_func}(id_str);\n"
        c_code += f"        if (!created_entity) return false; // Error logged in creator\n"
        c_code += f"        is_widget = false; // Mark as non-widget (doesn't take parent, no children)\n"
        c_code += f"    }}\n"
        first_creator = False

    # For "with" blocks, don't create entity - use parent.
    c_code += "    else if (strcmp(type_str, \"with\") == 0) {\n"
    c_code += "        created_entity = parent;\n"
    c_code += "    }\n"

    # Default: Create LVGL widget using lv_<type>_create(parent)
    c_code += f"    {'else ' if not first_creator else ''}{{\n" # Start of default widget creation block
    c_code += "        // Default: Assume it's a widget type (lv_obj_t based)\n"
    c_code += "        char create_func_name[64];\n"
    c_code += "        snprintf(create_func_name, sizeof(create_func_name), \"lv_%s_create\", type_str_for_create);\n" # MODIFIED LINE
    c_code += "        const invoke_table_entry_t* create_entry = find_invoke_entry(create_func_name);\n"
    c_code += "        if (!create_entry) {\n"
    #c_code += "            LOG_ERR(\"Render Error: Create function '%s' not found.\", create_func_name);\n"
    c_code += "            LOG_ERR_JSON(node, \"Render Error: Create function '%s' not found.\", create_func_name);\n"
    c_code += "            return false;\n"
    c_code += "        }\n\n"
    c_code += "        // Prepare arguments for creator (just the parent obj)\n"
    c_code += "        cJSON *create_args_json = cJSON_CreateArray();\n"
    c_code += "        if (!create_args_json) { LOG_ERR(\"Memory error creating args for %s\", create_func_name); return false; }\n\n"
    c_code += "        // How to pass the parent lv_obj_t*? \n"
    c_code += "        // Option A: Register parent temporarily? Seems complex.\n"
    c_code += "        // Option B: Modify invoke_fn_t signature? Chosen earlier to avoid.\n"
    c_code += "        // Option C: Assume create functions take exactly one lv_obj_t* arg \n"
    c_code += "        //           and handle it specially here or in the invoke func?\n"
    c_code += "        // Let's assume the invoker for lv_..._create(lv_obj_t*) handles this.\n"
    c_code += "        // The generated invoker needs to know its first arg is lv_obj_t* and get it from `parent`.\n"
    c_code += "        // -- RETHINK invoke_fn_t signature --\n"
    c_code += "        // Let's assume invoke_fn_t takes target_obj_ptr which is the parent here.\n"
    c_code += "        // We pass NULL for json args as lv_..._create only takes parent.\n"
    c_code += "        lv_obj_t* new_widget = NULL;\n"
    c_code += "        // We need the invoker to place the result into &new_widget.\n"
    #c_code += "        if (!create_entry->invoke((void*)parent, &new_widget, NULL, create_entry->func_ptr)) { \n"
    c_code += f"        if (!create_entry->invoke(create_entry, (void*)parent, &new_widget, NULL)) {{ \n" # <<< Pass entry
    #  c_code += "            LOG_ERR(\"Render Error: Failed to invoke %s.\", create_func_name);\n"
    c_code += "            LOG_ERR_JSON(node, \"Render Error: Failed to invoke %s.\", create_func_name);\n"
    c_code += "            // cJSON_Delete(create_args_json); // Not needed if passing NULL\n"
    c_code += "            return false;\n"
    c_code += "        }\n"
    c_code += "        // cJSON_Delete(create_args_json); // Not needed if passing NULL\n"
    c_code += "        if (!new_widget) { LOG_ERR(\"Render Error: %s returned NULL.\", create_func_name); return false; }\n"
    c_code += "        created_entity = (void*)new_widget;\n"
    c_code += "        is_widget = true;\n\n"
    c_code += "        // Register if ID is provided and it wasn't a custom-created type\n"
    c_code += "        if (id_str) {\n"
    c_code += "            char entity_type_name_buf[64];\n"
    c_code += "            // For widgets, type_str_for_create holds the base type like \"obj\", \"label\"\n"
    c_code += "            // For custom created (e.g. styles), type_str holds \"style\"\n"
    c_code += "            // We need to construct the C type name like \"lv_label_t\", \"lv_style_t\"\n"
    c_code += "            const char* base_type_for_reg = is_widget ? type_str_for_create : type_str;\n"
    c_code += "            snprintf(entity_type_name_buf, sizeof(entity_type_name_buf), \"lv_%s_t\", base_type_for_reg);\n"
    c_code += "            lvgl_json_register_ptr(id_str, entity_type_name_buf, created_entity);\n" # MODIFIED CALL
    c_code += "        }\n"
    c_code += "    }\n" # End of default widget creation block
    c_code += "\n"

    grid_setup_c_code = """
    // Special handling for \"grid\" type to set up column and row descriptors
    if (created_entity && is_widget && strcmp(type_str, "grid") == 0) { // type_str is original JSON type
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
                    if (!unmarshal_value(val_item, "int32_t", &col_dsc_array[i], created_entity)) { // Use unmarshal_value
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

        // Only proceed with rows if cols were OK and rows_item_json is valid
        if (grid_setup_ok && rows_item_json && cJSON_IsArray(rows_item_json)) {
            int num_rows = cJSON_GetArraySize(rows_item_json);
            row_dsc_array = (int32_t*)LV_MALLOC(sizeof(int32_t) * (num_rows + 1));
            if (row_dsc_array) {
                for (int i = 0; i < num_rows; i++) {
                    cJSON *val_item = cJSON_GetArrayItem(rows_item_json, i);
                    if (!unmarshal_value(val_item, "int32_t", &row_dsc_array[i], created_entity)) { // Use unmarshal_value
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
        } else if (grid_setup_ok) { // This implies rows_item_json was missing/invalid
             LOG_ERR_JSON(node, "Grid Error: 'rows' array is missing or not an array for grid type.");
             grid_setup_ok = false;
        }

        if (grid_setup_ok && col_dsc_array && row_dsc_array) {
            char temp_name_buf[64]; // For snprintf

            snprintf(temp_name_buf, sizeof(temp_name_buf), "grid_col_dsc_%p", (void*)grid_obj);
            lvgl_json_register_ptr(temp_name_buf, "lv_coord_array", (void*)col_dsc_array); // Registry will lv_strdup the name

            snprintf(temp_name_buf, sizeof(temp_name_buf), "grid_row_dsc_%p", (void*)grid_obj);
            lvgl_json_register_ptr(temp_name_buf, "lv_coord_array", (void*)row_dsc_array); // Registry will lv_strdup the name
            
            lv_obj_set_grid_dsc_array(grid_obj, col_dsc_array, row_dsc_array);
            // Note: Memory for col_dsc_array and row_dsc_array should ideally be freed on LV_EVENT_DELETE of grid_obj.
            // This is not added here to keep changes minimal as per request.
        } else {
            if (col_dsc_array) { LV_FREE(col_dsc_array); } // Free if allocated but not used
            if (row_dsc_array) { LV_FREE(row_dsc_array); } // Free if allocated but not used
            LOG_ERR_JSON(node, "Grid Error: Failed to set up complete grid descriptors. Grid layout will not apply.");
        }
    }
"""
    c_code += grid_setup_c_code

    c_code += "    // 3. Set Properties\n"
    c_code += "    cJSON *prop = NULL;\n"
    c_code += "    for (prop = node->child; prop != NULL; prop = prop->next) {\n"
    c_code += "        const char *prop_name = prop->string;\n"
    c_code += "        if (!prop_name) continue;\n"
    c_code += "        // Skip known control properties OR grid-specific OR context property setup properties handled earlier\n"
    c_code += "        if (strcmp(prop_name, \"type\") == 0 || strcmp(prop_name, \"id\") == 0 || strcmp(prop_name, \"children\") == 0 || \n"
    c_code += "            (strcmp(type_str, \"grid\") == 0 && (strcmp(prop_name, \"cols\") == 0 || strcmp(prop_name, \"rows\") == 0)) || \n"
    c_code += "            (strcmp(prop_name, \"context\") == 0)) {\n"
    c_code += "            continue;\n"
    c_code += "        }\n"
    c_code += "        // Property value must be an array of arguments\n"
    c_code += "        cJSON *old_prop = NULL;\n"
    c_code += "        if (!cJSON_IsArray(prop)) {\n"
    #c_code += "            LOG_WARN(\"Render Warning: Property '%s' value is not a JSON array. Skipping.\", prop_name);\n"
    #c_code += "            LOG_ERR_JSON(prop, \"Render Warning: Property '%s' value is not a JSON array. Skipping.\", prop_name);\n"
    # Evaluate/parse "with: obj: do:" blocks, if present.
    c_code += "            if (cJSON_IsObject(prop) && strcmp(prop_name, \"with\") == 0) {\n"
    c_code += "                cJSON *obj = cJSON_GetObjectItemCaseSensitive(prop, \"obj\");\n"
    c_code += "                if (!obj) { LOG_ERR_JSON(prop, \"Render Warning: with requires attributes 'obj' and 'do' - with '%s' obj value is missing. Skipping.\", prop_name); continue; } \n"
    c_code += "                lv_obj_t *new_parent = NULL; unmarshal_value(obj, \"lv_obj_t *\", &new_parent, created_entity);\n"
    c_code += "                cJSON *with = cJSON_GetObjectItemCaseSensitive(prop, \"do\");\n"
    c_code += "                if (!cJSON_IsObject(with)) { LOG_ERR_JSON(prop, \"Render Warning: with requires attributes 'obj' and 'do' - do block must be Object. Skipping.\"); continue; } \n"
    c_code += "                cJSON_AddItemToObject(with, \"type\", cJSON_CreateString(\"with\"));\n"
    c_code += "                if (!render_json_node(with, (lv_obj_t*) new_parent)) {\n"
    c_code += "                    LOG_ERR(\"Render Error: Failed to render with node. Skipping.\");\n"
    c_code += "                }\n"
    c_code += "                continue;\n"
    c_code += "            }\n"
    # Otherwise turn single value prop into array.
    c_code += "            old_prop = prop;\n"
    c_code += "            prop = cJSON_CreateArray();\n"
    c_code += "            cJSON_AddItemToArray(prop, cJSON_Duplicate(old_prop, true));\n" 
    #c_code += "            continue;\n"
    c_code += "        }\n\n"

    c_code += "        // Find the setter function: lv_<type>_set_<prop> or lv_obj_set_<prop>\n"
    c_code += "        char setter_name[128];\n"
    c_code += "        const invoke_table_entry_t* setter_entry = NULL;\n\n"
    c_code += "        // Try specific type setter first (only if it's a widget)\n"
    c_code += "        if (is_widget) {\n"
    c_code += "           snprintf(setter_name, sizeof(setter_name), \"lv_%s_set_%s\", type_str, prop_name);\n"
    c_code += "           setter_entry = find_invoke_entry(setter_name);\n"
    c_code += "        }\n\n"
    c_code += "        // Try generic lv_obj setter if specific not found or not applicable\n"
    c_code += "        if (!setter_entry && is_widget) { // Only widgets have generic obj setters\n"
    c_code += "            snprintf(setter_name, sizeof(setter_name), \"lv_obj_set_%s\", prop_name);\n"
    c_code += "            setter_entry = find_invoke_entry(setter_name);\n"
    c_code += "        }\n\n"
    c_code += "        // Try generic lv_obj method if not found or not applicable\n"
    c_code += "        if (!setter_entry && is_widget) { // Only widgets have generic obj setters\n"
    c_code += "            snprintf(setter_name, sizeof(setter_name), \"lv_obj_%s\", prop_name);\n"
    c_code += "            setter_entry = find_invoke_entry(setter_name);\n"
    c_code += "        }\n\n"
    c_code += "        // Try generic lv_obj style setter if specific not found or not applicable\n"
    c_code += "        if (!setter_entry && is_widget) { // Only widgets have generic obj setters\n"
    c_code += "            snprintf(setter_name, sizeof(setter_name), \"lv_obj_set_style_%s\", prop_name);\n"
    c_code += "            setter_entry = find_invoke_entry(setter_name);\n"
    c_code += "            if (setter_entry && cJSON_GetArraySize(prop) == 1) { cJSON_AddItemToArray(prop, cJSON_CreateNumber(0)); }\n"
    c_code += "        }\n\n"
    c_code += "        // Try style-setter (e.g., lv_style_set_...)\n"
    c_code += "        if (!setter_entry) {\n"
    c_code += "            snprintf(setter_name, sizeof(setter_name), \"lv_style_set_%s\", prop_name);\n"
    c_code += "            setter_entry = find_invoke_entry(setter_name);\n"
    c_code += "            //if (!setter_entry) { LOG_ERR_JSON(node, \"no style setter for %s\\n\", setter_name); }\n"
    c_code += "        }\n\n"
    c_code += "        // Try style-method (e.g., lv_style_...)\n"
    c_code += "        if (!setter_entry) {\n"
    c_code += "            snprintf(setter_name, sizeof(setter_name), \"lv_style_%s\", prop_name);\n"
    c_code += "            setter_entry = find_invoke_entry(setter_name);\n"
    c_code += "            //if (!setter_entry) { LOG_ERR_JSON(node, \"no style setter for %s\\n\", setter_name); }\n"
    c_code += "        }\n\n"
    c_code += "        // Try verbatim \n"
    c_code += "        if (!setter_entry) {\n"
    c_code += "            setter_entry = find_invoke_entry(prop_name);\n"
    c_code += "            //if (!setter_entry) { LOG_ERR_JSON(node, \"no verbatim method for %s\\n\", setter_name); }\n"
    c_code += "        }\n\n"
    c_code += "        if (!setter_entry) {\n"
    #c_code += "            LOG_WARN(\"Render Warning: No setter function found for property '%s' on type '%s'. Searched lv_%s_set_..., lv_obj_set_....\", prop_name, type_str, type_str);\n"
    c_code += "            LOG_ERR_JSON(node, \"Render Warning: No setter function found for property '%s' on type '%s'. Searched lv_%s_set_..., lv_obj_set_..., lv_obj_set_style_....\", prop_name, type_str, type_str);\n"
    c_code += "            continue;\n"
    c_code += "        }\n\n"

    c_code += "        // Invoke the setter\n"
    c_code += "        // Pass the created_entity as target_obj_ptr, NULL for dest (setters usually return void)\n"
    c_code += "        // Pass the property's JSON array value as args_array\n"
    c_code += "        if (!setter_entry->invoke(setter_entry, created_entity, NULL, prop)) {\n"
    #c_code += "             LOG_ERR(\"Render Error: Failed to set property '%s' using %s.\", prop_name, setter_name);\n"
    c_code += "             LOG_ERR_JSON(prop, \"Render Error: Failed to set property '%s' using %s.\", prop_name, setter_name);\n"
    c_code += "             // Continue or return false? Let's continue for now.\n"
    c_code += "        }\n"
    c_code += "        if (old_prop) { cJSON_Delete(prop); prop = old_prop; } // Delete prop if we had to create it (old-prop is set)\n"
    c_code += "    }\n\n"

    c_code += "    // 4. Process Children (only for widgets)\n"
    c_code += "    if (is_widget) {\n"
    c_code += "        cJSON *children_item = cJSON_GetObjectItemCaseSensitive(node, \"children\");\n"
    c_code += "        if (children_item) {\n"
    c_code += "            if (!cJSON_IsArray(children_item)) {\n"
    c_code += "                LOG_ERR(\"Render Error: 'children' property must be an array.\");\n"
    c_code += "                // Cleanup created widget?\n"
    c_code += "                return false;\n"
    c_code += "            }\n"
    c_code += "            cJSON *child_node = NULL;\n"
    c_code += "            cJSON_ArrayForEach(child_node, children_item) {\n"
    c_code += "                 if (!render_json_node(child_node, (lv_obj_t*)created_entity)) {\n"
    c_code += "                     // Error rendering child, stop processing siblings? Or continue?\n"
    c_code += "                     LOG_ERR(\"Render Error: Failed to render child node. Aborting sibling processing for this parent.\");\n"
    c_code += "                     // Cleanup? Difficult to manage partial tree creation.\n"
    c_code += "                     return false; // Propagate failure\n"
    c_code += "                 }\n"
    c_code += "            }\n"
    c_code += "        }\n"
    c_code += "    }\n"
    c_code += "    // --- Context management: Restore context if it was changed at this node's level ---\n"
    c_code += "    if (context_was_locally_changed_by_this_node) {\n"
    c_code += "        set_current_context(original_context_for_this_node_call);\n"
    c_code += "    }\n\n"
    c_code += "    return true;\n"
    c_code += "}\n\n"

    # Main entry point function
    c_code += "// --- Public API --- \n\n"
    c_code += "/**\n * @brief Renders a UI described by a cJSON object.\n *\n"
    c_code += " * @param root_json The root cJSON object (should be an array of UI nodes or a single node object).\n"
    c_code += " * @param implicit_root_parent The LVGL parent object for top-level UI elements.\n"
    c_code += " * @return true on success, false on failure.\n */\n"
    c_code += "bool lvgl_json_render_ui(cJSON *root_json, lv_obj_t *implicit_root_parent) {\n"
    c_code += "    if (!root_json) {\n"
    c_code += "        LOG_ERR(\"Render Error: root_json is NULL.\");\n"
    c_code += "        return false;\n"
    c_code += "    }\n"
    c_code += "    if (!implicit_root_parent) {\n"
    c_code += "        LOG_WARN(\"Render Warning: implicit_root_parent is NULL. Using lv_screen_active().\");\n"
    c_code += "        implicit_root_parent = lv_screen_active();\n"
    c_code += "        if (!implicit_root_parent) {\n"
    c_code += "             LOG_ERR(\"Render Error: Cannot get active screen.\");\n"
    c_code += "             return false;\n"
    c_code += "        }\n"
    c_code += "    }\n\n"
    c_code += "    // Clear registry before starting? Optional, depends on desired behavior.\n"
    c_code += "    // lvgl_json_registry_clear();\n\n"

    c_code += "    bool success = true;\n"
    c_code += "    if (cJSON_IsArray(root_json)) {\n"
    c_code += "        cJSON *node = NULL;\n"
    c_code += "        cJSON_ArrayForEach(node, root_json) {\n"
    c_code += "            if (!render_json_node(node, implicit_root_parent)) {\n"
    c_code += "                success = false;\n"
    c_code += "                LOG_ERR(\"Render Error: Failed to render top-level node. Aborting.\");\n"
    c_code += "                break; // Stop processing further nodes on failure\n"
    c_code += "            }\n"
    c_code += "        }\n"
    c_code += "    } else if (cJSON_IsObject(root_json)) {\n"
    c_code += "        success = render_json_node(root_json, implicit_root_parent);\n"
    c_code += "    } else {\n"
    c_code += "        LOG_ERR(\"Render Error: root_json must be a JSON object or array.\");\n"
    c_code += "        success = false;\n"
    c_code += "    }\n\n"
    c_code += "    if (!success) {\n"
    c_code += "         LOG_ERR(\"UI Rendering failed.\");\n"
    c_code += "         // Potential cleanup logic here?\n"
    c_code += "    } else {\n"
    c_code += "         LOG_INFO(\"UI Rendering completed successfully.\");\n"
    c_code += "    }\n"
    c_code += "    return success;\n"
    c_code += "}\n\n"

    return c_code