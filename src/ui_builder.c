// ui_builder.c

#include "ui_builder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For strtol, strtod

// Include necessary LVGL headers for widget creation and property setting
#include "../../lvgl/lvgl.h" // Main header likely includes core and basic widgets
// --- Implementation of TODOs: Include headers for new widgets ---
#include "../../lvgl/src/widgets/bar/lv_bar.h"
#include "../../lvgl/src/widgets/scale/lv_scale.h"
// --- End Implementation of TODOs ---
// Need class info for type checking
#include "../../lvgl/src/core/lv_obj_class_private.h"


// --- Logging Macros ---
// Define simple logging macros if not provided elsewhere
#ifndef LOG_INFO
#define LOG_INFO(s, ...) printf("[INFO] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(s, ...) printf("[ERROR] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif
#ifndef LOG_WARN
#define LOG_WARN(s, ...) printf("[WARN] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif
#ifndef LOG_USER
#define LOG_USER(s, ...) printf("[USER] " s "\n" __VA_OPT__(,) __VA_ARGS__)
#endif


// --- Forward Declarations ---
static lv_obj_t* create_lvgl_object_recursive(cJSON *json_node, lv_obj_t *parent);
static void apply_properties(lv_obj_t *obj, cJSON *props_json);
static void apply_styles(lv_obj_t *obj, cJSON *styles_json);
static void apply_single_style_prop(lv_obj_t *obj, const char *prop_key, cJSON *style_prop, lv_style_selector_t selector);


// --- Helper Functions ---

// Helper to parse color string (e.g., "#RRGGBB" or "#RGB")
static lv_color_t parse_color(const char *color_str) {
    if (!color_str || color_str[0] != '#') {
        return lv_color_black(); // Default or error color
    }
    uint32_t cval = (uint32_t)strtol(color_str + 1, NULL, 16);
    if (strlen(color_str) == 7) { // #RRGGBB
        return lv_color_hex(cval);
    } else if (strlen(color_str) == 4) { // #RGB -> #RRGGBB
        uint8_t r = (cval >> 8) & 0xF;
        uint8_t g = (cval >> 4) & 0xF;
        uint8_t b = cval & 0xF;
        return lv_color_make(r * 17, g * 17, b * 17);
    }
     LOG_WARN("Invalid color string format: %s", color_str);
    return lv_color_black();
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
    LOG_WARN("Unknown align value: %s, using default.", align_str);
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
    // Combine states if needed later (e.g., "focused | checked")
    // Check for numeric state values if the emulator generates them
    if (strncmp(state_str, "state_", 6) == 0) {
        return (lv_state_t)strtoul(state_str + 6, NULL, 10);
    }
    LOG_WARN("Unknown state value: %s, using default.", state_str);
    return LV_STATE_DEFAULT;
}

// Helper to parse part string (specific to widgets like slider, bar, etc.)
static lv_part_t parse_part(const char *part_str) {
    if (!part_str || strcmp(part_str, "default") == 0) return LV_PART_MAIN;
    if (strcmp(part_str, "indicator") == 0) return LV_PART_INDICATOR;
    if (strcmp(part_str, "knob") == 0) return LV_PART_KNOB;
    if (strcmp(part_str, "scrollbar") == 0) return LV_PART_SCROLLBAR;
    if (strcmp(part_str, "selected") == 0) return LV_PART_SELECTED;
    if (strcmp(part_str, "items") == 0) return LV_PART_ITEMS;
    if (strcmp(part_str, "cursor") == 0) return LV_PART_CURSOR;
    // Add more parts as needed
    // Check for numeric part values if the emulator generates them
    if (strncmp(part_str, "unknown_part_", 13) == 0) {
        return (lv_part_t)strtoul(part_str + 13, NULL, 10);
    }
    LOG_WARN("Unknown part value: %s, using main part.", part_str);
    return LV_PART_MAIN;
}


