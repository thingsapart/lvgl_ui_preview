// emul_lvgl.h

/**
 Adding a New Property Setter (e.g., lv_obj_set_custom_prop)

    emul_lvgl.h:
        - Declare the function (e.g., void lv_obj_set_custom_prop(lv_obj_t *obj, custom_type_t value);).
    emul_lvgl.c:
        - (If needed) Add Value Type: If custom_type_t isn't covered by existing Value types (int, coord, string, etc.), add a new VAL_TYPE_... enum, update the Value union (emul_lvgl_internal.h), create a value_mk_... helper, and update free_value.
        - Implement function, call emul_obj_add_property(obj, "json_key", value_mk_type(value));.
        - build_json_recursive, add case  VAL_TYPE_... (if created) or the existing one in the properties section to convert the stored value to cJSON using the chosen "json_key".
            - Define a string converter if needed (like color_to_hex_string).
    ui_builder.c:
        - apply_properties: Parse & Apply, add an `else if (strcmp(key, "json_key") == 0)` block.
            - convert cJSON to custom_type_t.
            - call LVGL function (e.g., lv_obj_set_custom_prop(obj, parsed_value);).

Adding a New Style Setter (e.g., lv_obj_set_style_custom_style)

    emul_lvgl.h:
        - Declare function (e.g., void lv_obj_set_style_custom_style(lv_obj_t *obj, custom_type_t value, lv_style_selector_t selector);).
    emul_lvgl.c:
        - (If needed) Add Value Type: Same as for properties if the type is new.
        - Implement Function: Define the function. Inside, use the ADD_STYLE macro: ADD_STYLE(obj, selector, "json_style_key", value_mk_type(value));.
        - build_json_recursive: add/update the case in the styles section to handle the value type and convert it to cJSON using "json_style_key".
    ui_builder.c:
        - apply_single_style_prop: add `else if (strcmp(prop_key, "json_style_key") == 0)` block.
            - convert cJSON to custom_type_t.
            - call LVGL function (e.g., lv_obj_set_style_custom_style(obj, parsed_value, selector);). Include necessary LVGL headers.
 *
 *
 */

#ifndef EMUL_LVGL_H
#define EMUL_LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

#include "debug.h"

// --- Basic Type Definitions (Mimicking LVGL) ---

#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning  /**< The default value just prevents GCC warning */

// lv_obj_t is the struct type
struct _lv_obj_t; // Forward declaration
typedef struct _lv_obj_t lv_obj_t; // lv_obj_t is now the struct type

// Coordinates
typedef int32_t                     lv_coord_t;

//#define LV_COORD_IS_PCT(v) ((v) <= -1000)
//#define LV_COORD_GET_PCT(v) (-1000 - (v))
//#define LV_COORD_SET_PCT(v) (-1000 - (v))

#define LV_COORD_TYPE_SHIFT    (29U)

#define LV_COORD_TYPE_MASK     (3 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE(x)       ((x) & LV_COORD_TYPE_MASK)  /*Extract type specifiers*/
#define LV_COORD_PLAIN(x)      ((x) & ~LV_COORD_TYPE_MASK) /*Remove type specifiers*/

#define LV_COORD_TYPE_PX       (0 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_SPEC     (1 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_PX_NEG   (3 << LV_COORD_TYPE_SHIFT)

#define LV_COORD_IS_PX(x)       (LV_COORD_TYPE(x) == LV_COORD_TYPE_PX || LV_COORD_TYPE(x) == LV_COORD_TYPE_PX_NEG)
#define LV_COORD_IS_SPEC(x)     (LV_COORD_TYPE(x) == LV_COORD_TYPE_SPEC)

#define LV_COORD_SET_SPEC(x)    ((x) | LV_COORD_TYPE_SPEC)

/** Max coordinate value */
#define LV_COORD_MAX            ((1 << LV_COORD_TYPE_SHIFT) - 1)
#define LV_COORD_MIN            (-LV_COORD_MAX)

