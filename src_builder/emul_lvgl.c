#include "emul_lvgl_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // For va_list

#include "cJSON.h" // Include cJSON header for JSON generation part

// --- Global State ---
EmulLvglObject* g_screen_obj = NULL;
EmulLvglObject** g_all_objects = NULL;
size_t g_all_objects_count = 0;
size_t g_all_objects_capacity = 0;

FontMapEntry* g_font_map = NULL;
size_t g_font_map_count = 0;
size_t g_font_map_capacity = 0;

// Simple growable array macro (replace with more robust implementation if needed)
#define GROW_ARRAY(type, ptr, count, capacity) \
    if (count >= capacity) { \
        size_t old_capacity = capacity; \
        capacity = (capacity == 0) ? 8 : capacity * 2; \
        type* new_ptr = (type*)realloc(ptr, capacity * sizeof(type)); \
        if (!new_ptr) { /* Handle realloc failure */ \
            EMUL_LOG("ERROR: Failed to realloc array\n"); \
            capacity = old_capacity; /* Restore old capacity */ \
            /* Return error or abort? */ \
            /* return false; *//* Example: return false from function */ \
            ptr = NULL; \
        } else { \
            ptr = new_ptr; \
        } \
    }

// --- Memory Management Helpers ---

void free_value(Value* value) {
    if (value->type == VAL_TYPE_STRING && value->data.s) {
        free(value->data.s);
        value->data.s = NULL;
    }
    value->type = VAL_TYPE_NONE;
}

void free_property(Property* prop) {
    if (prop->key) free(prop->key);
    free_value(&prop->value);
}

void free_style_entry(StyleEntry* entry) {
    if (entry->prop_name) free(entry->prop_name);
    free_value(&entry->value);
}

// Forward declaration for recursion
void free_emul_object_internal(EmulLvglObject* obj);

void free_emul_object_contents(EmulLvglObject* obj) {
     if (!obj) return;
    // Free properties
    for (size_t i = 0; i < obj->prop_count; ++i) {
        free_property(&obj->properties[i]);
    }
    free(obj->properties);
    obj->properties = NULL;
    obj->prop_count = 0;
    obj->prop_capacity = 0;

    // Free styles
    for (size_t i = 0; i < obj->style_count; ++i) {
        free_style_entry(&obj->styles[i]);
    }
    free(obj->styles);
    obj->styles = NULL;
    obj->style_count = 0;
    obj->style_capacity = 0;

    // Free children array (pointers only, children freed separately)
    free(obj->children);
    obj->children = NULL;
    obj->child_count = 0;
    obj->child_capacity = 0;
}

void free_emul_object_internal(EmulLvglObject* obj) {
    if (!obj) return;

    // Free children recursively *first* to handle hierarchy
    // Make a copy of children pointers because obj->children will be freed
    size_t num_children = obj->child_count;
    EmulLvglObject** children_copy = NULL;
    if (num_children > 0) {
        children_copy = (EmulLvglObject**)malloc(num_children * sizeof(EmulLvglObject*));
        if(children_copy) {
            memcpy(children_copy, obj->children, num_children * sizeof(EmulLvglObject*));
        } else {
            EMUL_LOG("ERROR: Failed to alloc children copy for deletion\n");
            // Potential memory leak of children if we continue without the copy
        }
    }

    for (size_t i = 0; i < num_children; ++i) {
        // Find the child in the global list and free it from there to avoid double free
        // Or rely on the global list cleanup later? Let's free directly for now.
         if(children_copy) {
            free_emul_object_internal(children_copy[i]);
         }
    }
    free(children_copy); // Free the temporary copy array

    // Free this object's contents
    free_emul_object_contents(obj);

    // Free the object itself
    free(obj);
}


// Function to remove object from global list (needed for lv_obj_del)
void remove_from_global_list(EmulLvglObject* obj) {
    for (size_t i = 0; i < g_all_objects_count; ++i) {
        if (g_all_objects[i] == obj) {
            // Shift remaining elements down
            if (i < g_all_objects_count - 1) {
                memmove(&g_all_objects[i], &g_all_objects[i + 1], (g_all_objects_count - 1 - i) * sizeof(EmulLvglObject*));
            }
            g_all_objects_count--;
            // Optional: Shrink g_all_objects array if desired
            return;
        }
    }
}


bool emul_obj_add_child(EmulLvglObject* parent, EmulLvglObject* child) {
    if (!parent || !child) return false;
    GROW_ARRAY(EmulLvglObject*, parent->children, parent->child_count, parent->child_capacity);
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    return true;
}