// Helper to parse coordinate (can be number, percentage string, or "content")
static lv_coord_t parse_coord(cJSON *value) {
    if (cJSON_IsNumber(value)) {
        return (lv_coord_t)cJSON_GetNumberValue(value);
    } else if (cJSON_IsString(value)) {
        const char *str = cJSON_GetStringValue(value);
        if (strcmp(str, "content") == 0) {
            return LV_SIZE_CONTENT;
        }
        char *endptr;
        long pct_val = strtol(str, &endptr, 10);
        // Check if the string ends with '%' immediately after the number
        if (endptr != str && *endptr == '%' && *(endptr + 1) == '\0') {
             if (pct_val < -LV_PCT_POS_MAX) pct_val = -LV_PCT_POS_MAX; // Clamp PCT range
             if (pct_val > LV_PCT_POS_MAX) pct_val = LV_PCT_POS_MAX;
            return lv_pct((int32_t)pct_val); // lv_pct takes int32_t
        } else {
             LOG_WARN("Invalid coordinate string format: '%s' (expected number, 'N%%', or 'content')", str);
            return 0;
        }
    }
     LOG_WARN("Invalid coordinate type (expected number or string)");
    return 0;
}

// Helper to get font by name (requires fonts to be enabled in lv_conf.h)
// Relies on standard LVGL font symbols being available.
static const lv_font_t* get_font_by_name(const char* name) {
    if (!name || strcmp(name, "default") == 0) return LV_FONT_DEFAULT;

    // --- Add more fonts as needed ---
    // --- Make sure they are enabled in lv_conf.h and linked ---

    #if LV_FONT_MONTSERRAT_14 // Check if enabled
    if (strcmp(name, "montserrat_14") == 0) return &lv_font_montserrat_14;
    #endif
    #if LV_FONT_MONTSERRAT_18
    if (strcmp(name, "montserrat_18") == 0) return &lv_font_montserrat_18;
    #endif
    #if LV_FONT_MONTSERRAT_10
    if (strcmp(name, "montserrat_10") == 0) return &lv_font_montserrat_10;
    #endif
     #if LV_FONT_MONTSERRAT_12
    if (strcmp(name, "montserrat_12") == 0) return &lv_font_montserrat_12;
    #endif
     #if LV_FONT_MONTSERRAT_16
    if (strcmp(name, "montserrat_16") == 0) return &lv_font_montserrat_16;
    #endif
     #if LV_FONT_MONTSERRAT_20
    if (strcmp(name, "montserrat_20") == 0) return &lv_font_montserrat_20;
    #endif
     #if LV_FONT_MONTSERRAT_24
    if (strcmp(name, "montserrat_24") == 0) return &lv_font_montserrat_24;
    #endif
    // Add other standard fonts or custom fonts here...

    // Fallback
    LOG_WARN("Font '%s' not found or not enabled in lv_conf.h, using default.", name);
    return LV_FONT_DEFAULT;
}

static lv_layout_t parse_layout(const char *layout_str) {
    if (!layout_str) return LV_LAYOUT_NONE;
    if (strcmp(layout_str, "flex") == 0) return LV_LAYOUT_FLEX;
    if (strcmp(layout_str, "grid") == 0) return LV_LAYOUT_GRID;
    if (strcmp(layout_str, "none") == 0) return LV_LAYOUT_NONE;
    LOG_WARN("Unknown layout type: %s, using none.", layout_str);
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
    LOG_WARN("Unknown grid align value: %s, using start.", align_str);
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
    LOG_WARN("Unknown flex align value: %s, using start.", align_str);
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
    LOG_WARN("Unknown flex flow value: %s, using row.", flow_str);
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
    LOG_WARN("Unknown scale mode value: %s, using horizontal_bottom.", mode_str);
    return LV_SCALE_MODE_HORIZONTAL_BOTTOM;
}

// Helper to parse a JSON array into a dynamically allocated lv_coord_t array ending with LV_GRID_TEMPLATE_LAST
static lv_coord_t* parse_coord_array_for_grid(cJSON *json_array) {
    if (!cJSON_IsArray(json_array)) return NULL;

    int size = cJSON_GetArraySize(json_array);
    lv_coord_t *coord_array = malloc((size + 1) * sizeof(lv_coord_t)); // +1 for terminator
    if (!coord_array) {
        LOG_ERROR("Failed to allocate memory for grid coordinate array");
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(json_array, i);
        // Need to parse each item as a coordinate (number or pct string)
        if (cJSON_IsNumber(item)) {
            coord_array[i] = (lv_coord_t)cJSON_GetNumberValue(item);
        } else if (cJSON_IsString(item)) {
            const char *str = cJSON_GetStringValue(item);
            if (strcmp(str, "content") == 0) {
                 coord_array[i] = LV_GRID_CONTENT;
            } else {
                char *endptr;
                long val = strtol(str, &endptr, 10);
                if (endptr != str && *endptr == '%' && *(endptr+1) == '\0') {
                    coord_array[i] = LV_GRID_FR((uint8_t)val); // Assuming FR units map directly to %
                } else if (endptr != str && *endptr == '\0') {
                    coord_array[i] = (lv_coord_t)val; // Plain number string? Treat as pixels
                } else {
                    LOG_WARN("Invalid grid descriptor value in array: %s", str);
                    coord_array[i] = 0; // Default to 0 on error
                }
            }
        } else {
            LOG_WARN("Invalid type in grid descriptor array (expected number or string)");
            coord_array[i] = 0; // Default to 0 on error
        }
    }
    coord_array[size] = LV_GRID_TEMPLATE_LAST; // Terminate the array

    return coord_array;
}