/*Special coordinates*/
#define LV_SIZE_CONTENT         LV_COORD_SET_SPEC(LV_COORD_MAX)
#define LV_PCT_STORED_MAX       (LV_COORD_MAX - 1)
#if LV_PCT_STORED_MAX % 2 != 0
#error LV_PCT_STORED_MAX should be an even number
#endif
#define LV_PCT_POS_MAX          (LV_PCT_STORED_MAX / 2)
#define LV_PCT(x)               (LV_COORD_SET_SPEC(((x) < 0 ? (LV_PCT_POS_MAX - LV_MAX((x), -LV_PCT_POS_MAX)) : LV_MIN((x), LV_PCT_POS_MAX))))
#define LV_COORD_IS_PCT(x)      ((LV_COORD_IS_SPEC(x) && LV_COORD_PLAIN(x) <= LV_PCT_STORED_MAX))
#define LV_COORD_GET_PCT(x)     (LV_COORD_PLAIN(x) > LV_PCT_POS_MAX ? LV_PCT_POS_MAX - LV_COORD_PLAIN(x) : LV_COORD_PLAIN(x))

#define LV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LV_MAX3(a, b, c) (LV_MAX(LV_MAX(a,b), c))
#define LV_MAX4(a, b, c, d) (LV_MAX(LV_MAX(a,b), LV_MAX(c,d)))
#define LV_CLAMP(min, val, max) (LV_MAX(min, (LV_MIN(val, max))))
#define LV_MAX_OF(t) ((unsigned long)(LV_IS_SIGNED(t) ? LV_SMAX_OF(t) : LV_UMAX_OF(t)))
#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LV_MIN3(a, b, c) (LV_MIN(LV_MIN(a,b), c))
#define LV_MIN4(a, b, c, d) (LV_MIN(LV_MIN(a,b), LV_MIN(c,d)))
#define LV_CLAMP(min, val, max) (LV_MAX(min, (LV_MIN(val, max))))

LV_EXPORT_CONST_INT(LV_COORD_MAX);
LV_EXPORT_CONST_INT(LV_COORD_MIN);
LV_EXPORT_CONST_INT(LV_SIZE_CONTENT);

// Color
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} lv_color_t;

// Alignment
typedef enum {
    LV_ALIGN_DEFAULT,
    LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_MID, LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_LEFT_MID, LV_ALIGN_CENTER, LV_ALIGN_RIGHT_MID,
    LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_MID, LV_ALIGN_BOTTOM_RIGHT
} lv_align_t;

#define LV_GRID_FR(x)          (LV_COORD_MAX - 100 + x)

#define LV_GRID_CONTENT        (LV_COORD_MAX - 101)
LV_EXPORT_CONST_INT(LV_GRID_CONTENT);

#define LV_GRID_TEMPLATE_LAST  (LV_COORD_MAX)
LV_EXPORT_CONST_INT(LV_GRID_TEMPLATE_LAST);

// Grid align
typedef enum {
    LV_GRID_ALIGN_START,
    LV_GRID_ALIGN_CENTER,
    LV_GRID_ALIGN_END,
    LV_GRID_ALIGN_STRETCH,
    LV_GRID_ALIGN_SPACE_EVENLY,
    LV_GRID_ALIGN_SPACE_AROUND,
    LV_GRID_ALIGN_SPACE_BETWEEN,
} lv_grid_align_t;

// Opacity.
enum {
    LV_OPA_TRANSP = 0,
    LV_OPA_0      = 0,
    LV_OPA_10     = 25,
    LV_OPA_20     = 51,
    LV_OPA_30     = 76,
    LV_OPA_40     = 102,
    LV_OPA_50     = 127,
    LV_OPA_60     = 153,
    LV_OPA_70     = 178,
    LV_OPA_80     = 204,
    LV_OPA_90     = 229,
    LV_OPA_100    = 255,
    LV_OPA_COVER  = 255,
};

// Scale mode
typedef enum {
    LV_SCALE_MODE_HORIZONTAL_TOP    = 0x00U,
    LV_SCALE_MODE_HORIZONTAL_BOTTOM = 0x01U,
    LV_SCALE_MODE_VERTICAL_LEFT     = 0x02U,
    LV_SCALE_MODE_VERTICAL_RIGHT    = 0x04U,
    LV_SCALE_MODE_ROUND_INNER       = 0x08U,
    LV_SCALE_MODE_ROUND_OUTER      = 0x10U,
    LV_SCALE_MODE_LAST
} lv_scale_mode_t;