void emul_obj_remove_child(EmulLvglObject* parent, EmulLvglObject* child_to_remove) {
    if (!parent || !child_to_remove || parent->child_count == 0) return;

    int found_idx = -1;
    for (size_t i = 0; i < parent->child_count; ++i) {
        if (parent->children[i] == child_to_remove) {
            found_idx = (int)i;
            break;
        }
    }

    if (found_idx != -1) {
        // Shift elements down
        if ((size_t)found_idx < parent->child_count - 1) {
             memmove(&parent->children[found_idx], &parent->children[found_idx + 1],
                     (parent->child_count - 1 - found_idx) * sizeof(EmulLvglObject*));
        }
        parent->child_count--;
        child_to_remove->parent = NULL; // Detach child
    }
}


Property* find_property(EmulLvglObject* obj, const char* key) {
    if (!obj || !key) return NULL;
    for (size_t i = 0; i < obj->prop_count; ++i) {
        if (obj->properties[i].key && strcmp(obj->properties[i].key, key) == 0) {
            return &obj->properties[i];
        }
    }
    return NULL;
}

bool emul_obj_add_property(EmulLvglObject* obj, const char* key, Value value) {
    if (!obj || !key) { free_value(&value); return false; }

    Property* existing = find_property(obj, key);
    if (existing) {
        // Update existing property
        free_value(&existing->value); // Free old value
        existing->value = value;      // Assign new value
        EMUL_LOG("Updated property '%s' on obj %p\n", key, obj);
        return true;
    } else {
        // Add new property
        GROW_ARRAY(Property, obj->properties, obj->prop_count, obj->prop_capacity);
        obj->properties[obj->prop_count].key = strdup(key);
        obj->properties[obj->prop_count].value = value;
        if (!obj->properties[obj->prop_count].key) { // strdup failed
            EMUL_LOG("ERROR: strdup failed for property key '%s'\n", key);
            free_value(&obj->properties[obj->prop_count].value);
            // Don't increment count
            return false;
        }
        obj->prop_count++;
        EMUL_LOG("Added property '%s' to obj %p\n", key, obj);
        return true;
    }
}


StyleEntry* find_style(EmulLvglObject* obj, lv_part_t part, lv_state_t state, const char* prop_name) {
     if (!obj || !prop_name) return NULL;
     for (size_t i = 0; i < obj->style_count; ++i) {
         if (obj->styles[i].part == part &&
             obj->styles[i].state == state &&
             obj->styles[i].prop_name &&
             strcmp(obj->styles[i].prop_name, prop_name) == 0)
         {
             return &obj->styles[i];
         }
     }
     return NULL;
}

bool emul_obj_add_style(EmulLvglObject* obj, lv_part_t part, lv_state_t state, const char* prop_name, Value value) {
     if (!obj || !prop_name) { free_value(&value); return false; }

     StyleEntry* existing = find_style(obj, part, state, prop_name);
     if(existing) {
         free_value(&existing->value);
         existing->value = value;
         EMUL_LOG("Updated style '%s' [part:%u state:%u] on obj %p\n", prop_name, part, state, obj);
         return true;
     } else {
         GROW_ARRAY(StyleEntry, obj->styles, obj->style_count, obj->style_capacity);
         obj->styles[obj->style_count].part = part;
         obj->styles[obj->style_count].state = state;
         obj->styles[obj->style_count].prop_name = strdup(prop_name);
         obj->styles[obj->style_count].value = value;
         if (!obj->styles[obj->style_count].prop_name) {
             EMUL_LOG("ERROR: strdup failed for style prop_name '%s'\n", prop_name);
             free_value(&obj->styles[obj->style_count].value);
             return false;
         }
         obj->style_count++;
         EMUL_LOG("Added style '%s' [part:%u state:%u] to obj %p\n", prop_name, part, state, obj);
         return true;
     }
}


// --- Value Creators ---
Value value_mk_string(const char* s) {
    Value v = {.type = VAL_TYPE_STRING, .data = {.s = s ? strdup(s) : NULL}};
    if (s && !v.data.s) { EMUL_LOG("ERROR: strdup failed for string value\n"); v.type = VAL_TYPE_NONE; }
    return v;
}
Value value_mk_int(int32_t i) { return (Value){.type = VAL_TYPE_INT, .data = {.i = i}}; }
Value value_mk_coord(lv_coord_t coord) { return (Value){.type = VAL_TYPE_COORD, .data = {.coord = coord}}; }
Value value_mk_color(lv_color_t color) { return (Value){.type = VAL_TYPE_COLOR, .data = {.color = color}}; }
Value value_mk_bool(bool b) { return (Value){.type = VAL_TYPE_BOOL, .data = {.b = b}}; }
Value value_mk_font(lv_font_t font) { return (Value){.type = VAL_TYPE_FONT, .data = {.font = font}}; }
Value value_mk_align(lv_align_t align) { return (Value){.type = VAL_TYPE_ALIGN, .data = {.align = align}}; }
Value value_mk_textalign(int32_t align) { return (Value){.type = VAL_TYPE_TEXTALIGN, .data = {.text_align = align}}; }


