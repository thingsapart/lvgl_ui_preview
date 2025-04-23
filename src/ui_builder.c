// ui_builder.c

#include "ui_builder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For strtol, strtod

#include "../../lvgl/src/core/lv_obj_class_private.h"

#define LOG(s, ...) printf(s __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(s, ...) do { printf("[INFO]"); printf(s __VA_OPT__(,) __VA_ARGS__); } while(0);
#define LOG_ERROR(s, ...) do { printf("[ERROR]"); printf(s __VA_OPT__(,) __VA_ARGS__); } while(0);
#define LOG_WARN(s, ...) do { printf("[WARN]"); printf(s __VA_OPT__(,) __VA_ARGS__); } while(0);
#define LOG_USER(s, ...) do { printf("[U]"); printf(s __VA_OPT__(,) __VA_ARGS__); } while(0);
#define LOG_TRACE LOG

// --- Forward Declarations ---
static lv_obj_t* create_lvgl_object_recursive(cJSON *json_node, lv_obj_t *parent);
static void apply_properties(lv_obj_t *obj, cJSON *props_json);
static void apply_styles(lv_obj_t *obj, cJSON *styles_json);

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
    LOG_WARN("Unknown align value: %s", align_str);
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
    LOG_WARN("Unknown state value: %s", state_str);
    return LV_STATE_DEFAULT;
}

// Helper to parse part string (specific to widgets like slider, bar, etc.)
static lv_part_t parse_part(const char *part_str) {
    if (!part_str || strcmp(part_str, "default") == 0) return LV_PART_MAIN;
    if (strcmp(part_str, "indicator") == 0) return LV_PART_INDICATOR;
    if (strcmp(part_str, "knob") == 0) return LV_PART_KNOB;
    if (strcmp(part_str, "scrollbar") == 0) return LV_PART_SCROLLBAR;
    // Add more parts as needed
    LOG_WARN("Unknown part value: %s", part_str);
    return LV_PART_MAIN;
}


// Helper to parse coordinate (can be number or percentage string)
static lv_coord_t parse_coord(cJSON *value) {
    if (cJSON_IsNumber(value)) {
        return (lv_coord_t)cJSON_GetNumberValue(value);
    } else if (cJSON_IsString(value)) {
        const char *str = cJSON_GetStringValue(value);
        if (str && str[strlen(str) - 1] == '%') {
            return lv_pct((int)strtol(str, NULL, 10));
        } else {
             LOG_WARN("Invalid coordinate string format: %s (expected number or 'N%%')", str);
            return 0;
        }
    }
     LOG_WARN("Invalid coordinate type (expected number or string)");
    return 0;
}

