#ifndef EMUL_LVGL_INTERNAL_H
#define EMUL_LVGL_INTERNAL_H

// Include the public header FIRST to get the definition of lv_obj_t (struct)
#include "emul_lvgl.h"
#include "emul_lvgl_config.h"
#include <stdint.h>
#include <stdbool.h>

// --- Data Structures ---

// Union for property/style values (remains the same)
typedef enum {
    VAL_TYPE_NONE, VAL_TYPE_STRING, VAL_TYPE_INT, VAL_TYPE_COORD,
    VAL_TYPE_COLOR, VAL_TYPE_BOOL, VAL_TYPE_FONT, VAL_TYPE_ALIGN,
    VAL_TYPE_TEXTALIGN
} ValueType;

typedef struct {
    ValueType type;
    union {
        char* s;        // Must be malloc'd
        int32_t i;
        lv_coord_t coord;
        lv_color_t color;
        bool b;
        lv_font_t font;
        lv_align_t align;
        int32_t text_align; // lv_text_align_t is int32_t based enum
    } data;
} Value;

// Structure for storing a property (remains the same)
typedef struct {
    char* key; // e.g., "width", "text" (must be malloc'd)
    Value value;
} Property;

// Structure for storing a style property (remains the same)
typedef struct {
    lv_part_t part;
    lv_state_t state;
    char* prop_name; // e.g., "bg_color", "radius" (must be malloc'd)
    Value value;
} StyleEntry;

// Structure for the emulated object
// ** This IS the definition of the struct lv_obj_t points to **
// Rename internal struct to match the public forward declaration name convention
typedef struct _lv_obj_t { // Matches public 'struct _lv_obj_t'
    uintptr_t id;             // Use address as ID for simplicity
    char* type;               // e.g., "label", "btn" (static string)
    struct _lv_obj_t* parent; // Pointer to the same struct type

    // Dynamic arrays for children, properties, styles
    struct _lv_obj_t** children; // Array of pointers to children
    size_t child_count;
    size_t child_capacity;

    Property* properties;
    size_t prop_count;
    size_t prop_capacity;

    StyleEntry* styles;
    size_t style_count;
    size_t style_capacity;

} EmulLvglObject; // Keep internal alias if helpful, but the struct name IS _lv_obj_t

// --- Internal Helper Functions ---
// ** These now operate directly on lv_obj_t* (pointers to the internal struct) **
bool emul_obj_add_child(lv_obj_t *parent, lv_obj_t *child);
void emul_obj_remove_child(lv_obj_t *parent, lv_obj_t *child);
bool emul_obj_add_property(lv_obj_t *obj, const char* key, Value value);
bool emul_obj_add_style(lv_obj_t *obj, lv_part_t part, lv_state_t state, const char* prop_name, Value value);
void free_value(Value* value);
void free_property(Property* prop);
void free_style_entry(StyleEntry* entry);
void free_emul_object_recursive(lv_obj_t *obj); // Takes pointer
Property* find_property(lv_obj_t *obj, const char* key); // Takes pointer
StyleEntry* find_style(lv_obj_t *obj, lv_part_t part, lv_state_t state, const char* prop_name); // Takes pointer
Value value_mk_string(const char* s);
Value value_mk_int(int32_t i);
Value value_mk_coord(lv_coord_t coord);
Value value_mk_color(lv_color_t color);
Value value_mk_bool(bool b);
Value value_mk_font(lv_font_t font);
Value value_mk_align(lv_align_t align);
Value value_mk_textalign(int32_t align); // lv_text_align_t

// Converters for JSON (remain the same)
const char* part_to_string(lv_part_t part);
const char* state_to_string(lv_state_t state);
const char* align_to_string(lv_align_t align);
const char* text_align_to_string(int32_t align);
void color_to_hex_string(lv_color_t color, char* buf);
const char* font_ptr_to_name(lv_font_t font_ptr);
void coord_to_string(lv_coord_t coord, char* buf);

// Internal Global State
// ** Store pointers to the internal struct type **
extern lv_obj_t * g_screen_obj; // Pointer to the screen object
extern lv_obj_t ** g_all_objects; // Array of pointers
extern size_t g_all_objects_count;
extern size_t g_all_objects_capacity;

// Font mapping (remains the same)
typedef struct {
    lv_font_t ptr;
    char* name;
} FontMapEntry;
extern FontMapEntry* g_font_map;
extern size_t g_font_map_count;
extern size_t g_font_map_capacity;


#endif // EMUL_LVGL_INTERNAL_H
