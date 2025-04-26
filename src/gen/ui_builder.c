
#include "ui_builder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For strtol, strtod

// Include necessary LVGL headers for widget creation and property setting
// Include core first
#include "lvgl.h"
// Include specific widget/feature headers USED BY THE BUILDER
#include "src/core/lv_obj_class_private.h" // For lv_obj_check_type
#include "src/widgets/label/lv_label.h"
#include "src/widgets/button/lv_button.h"
#include "src/widgets/slider/lv_slider.h"
#include "src/widgets/bar/lv_bar.h"
#include "src/widgets/scale/lv_scale.h"
// Add more includes for widgets referenced in create_lvgl_object_recursive
// e.g., #include "src/widgets/checkbox/lv_checkbox.h"

// --- Logging Macros ---
// Define simple logging macros if not provided elsewhere
#ifndef UI_BUILDER_LOG_INFO
#define UI_BUILDER_LOG_INFO(s, ...) printf("[UI_BUILDER INFO] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif
#ifndef UI_BUILDER_LOG_ERROR
#define UI_BUILDER_LOG_ERROR(s, ...) printf("[UI_BUILDER ERROR] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif
#ifndef UI_BUILDER_LOG_WARN
#define UI_BUILDER_LOG_WARN(s, ...) printf("[UI_BUILDER WARN] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif


// --- Font Registry ---
typedef struct {
    char *name;
    const lv_font_t *font;
} ui_builder_font_entry_t;

static ui_builder_font_entry_t *g_font_registry = NULL;
static size_t g_font_registry_count = 0;
static size_t g_font_registry_capacity = 0;

// --- Forward Declarations ---
static lv_obj_t* create_lvgl_object_recursive(cJSON *json_node, lv_obj_t *parent);
static void apply_properties(lv_obj_t *obj, cJSON *props_json);
static void apply_styles(lv_obj_t *obj, cJSON *styles_json);
static void apply_single_style_prop(lv_obj_t *obj, const char *prop_key, cJSON *style_prop, lv_style_selector_t selector);
static const lv_font_t* get_font_by_name(const char* name); // Now uses registry

// --- Helper Functions ---

// Helper to parse color string (e.g., "#RRGGBB" or "#RGB")
static lv_color_t parse_color(const char *color_str) {
    if (!color_str || color_str[0] != '#') {
         UI_BUILDER_LOG_WARN("Invalid color string format: %s. Using black.", color_str ? color_str : "NULL");
        return lv_color_black(); // Default or error color
    }
    // Skip '#' and parse hex
    char *end_ptr;
    unsigned long cval_ul = strtoul(color_str + 1, &end_ptr, 16);

    // Check if parsing consumed the whole string after '#'
    if (*end_ptr != '\0') {
        UI_BUILDER_LOG_WARN("Invalid characters in color string: %s. Using black.", color_str);
        return lv_color_black();
    }
    uint32_t cval = (uint32_t)cval_ul; // Cast result

    size_t len = strlen(color_str);
    if (len == 7) { // #RRGGBB
        return lv_color_hex(cval);
    } else if (len == 4) { // #RGB -> #RRGGBB
        uint8_t r = (cval >> 8) & 0xF;
        uint8_t g = (cval >> 4) & 0xF;
        uint8_t b = cval & 0xF;
        return lv_color_make(r * 17, g * 17, b * 17);
     } // Add support for #AARRGGBB or #ARGB if needed
     else {
        UI_BUILDER_LOG_WARN("Unsupported color string length (%zu): %s. Using black.", len, color_str);
        return lv_color_black();
     }
}

// Helper to parse alignment string
static lv_align_t parse_align(const char *align_str) {
    if (!align_str) return LV_ALIGN_DEFAULT;
    if (strcmp(align_str, "default") == 0) return LV_ALIGN_DEFAULT;
    if (strcmp(align_str, "top_left") == 0) return LV_ALIGN_TOP_LEFT;
    if (strcmp(align_str, "top_mid") == 0) return LV_ALIGN_TOP_MID;
    if (strcmp(align_str, "top_right") == 0) return LV_ALIGN_TOP_RIGHT;
    if (strcmp(align_str, "left_mid") == 0) return LV_ALIGN_LEFT_MID;
    if (strcmp(align_str, "center") == 0) return LV_ALIGN_CENTER;
    if (strcmp(align_str, "right_mid") == 0) return LV_ALIGN_RIGHT_MID;
    if (strcmp(align_str, "bottom_left") == 0) return LV_ALIGN_BOTTOM_LEFT;
    if (strcmp(align_str, "bottom_mid") == 0) return LV_ALIGN_BOTTOM_MID;
    if (strcmp(align_str, "bottom_right") == 0) return LV_ALIGN_BOTTOM_RIGHT;
    // Add other alignments like out_... if needed
    UI_BUILDER_LOG_WARN("Unknown align value: %s, using default.", align_str);
    return LV_ALIGN_DEFAULT;
}

// Helper to parse state string
static lv_state_t parse_state(const char *state_str) {
    if (!state_str || strcmp(state_str, "default") == 0) return LV_STATE_DEFAULT;
    if (strcmp(state_str, "checked") == 0) return LV_STATE_CHECKED;
    if (strcmp(state_str, "focused") == 0) return LV_STATE_FOCUSED;
    if (strcmp(state_str, "focus_key") == 0) return LV_STATE_FOCUS_KEY;
    if (strcmp(state_str, "edited") == 0) return LV_STATE_EDITED;
    if (strcmp(state_str, "hovered") == 0) return LV_STATE_HOVERED;
    if (strcmp(state_str, "pressed") == 0) return LV_STATE_PRESSED;
    if (strcmp(state_str, "scrolled") == 0) return LV_STATE_SCROLLED;
    if (strcmp(state_str, "disabled") == 0) return LV_STATE_DISABLED;
    if (strcmp(state_str, "state_any") == 0) return LV_STATE_ANY;
    // Check for numeric state values (hex) if the emulator generates them
    if (strncmp(state_str, "state_0x", 8) == 0) {
        return (lv_state_t)strtoul(state_str + 8, NULL, 16);
    }
    UI_BUILDER_LOG_WARN("Unknown state value: '%s', using default.", state_str);
    return LV_STATE_DEFAULT;
}

