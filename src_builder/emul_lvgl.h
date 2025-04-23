#ifndef EMUL_LVGL_H
#define EMUL_LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

// --- Basic Type Definitions (Mimicking LVGL) ---

// lv_obj_t is the struct type
struct _lv_obj_t; // Forward declaration
typedef struct _lv_obj_t lv_obj_t; // lv_obj_t is now the struct type

// Coordinates
typedef int16_t lv_coord_t;
#define LV_COORD_IS_PCT(v) ((v) <= -1000)
#define LV_COORD_GET_PCT(v) (-1000 - (v))
#define LV_COORD_SET_PCT(v) (-1000 - (v))

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
    LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_MID, LV_ALIGN_BOTTOM_RIGHT,
} lv_align_t;

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

// Flags
#define LV_OBJ_FLAG_HIDDEN   (1 << 0)
#define LV_OBJ_FLAG_CLICKABLE (1 << 1)
// Add more flag definitions...


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
// Add other lv_<widget>_create functions as needed...

// ** Object Deletion/Cleanup ** Takes pointer
void lv_obj_del(lv_obj_t *obj);
void lv_obj_clean(lv_obj_t *obj);

// ** Screen Access ** Returns pointer
lv_obj_t * lv_screen_active(void);
// lv_obj_t * lv_scr_act(void); // Alias if needed

// ** Basic Property Setters ** Take pointer
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h);
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

// Add setters for other widgets...


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

// Text
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t *obj, lv_font_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_align(lv_obj_t *obj, int32_t value, lv_style_selector_t selector); // text align enum

// Size (for specific parts like indicator, knob)
void lv_obj_set_style_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector);


// Add more style setters as needed...


// --- Helper Value Creators (Mimicking LVGL) ---
lv_color_t lv_color_hex(uint32_t c);
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_coord_t lv_pct(uint8_t v); // Creates the percentage coordinate

// --- Font Definitions (Placeholders) ---
// Client code using this library must define these pointers, e.g.:
// extern const lv_font_t lv_font_montserrat_14;
// extern const lv_font_t lv_font_montserrat_18;
// Then call emul_lvgl_register_font(&lv_font_montserrat_14, "montserrat_14");


#endif // EMUL_LVGL_H