// --- Main Recursive Builder ---

static lv_obj_t* create_lvgl_object_recursive(cJSON *json_node, lv_obj_t *parent) {
    if (!json_node || !cJSON_IsObject(json_node)) return NULL;

    cJSON *type_json = cJSON_GetObjectItemCaseSensitive(json_node, "type");
    if (!cJSON_IsString(type_json) || !type_json->valuestring) {
        LOG_ERROR("Object definition missing 'type'");
        return NULL;
    }
    const char *type = type_json->valuestring;

    lv_obj_t *obj = NULL;

    // --- Create Object based on Type ---
    if (strcmp(type, "obj") == 0 || strcmp(type, "cont") == 0) { // Treat cont as obj for simplicity
        obj = lv_obj_create(parent);
    } else if (strcmp(type, "label") == 0) {
        obj = lv_label_create(parent);
    } else if (strcmp(type, "btn") == 0) {
        obj = lv_btn_create(parent);
    } else if (strcmp(type, "slider") == 0) {
        obj = lv_slider_create(parent);
    }
    else if (strcmp(type, "bar") == 0) {
        obj = lv_bar_create(parent);
    } else if (strcmp(type, "scale") == 0) {
        obj = lv_scale_create(parent);
    }
    // --- Add more widget types here ---
    // else if (strcmp(type, "checkbox") == 0) { obj = lv_checkbox_create(parent); }
    // else if (strcmp(type, "textarea") == 0) { obj = lv_textarea_create(parent); }
    else {
        LOG_ERROR("Unknown object type: %s", type);
        return NULL;
    }

    if (!obj) {
        LOG_ERROR("Failed to create object of type: %s", type);
        return NULL;
    }

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
    cJSON *children_json = cJSON_GetObjectItemCaseSensitive(json_node, "children");
    if (cJSON_IsArray(children_json)) {
        cJSON *child_json = NULL;
        cJSON_ArrayForEach(child_json, children_json) {
            create_lvgl_object_recursive(child_json, obj); // Recursively create children
        }
    }

    return obj;
}

// --- Property Application ---