// Helper to parse part string
static lv_part_t parse_part(const char *part_str) {
    if (!part_str || strcmp(part_str, "default") == 0) return LV_PART_MAIN; // "default" in JSON maps to MAIN part
    if (strcmp(part_str, "main") == 0) return LV_PART_MAIN; // Allow "main" explicitly too
    if (strcmp(part_str, "indicator") == 0) return LV_PART_INDICATOR;
    if (strcmp(part_str, "knob") == 0) return LV_PART_KNOB;
    if (strcmp(part_str, "scrollbar") == 0) return LV_PART_SCROLLBAR;
    if (strcmp(part_str, "selected") == 0) return LV_PART_SELECTED;
    if (strcmp(part_str, "items") == 0) return LV_PART_ITEMS;
    if (strcmp(part_str, "cursor") == 0) return LV_PART_CURSOR;
    if (strcmp(part_str, "part_any") == 0) return LV_PART_ANY;
    // Check for numeric part values (hex) or custom parts
    if (strncmp(part_str, "unknown_part_0x", 15) == 0) {
        return (lv_part_t)strtoul(part_str + 15, NULL, 16);
    }
    if (strncmp(part_str, "custom_part_", 12) == 0) {
        return LV_PART_CUSTOM_FIRST + (lv_part_t)strtoul(part_str + 12, NULL, 10);
    }
    UI_BUILDER_LOG_WARN("Unknown part value: '%s', using main part.", part_str);
    return LV_PART_MAIN;
}

// Helper to parse coordinate (number, "N%", "content", "Nfr")
static lv_coord_t parse_coord(cJSON *value) {
    if (cJSON_IsNumber(value)) {
        return (lv_coord_t)cJSON_GetNumberValue(value);
    } else if (cJSON_IsString(value)) {
        const char *str = cJSON_GetStringValue(value);
        if (!str) return 0; // Handle null string case

        if (strcmp(str, "content") == 0) {
            return LV_SIZE_CONTENT;
        }

        char *endptr;
        long num_val = strtol(str, &endptr, 10);

        // Check what follows the number
        if (endptr == str) { // No number parsed
            UI_BUILDER_LOG_WARN("Invalid coordinate string (no number): '%s'", str);
            return 0;
        }

        if (*endptr == '%') { // Percentage
             // Check if string ends right after '%'
             if (*(endptr + 1) == '\0') {
                // Clamp LV_PCT range if necessary (though lv_pct might do this)
                if (num_val < -LV_PCT_POS_MAX) num_val = -LV_PCT_POS_MAX;
                if (num_val > LV_PCT_POS_MAX) num_val = LV_PCT_POS_MAX;
                return lv_pct((int32_t)num_val);
             } else {
                 UI_BUILDER_LOG_WARN("Invalid coordinate string (extra chars after %%): '%s'", str);
                 return 0;
             }
        } else if (strncmp(endptr, "fr", 2) == 0) { // Flex/Grid Fraction
             // Check if string ends right after 'fr'
             if (*(endptr + 2) == '\0') {
                 // LV_GRID_FR expects uint8_t, clamp if necessary
                 if (num_val < 0) num_val = 0;
                 if (num_val > 255) num_val = 255; // Example clamp
                 return LV_GRID_FR((uint8_t)num_val); // Use LV_GRID_FR macro
             } else {
                  UI_BUILDER_LOG_WARN("Invalid coordinate string (extra chars after fr): '%s'", str);
                  return 0;
             }
        } else if (*endptr == '\0') { // Plain number string
            return (lv_coord_t)num_val;
        } else { // Invalid format
             UI_BUILDER_LOG_WARN("Invalid coordinate string format: '%s'", str);
            return 0;
        }
    }
    UI_BUILDER_LOG_WARN("Invalid coordinate JSON type (expected number or string)");
    return 0;
}


// --- Font Registry Functions ---

bool ui_builder_register_font(const char *name, const lv_font_t *font) {
    if (!name || !font) return false;

    // Check if already exists
    for (size_t i = 0; i < g_font_registry_count; ++i) {
        if (g_font_registry[i].name && strcmp(g_font_registry[i].name, name) == 0) {
            UI_BUILDER_LOG_INFO("Updating registered font '%s'", name);
            g_font_registry[i].font = font; // Update pointer
            return true;
        }
    }

    // Grow registry if needed
    if (g_font_registry_count >= g_font_registry_capacity) {
        size_t old_capacity = g_font_registry_capacity;
        size_t new_capacity = (old_capacity == 0) ? 4 : old_capacity * 2;
        ui_builder_font_entry_t *new_registry = realloc(g_font_registry, new_capacity * sizeof(ui_builder_font_entry_t));
        if (!new_registry) {
            UI_BUILDER_LOG_ERROR("Failed to realloc font registry");
            return false;
        }
        g_font_registry = new_registry;
        g_font_registry_capacity = new_capacity;
    }

    // Add new entry
    g_font_registry[g_font_registry_count].name = strdup(name);
    if (!g_font_registry[g_font_registry_count].name) {
        UI_BUILDER_LOG_ERROR("Failed to allocate memory for font name '%s'", name);
        return false;
    }
    g_font_registry[g_font_registry_count].font = font;
    g_font_registry_count++;
    UI_BUILDER_LOG_INFO("Registered font '%s'", name);
    return true;
}