// Flags
typedef enum {
    LV_OBJ_FLAG_HIDDEN          = (1L << 0),  /**< Make the object hidden. (Like it wasn't there at all)*/
    LV_OBJ_FLAG_CLICKABLE       = (1L << 1),  /**< Make the object clickable by the input devices*/
    LV_OBJ_FLAG_CLICK_FOCUSABLE = (1L << 2),  /**< Add focused state to the object when clicked*/
    LV_OBJ_FLAG_CHECKABLE       = (1L << 3),  /**< Toggle checked state when the object is clicked*/
    LV_OBJ_FLAG_SCROLLABLE      = (1L << 4),  /**< Make the object scrollable*/
    LV_OBJ_FLAG_SCROLL_ELASTIC  = (1L << 5),  /**< Allow scrolling inside but with slower speed*/
    LV_OBJ_FLAG_SCROLL_MOMENTUM = (1L << 6),  /**< Make the object scroll further when "thrown"*/
    LV_OBJ_FLAG_SCROLL_ONE      = (1L << 7),  /**< Allow scrolling only one snappable children*/
    LV_OBJ_FLAG_SCROLL_CHAIN_HOR = (1L << 8), /**< Allow propagating the horizontal scroll to a parent*/
    LV_OBJ_FLAG_SCROLL_CHAIN_VER = (1L << 9), /**< Allow propagating the vertical scroll to a parent*/
    LV_OBJ_FLAG_SCROLL_CHAIN     = (LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER),
    LV_OBJ_FLAG_SCROLL_ON_FOCUS = (1L << 10),  /**< Automatically scroll object to make it visible when focused*/
    LV_OBJ_FLAG_SCROLL_WITH_ARROW  = (1L << 11), /**< Allow scrolling the focused object with arrow keys*/
    LV_OBJ_FLAG_SNAPPABLE       = (1L << 12), /**< If scroll snap is enabled on the parent it can snap to this object*/
    LV_OBJ_FLAG_PRESS_LOCK      = (1L << 13), /**< Keep the object pressed even if the press slid from the object*/
    LV_OBJ_FLAG_EVENT_BUBBLE    = (1L << 14), /**< Propagate the events to the parent too*/
    LV_OBJ_FLAG_GESTURE_BUBBLE  = (1L << 15), /**< Propagate the gestures to the parent*/
    LV_OBJ_FLAG_ADV_HITTEST     = (1L << 16), /**< Allow performing more accurate hit (click) test. E.g. consider rounded corners.*/
    LV_OBJ_FLAG_IGNORE_LAYOUT   = (1L << 17), /**< Make the object not positioned by the layouts*/
    LV_OBJ_FLAG_FLOATING        = (1L << 18), /**< Do not scroll the object when the parent scrolls and ignore layout*/
    LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS = (1L << 19), /**< Send `LV_EVENT_DRAW_TASK_ADDED` events*/
    LV_OBJ_FLAG_OVERFLOW_VISIBLE = (1L << 20),/**< Do not clip the children to the parent's ext draw size*/
    LV_OBJ_FLAG_FLEX_IN_NEW_TRACK = (1L << 21),     /**< Start a new flex track on this item*/
    LV_OBJ_FLAG_LAYOUT_1        = (1L << 23), /**< Custom flag, free to use by layouts*/
    LV_OBJ_FLAG_LAYOUT_2        = (1L << 24), /**< Custom flag, free to use by layouts*/
    LV_OBJ_FLAG_WIDGET_1        = (1L << 25), /**< Custom flag, free to use by widget*/
    LV_OBJ_FLAG_WIDGET_2        = (1L << 26), /**< Custom flag, free to use by widget*/
    LV_OBJ_FLAG_USER_1          = (1L << 27), /**< Custom flag, free to use by user*/
    LV_OBJ_FLAG_USER_2          = (1L << 28), /**< Custom flag, free to use by user*/
    LV_OBJ_FLAG_USER_3          = (1L << 29), /**< Custom flag, free to use by user*/
    LV_OBJ_FLAG_USER_4          = (1L << 30), /**< Custom flag, free to use by user*/
} lv_obj_flag_t;

// Layout
typedef enum {
    LV_LAYOUT_NONE = 0,
    LV_LAYOUT_FLEX,
    LV_LAYOUT_GRID,
    LV_LAYOUT_LAST
} lv_layout_t;

// Flex
typedef enum {
    LV_FLEX_ALIGN_START,
    LV_FLEX_ALIGN_END,
    LV_FLEX_ALIGN_CENTER,
    LV_FLEX_ALIGN_SPACE_EVENLY,
    LV_FLEX_ALIGN_SPACE_AROUND,
    LV_FLEX_ALIGN_SPACE_BETWEEN,
} lv_flex_align_t;