static void apply_properties(lv_obj_t *obj, cJSON *props_json) {
    cJSON *prop = NULL;
    // Temp storage for multi-part properties
    lv_coord_t* temp_col_dsc = NULL;
    lv_coord_t* temp_row_dsc = NULL;
    lv_grid_align_t temp_grid_col_align = LV_GRID_ALIGN_START; // Default
    lv_grid_align_t temp_grid_row_align = LV_GRID_ALIGN_START; // Default
    lv_flex_align_t temp_flex_main = LV_FLEX_ALIGN_START;
    lv_flex_align_t temp_flex_cross = LV_FLEX_ALIGN_START;
    lv_flex_align_t temp_flex_track = LV_FLEX_ALIGN_START;
    int32_t temp_scale_min = 0, temp_scale_max = 100; // Scale defaults

    cJSON_ArrayForEach(prop, props_json) {
        const char *key = prop->string;
        if (!key) continue;

        // --- Common Properties ---
        if (strcmp(key, "width") == 0) lv_obj_set_width(obj, parse_coord(prop));
        else if (strcmp(key, "height") == 0) lv_obj_set_height(obj, parse_coord(prop));
        else if (strcmp(key, "x") == 0 && cJSON_IsNumber(prop)) lv_obj_set_x(obj, (lv_coord_t)cJSON_GetNumberValue(prop));
        else if (strcmp(key, "y") == 0 && cJSON_IsNumber(prop)) lv_obj_set_y(obj, (lv_coord_t)cJSON_GetNumberValue(prop));
        else if (strcmp(key, "align") == 0 && cJSON_IsString(prop)) lv_obj_set_align(obj, parse_align(prop->valuestring));
        else if (strcmp(key, "flags") == 0 && cJSON_IsNumber(prop)) {
            // Overwrite all flags at once if this property exists
            lv_obj_remove_flag(obj, ~0); // Clear all flags
            lv_obj_add_flag(obj, (uint32_t)cJSON_GetNumberValue(prop));
        }
        // Specific flags (can be overridden by "flags" property if present)
        else if (strcmp(key, "hidden") == 0 && cJSON_IsBool(prop)) {
            if (cJSON_IsTrue(prop)) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        else if (strcmp(key, "clickable") == 0 && cJSON_IsBool(prop)) {
             if (cJSON_IsTrue(prop)) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
             else lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
         // Add more common flags here

        // --- Layout Properties ---
        else if (strcmp(key, "layout") == 0 && cJSON_IsString(prop)) {
             lv_obj_set_layout(obj, parse_layout(prop->valuestring));
        }
        // Flexbox
        else if (strcmp(key, "flex_flow") == 0 && cJSON_IsString(prop)) {
            lv_obj_set_flex_flow(obj, parse_flex_flow(prop->valuestring));
        }
        else if (strcmp(key, "flex_align_main_place") == 0 && cJSON_IsString(prop)) temp_flex_main = parse_flex_align(prop->valuestring);
        else if (strcmp(key, "flex_align_cross_place") == 0 && cJSON_IsString(prop)) temp_flex_cross = parse_flex_align(prop->valuestring);
        else if (strcmp(key, "flex_align_track_cross_place") == 0 && cJSON_IsString(prop)) temp_flex_track = parse_flex_align(prop->valuestring);
        else if (strcmp(key, "flex_grow") == 0 && cJSON_IsNumber(prop)) {
            lv_obj_set_flex_grow(obj, (uint8_t)cJSON_GetNumberValue(prop));
        }
        // Grid
        else if (strcmp(key, "grid_dsc_array_col_dsc") == 0 && cJSON_IsArray(prop)) {
            free(temp_col_dsc); // Free previous if reparsed
            temp_col_dsc = parse_coord_array_for_grid(prop);
        }
        else if (strcmp(key, "grid_dsc_array_row_dsc") == 0 && cJSON_IsArray(prop)) {
            free(temp_row_dsc); // Free previous if reparsed
            temp_row_dsc = parse_coord_array_for_grid(prop);
        }
        else if (strcmp(key, "grid_column_align") == 0 && cJSON_IsString(prop)) temp_grid_col_align = parse_grid_align(prop->valuestring);
        else if (strcmp(key, "grid_row_align") == 0 && cJSON_IsString(prop)) temp_grid_row_align = parse_grid_align(prop->valuestring);
        // Grid Cell (applied directly as these are per-object)
        else if (strcmp(key, "grid_cell_column_align") == 0 && cJSON_IsString(prop)) {
            // Assume other grid_cell props are also present if this one is
            lv_grid_align_t col_align = parse_grid_align(prop->valuestring);
            int32_t col_pos = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_col_pos"));
            int32_t col_span = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_col_span"));
            lv_grid_align_t row_align = parse_grid_align(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_row_align")));
            int32_t row_pos = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_row_pos"));
            int32_t row_span = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(props_json, "grid_cell_row_span"));
            lv_obj_set_grid_cell(obj, col_align, col_pos, col_span, row_align, row_pos, row_span);
        }

        // --- Widget Specific Properties ---

        // Label
        else if (strcmp(key, "text") == 0 && cJSON_IsString(prop) && lv_obj_check_type(obj, &lv_label_class)) {
            lv_label_set_text(obj, prop->valuestring);
        }

        // Slider / Bar - Value and Range
        else if ((strcmp(key, "value") == 0 || strcmp(key, "val") == 0) && cJSON_IsNumber(prop)) {
             if (lv_obj_check_type(obj, &lv_slider_class)) {
                lv_slider_set_value(obj, (int32_t)cJSON_GetNumberValue(prop), LV_ANIM_OFF);
             } else if (lv_obj_check_type(obj, &lv_bar_class)) {
                 lv_bar_set_value(obj, (int32_t)cJSON_GetNumberValue(prop), LV_ANIM_OFF);
             }
             // Note: Scale doesn't have a simple 'set_value'
        }
        else if (strcmp(key, "range_min") == 0 && cJSON_IsNumber(prop)) {
             if (lv_obj_check_type(obj, &lv_slider_class)) {
                lv_slider_set_range(obj, (int32_t)cJSON_GetNumberValue(prop), lv_slider_get_max_value(obj));
             } else if (lv_obj_check_type(obj, &lv_bar_class)) {
                 lv_bar_set_range(obj, (int32_t)cJSON_GetNumberValue(prop), lv_bar_get_max_value(obj));
             }
         }
        else if (strcmp(key, "range_max") == 0 && cJSON_IsNumber(prop)) {
             if (lv_obj_check_type(obj, &lv_slider_class)) {
                lv_slider_set_range(obj, lv_slider_get_min_value(obj), (int32_t)cJSON_GetNumberValue(prop));
             } else if (lv_obj_check_type(obj, &lv_bar_class)) {
                 lv_bar_set_range(obj, lv_bar_get_min_value(obj), (int32_t)cJSON_GetNumberValue(prop));
             }
         }
         // Scale specific properties
         else if (strcmp(key, "scale_range_min") == 0 && cJSON_IsNumber(prop) && lv_obj_check_type(obj, &lv_scale_class)) {
             temp_scale_min = (int32_t)cJSON_GetNumberValue(prop);
         }
         else if (strcmp(key, "scale_range_max") == 0 && cJSON_IsNumber(prop) && lv_obj_check_type(obj, &lv_scale_class)) {
             temp_scale_max = (int32_t)cJSON_GetNumberValue(prop);
         }
         else if (strcmp(key, "scale_major_tick_every") == 0 && cJSON_IsNumber(prop) && lv_obj_check_type(obj, &lv_scale_class)) {
             lv_scale_set_major_tick_every(obj, (uint32_t)cJSON_GetNumberValue(prop));
         }
         else if (strcmp(key, "scale_mode") == 0 && cJSON_IsString(prop) && lv_obj_check_type(obj, &lv_scale_class)) {
             lv_scale_set_mode(obj, parse_scale_mode(prop->valuestring));
         }

        else {
            // Don't warn for every unknown property, could be custom data or handled by grid_cell
             if (strncmp(key, "grid_cell_", 10) != 0 &&
                 strcmp(key, "grid_dsc_array_col_dsc") != 0 &&
                 strcmp(key, "grid_dsc_array_row_dsc") != 0 &&
                 strcmp(key, "grid_column_align") != 0 &&
                 strcmp(key, "grid_row_align") != 0 &&
                 strcmp(key, "flex_align_main_place") != 0 &&
                 strcmp(key, "flex_align_cross_place") != 0 &&
                 strcmp(key, "flex_align_track_cross_place") != 0 &&
                 strcmp(key, "scale_range_min") != 0 &&
                 strcmp(key, "scale_range_max") != 0
                 ) {
                LOG_WARN("Unknown or unhandled property: '%s' for object type %s", key, lv_obj_get_class(obj)->name);
             }
        }
    }

    // Apply multi-part properties after iterating through all keys
    if (temp_col_dsc && temp_row_dsc) {
        lv_obj_set_grid_dsc_array(obj, temp_col_dsc, temp_row_dsc);
        // Don't free allocated memory - grid needs the underlying memory.
    }
    // Apply grid align if either was set (check defaults?)
    if (cJSON_GetObjectItemCaseSensitive(props_json, "grid_column_align") || cJSON_GetObjectItemCaseSensitive(props_json, "grid_row_align")) {
        lv_obj_set_grid_align(obj, temp_grid_col_align, temp_grid_row_align);
    }
    // Apply flex align if any part was set
    if (cJSON_GetObjectItemCaseSensitive(props_json, "flex_align_main_place") ||
        cJSON_GetObjectItemCaseSensitive(props_json, "flex_align_cross_place") ||
        cJSON_GetObjectItemCaseSensitive(props_json, "flex_align_track_cross_place")) {
        lv_obj_set_flex_align(obj, temp_flex_main, temp_flex_cross, temp_flex_track);
    }
    // Apply scale range if either part was set
    if (cJSON_GetObjectItemCaseSensitive(props_json, "scale_range_min") || cJSON_GetObjectItemCaseSensitive(props_json, "scale_range_max")) {
        if (lv_obj_check_type(obj, &lv_scale_class)) {
             lv_scale_set_range(obj, temp_scale_min, temp_scale_max);
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
    else if (strcmp(prop_key, "border_width") == 0) lv_obj_set_style_border_width(obj, parse_coord(style_prop), selector); // Use parse_coord
    else if (strcmp(prop_key, "border_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_border_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "border_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_border_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "outline_width") == 0) lv_obj_set_style_outline_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "outline_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_outline_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "outline_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_outline_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "pad_all") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_all(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "pad_top") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_top(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "pad_left") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_left(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "pad_right") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_right(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "pad_bottom") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_bottom(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    // Assuming row/col gap are pad_row/pad_column in LVGL v8/v9
    else if (strcmp(prop_key, "pad_row") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_row(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "pad_column") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_pad_column(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "margin_all") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_margin_all(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "margin_top") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_margin_top(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "margin_left") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_margin_left(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "margin_right") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_margin_right(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "margin_bottom") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_margin_bottom(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
    else if (strcmp(prop_key, "min_height") == 0) lv_obj_set_style_min_height(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "min_width") == 0) lv_obj_set_style_min_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "max_height") == 0) lv_obj_set_style_max_height(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "max_width") == 0) lv_obj_set_style_max_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "text_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_color(obj, parse_color(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "text_font") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_font(obj, get_font_by_name(style_prop->valuestring), selector);
    else if (strcmp(prop_key, "text_align") == 0 && cJSON_IsString(style_prop)) {
        lv_text_align_t align = LV_TEXT_ALIGN_AUTO;
        if(strcmp(style_prop->valuestring, "left") == 0) align = LV_TEXT_ALIGN_LEFT;
        else if(strcmp(style_prop->valuestring, "center") == 0) align = LV_TEXT_ALIGN_CENTER;
        else if(strcmp(style_prop->valuestring, "right") == 0) align = LV_TEXT_ALIGN_RIGHT;
        lv_obj_set_style_text_align(obj, align, selector);
    }
    else if (strcmp(prop_key, "flex_flow") == 0 && cJSON_IsString(style_prop)) {
        lv_obj_set_style_flex_flow(obj, parse_flex_flow(style_prop->valuestring), selector);
    }
    // Special case for slider/bar parts that have width/height styles
    // Check that part is NOT main part before applying width/height style
    else if (strcmp(prop_key, "width") == 0 && (selector & LV_PART_MAIN) != LV_PART_MAIN) lv_obj_set_style_width(obj, parse_coord(style_prop), selector);
    else if (strcmp(prop_key, "height") == 0 && (selector & LV_PART_MAIN) != LV_PART_MAIN) lv_obj_set_style_height(obj, parse_coord(style_prop), selector);

    // --- Add more style properties here ---
    // e.g., shadow, image, line, arc, etc.

    else {
         // Only log warning if it's not a known property handled elsewhere (like width/height for main part)
         if (!((strcmp(prop_key, "width") == 0 || strcmp(prop_key, "height") == 0) && (selector & LV_PART_MAIN) == LV_PART_MAIN)) {
            LOG_WARN("Unknown or unhandled style property: '%s' for part/state %u", prop_key, selector);
         }
    }
}

// --- Style Application Main Function ---

static void apply_styles(lv_obj_t *obj, cJSON *styles_json) {
    cJSON *part_style_obj = NULL;
    cJSON_ArrayForEach(part_style_obj, styles_json) { // Iterate through parts ("default", "indicator", "knob", etc.)
        const char *part_key = part_style_obj->string;
        if (!part_key || !cJSON_IsObject(part_style_obj)) continue;

        lv_part_t part = parse_part(part_key);

        cJSON *state_style_obj = NULL;
        // Heuristic: Check if the first *child* object has a key that looks like a state or is an object itself
        bool has_states = false;
        cJSON *first_child = part_style_obj->child;
        if (first_child && first_child->string) {
             // If the value of the part key is an object, assume it contains states.
            if (cJSON_IsObject(first_child)) {
                // Check if keys look like states (e.g., "default", "pressed")
                // This is less reliable if style props share names with states.
                // Let's rely on the structure: if part_style_obj contains objects, those are states.
                // A simple check: does the first key look like a state?
                 if (strcmp(first_child->string, "default") == 0 ||
                     strcmp(first_child->string, "checked") == 0 ||
                     strcmp(first_child->string, "focused") == 0 ||
                     strcmp(first_child->string, "pressed") == 0 ||
                     strcmp(first_child->string, "disabled") == 0 ||
                     strcmp(first_child->string, "hovered") == 0 ||
                     strncmp(first_child->string, "state_", 6) == 0) {
                     has_states = true;
                 }
            }
        }


        if(has_states)
        {
            // It has state definitions within the part
            cJSON_ArrayForEach(state_style_obj, part_style_obj) { // Iterate through states ("default", "pressed", etc.)
                const char *state_key = state_style_obj->string;
                if (!state_key || !cJSON_IsObject(state_style_obj)) continue;

                lv_state_t state = parse_state(state_key);
                lv_style_selector_t selector = part | state;

                cJSON *style_prop = NULL;
                cJSON_ArrayForEach(style_prop, state_style_obj) { // Iterate through style properties
                    apply_single_style_prop(obj, style_prop->string, style_prop, selector);
                }
            }
        } else {
             // Styles are directly under the part key (implicit default state)
             lv_state_t state = LV_STATE_DEFAULT;
             lv_style_selector_t selector = part | state;
             cJSON *style_prop = NULL;
             cJSON_ArrayForEach(style_prop, part_style_obj) { // Iterate through style properties
                 apply_single_style_prop(obj, style_prop->string, style_prop, selector);
             }
        }
    }
}


// --- Public API ---

bool build_ui_from_json(const char *json_string) {
    if (!json_string) {
        LOG_ERROR("build_ui_from_json called with NULL string");
        return false;
    }

    cJSON *root_json = cJSON_Parse(json_string);
    if (root_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            // Calculate the position of the error
            int line = 1;
            int col = 1;
            const char *ptr = json_string;
            while (ptr < error_ptr) {
                if (*ptr == '\n') {
                    line++;
                    col = 1;
                } else {
                    col++;
                }
                ptr++;
            }
             // Print a snippet around the error
            int context = 15;
            int start = (int)(error_ptr - json_string) - context;
            if(start < 0) start = 0;
            int end = (int)(error_ptr - json_string) + context;
            if(end >= (int)strlen(json_string)) end = (int)strlen(json_string) -1;

            char snippet[context*2 + 2]; // Allow for null terminator
            if (end >= start) {
                 strncpy(snippet, json_string + start, end - start + 1);
                 snippet[end - start + 1] = '\0';
            } else {
                 snippet[0] = '\0';
            }


            LOG_ERROR("JSON Parse Error at Line %d, Column %d (near '%s')", line, col, snippet);
        } else {
            LOG_ERROR("JSON Parse Error (unknown location)");
        }
        return false;
    }

    cJSON *root_obj_json = cJSON_GetObjectItemCaseSensitive(root_json, "root");
    if (!cJSON_IsObject(root_obj_json)) {
         LOG_ERROR("JSON missing 'root' object or it's not an object");
         cJSON_Delete(root_json);
         return false;
    }


    // --- Get Active Screen ---
    // It's better to get the screen *before* cleaning, in case cleaning fails
    lv_obj_t *scr = lv_screen_active(); // lv_scr_act() deprecated in v9
    if (!scr) {
         LOG_ERROR("No active screen found! Cannot build UI.");
         cJSON_Delete(root_json);
         return false;
    }

    // --- Clear Screen ---
    // It's generally recommended to clear the screen *before* building the new UI
    LOG_INFO("Cleaning active screen...");
    lv_obj_clean(scr); // Remove all children

    // --- Build UI ---
    LOG_INFO("Building UI recursively from JSON root...");
    lv_obj_t *created_root = create_lvgl_object_recursive(root_obj_json, scr);

    cJSON_Delete(root_json); // Clean up cJSON object tree

    if (!created_root) {
        LOG_ERROR("Failed to create root object from JSON");
        // Screen might be partially populated if error occurred mid-build.
        // Consider cleaning again? lv_obj_clean(scr);
        return false;
    }

    LOG_USER("UI Built Successfully from JSON");
    return true;
}