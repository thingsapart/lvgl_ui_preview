#ifndef EMUL_LVGL_INTERNAL_H
#define EMUL_LVGL_INTERNAL_H

#include "emul_lvgl.h"
#include "emul_lvgl_config.h"
#include <stdint.h>
#include <stdbool.h>

// --- Data Structures ---

// Union for property/style values
typedef enum {
    VAL_TYPE_NONE,
    VAL_TYPE_STRING,
    VAL_TYPE_INT,
    VAL_TYPE_COORD, // Special handling for pixels vs pct
    VAL_TYPE_COLOR,
    VAL_TYPE_BOOL,
    VAL_TYPE_FONT, // Store pointer, map to name later
    VAL_TYPE_ALIGN, // Store enum
    VAL_TYPE_TEXTALIGN // Store enum for text align
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

// Structure for storing a property
typedef struct {
    char* key; // e.g., "width", "text" (must be malloc'd)
    Value value;
} Property;

// Structure for storing a style property
typedef struct {
    lv_part_t part;
    lv_state_t state;
    char* prop_name; // e.g., "bg_color", "radius" (must be malloc'd)
    Value value;
} StyleEntry;

// Structure for the emulated object
typedef struct EmulLvglObject {
    uintptr_t id;             // Use address as ID for simplicity
    char* type;               // e.g., "label", "btn" (static string)
    struct EmulLvglObject* parent;

    // Dynamic arrays for children, properties, styles
    struct EmulLvglObject** children;
    size_t child_count;
    size_t child_capacity;

    Property* properties;
    size_t prop_count;
    size_t prop_capacity;

    StyleEntry* styles;
    size_t style_count;
    size_t style_capacity;

} EmulLvglObject;


// --- Internal Helper Functions ---

// Memory/Array Management
bool emul_obj_add_child(EmulLvglObject* parent, EmulLvglObject* child);
void emul_obj_remove_child(EmulLvglObject* parent, EmulLvglObject* child);
bool emul_obj_add_property(EmulLvglObject* obj, const char* key, Value value);
bool emul_obj_add_style(EmulLvglObject* obj, lv_part_t part, lv_state_t state, const char* prop_name, Value value);
void free_value(Value* value);
void free_property(Property* prop);
void free_style_entry(StyleEntry* entry);
void free_emul_object_recursive(EmulLvglObject* obj); // Frees object and its contents/children

// Finders
Property* find_property(EmulLvglObject* obj, const char* key);
StyleEntry* find_style(EmulLvglObject* obj, lv_part_t part, lv_state_t state, const char* prop_name);

// Value creators (internal versions if needed)
Value value_mk_string(const char* s);
Value value_mk_int(int32_t i);
Value value_mk_coord(lv_coord_t coord);
Value value_mk_color(lv_color_t color);
Value value_mk_bool(bool b);
Value value_mk_font(lv_font_t font);
Value value_mk_align(lv_align_t align);
Value value_mk_textalign(int32_t align); // lv_text_align_t

// Converters for JSON
const char* part_to_string(lv_part_t part);
const char* state_to_string(lv_state_t state); // Gets primary state name
const char* align_to_string(lv_align_t align);
const char* text_align_to_string(int32_t align); // lv_text_align_t
void color_to_hex_string(lv_color_t color, char* buf); // buf must be >= 8 chars
const char* font_ptr_to_name(lv_font_t font_ptr);
void coord_to_string(lv_coord_t coord, char* buf); // buf must be sufficient size (e.g., 10 chars)

// Internal Global State
extern EmulLvglObject* g_screen_obj;
extern EmulLvglObject** g_all_objects; // Dynamic array of all allocated objects
extern size_t g_all_objects_count;
extern size_t g_all_objects_capacity;

// Font mapping
typedef struct {
    lv_font_t ptr;
    char* name;
} FontMapEntry;
extern FontMapEntry* g_font_map;
extern size_t g_font_map_count;
extern size_t g_font_map_capacity;


#endif // EMUL_LVGL_INTERNAL_H