#define LV_FLEX_COLUMN        (1 << 0)
#define LV_FLEX_WRAP       (1 << 2)
#define LV_FLEX_REVERSE    (1 << 3)

typedef enum {
    LV_FLEX_FLOW_ROW                 = 0x00,
    LV_FLEX_FLOW_COLUMN              = LV_FLEX_COLUMN,
    LV_FLEX_FLOW_ROW_WRAP            = LV_FLEX_FLOW_ROW | LV_FLEX_WRAP,
    LV_FLEX_FLOW_ROW_REVERSE         = LV_FLEX_FLOW_ROW | LV_FLEX_REVERSE,
    LV_FLEX_FLOW_ROW_WRAP_REVERSE    = LV_FLEX_FLOW_ROW | LV_FLEX_WRAP | LV_FLEX_REVERSE,
    LV_FLEX_FLOW_COLUMN_WRAP         = LV_FLEX_FLOW_COLUMN | LV_FLEX_WRAP,
    LV_FLEX_FLOW_COLUMN_REVERSE      = LV_FLEX_FLOW_COLUMN | LV_FLEX_REVERSE,
    LV_FLEX_FLOW_COLUMN_WRAP_REVERSE = LV_FLEX_FLOW_COLUMN | LV_FLEX_WRAP | LV_FLEX_REVERSE,
} lv_flex_flow_t;

// States
typedef uint32_t lv_state_t;
#define LV_STATE_DEFAULT    0x0000
#define LV_STATE_CHECKED    0x0001
#define LV_STATE_FOCUSED    0x0002
#define LV_STATE_FOCUS_KEY  0x0004
#define LV_STATE_EDITED     0x0008
#define LV_STATE_HOVERED    0x0010
#define LV_STATE_PRESSED    0x0020
#define LV_STATE_SCROLLED   0x0040
#define LV_STATE_DISABLED   0x0080
#define LV_STATE_ANY        0xFFFF

// Parts
typedef uint32_t lv_part_t;
#define LV_PART_MAIN        0x000000
#define LV_PART_SCROLLBAR   0x010000
#define LV_PART_INDICATOR   0x020000
#define LV_PART_KNOB        0x030000
#define LV_PART_SELECTED    0x040000
#define LV_PART_ITEMS       0x050000
#define LV_PART_CURSOR      0x060000
#define LV_PART_CUSTOM_FIRST 0x070000
#define LV_PART_ANY         0xFF0000

// Selector
typedef uint32_t lv_style_selector_t;

// Font - Opaque pointer type
typedef const void * lv_font_t;

// Animation enable/disable
typedef enum {
    LV_ANIM_OFF,
    LV_ANIM_ON
} lv_anim_enable_t;

// --- Emulation Library Control ---

/**
 * @brief Initializes the LVGL emulation library. Must be called first.
 */
void emul_lvgl_init(void);

/**
 * @brief Resets the internal state, deleting all created objects and styles.
 * Call this before creating a new UI description.
 */
void emul_lvgl_reset(void);

/**
 * @brief Cleans up all resources used by the emulation library. Call at program exit.
 */
void emul_lvgl_deinit(void);

/**
 * @brief Registers a known font pointer with a name for JSON serialization.
 *
 * @param font_ptr The lv_font_t pointer used in the client code.
 * @param name The name to use in the JSON output (e.g., "montserrat_18").
 */
void emul_lvgl_register_font(lv_font_t font_ptr, const char* name);


/**
 * @brief Generates the JSON representation of the UI hierarchy starting from the given root object.
 *
 * @param root_obj Pointer to the root object of the UI tree to serialize (usually the object created on the screen).
 * @return A newly allocated string containing the JSON data. The caller MUST free this string using free().
 *         Returns NULL on failure or if root_obj is invalid.
 */
char* emul_lvgl_get_json(lv_obj_t *root_obj); // Takes pointer


// --- LVGL API Subset Implementation ---
// ** ALL signatures updated to use lv_obj_t * **

// ** Object Creation ** Returns pointer
lv_obj_t * lv_obj_create(lv_obj_t *parent);
lv_obj_t * lv_label_create(lv_obj_t *parent);
lv_obj_t * lv_btn_create(lv_obj_t *parent);
lv_obj_t * lv_slider_create(lv_obj_t *parent);
lv_obj_t * lv_bar_create(lv_obj_t *parent);
lv_obj_t * lv_scale_create(lv_obj_t *parent);