// --- Internal Object Creation Helper ---
static lv_obj_t create_object_internal(lv_obj_t parent_obj, const char* type_name) {
    EmulLvglObject* parent_emul = (EmulLvglObject*)parent_obj; // Can be NULL

    EmulLvglObject* new_obj = (EmulLvglObject*)calloc(1, sizeof(EmulLvglObject));
    if (!new_obj) {
        EMUL_LOG("ERROR: Failed to allocate memory for new object\n");
        return NULL;
    }

    new_obj->id = (uintptr_t)new_obj; // Use address as ID
    new_obj->type = (char*)type_name; // Assign static type string
    new_obj->parent = parent_emul;

    // Add to parent's children list
    if (parent_emul) {
        if (!emul_obj_add_child(parent_emul, new_obj)) {
            free(new_obj); // Failed to add to parent
            return NULL;
        }
    }

    // Add to global list of all objects
    GROW_ARRAY(EmulLvglObject*, g_all_objects, g_all_objects_count, g_all_objects_capacity);
    g_all_objects[g_all_objects_count++] = new_obj;

    EMUL_LOG("Created object %p (type: %s), parent: %p\n", new_obj, type_name, parent_emul);
    return (lv_obj_t)new_obj;
}

// --- Library Control ---

void emul_lvgl_init(void) {
    EMUL_LOG("Initializing LVGL Emulation Library\n");
    if (g_screen_obj != NULL) {
        EMUL_LOG("WARN: Already initialized. Resetting state.\n");
        emul_lvgl_reset(); // Reset if called again
    }

    // Create the dummy screen object
    g_screen_obj = (EmulLvglObject*)calloc(1, sizeof(EmulLvglObject));
     if (!g_screen_obj) {
         EMUL_LOG("ERROR: Failed to allocate screen object\n");
         return; // Critical failure
     }
    g_screen_obj->id = (uintptr_t)g_screen_obj;
    g_screen_obj->type = "screen"; // Special type
    g_screen_obj->parent = NULL;

    // Add screen to global list
    GROW_ARRAY(EmulLvglObject*, g_all_objects, g_all_objects_count, g_all_objects_capacity);
    g_all_objects[g_all_objects_count++] = g_screen_obj;

    // Init font map
    g_font_map = NULL;
    g_font_map_count = 0;
    g_font_map_capacity = 0;

    EMUL_LOG("Screen object %p created.\n", g_screen_obj);
}

void emul_lvgl_reset(void) {
    EMUL_LOG("Resetting LVGL Emulation state...\n");

    // Free all objects *except* the screen object (which is static conceptually)
    // Iterate backwards as deletion might modify the list structure
    while(g_all_objects_count > 0) {
        EmulLvglObject* obj_to_del = g_all_objects[g_all_objects_count - 1];
         if (obj_to_del == g_screen_obj) {
            // Clear screen contents but keep the screen obj itself
             free_emul_object_contents(g_screen_obj);
             // Remove from global list temporarily so it's not double-handled
             g_all_objects_count--;
         } else {
            // This will remove it from the global list internally
             lv_obj_del((lv_obj_t)obj_to_del);
             // Note: lv_obj_del handles removal from parent and global list now
         }
    }
    // Sanity check: g_all_objects_count should be 0 now
    // Re-add screen object to the global list (which should be empty)
    if(g_screen_obj && g_all_objects_count == 0) {
         GROW_ARRAY(EmulLvglObject*, g_all_objects, g_all_objects_count, g_all_objects_capacity);
         g_all_objects[g_all_objects_count++] = g_screen_obj;
    } else if (!g_screen_obj) {
         EMUL_LOG("ERROR: Screen object was lost during reset?\n");
    } else {
         EMUL_LOG("ERROR: Global object list not empty after reset?\n");
    }


    // Free font map entries
    for (size_t i = 0; i < g_font_map_count; ++i) {
        free(g_font_map[i].name);
    }
    free(g_font_map);
    g_font_map = NULL;
    g_font_map_count = 0;
    g_font_map_capacity = 0;

    EMUL_LOG("Reset complete. Screen object %p remains.\n", g_screen_obj);
}

