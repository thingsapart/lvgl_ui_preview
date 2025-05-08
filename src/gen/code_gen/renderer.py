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
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest);\n"
    c_code += "extern void* lvgl_json_get_registered_ptr(const char *name);\n"
    c_code += "extern void lvgl_json_register_ptr(const char *name, void *ptr);\n"

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

    c_code += "    // 1. Determine Object Type and ID\n"
    c_code += "    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(node, \"type\");\n"
    c_code += "    const char *type_str = \"obj\"; // Default type\n"
    c_code += "    if (type_item && cJSON_IsString(type_item)) {\n"
    c_code += "        type_str = type_item->valuestring;\n"
    c_code += "    }\n"
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
    c_code += "            lvgl_json_register_ptr(id_str, created_entity);\n"
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
                    if (!unmarshal_value(val_item, "int32_t", &col_dsc_array[i])) { // Use unmarshal_value
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
                    if (!unmarshal_value(val_item, "int32_t", &row_dsc_array[i])) { // Use unmarshal_value
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
            lvgl_json_register_ptr(temp_name_buf, (void*)col_dsc_array); // Registry will lv_strdup the name

            snprintf(temp_name_buf, sizeof(temp_name_buf), "grid_row_dsc_%p", (void*)grid_obj);
            lvgl_json_register_ptr(temp_name_buf, (void*)row_dsc_array); // Registry will lv_strdup the name
            
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
    c_code += "        // Skip known control properties OR grid-specific setup properties handled earlier\n" # MODIFIED COMMENT
    c_code += "        if (strcmp(prop_name, \"type\") == 0 || strcmp(prop_name, \"id\") == 0 || strcmp(prop_name, \"children\") == 0 || \n"
    c_code += "            (strcmp(type_str, \"grid\") == 0 && (strcmp(prop_name, \"cols\") == 0 || strcmp(prop_name, \"rows\") == 0))) {\n"
    c_code += "            continue;\n"
    c_code += "        }\n"
    c_code += "        // Property value must be an array of arguments\n"
    c_code += "        cJSON *old_prop = NULL;\n"
    c_code += "        if (!cJSON_IsArray(prop)) {\n"
    #c_code += "            LOG_WARN(\"Render Warning: Property '%s' value is not a JSON array. Skipping.\", prop_name);\n"
    #c_code += "            LOG_ERR_JSON(prop, \"Render Warning: Property '%s' value is not a JSON array. Skipping.\", prop_name);\n"
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