// ** Object Deletion/Cleanup ** Takes pointer
void lv_obj_del(lv_obj_t *obj);
void lv_obj_clean(lv_obj_t *obj);

// ** Screen Access ** Returns pointer
lv_obj_t * lv_screen_active(void);
// lv_obj_t * lv_scr_act(void); // Alias if needed

void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent);

// ** Basic Property Setters ** Take pointer
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h);
void lv_obj_set_min_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_max_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y);
void lv_obj_set_align(lv_obj_t *obj, lv_align_t align);
void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs); // More complex align

// ** Flags ** Take pointer
void lv_obj_add_flag(lv_obj_t *obj, uint32_t f);
void lv_obj_clear_flag(lv_obj_t *obj, uint32_t f);


// ** Label Specific ** Take pointer
void lv_label_set_text(lv_obj_t *obj, const char * text);
void lv_label_set_text_fmt(lv_obj_t *obj, const char * fmt, ...); // Needs vsnprintf

// ** Slider Specific ** Take pointer
void lv_slider_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_range(lv_obj_t *obj, int32_t min, int32_t max);

// Bar Specific
void lv_bar_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
void lv_bar_set_range(lv_obj_t *obj, int32_t min, int32_t max);
// Add other bar property setters if needed

// Scale Specific
void lv_scale_set_range(lv_obj_t *obj, int32_t min, int32_t max);
void lv_scale_set_major_tick_every(lv_obj_t * obj, uint32_t nth);
void lv_scale_set_mode(lv_obj_t * obj, lv_scale_mode_t mode);
// Add other scale property setters if needed

// Layout.
void lv_obj_update_layout(lv_obj_t *obj);
void lv_obj_set_layout(lv_obj_t *obj, lv_layout_t layout);

// Flex.
void lv_obj_set_flex_flow(lv_obj_t *obj, lv_flex_flow_t flow);
void lv_obj_set_flex_align(lv_obj_t *obj, lv_flex_align_t main_place, lv_flex_align_t cross_place, lv_flex_align_t track_cross_place);
void lv_obj_set_style_flex_flow(lv_obj_t *obj, lv_flex_flow_t value, lv_style_selector_t selector);
void lv_obj_set_flex_grow(lv_obj_t *obj, uint8_t grow);

// Grid specific.
void lv_obj_set_grid_dsc_array(lv_obj_t *obj, const int32_t col_dsc[], const int32_t row_dsc[]);
void lv_obj_set_grid_align(lv_obj_t *obj, lv_grid_align_t column_align, lv_grid_align_t row_align);
void lv_obj_set_grid_cell(lv_obj_t *obj, lv_grid_align_t column_align, int32_t col_pos, int32_t col_span, lv_grid_align_t row_align, int32_t row_pos, int32_t row_span);

// ** Style Property Setters ** Take pointer
// These take lv_style_selector_t which combines part and state

// Background
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_bg_opa(lv_obj_t *obj, uint8_t value, lv_style_selector_t selector); // lv_opa_t is uint8_t
void lv_obj_set_style_radius(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);

// Border
void lv_obj_set_style_border_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_border_opa(lv_obj_t *obj, uint8_t value, lv_style_selector_t selector);

// Pad
void lv_obj_set_style_pad_all(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_pad_top(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_pad_left(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_pad_right(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_pad_bottom(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);

// Margin
void lv_obj_set_style_margin_all(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_margin_top(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_margin_left(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_margin_right(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_margin_bottom(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);

// Text
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t *obj, lv_font_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_align(lv_obj_t *obj, int32_t value, lv_style_selector_t selector); // text align enum

// Size (for specific parts like indicator, knob)
void lv_obj_set_style_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_min_width(lv_obj_t *t, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_max_width(lv_obj_t *t, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_max_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);

// Outline
void lv_obj_set_style_outline_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_outline_opa(lv_obj_t *obj, uint8_t value, lv_style_selector_t selector);

// Size Constraints
void lv_obj_set_style_min_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);


// --- Helper Value Creators (Mimicking LVGL) ---
lv_color_t lv_color_hex(uint32_t c);
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_white(void);
int32_t lv_pct(int32_t v); // Creates the percentage coordinate

// --- Font Definitions (Placeholders) ---
// Client code using this library must define these pointers, e.g.:
extern const lv_font_t lv_font_montserrat_14;
extern const lv_font_t lv_font_montserrat_18;
extern const lv_font_t lv_font_montserrat_24;

#endif // EMUL_LVGL_H