void emul_lvgl_deinit(void) {
    EMUL_LOG("Deinitializing LVGL Emulation Library...\n");
    emul_lvgl_reset(); // Clear all dynamic objects first

    // Free the screen object itself now
    if (g_screen_obj) {
        free(g_screen_obj);
        g_screen_obj = NULL;
    }
    // Free the global object list array
    free(g_all_objects);
    g_all_objects = NULL;
    g_all_objects_count = 0;
    g_all_objects_capacity = 0;

     // Font map already freed by reset
    EMUL_LOG("Deinitialization complete.\n");
}


void emul_lvgl_register_font(lv_font_t font_ptr, const char* name) {
    if (!font_ptr || !name) return;

    // Check if already registered
    for (size_t i = 0; i < g_font_map_count; ++i) {
        if (g_font_map[i].ptr == font_ptr) {
            // Update name if different (optional)
            if (strcmp(g_font_map[i].name, name) != 0) {
                free(g_font_map[i].name);
                g_font_map[i].name = strdup(name);
            }
            return;
        }
    }

    // Add new entry
    GROW_ARRAY(FontMapEntry, g_font_map, g_font_map_count, g_font_map_capacity);
    g_font_map[g_font_map_count].ptr = font_ptr;
    g_font_map[g_font_map_count].name = strdup(name);
     if (!g_font_map[g_font_map_count].name) {
         EMUL_LOG("ERROR: strdup failed for font name '%s'\n", name);
         return; // Don't increment count
     }
    g_font_map_count++;
    EMUL_LOG("Registered font %p as '%s'\n", font_ptr, name);
}


// --- LVGL API Implementation ---

// ** Object Creation **
lv_obj_t lv_obj_create(lv_obj_t parent) { return create_object_internal(parent, "obj"); }
lv_obj_t lv_label_create(lv_obj_t parent) { return create_object_internal(parent, "label"); }
lv_obj_t lv_btn_create(lv_obj_t parent) { return create_object_internal(parent, "btn"); }
lv_obj_t lv_slider_create(lv_obj_t parent) { return create_object_internal(parent, "slider"); }
// ... add others

// ** Object Deletion/Cleanup **
void lv_obj_del(lv_obj_t obj) {
    EmulLvglObject* emul_obj = (EmulLvglObject*)obj;
    if (!emul_obj || obj == g_screen_obj) { // Don't delete the screen
        EMUL_LOG("WARN: Attempt to delete NULL or screen object (%p)\n", obj);
        return;
    }

    EMUL_LOG("Deleting object %p (type: %s)...\n", emul_obj, emul_obj->type);

    // 1. Remove from parent's children list
    if (emul_obj->parent) {
        emul_obj_remove_child(emul_obj->parent, emul_obj);
    }

    // 2. Remove from global list *before* freeing to avoid use-after-free if called recursively
    remove_from_global_list(emul_obj);

    // 3. Free the object and its children recursively (this handles the actual memory)
    // Note: This assumes children haven't been reparented elsewhere.
    // The free_emul_object_internal function handles recursive deletion.
     // We need a non-recursive free here since children are deleted via their own lv_obj_del calls usually.
     // Let's modify free_emul_object_internal slightly or create a new one.
     // Let free_emul_object_internal just free the object itself and its *contents*.
     // The recursive deletion happens because lv_obj_del calls itself on children.

     // Redefine: free_emul_object_internal frees object + contents + children array (NOT recursive children free)
     // Redefine: free_emul_object_recursive frees object + contents + recursively frees children

    // Let's use a recursive free here, assuming lv_obj_del is the main entry point for deletion.
    // Need to be careful about double frees if children are also in the global list.
    // Safest: Just free the object's *contents* and the object itself. Children deletion
    // should happen via their own lv_obj_del calls triggered by the application or lv_obj_clean.

     // Let's try recursive free, but need to ensure remove_from_global_list happens first for all.
     // This is tricky. Let's stick to the original plan: lv_obj_del frees the target object's
     // contents and the object itself after removing from parent/global list.
     // Recursive deletion is achieved if the app calls lv_obj_del on children or lv_obj_clean.


     // Call the recursive free - it should handle children correctly now.
     // We need to temporarily detach children before freeing to avoid issues if parent is freed first.
     size_t num_children = emul_obj->child_count;
     EmulLvglObject** children_copy = NULL;
     if(num_children > 0) {
        children_copy = malloc(num_children * sizeof(EmulLvglObject*));
        if(children_copy) memcpy(children_copy, emul_obj->children, num_children * sizeof(EmulLvglObject*));
     }

     // Free current object's direct contents and structure
      free_emul_object_contents(emul_obj);
      free(emul_obj); // Free the object itself


     // Now recursively delete children that were detached
     if(children_copy) {
         for(size_t i = 0; i < num_children; ++i) {
             // Check if child still exists in global list (might have been deleted independently)
             bool child_exists = false;
             for(size_t j=0; j<g_all_objects_count; ++j) {
                 if(g_all_objects[j] == children_copy[i]) {
                     child_exists = true;
                     break;
                 }
             }
             if(child_exists) {
                lv_obj_del((lv_obj_t)children_copy[i]); // Recursive call
             }
         }
         free(children_copy);
     }
     EMUL_LOG("Deletion complete for obj %p.\n", obj);

}