// Helper to get font by name using the registry
static const lv_font_t* get_font_by_name(const char* name) {
    if (!name || strcmp(name, "default") == 0) {
        return LV_FONT_DEFAULT;
    }

    // Check registry
    for (size_t i = 0; i < g_font_registry_count; ++i) {
        if (g_font_registry[i].name && strcmp(g_font_registry[i].name, name) == 0) {
            return g_font_registry[i].font;
        }
    }

    // Check known built-in fonts as a fallback (requires LVGL symbols)
    #if LV_FONT_MONTSERRAT_14
    if (strcmp(name, "montserrat_14") == 0) return &lv_font_montserrat_14;
    #endif
    #if LV_FONT_MONTSERRAT_18
    if (strcmp(name, "montserrat_18") == 0) return &lv_font_montserrat_18;
    #endif
    #if LV_FONT_MONTSERRAT_24
    if (strcmp(name, "montserrat_24") == 0) return &lv_font_montserrat_24;
    #endif
    // Add more built-ins if needed...

    // Check for pointer address format generated by emulator as last resort
     if (strncmp(name, "font_ptr_", 9) == 0) {
        void *ptr = NULL;
        if (sscanf(name + 9, "%p", &ptr) == 1 && ptr != NULL) {
            // WARNING: This is generally unsafe! Using a pointer address from a different
            // run/process is highly unreliable. Only works if the font address is constant
            // across the emulator and builder runs (e.g., ROM constant).
             UI_BUILDER_LOG_WARN("Attempting to use font pointer address '%s'. This is unsafe unless the address is constant.", name);
            return (const lv_font_t*)ptr;
        }
     }


    UI_BUILDER_LOG_WARN("Font '%s' not found in registry or built-ins. Using default.", name);
    return LV_FONT_DEFAULT;
}

// Free the font registry (call this on deinit if needed)
static void ui_builder_free_font_registry() {
    for (size_t i = 0; i < g_font_registry_count; ++i) {
        free(g_font_registry[i].name);
    }
    free(g_font_registry);
    g_font_registry = NULL;
    g_font_registry_count = 0;
    g_font_registry_capacity = 0;
}

// --- Layout/Grid/Flex Parsers ---

static lv_layout_t parse_layout(const char *layout_str) {
    if (!layout_str) return LV_LAYOUT_NONE;
    if (strcmp(layout_str, "flex") == 0) return LV_LAYOUT_FLEX;
    if (strcmp(layout_str, "grid") == 0) return LV_LAYOUT_GRID;
    if (strcmp(layout_str, "none") == 0) return LV_LAYOUT_NONE;
    UI_BUILDER_LOG_WARN("Unknown layout type: %s, using none.", layout_str);
    return LV_LAYOUT_NONE;
}

static lv_grid_align_t parse_grid_align(const char *align_str) {
    if (!align_str) return LV_GRID_ALIGN_START; // Default
    if (strcmp(align_str, "start") == 0) return LV_GRID_ALIGN_START;
    if (strcmp(align_str, "center") == 0) return LV_GRID_ALIGN_CENTER;
    if (strcmp(align_str, "end") == 0) return LV_GRID_ALIGN_END;
    if (strcmp(align_str, "stretch") == 0) return LV_GRID_ALIGN_STRETCH;
    if (strcmp(align_str, "space_evenly") == 0) return LV_GRID_ALIGN_SPACE_EVENLY;
    if (strcmp(align_str, "space_around") == 0) return LV_GRID_ALIGN_SPACE_AROUND;
    if (strcmp(align_str, "space_between") == 0) return LV_GRID_ALIGN_SPACE_BETWEEN;
    UI_BUILDER_LOG_WARN("Unknown grid align value: %s, using start.", align_str);
    return LV_GRID_ALIGN_START;
}

static lv_flex_align_t parse_flex_align(const char *align_str) {
    if (!align_str) return LV_FLEX_ALIGN_START; // Default
    if (strcmp(align_str, "start") == 0) return LV_FLEX_ALIGN_START;
    if (strcmp(align_str, "end") == 0) return LV_FLEX_ALIGN_END;
    if (strcmp(align_str, "center") == 0) return LV_FLEX_ALIGN_CENTER;
    if (strcmp(align_str, "space_evenly") == 0) return LV_FLEX_ALIGN_SPACE_EVENLY;
    if (strcmp(align_str, "space_around") == 0) return LV_FLEX_ALIGN_SPACE_AROUND;
    if (strcmp(align_str, "space_between") == 0) return LV_FLEX_ALIGN_SPACE_BETWEEN;
    UI_BUILDER_LOG_WARN("Unknown flex align value: %s, using start.", align_str);
    return LV_FLEX_ALIGN_START;
}

static lv_flex_flow_t parse_flex_flow(const char *flow_str) {
    if (!flow_str) return LV_FLEX_FLOW_ROW; // Default
    if (strcmp(flow_str, "row") == 0) return LV_FLEX_FLOW_ROW;
    if (strcmp(flow_str, "column") == 0) return LV_FLEX_FLOW_COLUMN;
    if (strcmp(flow_str, "row_wrap") == 0) return LV_FLEX_FLOW_ROW_WRAP;
    if (strcmp(flow_str, "row_reverse") == 0) return LV_FLEX_FLOW_ROW_REVERSE;
    if (strcmp(flow_str, "row_wrap_reverse") == 0) return LV_FLEX_FLOW_ROW_WRAP_REVERSE;
    if (strcmp(flow_str, "column_wrap") == 0) return LV_FLEX_FLOW_COLUMN_WRAP;
    if (strcmp(flow_str, "column_reverse") == 0) return LV_FLEX_FLOW_COLUMN_REVERSE;
    if (strcmp(flow_str, "column_wrap_reverse") == 0) return LV_FLEX_FLOW_COLUMN_WRAP_REVERSE;
    UI_BUILDER_LOG_WARN("Unknown flex flow value: %s, using row.", flow_str);
    return LV_FLEX_FLOW_ROW;
}