// Helper to get font by name (requires fonts to be enabled in lv_conf.h)
static const lv_font_t* get_font_by_name(const char* name) {
    if (!name) return LV_FONT_DEFAULT;
    // Add more fonts as needed, ensure they are enabled in lv_conf.h
    #if LV_USE_FONT_MONTSERRAT_18
    if (strcmp(name, "montserrat_18") == 0) return &lv_font_montserrat_18;
    #endif
     #if LV_USE_FONT_MONTSERRAT_14
    if (strcmp(name, "montserrat_14") == 0) return &lv_font_montserrat_14;
    #endif
     #if LV_USE_FONT_DEFAULT // Always have a fallback
    if (strcmp(name, "default") == 0) return LV_FONT_DEFAULT;
    #endif

    LOG_WARN("Font '%s' not found or not enabled", name);
    return LV_FONT_DEFAULT; // Fallback
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
    cJSON_ArrayForEach(prop, props_json) {
        const char *key = prop->string;
        if (!key) continue;

        // --- Common Properties ---
        if (strcmp(key, "width") == 0) lv_obj_set_width(obj, parse_coord(prop));
        else if (strcmp(key, "height") == 0) lv_obj_set_height(obj, parse_coord(prop));
        else if (strcmp(key, "x") == 0 && cJSON_IsNumber(prop)) lv_obj_set_x(obj, (lv_coord_t)cJSON_GetNumberValue(prop));
        else if (strcmp(key, "y") == 0 && cJSON_IsNumber(prop)) lv_obj_set_y(obj, (lv_coord_t)cJSON_GetNumberValue(prop));
        else if (strcmp(key, "align") == 0 && cJSON_IsString(prop)) lv_obj_set_align(obj, parse_align(prop->valuestring));
        else if (strcmp(key, "hidden") == 0 && cJSON_IsBool(prop)) {
            if (cJSON_IsTrue(prop)) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        else if (strcmp(key, "clickable") == 0 && cJSON_IsBool(prop)) {
             if (cJSON_IsTrue(prop)) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
             else lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
         // Add more common flags: scrollable, checkable, etc.

        // --- Widget Specific Properties ---
        // Label
        else if (strcmp(key, "text") == 0 && cJSON_IsString(prop) && lv_obj_check_type(obj, &lv_label_class)) {
            lv_label_set_text(obj, prop->valuestring);
        }
        // Slider / Bar / Arc
        else if ((strcmp(key, "value") == 0 || strcmp(key, "val") == 0) && cJSON_IsNumber(prop) && (lv_obj_check_type(obj, &lv_slider_class) /*|| lv_obj_check_type(obj, &lv_bar_class)*/ )) {
             // Need to check specific class or use a more generic approach if available
             if (lv_obj_check_type(obj, &lv_slider_class)) {
                lv_slider_set_value(obj, (int32_t)cJSON_GetNumberValue(prop), LV_ANIM_OFF);
             }
             // else if (lv_obj_check_type(obj, &lv_bar_class)) { ... }
        }
         else if (strcmp(key, "range_min") == 0 && cJSON_IsNumber(prop) && (lv_obj_check_type(obj, &lv_slider_class) /*|| ... */ )) {
             if (lv_obj_check_type(obj, &lv_slider_class)) {
                lv_slider_set_range(obj, (int32_t)cJSON_GetNumberValue(prop), lv_slider_get_max_value(obj));
             }
         }
        else if (strcmp(key, "range_max") == 0 && cJSON_IsNumber(prop) && (lv_obj_check_type(obj, &lv_slider_class) /*|| ... */ )) {
             if (lv_obj_check_type(obj, &lv_slider_class)) {
                lv_slider_set_range(obj, lv_slider_get_min_value(obj), (int32_t)cJSON_GetNumberValue(prop));
             }
         }

        // --- Add more properties for other widgets ---

        else {
            LOG_WARN("Unknown or unhandled property: '%s' for object type %s", key, lv_obj_get_class(obj)->name);
        }
    }
}


// --- Style Application ---

static void apply_styles(lv_obj_t *obj, cJSON *styles_json) {
    cJSON *part_style_obj = NULL;
    cJSON_ArrayForEach(part_style_obj, styles_json) { // Iterate through parts ("default", "indicator", "knob", etc.)
        const char *part_key = part_style_obj->string;
        if (!part_key || !cJSON_IsObject(part_style_obj)) continue;

        lv_part_t part = parse_part(part_key);

        cJSON *state_style_obj = NULL;
         // Check if the value associated with the part key is another object (meaning it has states like "default", "pressed")
         // OR if it's directly the style properties (implicitly for the default state)
        if(cJSON_IsObject(cJSON_GetArrayItem(part_style_obj, 0))) // Heuristic: Check if first item is an object (likely state names)
        {
            // It has state definitions within the part
            cJSON_ArrayForEach(state_style_obj, part_style_obj) { // Iterate through states ("default", "pressed", etc.)
                const char *state_key = state_style_obj->string;
                if (!state_key || !cJSON_IsObject(state_style_obj)) continue;

                lv_state_t state = parse_state(state_key);
                lv_style_selector_t selector = part | state;

                cJSON *style_prop = NULL;
                cJSON_ArrayForEach(style_prop, state_style_obj) { // Iterate through style properties
                    const char *prop_key = style_prop->string;
                    if (!prop_key) continue;

                    // --- Apply Style Properties ---
                    if (strcmp(prop_key, "bg_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_bg_color(obj, parse_color(style_prop->valuestring), selector);
                    else if (strcmp(prop_key, "bg_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_bg_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
                    else if (strcmp(prop_key, "radius") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_radius(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
                    else if (strcmp(prop_key, "border_width") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_border_width(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
                    else if (strcmp(prop_key, "border_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_border_color(obj, parse_color(style_prop->valuestring), selector);
                    else if (strcmp(prop_key, "border_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_border_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
                    else if (strcmp(prop_key, "text_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_color(obj, parse_color(style_prop->valuestring), selector);
                    else if (strcmp(prop_key, "text_font") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_font(obj, get_font_by_name(style_prop->valuestring), selector);
                    else if (strcmp(prop_key, "text_align") == 0 && cJSON_IsString(style_prop)) {
                         // Map string like "left", "center", "right" to LV_TEXT_ALIGN_...
                        lv_text_align_t align = LV_TEXT_ALIGN_AUTO;
                        if(strcmp(style_prop->valuestring, "left") == 0) align = LV_TEXT_ALIGN_LEFT;
                        else if(strcmp(style_prop->valuestring, "center") == 0) align = LV_TEXT_ALIGN_CENTER;
                        else if(strcmp(style_prop->valuestring, "right") == 0) align = LV_TEXT_ALIGN_RIGHT;
                         lv_obj_set_style_text_align(obj, align, selector);
                    }
                     // Special case for slider/bar parts that have width/height styles
                    else if (strcmp(prop_key, "width") == 0 && part != LV_PART_MAIN) lv_obj_set_style_width(obj, parse_coord(style_prop), selector);
                    else if (strcmp(prop_key, "height") == 0 && part != LV_PART_MAIN) lv_obj_set_style_height(obj, parse_coord(style_prop), selector);

                    // --- Add more style properties here ---
                    // e.g., padding, outline, shadow, image, line, arc, etc.

                    else {
                         LOG_WARN("Unknown or unhandled style property: '%s'", prop_key);
                    }
                }
            }
        } else {
             // Styles are directly under the part key (implicit default state)
             lv_state_t state = LV_STATE_DEFAULT;
             lv_style_selector_t selector = part | state;
             cJSON *style_prop = NULL;
             cJSON_ArrayForEach(style_prop, part_style_obj) { // Iterate through style properties
                 const char *prop_key = style_prop->string;
                 if (!prop_key) continue;
                // --- Apply Style Properties (duplicate code - refactor potential) ---
                if (strcmp(prop_key, "bg_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_bg_color(obj, parse_color(style_prop->valuestring), selector);
                else if (strcmp(prop_key, "bg_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_bg_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
                else if (strcmp(prop_key, "radius") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_radius(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
                else if (strcmp(prop_key, "border_width") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_border_width(obj, (lv_coord_t)cJSON_GetNumberValue(style_prop), selector);
                else if (strcmp(prop_key, "border_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_border_color(obj, parse_color(style_prop->valuestring), selector);
                else if (strcmp(prop_key, "border_opa") == 0 && cJSON_IsNumber(style_prop)) lv_obj_set_style_border_opa(obj, (lv_opa_t)cJSON_GetNumberValue(style_prop), selector);
                else if (strcmp(prop_key, "text_color") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_color(obj, parse_color(style_prop->valuestring), selector);
                else if (strcmp(prop_key, "text_font") == 0 && cJSON_IsString(style_prop)) lv_obj_set_style_text_font(obj, get_font_by_name(style_prop->valuestring), selector);
                else if (strcmp(prop_key, "text_align") == 0 && cJSON_IsString(style_prop)) {
                    lv_text_align_t align = LV_TEXT_ALIGN_AUTO;
                    if(strcmp(style_prop->valuestring, "left") == 0) align = LV_TEXT_ALIGN_LEFT;
                    else if(strcmp(style_prop->valuestring, "center") == 0) align = LV_TEXT_ALIGN_CENTER;
                    else if(strcmp(style_prop->valuestring, "right") == 0) align = LV_TEXT_ALIGN_RIGHT;
                    lv_obj_set_style_text_align(obj, align, selector);
                }
                else if (strcmp(prop_key, "width") == 0 && part != LV_PART_MAIN) lv_obj_set_style_width(obj, parse_coord(style_prop), selector);
                else if (strcmp(prop_key, "height") == 0 && part != LV_PART_MAIN) lv_obj_set_style_height(obj, parse_coord(style_prop), selector);
                else {
                    LOG_WARN("Unknown or unhandled style property: '%s'", prop_key);
                }
             }
        }


    }
}


// --- Public API ---

bool build_ui_from_json(const char *json_string) {
    cJSON *root_json = cJSON_Parse(json_string);
    if (root_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            LOG_ERROR("JSON Parse Error before: %s", error_ptr);
        } else {
            LOG_ERROR("JSON Parse Error (unknown location)");
        }
        return false;
    }

    cJSON *root_obj_json = cJSON_GetObjectItemCaseSensitive(root_json, "root");
    if (!cJSON_IsObject(root_obj_json)) {
         LOG_ERROR("JSON missing 'root' object");
         cJSON_Delete(root_json);
         return false;
    }


    // --- Clear Screen ---
    lv_obj_t *scr = lv_screen_active(); // lv_scr_act() deprecated in v9
    if(scr) {
        lv_obj_clean(scr); // Remove all children
    } else {
         LOG_ERROR("No active screen found!");
         cJSON_Delete(root_json);
         return false;
    }

    // --- Build UI ---
    lv_obj_t *created_root = create_lvgl_object_recursive(root_obj_json, scr);

    cJSON_Delete(root_json); // Clean up cJSON object

    if (!created_root) {
        LOG_ERROR("Failed to create root object from JSON");
        return false;
    }

    LOG_USER("UI Built Successfully from JSON");
    return true;
}