void lv_obj_clean(lv_obj_t obj) {
     EmulLvglObject* emul_obj = (EmulLvglObject*)obj;
     if (!emul_obj) return;

     EMUL_LOG("Cleaning children of object %p\n", emul_obj);

     // Make a copy of children pointers because lv_obj_del will modify the list
     size_t num_children = emul_obj->child_count;
     if (num_children == 0) return;

     EmulLvglObject** children_to_delete = (EmulLvglObject**)malloc(num_children * sizeof(EmulLvglObject*));
     if (!children_to_delete) {
          EMUL_LOG("ERROR: Failed to allocate memory for cleaning children\n");
          return;
     }
     memcpy(children_to_delete, emul_obj->children, num_children * sizeof(EmulLvglObject*));

     // Delete each child using lv_obj_del
     for (size_t i = 0; i < num_children; ++i) {
         lv_obj_del((lv_obj_t)children_to_delete[i]);
     }

     free(children_to_delete);

     // Ensure the object's own children list is now empty (should be handled by lv_obj_del calling remove_child)
     if (emul_obj->child_count != 0) {
         EMUL_LOG("WARN: Child count not zero after cleaning obj %p\n", emul_obj);
         emul_obj->child_count = 0; // Force it
     }
     EMUL_LOG("Cleaning complete for obj %p\n", emul_obj);
}


// ** Screen Access **
lv_obj_t lv_screen_active(void) {
    if (!g_screen_obj) {
        EMUL_LOG("WARN: lv_screen_active called before emul_lvgl_init or after deinit.\n");
        // Maybe initialize implicitly? Or return NULL? Let's return NULL.
        return NULL;
    }
    return (lv_obj_t)g_screen_obj;
}
// lv_obj_t lv_scr_act(void) { return lv_screen_active(); }


// ** Basic Property Setters **
void lv_obj_set_width(lv_obj_t obj, lv_coord_t w) { emul_obj_add_property((EmulLvglObject*)obj, "width", value_mk_coord(w)); }
void lv_obj_set_height(lv_obj_t obj, lv_coord_t h) { emul_obj_add_property((EmulLvglObject*)obj, "height", value_mk_coord(h)); }
void lv_obj_set_size(lv_obj_t obj, lv_coord_t w, lv_coord_t h) { lv_obj_set_width(obj, w); lv_obj_set_height(obj, h); }
void lv_obj_set_pos(lv_obj_t obj, lv_coord_t x, lv_coord_t y) { emul_obj_add_property((EmulLvglObject*)obj, "x", value_mk_coord(x)); emul_obj_add_property((EmulLvglObject*)obj, "y", value_mk_coord(y));}
void lv_obj_set_x(lv_obj_t obj, lv_coord_t x) { emul_obj_add_property((EmulLvglObject*)obj, "x", value_mk_coord(x)); }
void lv_obj_set_y(lv_obj_t obj, lv_coord_t y) { emul_obj_add_property((EmulLvglObject*)obj, "y", value_mk_coord(y)); }
void lv_obj_set_align(lv_obj_t obj, lv_align_t align) { emul_obj_add_property((EmulLvglObject*)obj, "align", value_mk_align(align)); }

void lv_obj_align(lv_obj_t obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
     // Store align, x, y directly. The viewer app understands this combination.
     lv_obj_set_align(obj, align);
     lv_obj_set_x(obj, x_ofs);
     lv_obj_set_y(obj, y_ofs);
 }