static lv_scale_mode_t parse_scale_mode(const char *mode_str) {
    if (!mode_str) return LV_SCALE_MODE_HORIZONTAL_BOTTOM; // Default? Check LVGL default
    if (strcmp(mode_str, "horizontal_top") == 0) return LV_SCALE_MODE_HORIZONTAL_TOP;
    if (strcmp(mode_str, "horizontal_bottom") == 0) return LV_SCALE_MODE_HORIZONTAL_BOTTOM;
    if (strcmp(mode_str, "vertical_left") == 0) return LV_SCALE_MODE_VERTICAL_LEFT;
    if (strcmp(mode_str, "vertical_right") == 0) return LV_SCALE_MODE_VERTICAL_RIGHT;
    if (strcmp(mode_str, "round_inner") == 0) return LV_SCALE_MODE_ROUND_INNER;
    if (strcmp(mode_str, "round_outer") == 0) return LV_SCALE_MODE_ROUND_OUTER;
    UI_BUILDER_LOG_WARN("Unknown scale mode value: %s, using horizontal_bottom.", mode_str);
    return LV_SCALE_MODE_HORIZONTAL_BOTTOM;
}

static lv_grad_dir_t parse_grad_dir(const char *dir_str) {
    if(!dir_str) return LV_GRAD_DIR_NONE;
    if(strcmp(dir_str, "none") == 0) return LV_GRAD_DIR_NONE;
    if(strcmp(dir_str, "ver") == 0) return LV_GRAD_DIR_VER;
    if(strcmp(dir_str, "hor") == 0) return LV_GRAD_DIR_HOR;
    if(strcmp(dir_str, "linear") == 0) return LV_GRAD_DIR_LINEAR;
    if(strcmp(dir_str, "radial") == 0) return LV_GRAD_DIR_RADIAL;
    if(strcmp(dir_str, "conical") == 0) return LV_GRAD_DIR_CONICAL;
    UI_BUILDER_LOG_WARN("Unknown grad dir value: %s, using none.", dir_str);
    return LV_GRAD_DIR_NONE;
}


// Helper to parse a JSON array into a dynamically allocated lv_coord_t array ending with LV_GRID_TEMPLATE_LAST
// The caller is responsible for freeing the returned array!
static lv_coord_t* parse_coord_array_for_grid(cJSON *json_array) {
    if (!cJSON_IsArray(json_array)) {
        UI_BUILDER_LOG_WARN("Expected JSON array for grid descriptor, got other type.");
        return NULL;
    }

    int size = cJSON_GetArraySize(json_array);
    // Need size + 1 for the LV_GRID_TEMPLATE_LAST terminator
    lv_coord_t *coord_array = malloc((size + 1) * sizeof(lv_coord_t));
    if (!coord_array) {
        UI_BUILDER_LOG_ERROR("Failed to allocate memory for grid coordinate array (%d elements)", size);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(json_array, i);
        coord_array[i] = parse_coord(item); // Use the existing coord parser
    }
    coord_array[size] = LV_GRID_TEMPLATE_LAST; // Terminate the array

    return coord_array;
}

// --- Main Recursive Builder ---

static lv_obj_t* create_lvgl_object_recursive(cJSON *json_node, lv_obj_t *parent) {
    if (!json_node || !cJSON_IsObject(json_node)) return NULL;

    cJSON *type_json = cJSON_GetObjectItemCaseSensitive(json_node, "type");
    if (!cJSON_IsString(type_json) || !type_json->valuestring) {
        UI_BUILDER_LOG_ERROR("Object definition missing 'type' string");
        return NULL;
    }
    const char *type = type_json->valuestring;

    // Log the object ID from JSON for correlation if present
    cJSON *id_json = cJSON_GetObjectItemCaseSensitive(json_node, "id");
    const char *json_id_str = cJSON_IsString(id_json) ? id_json->valuestring : "N/A";

    UI_BUILDER_LOG_INFO("Creating object type '%s' (JSON ID: %s) with parent %p", type, json_id_str, (void*)parent);


    lv_obj_t *obj = NULL;

    // --- Create Object based on Type ---
    if (strcmp(type, "obj") == 0 || strcmp(type, "cont") == 0 || strcmp(type, "screen") == 0) {
        // Special case: if type is "screen", we don't create a new object,
        // we use the parent passed in (which should be the actual screen).
        if (strcmp(type, "screen") == 0) {
             obj = parent; // Use the screen object directly
             if (parent != lv_screen_active()) {
                 UI_BUILDER_LOG_WARN("JSON 'screen' type found, but parent %p is not the active screen %p!", (void*)parent, (void*)lv_screen_active());
                 // Still proceed using the passed parent
             } else {
                  UI_BUILDER_LOG_INFO(" Applying properties/styles to existing screen object %p", (void*)obj);
             }
        } else {
             obj = lv_obj_create(parent); // Create a standard object
        }
    } else if (strcmp(type, "label") == 0) {
        obj = lv_label_create(parent);
    } else if (strcmp(type, "btn") == 0) {
        obj = lv_button_create(parent); // Use lv_button_create
    } else if (strcmp(type, "slider") == 0) {
        obj = lv_slider_create(parent);
    }
    else if (strcmp(type, "bar") == 0) {
        obj = lv_bar_create(parent);
    } else if (strcmp(type, "scale") == 0) {
        obj = lv_scale_create(parent);
    }
    // --- Add more widget types here ---
    // Example:
    // else if (strcmp(type, "checkbox") == 0) { obj = lv_checkbox_create(parent); }
    // else if (strcmp(type, "textarea") == 0) { obj = lv_textarea_create(parent); }
    // else if (strcmp(type, "image") == 0) { obj = lv_image_create(parent); }
    // ... etc ...
    else {
        UI_BUILDER_LOG_ERROR("Unknown object type in JSON: '%s'", type);
        return NULL; // Stop creation if type is unknown
    }

    if (!obj) {
        // If obj is NULL here and type wasn't "screen", creation failed
        if (strcmp(type, "screen") != 0) {
            UI_BUILDER_LOG_ERROR("Failed to create object of type: %s", type);
            return NULL;
        }
        // If type was "screen", obj is expected to be the parent (screen)
        if (!obj) {
             UI_BUILDER_LOG_ERROR("Parent (screen) is NULL for 'screen' type node.");
             return NULL;
        }
    }

     UI_BUILDER_LOG_INFO(" Object %p created/assigned for type '%s'", (void*)obj, type);

    // --- Apply Properties ---
    cJSON *props_json = cJSON_GetObjectItemCaseSensitive(json_node, "properties");
    if (cJSON_IsObject(props_json)) {
        apply_properties(obj, props_json);
    }

    // --- Apply Styles ---
    cJSON *styles_json = cJSON_GetObjectItemCaseSensitive(json_node, "styles");
    if (cJSON_IsObject(styles_json)) {
        apply_styles(obj, styles_json);
    }

    // --- Create Children ---
    // Do not create children for the special "screen" type node,
    // as its children in JSON represent top-level objects on the screen.
    if (strcmp(type, "screen") != 0) {
        cJSON *children_json = cJSON_GetObjectItemCaseSensitive(json_node, "children");
        if (cJSON_IsArray(children_json)) {
            cJSON *child_json = NULL;
            cJSON_ArrayForEach(child_json, children_json) {
                // Pass the newly created object 'obj' as the parent for its children
                lv_obj_t *child_obj = create_lvgl_object_recursive(child_json, obj);
                if (!child_obj) {
                     UI_BUILDER_LOG_ERROR("Failed to create child object for parent %p (type %s). Skipping remaining children.", (void*)obj, type);
                     // Optionally stop processing further children of this node on error
                     // return NULL; // Or just continue
                }
            }
        }
    } else {
        // If the current node is the "screen" type, its "children" in JSON
        // should be created directly on the screen object.
        cJSON *children_json = cJSON_GetObjectItemCaseSensitive(json_node, "children");
        if (cJSON_IsArray(children_json)) {
             UI_BUILDER_LOG_INFO(" Creating screen's top-level children...");
            cJSON *child_json = NULL;
            cJSON_ArrayForEach(child_json, children_json) {
                 // Pass the screen object 'obj' as the parent
                 lv_obj_t *child_obj = create_lvgl_object_recursive(child_json, obj);
                 if (!child_obj) {
                      UI_BUILDER_LOG_ERROR("Failed to create top-level child object for screen %p. Skipping remaining children.", (void*)obj);
                 }
            }
        }

    }


    return obj;
}