// ** Flags **
void lv_obj_add_flag(lv_obj_t obj, uint32_t f) {
    // Simplification: Map known flags to boolean properties
    if (f & LV_OBJ_FLAG_HIDDEN) emul_obj_add_property((EmulLvglObject*)obj, "hidden", value_mk_bool(true));
    if (f & LV_OBJ_FLAG_CLICKABLE) emul_obj_add_property((EmulLvglObject*)obj, "clickable", value_mk_bool(true));
    // Add other flags...
}
void lv_obj_clear_flag(lv_obj_t obj, uint32_t f) {
    if (f & LV_OBJ_FLAG_HIDDEN) emul_obj_add_property((EmulLvglObject*)obj, "hidden", value_mk_bool(false));
    if (f & LV_OBJ_FLAG_CLICKABLE) emul_obj_add_property((EmulLvglObject*)obj, "clickable", value_mk_bool(false));
    // Add other flags...
}

// ** Label Specific **
void lv_label_set_text(lv_obj_t obj, const char * text) { emul_obj_add_property((EmulLvglObject*)obj, "text", value_mk_string(text)); }
void lv_label_set_text_fmt(lv_obj_t obj, const char * fmt, ...) {
    char buf[256]; // Adjust buffer size if needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    lv_label_set_text(obj, buf);
}

// ** Slider Specific **
void lv_slider_set_value(lv_obj_t obj, int32_t value, lv_anim_enable_t anim) {
    (void)anim; // Ignore animation parameter
    emul_obj_add_property((EmulLvglObject*)obj, "value", value_mk_int(value));
}
void lv_slider_set_range(lv_obj_t obj, int32_t min, int32_t max) {
    emul_obj_add_property((EmulLvglObject*)obj, "range_min", value_mk_int(min));
    emul_obj_add_property((EmulLvglObject*)obj, "range_max", value_mk_int(max));
}


// --- Style Property Setters ---
// Helper macro to reduce boilerplate
#define ADD_STYLE(obj, selector, prop_name_str, value_func) \
    do { \
        EmulLvglObject* emul_obj = (EmulLvglObject*)obj; \
        if (!emul_obj) break; \
        lv_part_t part = selector & 0xFF0000; /* Extract part */ \
        lv_state_t state = selector & 0x00FFFF; /* Extract state */ \
        emul_obj_add_style(emul_obj, part, state, prop_name_str, value_func); \
    } while(0)

// Background
void lv_obj_set_style_bg_color(lv_obj_t obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "bg_color", value_mk_color(value)); }
void lv_obj_set_style_bg_opa(lv_obj_t obj, uint8_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "bg_opa", value_mk_int(value)); }
void lv_obj_set_style_radius(lv_obj_t obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "radius", value_mk_coord(value)); }

// Border
void lv_obj_set_style_border_width(lv_obj_t obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "border_width", value_mk_coord(value)); }
void lv_obj_set_style_border_color(lv_obj_t obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "border_color", value_mk_color(value)); }
void lv_obj_set_style_border_opa(lv_obj_t obj, uint8_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "border_opa", value_mk_int(value)); }

// Text
void lv_obj_set_style_text_color(lv_obj_t obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "text_color", value_mk_color(value)); }
void lv_obj_set_style_text_font(lv_obj_t obj, lv_font_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "text_font", value_mk_font(value)); }
// LVGL v9 uses lv_text_align_t enum which is compatible with int32_t
void lv_obj_set_style_text_align(lv_obj_t obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "text_align", value_mk_textalign(value)); }


// Size (for specific parts)
void lv_obj_set_style_width(lv_obj_t obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "width", value_mk_coord(value)); }
void lv_obj_set_style_height(lv_obj_t obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "height", value_mk_coord(value)); }

// --- Helper Value Creators ---
lv_color_t lv_color_hex(uint32_t c) {
    return lv_color_make((uint8_t)((c >> 16) & 0xFF), (uint8_t)((c >> 8) & 0xFF), (uint8_t)(c & 0xFF));
}
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b) {
    return (lv_color_t){.r = r, .g = g, .b = b};
}
lv_coord_t lv_pct(uint8_t v) {
    if (v > 100) v = 100; // Cap at 100%
    return LV_COORD_SET_PCT(v);
}

// --- JSON Generation (Implementation in emul_json.c typically) ---
// Forward declare the recursive builder
cJSON* build_json_recursive(EmulLvglObject* obj);

char* emul_lvgl_get_json(lv_obj_t root_obj) {
    EmulLvglObject* root_emul = (EmulLvglObject*)root_obj;
    if (!root_emul) {
        EMUL_LOG("ERROR: emul_lvgl_get_json called with NULL root object\n");
        return NULL;
    }

    // Need a top-level structure like {"root": {...}} expected by the viewer
    cJSON *root_container = cJSON_CreateObject();
     if (!root_container) {
         EMUL_LOG("ERROR: Failed to create root cJSON object\n");
         return NULL;
     }

    cJSON* ui_tree_json = build_json_recursive(root_emul);
    if (!ui_tree_json) {
        cJSON_Delete(root_container);
        EMUL_LOG("ERROR: Failed to build JSON structure recursively\n");
        return NULL;
    }

    cJSON_AddItemToObject(root_container, "root", ui_tree_json);

    char* json_string = cJSON_PrintUnformatted(root_container); // Or cJSON_Print for pretty print
    cJSON_Delete(root_container); // Clean up cJSON structure

    if (!json_string) {
         EMUL_LOG("ERROR: cJSON_Print failed\n");
    } else {
         EMUL_LOG("Generated JSON successfully for root %p\n", root_obj);
    }

    return json_string; // Caller must free this
}

// (Converter implementations would go here or in emul_json.c)
// --- Converters for JSON ---

const char* part_to_string(lv_part_t part) {
    // Map defined parts back to strings used in viewer JSON
    switch(part) {
        case LV_PART_MAIN: return "default"; // Map main back to "default" key
        case LV_PART_SCROLLBAR: return "scrollbar";
        case LV_PART_INDICATOR: return "indicator";
        case LV_PART_KNOB: return "knob";
        case LV_PART_SELECTED: return "selected";
        case LV_PART_ITEMS: return "items";
        case LV_PART_CURSOR: return "cursor";
        // Add others...
        default: return "unknown_part";
    }
}

const char* state_to_string(lv_state_t state) {
     // Only handle single primary states for simplicity in JSON keys
     if (state & LV_STATE_DISABLED) return "disabled";
     if (state & LV_STATE_CHECKED) return "checked";
     if (state & LV_STATE_FOCUSED) return "focused"; // Includes FOCUS_KEY
     if (state & LV_STATE_EDITED) return "edited";
     if (state & LV_STATE_PRESSED) return "pressed";
     if (state & LV_STATE_HOVERED) return "hovered";
     if (state & LV_STATE_SCROLLED) return "scrolled";
     return "default"; // Fallback for LV_STATE_DEFAULT or combined states
}

const char* align_to_string(lv_align_t align) {
    switch(align) {
        case LV_ALIGN_DEFAULT: return "default";
        case LV_ALIGN_TOP_LEFT: return "top_left";
        case LV_ALIGN_TOP_MID: return "top_mid";
        case LV_ALIGN_TOP_RIGHT: return "top_right";
        case LV_ALIGN_LEFT_MID: return "left_mid";
        case LV_ALIGN_CENTER: return "center";
        case LV_ALIGN_RIGHT_MID: return "right_mid";
        case LV_ALIGN_BOTTOM_LEFT: return "bottom_left";
        case LV_ALIGN_BOTTOM_MID: return "bottom_mid";
        case LV_ALIGN_BOTTOM_RIGHT: return "bottom_right";
        default: return "default";
    }
}

const char* text_align_to_string(int32_t align) { // lv_text_align_t enum values
     switch(align) {
         // Assumes standard LV_TEXT_ALIGN_... values
         case 0: return "auto";   // LV_TEXT_ALIGN_AUTO
         case 1: return "left";   // LV_TEXT_ALIGN_LEFT
         case 2: return "center"; // LV_TEXT_ALIGN_CENTER
         case 3: return "right";  // LV_TEXT_ALIGN_RIGHT
         default: return "auto";
     }
 }


void color_to_hex_string(lv_color_t color, char* buf) {
    // Simple hex conversion
    sprintf(buf, "#%02X%02X%02X", color.r, color.g, color.b);
}

const char* font_ptr_to_name(lv_font_t font_ptr) {
    if (!font_ptr) return "default"; // Or NULL? Let's return "default"
    for (size_t i = 0; i < g_font_map_count; ++i) {
        if (g_font_map[i].ptr == font_ptr) {
            return g_font_map[i].name;
        }
    }
    EMUL_LOG("WARN: Font pointer %p not registered. Using 'unknown_font'.\n", font_ptr);
    return "unknown_font"; // Fallback if not registered
}

void coord_to_string(lv_coord_t coord, char* buf) {
    if (LV_COORD_IS_PCT(coord)) {
        sprintf(buf, "%d%%", LV_COORD_GET_PCT(coord));
    } else {
        sprintf(buf, "%d", coord);
    }
}