// --- Property Application ---

static void apply_properties(lv_obj_t *obj, cJSON *props_json) {
    cJSON *prop = NULL;
    // Temp storage for multi-part properties that need applying together
    lv_coord_t* temp_col_dsc = NULL;
    lv_coord_t* temp_row_dsc = NULL;
    lv_grid_align_t temp_grid_col_align = LV_GRID_ALIGN_START; // Default
    lv_grid_align_t temp_grid_row_align = LV_GRID_ALIGN_START; // Default
    bool grid_align_set = false; // Track if grid align was explicitly set

    lv_flex_align_t temp_flex_main = LV_FLEX_ALIGN_START;
    lv_flex_align_t temp_flex_cross = LV_FLEX_ALIGN_START;
    lv_flex_align_t temp_flex_track = LV_FLEX_ALIGN_START;
    bool flex_align_set = false; // Track if flex align was explicitly set

    int32_t temp_range_min = 0, temp_range_max = 100; // For slider/bar/scale defaults
    bool range_min_set = false, range_max_set = false;

    // --- First Pass: Apply simple, direct properties ---
    cJSON_ArrayForEach(prop, props_json) {
        const char *key = prop->string;
        if (!key) continue;

        // --- Common Properties ---
        if (strcmp(key, "width") == 0) lv_obj_set_width(obj, parse_coord(prop));
        else if (strcmp(key, "height") == 0) lv_obj_set_height(obj, parse_coord(prop));
        else if (strcmp(key, "x") == 0) lv_obj_set_x(obj, parse_coord(prop)); // Coord can be pct
        else if (strcmp(key, "y") == 0) lv_obj_set_y(obj, parse_coord(prop)); // Coord can be pct
        else if (strcmp(key, "align") == 0 && cJSON_IsString(prop)) lv_obj_set_align(obj, parse_align(prop->valuestring));

        // --- Flags ---
        // Handle individual flags if needed (e.g., "clickable": true)
        // Or handle a single "flags" property that sets all flags at once
        else if (strcmp(key, "flags") == 0 && cJSON_IsNumber(prop)) {
            // Overwrite all flags (consider clearing existing first?)
            uint32_t flags_to_set = (uint32_t)cJSON_GetNumberValue(prop);
             // lv_obj_remove_flag(obj, ~0); // Clear all existing - maybe too destructive?
             // Let's just add the flags specified in the JSON.
             // If clearing is needed, the JSON should explicitly list flags to remove?
             // Or we assume "flags" property OVERWRITES all others. Let's assume overwrite for now.
             // This is complex. Let's stick to ADDING flags from the JSON value for now.
             // To achieve overwrite, one would need to know the original flags.
             // For simplicity: This will ADD the flags from the number.
             lv_obj_add_flag(obj, flags_to_set);
             UI_BUILDER_LOG_INFO(" Added flags 0x%X to obj %p", flags_to_set, (void*)obj);

             // Alternative: if specific flag keys exist, handle them:
             // else if (strcmp(key, "clickable") == 0 && cJSON_IsBool(prop)) {
             //     if(cJSON_IsTrue(prop)) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
             //     else lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
             // } // etc.
        }

        // --- Layout Base Type ---
        else if (strcmp(key, "layout") == 0 && cJSON_IsString(prop)) {
             lv_obj_set_layout(obj, parse_layout(prop->valuestring));
        }

        // --- Widget Specific Simple Properties ---
        // Label
        else if (strcmp(key, "text") == 0 && cJSON_IsString(prop) && lv_obj_has_class(obj, &lv_label_class)) {
            lv_label_set_text(obj, prop->valuestring);
        }
        // Slider / Bar Value (handle both 'value' and 'val')
        else if ((strcmp(key, "value") == 0 || strcmp(key, "val") == 0) && cJSON_IsNumber(prop)) {
             if (lv_obj_has_class(obj, &lv_slider_class)) {
                lv_slider_set_value(obj, (int32_t)cJSON_GetNumberValue(prop), LV_ANIM_OFF);
             } else if (lv_obj_has_class(obj, &lv_bar_class)) {
                 lv_bar_set_value(obj, (int32_t)cJSON_GetNumberValue(prop), LV_ANIM_OFF);
             }
             // Scale doesn't have a simple 'set_value'
        }
        // Scale Major Tick
         else if (strcmp(key, "scale_major_tick_every") == 0 && cJSON_IsNumber(prop) && lv_obj_has_class(obj, &lv_scale_class)) {
             lv_scale_set_major_tick_every(obj, (uint32_t)cJSON_GetNumberValue(prop));
         }
         // Scale Mode
         else if (strcmp(key, "scale_mode") == 0 && cJSON_IsString(prop) && lv_obj_has_class(obj, &lv_scale_class)) {
             lv_scale_set_mode(obj, parse_scale_mode(prop->valuestring));
         }

        // --- Store values for multi-part properties ---
        // Flexbox Align
        else if (strcmp(key, "flex_align_main_place") == 0 && cJSON_IsString(prop)) { temp_flex_main = parse_flex_align(prop->valuestring); flex_align_set = true; }
        else if (strcmp(key, "flex_align_cross_place") == 0 && cJSON_IsString(prop)) { temp_flex_cross = parse_flex_align(prop->valuestring); flex_align_set = true; }
        else if (strcmp(key, "flex_align_track_cross_place") == 0 && cJSON_IsString(prop)) { temp_flex_track = parse_flex_align(prop->valuestring); flex_align_set = true; }
        // Flexbox Flow (can be applied directly)
        else if (strcmp(key, "flex_flow") == 0 && cJSON_IsString(prop)) {
            lv_obj_set_flex_flow(obj, parse_flex_flow(prop->valuestring));
        }
        // Flex Grow (can be applied directly)
        else if (strcmp(key, "flex_grow") == 0 && cJSON_IsNumber(prop)) {
            lv_obj_set_flex_grow(obj, (uint8_t)cJSON_GetNumberValue(prop));
        }
        // Grid DSC Arrays
        else if (strcmp(key, "grid_dsc_array_col_dsc") == 0 && cJSON_IsArray(prop)) {
            free(temp_col_dsc); // Free previous if reparsed
            temp_col_dsc = parse_coord_array_for_grid(prop);
        }
        else if (strcmp(key, "grid_dsc_array_row_dsc") == 0 && cJSON_IsArray(prop)) {
            free(temp_row_dsc); // Free previous if reparsed
            temp_row_dsc = parse_coord_array_for_grid(prop);
        }
        // Grid Align
        else if (strcmp(key, "grid_column_align") == 0 && cJSON_IsString(prop)) { temp_grid_col_align = parse_grid_align(prop->valuestring); grid_align_set = true; }
        else if (strcmp(key, "grid_row_align") == 0 && cJSON_IsString(prop)) { temp_grid_row_align = parse_grid_align(prop->valuestring); grid_align_set = true; }
        // Grid Cell (Applied directly as these are per-child)
        else if (strcmp(key, "grid_cell_column_align") == 0 && cJSON_IsString(prop)) {
            // Assume other grid_cell props are also present if this one is
            lv_grid_align_t cell_col_align = parse_grid_align(prop->valuestring);
            int32_t col_pos = 0;
            int32_t col_span = 1;
            lv_grid_align_t cell_row_align = LV_GRID_ALIGN_STRETCH; // Default cell align? Check LVGL
            int32_t row_pos = 0;
            int32_t row_span = 1;

            cJSON *val = cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_col_pos");
            if(cJSON_IsNumber(val)) col_pos = (int32_t)val->valuedouble;
            val = cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_col_span");
            if(cJSON_IsNumber(val)) col_span = (int32_t)val->valuedouble;

            val = cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_row_align");
             if(cJSON_IsString(val)) cell_row_align = parse_grid_align(val->valuestring);
             val = cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_row_pos");
             if(cJSON_IsNumber(val)) row_pos = (int32_t)val->valuedouble;
             val = cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_row_span");
             if(cJSON_IsNumber(val)) row_span = (int32_t)val->valuedouble;

             lv_obj_set_grid_cell(obj, cell_col_align, col_pos, col_span, cell_row_align, row_pos, row_span);
             UI_BUILDER_LOG_INFO(" Applied grid cell to obj %p: col(pos:%d span:%d align:%d) row(pos:%d span:%d align:%d)",
                                (void*)obj, col_pos, col_span, cell_col_align, row_pos, row_span, cell_row_align);
        }
        // Range (Slider/Bar/Scale)
        else if (strcmp(key, "range_min") == 0 && cJSON_IsNumber(prop)) { temp_range_min = (int32_t)cJSON_GetNumberValue(prop); range_min_set = true; }
        else if (strcmp(key, "range_max") == 0 && cJSON_IsNumber(prop)) { temp_range_max = (int32_t)cJSON_GetNumberValue(prop); range_max_set = true; }
         // Handle scale specific aliases if needed ("scale_range_min")
         else if (strcmp(key, "scale_range_min") == 0 && cJSON_IsNumber(prop)) { temp_range_min = (int32_t)cJSON_GetNumberValue(prop); range_min_set = true; }
         else if (strcmp(key, "scale_range_max") == 0 && cJSON_IsNumber(prop)) { temp_range_max = (int32_t)cJSON_GetNumberValue(prop); range_max_set = true; }

        // --- Other property handlers ---
        else {
            // Don't warn about keys handled in multi-part application below
            if (strncmp(key, "grid_cell_", 10) != 0 &&
                strncmp(key, "flex_align_", 11) != 0 &&
                 strncmp(key, "grid_dsc_array_", 15) != 0 &&
                 strncmp(key, "grid_", 5) != 0 && // Catches grid_column/row_align
                 strncmp(key, "range_", 6) != 0 &&
                 strncmp(key, "scale_range_", 12) != 0 )
             {
                UI_BUILDER_LOG_WARN("Unknown or unhandled property: '%s' for object type %s", key, lv_obj_get_class(obj)->name);
             }
        }
    }

    // --- Second Pass: Apply multi-part properties ---
    if (temp_col_dsc || temp_row_dsc) {
         // LVGL requires both arrays, even if one is NULL (which parse returns if not found/invalid)
         UI_BUILDER_LOG_INFO(" Applying grid dsc array to obj %p (col: %p, row: %p)", (void*)obj, (void*)temp_col_dsc, (void*)temp_row_dsc);
        lv_obj_set_grid_dsc_array(obj, temp_col_dsc, temp_row_dsc);
        // Free the arrays after LVGL has potentially copied them (check LVGL docs)
        // Assuming LVGL copies the descriptor arrays, we free them.
        // If LVGL uses the pointers directly, DO NOT FREE here.
        // Based on LVGL v8/v9 behavior, it *copies* the descriptors.
        free(temp_col_dsc);
        free(temp_row_dsc);
    }

    if (grid_align_set) {
        UI_BUILDER_LOG_INFO(" Applying grid align to obj %p (col: %d, row: %d)", (void*)obj, temp_grid_col_align, temp_grid_row_align);
        lv_obj_set_grid_align(obj, temp_grid_col_align, temp_grid_row_align);
    }

    if (flex_align_set) {
        UI_BUILDER_LOG_INFO(" Applying flex align to obj %p (main: %d, cross: %d, track: %d)", (void*)obj, temp_flex_main, temp_flex_cross, temp_flex_track);
        lv_obj_set_flex_align(obj, temp_flex_main, temp_flex_cross, temp_flex_track);
    }

    if (range_min_set || range_max_set) {
         // Apply range to relevant widgets
         if (lv_obj_has_class(obj, &lv_slider_class)) {
             // If only one was set, get the current value for the other?
             if (!range_min_set) temp_range_min = lv_slider_get_min_value(obj);
             if (!range_max_set) temp_range_max = lv_slider_get_max_value(obj);
             UI_BUILDER_LOG_INFO(" Applying slider range to obj %p (min: %d, max: %d)", (void*)obj, temp_range_min, temp_range_max);
             lv_slider_set_range(obj, temp_range_min, temp_range_max);
         } else if (lv_obj_has_class(obj, &lv_bar_class)) {
             if (!range_min_set) temp_range_min = lv_bar_get_min_value(obj);
             if (!range_max_set) temp_range_max = lv_bar_get_max_value(obj);
             UI_BUILDER_LOG_INFO(" Applying bar range to obj %p (min: %d, max: %d)", (void*)obj, temp_range_min, temp_range_max);
             lv_bar_set_range(obj, temp_range_min, temp_range_max);
         } else if (lv_obj_has_class(obj, &lv_scale_class)) {
             // Scale has its own range function, assuming defaults if not set
             UI_BUILDER_LOG_INFO(" Applying scale range to obj %p (min: %d, max: %d)", (void*)obj, temp_range_min, temp_range_max);
             lv_scale_set_range(obj, temp_range_min, temp_range_max);
         }
    }
}