// --- Recursive JSON Builder ---
cJSON* build_json_recursive(EmulLvglObject* obj) {
    if (!obj) return NULL;

    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) return NULL;

    // Type
    cJSON_AddStringToObject(json_obj, "type", obj->type);

    // Properties
    if (obj->prop_count > 0) {
        cJSON* props_json = cJSON_CreateObject();
        cJSON_AddItemToObject(json_obj, "properties", props_json);
        char coord_buf[12]; // Buffer for coordinate strings
        for (size_t i = 0; i < obj->prop_count; ++i) {
            Property* prop = &obj->properties[i];
            switch (prop->value.type) {
                case VAL_TYPE_STRING:
                    cJSON_AddStringToObject(props_json, prop->key, prop->value.data.s ? prop->value.data.s : "");
                    break;
                case VAL_TYPE_INT:
                    cJSON_AddNumberToObject(props_json, prop->key, prop->value.data.i);
                    break;
                case VAL_TYPE_COORD:
                    coord_to_string(prop->value.data.coord, coord_buf);
                    // Check if it's a percentage string or plain number
                    if (strchr(coord_buf, '%')) {
                         cJSON_AddStringToObject(props_json, prop->key, coord_buf);
                    } else {
                         cJSON_AddNumberToObject(props_json, prop->key, prop->value.data.coord);
                    }
                    break;
                case VAL_TYPE_BOOL:
                    cJSON_AddBoolToObject(props_json, prop->key, prop->value.data.b);
                    break;
                case VAL_TYPE_ALIGN:
                     cJSON_AddStringToObject(props_json, prop->key, align_to_string(prop->value.data.align));
                     break;
                // Color, Font, TextAlign are typically styles, but handle if needed
                default: break; // Ignore others for properties
            }
        }
    }

    // Styles
    if (obj->style_count > 0) {
        cJSON* styles_json = cJSON_CreateObject(); // Root styles object: { "default": {...}, "indicator": {...} }
        cJSON_AddItemToObject(json_obj, "styles", styles_json);

        // Group styles by part, then state
        for (size_t i = 0; i < obj->style_count; ++i) {
            StyleEntry* entry = &obj->styles[i];
            const char* part_str = part_to_string(entry->part);
            const char* state_str = state_to_string(entry->state); // Gets primary state string

            // Get or create part object: styles_json -> part_str
            cJSON* part_obj = cJSON_GetObjectItemCaseSensitive(styles_json, part_str);
            if (!part_obj) {
                part_obj = cJSON_CreateObject();
                cJSON_AddItemToObject(styles_json, part_str, part_obj);
            }

            // Get or create state object: part_obj -> state_str
            cJSON* state_obj = cJSON_GetObjectItemCaseSensitive(part_obj, state_str);
            if (!state_obj) {
                state_obj = cJSON_CreateObject();
                cJSON_AddItemToObject(part_obj, state_str, state_obj);
            }

            // Add the actual style property to the state object
            char buf[12]; // Shared buffer for conversions
            switch (entry->value.type) {
                case VAL_TYPE_COLOR:
                    color_to_hex_string(entry->value.data.color, buf);
                    cJSON_AddStringToObject(state_obj, entry->prop_name, buf);
                    break;
                case VAL_TYPE_COORD:
                    coord_to_string(entry->value.data.coord, buf);
                     if (strchr(buf, '%')) {
                         cJSON_AddStringToObject(state_obj, entry->prop_name, buf);
                     } else {
                         cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.coord);
                     }
                    break;
                case VAL_TYPE_INT: // Used for opa, border_width etc. that might not be coords
                    cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.i);
                    break;
                case VAL_TYPE_FONT:
                     cJSON_AddStringToObject(state_obj, entry->prop_name, font_ptr_to_name(entry->value.data.font));
                     break;
                 case VAL_TYPE_TEXTALIGN:
                     cJSON_AddStringToObject(state_obj, entry->prop_name, text_align_to_string(entry->value.data.text_align));
                     break;
                // Add other types (bool, string if styles use them)
                default: break;
            }
        }
    }

    // Children
    if (obj->child_count > 0) {
        cJSON* children_json = cJSON_CreateArray();
        cJSON_AddItemToObject(json_obj, "children", children_json);
        for (size_t i = 0; i < obj->child_count; ++i) {
            cJSON* child_json_obj = build_json_recursive(obj->children[i]);
            if (child_json_obj) {
                cJSON_AddItemToArray(children_json, child_json_obj);
            }
        }
    }

    return json_obj;
}