// --- Style Application Helper ---

static void apply_single_style_prop(lv_obj_t *obj, const char *prop_key, cJSON *style_prop, lv_style_selector_t selector) {
    if (!prop_key || !style_prop) return;

    // --- Apply Style Properties ---
    if (strcmp(prop_key, "bg_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_bg_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "bg_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_bg_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "radius") == 0) lv_obj_set_style_radius(obj, parse_coord(style_prop), selector);

    else if (strcmp(prop_key, "border_width") == 0) lv_obj_set_style_border_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "border_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_border_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "border_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_border_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);

    else if (strcmp(prop_key, "outline_width") == 0) lv_obj_set_style_outline_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "outline_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_outline_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "outline_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_outline_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "outline_pad") == 0) lv_obj_set_style_outline_pad(obj, parse_coord(style_prop), selector);

    else if (strcmp(prop_key, "pad_all") == 0) lv_obj_set_style_pad_all(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "pad_top") == 0) lv_obj_set_style_pad_top(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "pad_left") == 0) lv_obj_set_style_pad_left(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "pad_right") == 0) lv_obj_set_style_pad_right(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "pad_bottom") == 0) lv_obj_set_style_pad_bottom(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "pad_row") == 0) lv_obj_set_style_pad_row(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "pad_column") == 0) lv_obj_set_style_pad_column(obj, parse_coord(style_prop), selector);

    else if (strcmp(prop_key, "margin_all") == 0) lv_obj_set_style_margin_all(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "margin_top") == 0) lv_obj_set_style_margin_top(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "margin_left") == 0) lv_obj_set_style_margin_left(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "margin_right") == 0) lv_obj_set_style_margin_right(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "margin_bottom") == 0) lv_obj_set_style_margin_bottom(obj, parse_coord(style_prop), selector);

    else if (strcmp(prop_key, "width") == 0) lv_obj_set_style_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "height") == 0) lv_obj_set_style_height(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "min_width") == 0) lv_obj_set_style_min_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "max_width") == 0) lv_obj_set_style_max_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "min_height") == 0) lv_obj_set_style_min_height(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "max_height") == 0) lv_obj_set_style_max_height(obj, parse_coord(style_prop), selector);

    else if (strcmp(prop_key, "text_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "text_font") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_font(obj, get_font_by_name(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "text_align") == 0 && cJSON_IsString(style_prop)) {
        lv_text_align_t align = LV_TEXT_ALIGN_AUTO; // Default
        const char* align_str = style_prop->valuestring;
        if(strcmp(align_str, "left") == 0) align = LV_TEXT_ALIGN_LEFT;
        else if(strcmp(align_str, "center") == 0) align = LV_TEXT_ALIGN_CENTER;
        else if(strcmp(align_str, "right") == 0) align = LV_TEXT_ALIGN_RIGHT;
        else if(strcmp(align_str, "auto") != 0) { // Warn if not known and not auto
             UI_BUILDER_LOG_WARN("Unknown text_align value: %s", align_str);
        }
        lv_obj_set_style_text_align(obj, align, selector);
    }
     else if (strcmp(prop_key, "text_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_text_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
     else if (strcmp(prop_key, "line_width") == 0) lv_obj_set_style_line_width(obj, parse_coord(style_prop), selector);
     else if (strcmp(prop_key, "line_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_line_color(obj, parse_color(style_prop->valuestring), selector);
     else if (strcmp(prop_key, "line_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_line_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
     // Arc properties...
     else if (strcmp(prop_key, "arc_width") == 0) lv_obj_set_style_arc_width(obj, parse_coord(style_prop), selector);
     else if (strcmp(prop_key, "arc_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_arc_color(obj, parse_color(style_prop->valuestring), selector);
     else if (strcmp(prop_key, "arc_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_arc_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
     // Gradient properties...
    else if (strcmp(prop_key, "bg_grad_dir") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_bg_grad_dir(obj, parse_grad_dir(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "bg_main_stop") == 0) lv_obj_set_style_bg_main_stop(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "bg_grad_stop") == 0) lv_obj_set_style_bg_grad_stop(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "bg_grad_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_bg_grad_color(obj, parse_color(style_prop->valuestring), selector);
    // Flex/Grid styles (can also be properties)
    else if (strcmp(prop_key, "flex_flow") == 0 && cJSON_IsString(style_prop)) {
        lv_obj_set_style_flex_flow(obj, parse_flex_flow(style_prop->valuestring), selector);
    }
    // Add more style properties here... e.g., shadow, image, transform, etc.

    else {
        UI_BUILDER_LOG_WARN("Unknown or unhandled style property: '%s' for part/state selector 0x%X", prop_key, selector);
    }
}

// --- Style Application Main Function ---
static void apply_styles(lv_obj_t *obj, cJSON *styles_json) {
    cJSON *part_style_obj = NULL;

    // Iterate through parts ("default", "indicator", "knob", etc.)
    cJSON_ArrayForEach(part_style_obj, styles_json) {
        const char *part_key = part_style_obj->string;
        if (!part_key || !cJSON_IsObject(part_style_obj)) continue;

        lv_part_t part = parse_part(part_key);

        cJSON *state_style_obj = NULL;
        // Iterate through states ("default", "pressed", etc.) within the part
        cJSON_ArrayForEach(state_style_obj, part_style_obj) {
            const char *state_key = state_style_obj->string;
            if (!state_key || !cJSON_IsObject(state_style_obj)) continue;

            lv_state_t state = parse_state(state_key);
            lv_style_selector_t selector = part | state;

            cJSON *style_prop = NULL;
            // Iterate through the actual style properties (bg_color, radius, etc.)
            cJSON_ArrayForEach(style_prop, state_style_obj) {
                apply_single_style_prop(obj, style_prop->string, style_prop, selector);
            }
        }
    }
}


// --- Public API ---

bool build_ui_from_json(const char *json_string) {
    if (!json_string) {
        UI_BUILDER_LOG_ERROR("build_ui_from_json called with NULL string");
        return false;
    }

    cJSON *root_json = cJSON_Parse(json_string);
    if (root_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            // Calculate rough position for context
            int error_offset = (int)(error_ptr - json_string);
            int context_before = (error_offset > 20) ? 20 : error_offset;
            int context_after = 20;
            char context_buf[41]; // 20 + 1 + 20
            strncpy(context_buf, error_ptr - context_before, 40);
            context_buf[40] = '\0';
            UI_BUILDER_LOG_ERROR("JSON Parse Error near offset %d: ...%s...", error_offset, context_buf);
        } else {
            UI_BUILDER_LOG_ERROR("JSON Parse Error (unknown location)");
        }
        return false;
    }

    // Expecting the actual UI tree under a "root" key
    cJSON *root_obj_json = cJSON_GetObjectItemCaseSensitive(root_json, "root");
    if (!cJSON_IsObject(root_obj_json)) {
         UI_BUILDER_LOG_ERROR("JSON missing 'root' object or it's not an object");
         cJSON_Delete(root_json);
         return false;
    }

    // --- Get Active Screen ---
    lv_obj_t *scr = lv_screen_active(); // lv_scr_act() deprecated
    if (!scr) {
         UI_BUILDER_LOG_ERROR("No active screen found! Cannot build UI.");
         cJSON_Delete(root_json);
         return false;
    }

    // --- Clear Screen ---
    UI_BUILDER_LOG_INFO("Cleaning active screen %p...", (void*)scr);
    lv_obj_clean(scr); // Remove all children from the screen

    // --- Build UI ---
    UI_BUILDER_LOG_INFO("Building UI recursively from JSON root...");
    // The root node in JSON often represents the screen itself.
    // create_lvgl_object_recursive handles the "screen" type specially.
    lv_obj_t *created_root = create_lvgl_object_recursive(root_obj_json, scr);

    cJSON_Delete(root_json); // Clean up cJSON object tree

    if (!created_root) {
        // If the root was the "screen" type, created_root will be == scr,
        // so this check might indicate failure during processing screen properties/styles/children.
        // If root was a different type, it means the top-level object failed creation.
        UI_BUILDER_LOG_ERROR("Failed to create root UI element(s) from JSON");
        // Screen might be partially populated if error occurred mid-build.
        // Consider cleaning again? lv_obj_clean(scr);
        return false;
    }
     // Check if the returned object is indeed the screen if the root type was screen
     if(cJSON_IsObject(root_obj_json)) {
         cJSON *type_json = cJSON_GetObjectItemCaseSensitive(root_obj_json, "type");
         if(cJSON_IsString(type_json) && strcmp(type_json->valuestring, "screen") == 0 && created_root != scr) {
             UI_BUILDER_LOG_WARN("JSON root type was 'screen', but builder returned %p instead of screen %p", (void*)created_root, (void*)scr);
         }
     }


    UI_BUILDER_LOG_INFO("UI Built Successfully from JSON on screen %p", (void*)scr);

    // Optional: Invalidate the screen to force redraw
    lv_obj_invalidate(scr);

    return true;
}
