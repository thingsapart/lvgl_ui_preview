// emul_lvgl.c

#include "emul_lvgl_internal.h" // Includes public header first
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "cJSON.h"

#ifndef JSON_print
#  ifdef DEBUG
#    define JSON_print cJSON_Print
#  else
#    define JSON_print cJSON_PrintUnformatted
#  endif
#endif

static const char *TAG = "emul_lvgl";

// --- Global State (using lv_obj_t *) ---
lv_obj_t * g_screen_obj = NULL;
lv_obj_t ** g_all_objects = NULL; // Array of lv_obj_t *
size_t g_all_objects_count = 0;
size_t g_all_objects_capacity = 0;

FontMapEntry* g_font_map = NULL;
size_t g_font_map_count = 0;
size_t g_font_map_capacity = 0;

// --- Grow Array Macro (updated for lv_obj_t *) ---
#define GROW_ARRAY(type, ptr, count, capacity) \
    if (count >= capacity) { \
        size_t old_capacity = capacity; \
        capacity = (capacity == 0) ? 8 : capacity * 2; \
        type* new_ptr = (type*)realloc(ptr, capacity * sizeof(type)); \
        if (!new_ptr) { /* Handle realloc failure */ \
            EMUL_LOG("ERROR: Failed to realloc array\n"); \
            capacity = old_capacity; /* Restore old capacity */ \
            /*return false; *//* Example: return false from function */ \
            ptr = NULL; \
        } else { \
            ptr = new_ptr; \
        } \
    }


// --- Memory Management Helpers (operate on lv_obj_t *) ---

void free_value(Value* value) {
    if (value->type == VAL_TYPE_STRING && value->data.s) {
        free(value->data.s);
        value->data.s = NULL;
    }
    if (value->type == VAL_TYPE_INT_ARRAY && value->data.int_array) {
        free(value->data.int_array);
        value->data.s = NULL; // Corrected from value->data.s = NULL
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

// Frees the contents of an object, not the object itself or children recursively
void free_emul_object_contents(lv_obj_t *obj) {
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


// Function to remove object pointer from global list
void remove_from_global_list(lv_obj_t *obj) {
    for (size_t i = 0; i < g_all_objects_count; ++i) {
        if (g_all_objects[i] == obj) {
            // Shift remaining elements down
            if (i < g_all_objects_count - 1) {
                memmove(&g_all_objects[i], &g_all_objects[i + 1], (g_all_objects_count - 1 - i) * sizeof(lv_obj_t *));
            }
            g_all_objects_count--;
            return;
        }
    }
}


bool emul_obj_add_child(lv_obj_t *parent, lv_obj_t *child) {
    if (!parent || !child) return false;
    // GROW_ARRAY operates on parent->children which is lv_obj_t**
    GROW_ARRAY(lv_obj_t *, parent->children, parent->child_count, parent->child_capacity);
    if (!parent->children) return false; // Grow array failed
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    return true;
}

void emul_obj_remove_child(lv_obj_t *parent, lv_obj_t *child_to_remove) {
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
                     (parent->child_count - 1 - found_idx) * sizeof(lv_obj_t *));
        }
        parent->child_count--;
        child_to_remove->parent = NULL; // Detach child
    }
}


Property* find_property(lv_obj_t *obj, const char* key) {
    if (!obj || !key) return NULL;
    for (size_t i = 0; i < obj->prop_count; ++i) {
        if (obj->properties[i].key && strcmp(obj->properties[i].key, key) == 0) {
            return &obj->properties[i];
        }
    }
    return NULL;
}

bool emul_obj_add_property(lv_obj_t *obj, const char* key, Value value) {
    if (!obj || !key) { free_value(&value); return false; }

    Property* existing = find_property(obj, key);
    if (existing) {
        // Update existing property
        free_value(&existing->value); // Free old value
        existing->value = value;      // Assign new value
        EMUL_LOG("Updated property '%s' on obj %p\n", key, (void*)obj);
        return true;
    } else {
        // Add new property
        GROW_ARRAY(Property, obj->properties, obj->prop_count, obj->prop_capacity);
        if (!obj->properties) { free_value(&value); return false; } // Grow array failed
        obj->properties[obj->prop_count].key = strdup(key);
        obj->properties[obj->prop_count].value = value;
        if (!obj->properties[obj->prop_count].key) { // strdup failed
            EMUL_LOG("ERROR: strdup failed for property key '%s'\n", key);
            free_value(&obj->properties[obj->prop_count].value);
            // Don't increment count if key alloc fails
            return false;
        }
        obj->prop_count++;
        EMUL_LOG("Added property '%s' to obj %p\n", key, (void*)obj);
        return true;
    }
}


StyleEntry* find_style(lv_obj_t *obj, lv_part_t part, lv_state_t state, const char* prop_name) {
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

bool emul_obj_add_style(lv_obj_t *obj, lv_part_t part, lv_state_t state, const char* prop_name, Value value) {
     if (!obj || !prop_name) { free_value(&value); return false; }

     StyleEntry* existing = find_style(obj, part, state, prop_name);
     if(existing) {
         free_value(&existing->value);
         existing->value = value;
         EMUL_LOG("Updated style '%s' [part:%u state:%u] on obj %p\n", prop_name, part, state, (void*)obj);
         return true;
     } else {
         GROW_ARRAY(StyleEntry, obj->styles, obj->style_count, obj->style_capacity);
         if (!obj->styles) { free_value(&value); return false; } // Grow array failed
         obj->styles[obj->style_count].part = part;
         obj->styles[obj->style_count].state = state;
         obj->styles[obj->style_count].prop_name = strdup(prop_name);
         obj->styles[obj->style_count].value = value;
         if (!obj->styles[obj->style_count].prop_name) {
             EMUL_LOG("ERROR: strdup failed for style prop_name '%s'\n", prop_name);
             free_value(&obj->styles[obj->style_count].value);
             // Don't increment count if prop_name alloc fails
             return false;
         }
         obj->style_count++;
         EMUL_LOG("Added style '%s' [part:%u state:%u] to obj %p\n", prop_name, part, state, (void*)obj);
         return true;
     }
}


// --- Value Creators ---
Value value_mk_string(const char* s) {
    Value v = {.type = VAL_TYPE_STRING, .data = {.s = s ? strdup(s) : NULL}};
    if (s && !v.data.s) { EMUL_LOG("ERROR: strdup failed for string value\n"); v.type = VAL_TYPE_NONE; }
    return v;
}
Value value_mk_int_array(const int32_t *array, size_t num_elems) {
    int32_t *int_array = NULL;
    if (array) {
        // Allocate space for length + elements
        int_array = malloc((num_elems + 1) * sizeof(int32_t));
        if (int_array) {
            int_array[0] = (int32_t)num_elems; // Store length at index 0
            memcpy(&int_array[1], array, num_elems * sizeof(int32_t)); // Copy elements after length
        }
    }

    Value v = {.type = VAL_TYPE_INT_ARRAY, .data = {.int_array = int_array}};
    if (array && !v.data.int_array) { EMUL_LOG("ERROR: mem dup failed for int array\n"); v.type = VAL_TYPE_NONE; }
    return v;
}
Value value_mk_int(int32_t i) { return (Value){.type = VAL_TYPE_INT, .data = {.i = i}}; }
Value value_mk_coord(lv_coord_t coord) { return (Value){.type = VAL_TYPE_COORD, .data = {.coord = coord}}; }
Value value_mk_color(lv_color_t color) { return (Value){.type = VAL_TYPE_COLOR, .data = {.color = color}}; }
Value value_mk_bool(bool b) { return (Value){.type = VAL_TYPE_BOOL, .data = {.b = b}}; }
Value value_mk_font(lv_font_t font) { return (Value){.type = VAL_TYPE_FONT, .data = {.font = font}}; }
Value value_mk_align(lv_align_t align) { return (Value){.type = VAL_TYPE_ALIGN, .data = {.align = align}}; }
Value value_mk_layout(lv_layout_t layout) { return (Value){.type = VAL_TYPE_LAYOUT, .data = {.layout = layout}}; }
Value value_mk_grid_align(lv_grid_align_t align) { return (Value){.type = VAL_TYPE_GRID_ALIGN, .data = {.grid_align = align}}; }
Value value_mk_textalign(int32_t align) { return (Value){.type = VAL_TYPE_TEXTALIGN, .data = {.text_align = align}}; }
Value value_mk_flex_align(lv_flex_align_t al) { return (Value){.type = VAL_TYPE_FLEX_ALIGN, .data = {.flex_align = al}}; }
Value value_mk_flex_flow(lv_flex_flow_t al) { return (Value){.type = VAL_TYPE_FLEX_FLOW, .data = {.flex_flow = al}}; }
Value value_mk_scale_mode(lv_scale_mode_t val) { return (Value){.type = VAL_TYPE_SCALE_MODE, .data = {.scale_mode = val}}; }

// --- Internal Object Creation Helper ---
// Creates the internal struct, returns pointer lv_obj_t *
static lv_obj_t * create_object_internal(lv_obj_t *parent, const char* type_name) {
    // Allocate the internal structure (struct _lv_obj_t)
    lv_obj_t *new_obj = (lv_obj_t *)calloc(1, sizeof(lv_obj_t)); // sizeof(lv_obj_t) is sizeof(struct _lv_obj_t)
    if (!new_obj) {
        EMUL_LOG("ERROR: Failed to allocate memory for new object\n");
        return NULL;
    }

    // Initialize internal structure members
    new_obj->id = (uintptr_t)new_obj; // Use address as ID
    new_obj->type = (char*)type_name; // Assign static type string
    new_obj->parent = parent; // Store parent pointer

    // Add to parent's children list
    if (parent) {
        if (!emul_obj_add_child(parent, new_obj)) {
            free(new_obj); // Failed to add to parent
             EMUL_LOG("ERROR: Failed to add child %p to parent %p\n", (void*)new_obj, (void*)parent);
            return NULL;
        }
    }

    // Add to global list of all objects
    GROW_ARRAY(lv_obj_t *, g_all_objects, g_all_objects_count, g_all_objects_capacity);
    if (!g_all_objects) { // Grow array failed
        if(parent) emul_obj_remove_child(parent, new_obj); // Remove from parent if added
        free(new_obj);
        return NULL;
    }
    g_all_objects[g_all_objects_count++] = new_obj;

    EMUL_LOG("Created object %p (type: %s), parent: %p\n", (void*)new_obj, type_name, (void*)parent);

    // Return the pointer to the newly created object struct
    return new_obj;
}

// --- Library Control ---

void emul_lvgl_init(void) {
    EMUL_LOG("Initializing LVGL Emulation Library\n");
    if (g_screen_obj != NULL) {
        EMUL_LOG("WARN: Already initialized. Resetting state.\n");
        emul_lvgl_reset(); // Reset if called again
    }

    // Create the screen object struct
    g_screen_obj = (lv_obj_t *)calloc(1, sizeof(lv_obj_t));
     if (!g_screen_obj) {
         EMUL_LOG("ERROR: Failed to allocate screen object\n");
         return; // Critical failure
     }
    g_screen_obj->id = (uintptr_t)g_screen_obj;
    g_screen_obj->type = "screen"; // Special type
    g_screen_obj->parent = NULL;

    // Add screen pointer to global list
    GROW_ARRAY(lv_obj_t *, g_all_objects, g_all_objects_count, g_all_objects_capacity);
    if (!g_all_objects) { // Grow array failed
        free(g_screen_obj);
        g_screen_obj = NULL;
        EMUL_LOG("ERROR: Failed to allocate global object list during init\n");
        return;
    }
    g_all_objects[g_all_objects_count++] = g_screen_obj;

    // Init font map
    g_font_map = NULL;
    g_font_map_count = 0;
    g_font_map_capacity = 0;

    EMUL_LOG("Screen object %p created.\n", (void*)g_screen_obj);
}

void emul_lvgl_reset(void) {
    EMUL_LOG("Resetting LVGL Emulation state...\n");

    // Free all objects *except* the screen object
    while(g_all_objects_count > 0) {
        lv_obj_t *obj_to_del = g_all_objects[g_all_objects_count - 1]; // Get pointer
         if (obj_to_del == g_screen_obj) {
            // Clear screen contents but keep the screen obj itself
             free_emul_object_contents(g_screen_obj);
             g_all_objects_count--; // Remove from list temporarily
         } else {
             // Call lv_obj_del with the pointer
             // lv_obj_del will remove it from the list, so just pop
             lv_obj_del(obj_to_del);
             // Check if lv_obj_del actually removed it (it should have)
             if (g_all_objects_count > 0 && g_all_objects[g_all_objects_count - 1] == obj_to_del) {
                 EMUL_LOG("ERROR: lv_obj_del failed to remove object %p from global list during reset\n", (void*)obj_to_del);
                 g_all_objects_count--; // Force remove if del failed internally
             }
         }
    }

    // Re-add screen pointer to the global list
    if(g_screen_obj && g_all_objects_count == 0) {
         GROW_ARRAY(lv_obj_t *, g_all_objects, g_all_objects_count, g_all_objects_capacity);
         if (!g_all_objects) { // Grow array failed
             EMUL_LOG("ERROR: Failed to re-grow global object list after reset\n");
             free(g_screen_obj); // Cannot recover screen object if list fails
             g_screen_obj = NULL;
         } else {
             g_all_objects[g_all_objects_count++] = g_screen_obj;
         }
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

    EMUL_LOG("Reset complete. Screen object %p remains.\n", (void*)g_screen_obj);
}

void emul_lvgl_deinit(void) {
    EMUL_LOG("Deinitializing LVGL Emulation Library...\n");
    emul_lvgl_reset(); // Clear all dynamic objects first

    // Free the screen object struct itself now
    if (g_screen_obj) {
        free(g_screen_obj); // Free the struct pointed to by g_screen_obj
        g_screen_obj = NULL;
    }
    // Free the global object list array (array of pointers)
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
            if (g_font_map[i].name == NULL || strcmp(g_font_map[i].name, name) != 0) {
                free(g_font_map[i].name);
                g_font_map[i].name = strdup(name);
                 if (!g_font_map[i].name) {
                     EMUL_LOG("ERROR: strdup failed updating font name '%s'\n", name);
                     // Should maybe remove the entry? Or leave old name? Let's leave old.
                     g_font_map[i].name = NULL; // Mark as failed
                 }
            }
            return;
        }
    }

    // Add new entry
    GROW_ARRAY(FontMapEntry, g_font_map, g_font_map_count, g_font_map_capacity);
    if (!g_font_map) return; // Grow array failed
    g_font_map[g_font_map_count].ptr = font_ptr;
    g_font_map[g_font_map_count].name = strdup(name);
     if (!g_font_map[g_font_map_count].name) {
         EMUL_LOG("ERROR: strdup failed for font name '%s'\n", name);
         // Don't increment count
         return;
     }
    g_font_map_count++;
    EMUL_LOG("Registered font %p as '%s'\n", font_ptr, name);
}


// --- LVGL API Implementation (using lv_obj_t *) ---

// ** Object Creation **
lv_obj_t * lv_obj_create(lv_obj_t *parent) { return create_object_internal(parent, "obj"); }
lv_obj_t * lv_label_create(lv_obj_t *parent) { return create_object_internal(parent, "label"); }
lv_obj_t * lv_btn_create(lv_obj_t *parent) { return create_object_internal(parent, "btn"); }
lv_obj_t * lv_slider_create(lv_obj_t *parent) { return create_object_internal(parent, "slider"); }
lv_obj_t * lv_bar_create(lv_obj_t *parent) { return create_object_internal(parent, "bar"); }
lv_obj_t * lv_scale_create(lv_obj_t *parent) { return create_object_internal(parent, "scale"); }


// ** Object Deletion/Cleanup **
void lv_obj_del(lv_obj_t *obj) {
    // Check incoming pointer and prevent deleting screen
    if (!obj || obj == g_screen_obj) {
        EMUL_LOG("WARN: Attempt to delete NULL or screen object (%p)\n", (void*)obj);
        return;
    }
    EMUL_LOG("Deleting object %p (type: %s)...\n", (void*)obj, obj->type);

    // 0. Check if object is still in the global list (sanity check)
    bool found_in_global = false;
    for (size_t i = 0; i < g_all_objects_count; ++i) {
        if (g_all_objects[i] == obj) {
            found_in_global = true;
            break;
        }
    }
    if (!found_in_global) {
        EMUL_LOG("WARN: Attempting to delete object %p which is not in the global list (already deleted?)\n", (void*)obj);
        // Still try to remove from parent if it has one, but don't free memory again
        if (obj->parent) {
            emul_obj_remove_child(obj->parent, obj);
        }
        return;
    }


    // 1. Remove from parent's children list (uses internal parent pointer)
    if (obj->parent) {
        emul_obj_remove_child(obj->parent, obj);
    }

    // 2. Remove pointer from global list
    remove_from_global_list(obj);

     // 3. Recursively delete children by calling lv_obj_del on their pointers
     size_t num_children = obj->child_count;
     lv_obj_t **children_copy = NULL; // Array of pointers
     if(num_children > 0) {
        children_copy = malloc(num_children * sizeof(lv_obj_t *));
        if(children_copy) memcpy(children_copy, obj->children, num_children * sizeof(lv_obj_t *));
        else { EMUL_LOG("Error allocating children copy for delete\n");}
     }

     // 4. Free current object's direct contents and the struct itself
      free_emul_object_contents(obj);
      free(obj); // Free the struct pointed to by obj

     // 5. Now recursively delete children that were detached
     if(children_copy) {
         for(size_t i = 0; i < num_children; ++i) {
             lv_obj_t *child_ptr = children_copy[i];
             // No need to check global list here, just call del recursively
             // lv_obj_del itself handles double-deletion checks
             lv_obj_del(child_ptr); // Recursive call with the pointer
         }
         free(children_copy);
     }
     EMUL_LOG("Deletion complete for obj %p.\n", (void*)obj); // Log the original pointer value
}

void lv_obj_clean(lv_obj_t *obj) {
     if (!obj) return;
     EMUL_LOG("Cleaning children of object %p\n", (void*)obj);

     // Make a copy of children pointers
     size_t num_children = obj->child_count;
     if (num_children == 0) return;
     lv_obj_t **children_to_delete = malloc(num_children * sizeof(lv_obj_t *));
     if (!children_to_delete) { EMUL_LOG("ERROR: Failed alloc for clean\n"); return; }
     memcpy(children_to_delete, obj->children, num_children * sizeof(lv_obj_t *));

     // Delete each child using lv_obj_del with the pointer
     // This will also modify obj->children and obj->child_count via emul_obj_remove_child
     for (size_t i = 0; i < num_children; ++i) {
         lv_obj_del(children_to_delete[i]);
     }
     free(children_to_delete);

     // After deletion, the child count should be 0
     if (obj->child_count != 0) {
        EMUL_LOG("WARN: Child count not zero after clean (%zu). Forcibly clearing.\n", obj->child_count);
        obj->child_count = 0;
        free(obj->children); // Free the potentially non-null pointer
        obj->children = NULL;
        obj->child_capacity = 0;
     }
     EMUL_LOG("Cleaning complete for obj %p\n", (void*)obj);
}


// ** Screen Access ** Returns pointer
lv_obj_t * lv_screen_active(void) {
    if (!g_screen_obj) {
        EMUL_LOG("WARN: lv_screen_active called before emul_lvgl_init.\n");
        // Maybe initialize here? Or just return NULL. Let's return NULL.
        return NULL;
    }
    return g_screen_obj; // Return the pointer stored globally
}


// ** Basic Property Setters ** Take pointer
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w) { if(obj) emul_obj_add_property(obj, "width", value_mk_coord(w)); }
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h) { if(obj) emul_obj_add_property(obj, "height", value_mk_coord(h)); }
void lv_obj_set_min_width(lv_obj_t *obj, lv_coord_t w) { if(obj) emul_obj_add_property(obj, "min_width", value_mk_coord(w)); }
void lv_obj_set_max_width(lv_obj_t *obj, lv_coord_t w) { if(obj) emul_obj_add_property(obj, "max_width", value_mk_coord(w)); }
void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h) { lv_obj_set_width(obj, w); lv_obj_set_height(obj, h); }
void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y) { if(obj) { emul_obj_add_property(obj, "x", value_mk_coord(x)); emul_obj_add_property(obj, "y", value_mk_coord(y)); } }
void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x) { if(obj) emul_obj_add_property(obj, "x", value_mk_coord(x)); }
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y) { if(obj) emul_obj_add_property(obj, "y", value_mk_coord(y)); }
void lv_obj_set_align(lv_obj_t *obj, lv_align_t align) { if(obj) emul_obj_add_property(obj, "align", value_mk_align(align)); }
void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) { if(obj) { lv_obj_set_align(obj, align); lv_obj_set_x(obj, x_ofs); lv_obj_set_y(obj, y_ofs); } }

void lv_obj_update_layout(lv_obj_t *obj) { /* No-op in emulation for now */ }
void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent) {
    if (!obj) return;
    // Remove from old parent
    if (obj->parent) {
        emul_obj_remove_child(obj->parent, obj);
    }
    // Add to new parent
    if (parent) {
        emul_obj_add_child(parent, obj); // This sets obj->parent = parent internally
    } else {
        obj->parent = NULL;
    }
    // Log this change as a property? Or assume the hierarchy is the source of truth?
    // Let's rely on hierarchy for now.
    EMUL_LOG("Set parent of obj %p to %p\n", (void*)obj, (void*)parent);
}

void lv_obj_set_layout(lv_obj_t *obj, lv_layout_t layout) { if(obj) emul_obj_add_property(obj, "layout", value_mk_layout(layout)); }

// ** Flags ** Take pointer
void lv_obj_add_flag(lv_obj_t *obj, uint32_t f) {
    if (!obj) return;
    // Store flags as a single integer property for now
    Property* prop = find_property(obj, "flags");
    uint32_t current_flags = 0;
    if (prop && prop->value.type == VAL_TYPE_INT) {
        current_flags = (uint32_t)prop->value.data.i;
    }
    current_flags |= f;
    emul_obj_add_property(obj, "flags", value_mk_int((int32_t)current_flags));
}

void lv_obj_clear_flag(lv_obj_t *obj, uint32_t f) {
    if (!obj) return;
    Property* prop = find_property(obj, "flags");
    uint32_t current_flags = 0;
    if (prop && prop->value.type == VAL_TYPE_INT) {
        current_flags = (uint32_t)prop->value.data.i;
    }
    current_flags &= ~f;
    emul_obj_add_property(obj, "flags", value_mk_int((int32_t)current_flags));
}


// ** Label Specific ** Take pointer
void lv_label_set_text(lv_obj_t *obj, const char * text) { if(obj) emul_obj_add_property(obj, "text", value_mk_string(text)); }
void lv_label_set_text_fmt(lv_obj_t *obj, const char * fmt, ...) {
    if (!obj) return;
    char *buf = NULL;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args); // Get required length
    va_end(args);

    if (len < 0) {
        EMUL_LOG("ERROR: vsnprintf failed to determine length for label format.\n");
        return;
    }

    buf = malloc(len + 1);
    if (!buf) {
        EMUL_LOG("ERROR: malloc failed for label format buffer.\n");
        return;
    }

    va_start(args, fmt);
    vsnprintf(buf, len + 1, fmt, args);
    va_end(args);

    lv_label_set_text(obj, buf); // Calls the pointer version
    free(buf);
}

// ** Slider Specific ** Take pointer
void lv_slider_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim) {
    if (!obj) return;
    (void)anim; // Ignore animation parameter
    emul_obj_add_property(obj, "value", value_mk_int(value));
}
void lv_slider_set_range(lv_obj_t *obj, int32_t min, int32_t max) {
    if (!obj) return;
    emul_obj_add_property(obj, "range_min", value_mk_int(min));
    emul_obj_add_property(obj, "range_max", value_mk_int(max));
}

// Bar Specific
void lv_bar_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim) {
    if (!obj) return;
    (void)anim; // Ignore animation parameter
    emul_obj_add_property(obj, "value", value_mk_int(value));
}
void lv_bar_set_range(lv_obj_t *obj, int32_t min, int32_t max) {
    if (!obj) return;
    emul_obj_add_property(obj, "range_min", value_mk_int(min));
    emul_obj_add_property(obj, "range_max", value_mk_int(max));
}

// Scale Specific
void lv_scale_set_range(lv_obj_t *obj, int32_t min, int32_t max) {
    if (!obj) return;
    // Use different keys to avoid conflict with slider/bar range
    emul_obj_add_property(obj, "scale_range_min", value_mk_int(min));
    emul_obj_add_property(obj, "scale_range_max", value_mk_int(max));
}
void lv_scale_set_major_tick_every(lv_obj_t * obj, uint32_t nth) {
    if (!obj) return;
    emul_obj_add_property(obj, "scale_major_tick_every", value_mk_int((int32_t)nth)); // Store as int32 for simplicity
}

void lv_scale_set_mode(lv_obj_t * obj, lv_scale_mode_t mode) {
    if (!obj) return;
    emul_obj_add_property(obj, "scale_mode", value_mk_scale_mode(mode));
}

void lv_obj_set_grid_dsc_array(lv_obj_t *obj, const int32_t col_dsc[], const int32_t row_dsc[]) {
    if (!obj) return;
    size_t rowlen = 0;
    if (row_dsc) {
        while(row_dsc[rowlen] != LV_GRID_TEMPLATE_LAST) rowlen++;
    }
    size_t collen = 0;
    if (col_dsc) {
        while(col_dsc[collen] != LV_GRID_TEMPLATE_LAST) collen++;
    }
    // Add +1 to store LV_GRID_TEMPLATE_LAST if needed, but value_mk handles length internally
    emul_obj_add_property(obj, "grid_dsc_array_row_dsc", value_mk_int_array(row_dsc, rowlen));
    emul_obj_add_property(obj, "grid_dsc_array_col_dsc", value_mk_int_array(col_dsc, collen));
}

void lv_obj_set_grid_align(lv_obj_t *obj, lv_grid_align_t column_align, lv_grid_align_t row_align) {
    if (!obj) return;
    emul_obj_add_property(obj, "grid_column_align", value_mk_grid_align(column_align));
    emul_obj_add_property(obj, "grid_row_align", value_mk_grid_align(row_align));
}

void lv_obj_set_grid_cell(lv_obj_t *obj, lv_grid_align_t column_align, int32_t col_pos, int32_t col_span, lv_grid_align_t row_align, int32_t row_pos, int32_t row_span) {
    if (!obj) return;
    emul_obj_add_property(obj, "grid_cell_column_align", value_mk_grid_align(column_align));
    emul_obj_add_property(obj, "grid_cell_col_pos", value_mk_int(col_pos));
    emul_obj_add_property(obj, "grid_cell_col_span", value_mk_int(col_span));
    emul_obj_add_property(obj, "grid_cell_row_align", value_mk_grid_align(row_align));
    emul_obj_add_property(obj, "grid_cell_row_pos", value_mk_int(row_pos));
    emul_obj_add_property(obj, "grid_cell_row_span", value_mk_int(row_span));
}

void lv_obj_set_flex_flow(lv_obj_t *obj, lv_flex_flow_t flow) { if(obj) emul_obj_add_property(obj, "flex_flow", value_mk_flex_flow(flow)); }
void lv_obj_set_flex_align(lv_obj_t *obj, lv_flex_align_t main_place, lv_flex_align_t cross_place, lv_flex_align_t track_cross_place) {
     if(!obj) return;
     emul_obj_add_property(obj, "flex_align_main_place", value_mk_flex_align(main_place));
     emul_obj_add_property(obj, "flex_align_cross_place", value_mk_flex_align(cross_place));
     emul_obj_add_property(obj, "flex_align_track_cross_place", value_mk_flex_align(track_cross_place));
}
void lv_obj_set_flex_grow(lv_obj_t *obj, uint8_t grow) {
    if(!obj) return;
    emul_obj_add_property(obj, "flex_grow", value_mk_int(grow));
}


// --- Style Property Setters ---

// Macro to simplify adding styles, operates on lv_obj_t *
#define ADD_STYLE(obj_ptr, selector, prop_name_str, value_func) \
    do { \
        if (!(obj_ptr)) break; /* Check the pointer */ \
        lv_part_t part = selector & LV_PART_ANY; /* Use mask */ \
        lv_state_t state = selector & LV_STATE_ANY; /* Use mask */ \
        emul_obj_add_style(obj_ptr, part, state, prop_name_str, value_func); \
    } while(0)

void lv_obj_set_style_flex_flow(lv_obj_t *obj, lv_flex_flow_t value, lv_style_selector_t selector) {
    ADD_STYLE(obj, selector, "flex_flow", value_mk_flex_flow(value));
}

// Background
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "bg_color", value_mk_color(value)); }
void lv_obj_set_style_bg_opa(lv_obj_t *obj, uint8_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "bg_opa", value_mk_int(value)); }
void lv_obj_set_style_radius(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "radius", value_mk_coord(value)); }

// Border
void lv_obj_set_style_border_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "border_width", value_mk_coord(value)); }
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "border_color", value_mk_color(value)); }
void lv_obj_set_style_border_opa(lv_obj_t *obj, uint8_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "border_opa", value_mk_int(value)); }

// Pad
void lv_obj_set_style_pad_all(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "pad_all", value_mk_int(value)); }
void lv_obj_set_style_pad_top(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "pad_top", value_mk_int(value)); }
void lv_obj_set_style_pad_left(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "pad_left", value_mk_int(value)); }
void lv_obj_set_style_pad_right(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "pad_right", value_mk_int(value)); }
void lv_obj_set_style_pad_bottom(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "pad_bottom", value_mk_int(value)); }

// Margin
void lv_obj_set_style_margin_all(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "margin_all", value_mk_int(value)); }
void lv_obj_set_style_margin_top(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "margin_top", value_mk_int(value)); }
void lv_obj_set_style_margin_left(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "margin_left", value_mk_int(value)); }
void lv_obj_set_style_margin_right(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "margin_right", value_mk_int(value)); }
void lv_obj_set_style_margin_bottom(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "margin_bottom", value_mk_int(value)); }

// Outline
void lv_obj_set_style_outline_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "outline_width", value_mk_coord(value)); }
void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "outline_color", value_mk_color(value)); }
void lv_obj_set_style_outline_opa(lv_obj_t *obj, uint8_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "outline_opa", value_mk_int(value)); }

// Text
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "text_color", value_mk_color(value)); }
void lv_obj_set_style_text_font(lv_obj_t *obj, lv_font_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "text_font", value_mk_font(value)); }
void lv_obj_set_style_text_align(lv_obj_t *obj, int32_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "text_align", value_mk_textalign(value)); }


// Size (for specific parts)
void lv_obj_set_style_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "width", value_mk_coord(value)); }
void lv_obj_set_style_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "height", value_mk_coord(value)); }

// Size Constraints
void lv_obj_set_style_min_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "min_height", value_mk_coord(value)); }
void lv_obj_set_style_min_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "min_width", value_mk_coord(value)); }
void lv_obj_set_style_max_height(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "max_height", value_mk_coord(value)); }
void lv_obj_set_style_max_width(lv_obj_t *obj, lv_coord_t value, lv_style_selector_t selector) { ADD_STYLE(obj, selector, "max_width", value_mk_coord(value)); }

// --- Helper Value Creators ---
lv_color_t lv_color_hex(uint32_t c) { return lv_color_make((uint8_t)((c >> 16) & 0xFF), (uint8_t)((c >> 8) & 0xFF), (uint8_t)(c & 0xFF)); }
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b) { return (lv_color_t){.r = r, .g = g, .b = b}; }
lv_color_t lv_color_white(void) { return lv_color_make(255, 255, 255); }
int32_t lv_pct(int32_t v) { return LV_PCT(v); }


// --- JSON Generation ---

// --- Forward Declarations for Converters ---
const char* layout_to_string(lv_layout_t layout);
const char* grid_align_to_string(lv_grid_align_t align);
const char* flex_align_to_string(lv_flex_align_t align);
const char* flex_flow_to_string(lv_flex_flow_t flow);
const char* scale_mode_to_string(lv_scale_mode_t mode);
cJSON* int_array_to_json_array(const int32_t* arr);

// Forward declare recursive builder (takes lv_obj_t *)
cJSON* build_json_recursive(lv_obj_t *obj);

char* emul_lvgl_get_json(lv_obj_t *root_obj) { // Takes pointer
    if (!root_obj) {
        EMUL_LOG("ERROR: emul_lvgl_get_json called with NULL root object pointer\n");
        return NULL;
    }

    cJSON *root_container = cJSON_CreateObject();
     if (!root_container) { EMUL_LOG("ERROR: Failed to create root cJSON object\n"); return NULL; }

    // Call recursive builder with the root pointer
    cJSON* ui_tree_json = build_json_recursive(root_obj);
    if (!ui_tree_json) { cJSON_Delete(root_container); EMUL_LOG("ERROR: Failed to build JSON structure recursively\n"); return NULL; }

    cJSON_AddItemToObject(root_container, "root", ui_tree_json);
    char* json_string = JSON_print(root_container); // Use configured print function
    cJSON_Delete(root_container);

    if (!json_string) { EMUL_LOG("ERROR: cJSON_Print failed\n"); }
    else { EMUL_LOG("Generated JSON successfully for root %p\n", (void*)root_obj); }
    return json_string;
}

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
        default: {
            // Handle custom parts if needed, or return generic string
            // For now, just indicate unknown part with value
            static char unknown_buf[32];
            snprintf(unknown_buf, sizeof(unknown_buf), "unknown_part_%u", part);
            return unknown_buf;
        }
    }
}

const char* state_to_string(lv_state_t state) {
     // Only handle single primary states for simplicity in JSON keys
     // Check most specific/override states first potentially
     if (state == LV_STATE_DEFAULT) return "default"; // Explicit check
     if (state & LV_STATE_DISABLED) return "disabled";
     if (state & LV_STATE_PRESSED) return "pressed"; // Pressed often overrides others visually
     if (state & LV_STATE_FOCUSED) return "focused"; // Includes FOCUS_KEY
     if (state & LV_STATE_HOVERED) return "hovered";
     if (state & LV_STATE_CHECKED) return "checked";
     if (state & LV_STATE_EDITED) return "edited";
     if (state & LV_STATE_SCROLLED) return "scrolled";
     // If multiple bits are set, we currently only return one.
     // A more complex system could generate combined keys like "focused_checked".
     // For now, this priority order is used.
     static char combined_buf[32];
     snprintf(combined_buf, sizeof(combined_buf), "state_%u", state);
     return combined_buf; // Fallback for combined or unknown states
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
         // Assumes standard LV_TEXT_ALIGN_... values (defined in lv_draw_label.h usually)
         case 0: return "auto";   // LV_TEXT_ALIGN_AUTO
         case 1: return "left";   // LV_TEXT_ALIGN_LEFT
         case 2: return "center"; // LV_TEXT_ALIGN_CENTER
         case 3: return "right";  // LV_TEXT_ALIGN_RIGHT
         default: return "auto";
     }
 }

void color_to_hex_string(lv_color_t color, char* buf, size_t buf_size) {
    // Simple hex conversion
    snprintf(buf, buf_size, "#%02X%02X%02X", color.r, color.g, color.b);
}

const char* font_ptr_to_name(lv_font_t font_ptr) {
    if (!font_ptr) return "default"; // Or NULL? Let's return "default"
    for (size_t i = 0; i < g_font_map_count; ++i) {
        if (g_font_map[i].ptr == font_ptr) {
            return g_font_map[i].name ? g_font_map[i].name : "unnamed_font";
        }
    }
    EMUL_LOG("WARN: Font pointer %p not registered. Using 'unknown_font'.\n", font_ptr);
    return "unknown_font"; // Fallback if not registered
}

void coord_to_string(lv_coord_t coord, char* buf, size_t buf_size) {
    if (LV_COORD_IS_PCT(coord)) {
        snprintf(buf, buf_size, "%d%%", LV_COORD_GET_PCT(coord));
    } else if (coord == LV_SIZE_CONTENT) {
        snprintf(buf, buf_size, "content");
    } else {
        snprintf(buf, buf_size, "%d", (int)coord); // Cast to int for printf
    }
}

const char* layout_to_string(lv_layout_t layout) {
    switch(layout) {
        case LV_LAYOUT_NONE: return "none";
        case LV_LAYOUT_FLEX: return "flex";
        case LV_LAYOUT_GRID: return "grid";
        default: return "unknown";
    }
}

const char* grid_align_to_string(lv_grid_align_t align) {
    switch(align) {
        case LV_GRID_ALIGN_START: return "start";
        case LV_GRID_ALIGN_CENTER: return "center";
        case LV_GRID_ALIGN_END: return "end";
        case LV_GRID_ALIGN_STRETCH: return "stretch";
        case LV_GRID_ALIGN_SPACE_EVENLY: return "space_evenly";
        case LV_GRID_ALIGN_SPACE_AROUND: return "space_around";
        case LV_GRID_ALIGN_SPACE_BETWEEN: return "space_between";
        default: return "unknown";
    }
}

const char* flex_align_to_string(lv_flex_align_t align) {
    switch(align) {
        case LV_FLEX_ALIGN_START: return "start";
        case LV_FLEX_ALIGN_END: return "end";
        case LV_FLEX_ALIGN_CENTER: return "center";
        case LV_FLEX_ALIGN_SPACE_EVENLY: return "space_evenly";
        case LV_FLEX_ALIGN_SPACE_AROUND: return "space_around";
        case LV_FLEX_ALIGN_SPACE_BETWEEN: return "space_between";
        default: return "unknown";
    }
}

const char* flex_flow_to_string(lv_flex_flow_t flow) {
    switch(flow) {
        case LV_FLEX_FLOW_ROW: return "row";
        case LV_FLEX_FLOW_COLUMN: return "column";
        case LV_FLEX_FLOW_ROW_WRAP: return "row_wrap";
        case LV_FLEX_FLOW_ROW_REVERSE: return "row_reverse";
        case LV_FLEX_FLOW_ROW_WRAP_REVERSE: return "row_wrap_reverse";
        case LV_FLEX_FLOW_COLUMN_WRAP: return "column_wrap";
        case LV_FLEX_FLOW_COLUMN_REVERSE: return "column_reverse";
        case LV_FLEX_FLOW_COLUMN_WRAP_REVERSE: return "column_wrap_reverse";
        default: return "unknown";
    }
}

const char* scale_mode_to_string(lv_scale_mode_t mode) {
     switch(mode) {
         case LV_SCALE_MODE_HORIZONTAL_TOP: return "horizontal_top";
         case LV_SCALE_MODE_HORIZONTAL_BOTTOM: return "horizontal_bottom";
         case LV_SCALE_MODE_VERTICAL_LEFT: return "vertical_left";
         case LV_SCALE_MODE_VERTICAL_RIGHT: return "vertical_right";
         case LV_SCALE_MODE_ROUND_INNER: return "round_inner";
         case LV_SCALE_MODE_ROUND_OUTER: return "round_outer";
         default: return "unknown";
     }
}


cJSON* int_array_to_json_array(const int32_t* arr) {
    if (!arr) return NULL;
    cJSON* json_arr = cJSON_CreateArray();
    if (!json_arr) return NULL;

    int32_t len = arr[0]; // Length stored at index 0
    for (int32_t i = 0; i < len; i++) {
        cJSON_AddItemToArray(json_arr, cJSON_CreateNumber(arr[i + 1])); // Access elements starting from index 1
    }
    return json_arr;
}


// --- Recursive JSON Builder (takes lv_obj_t *) ---
cJSON* build_json_recursive(lv_obj_t *obj) { // Takes pointer
    if (!obj) return NULL;

    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) return NULL;

    // Type
    cJSON_AddStringToObject(json_obj, "type", obj->type);

    // Properties
    if (obj->prop_count > 0) {
        cJSON* props_json = cJSON_CreateObject();
        if (!props_json) { cJSON_Delete(json_obj); return NULL; }
        cJSON_AddItemToObject(json_obj, "properties", props_json);
        char coord_buf[20]; // Increased size for "content"
        char color_buf[10];
        for (size_t i = 0; i < obj->prop_count; ++i) {
            Property* prop = &obj->properties[i];
            if (!prop->key) continue; // Skip if key allocation failed

            switch (prop->value.type) {
                case VAL_TYPE_STRING:
                    cJSON_AddStringToObject(props_json, prop->key, prop->value.data.s ? prop->value.data.s : "");
                    break;
                case VAL_TYPE_INT:
                    cJSON_AddNumberToObject(props_json, prop->key, prop->value.data.i);
                    break;
                case VAL_TYPE_COORD:
                    coord_to_string(prop->value.data.coord, coord_buf, sizeof(coord_buf));
                    // Store percentages and "content" as strings, numbers as numbers
                    if (strchr(coord_buf, '%') || strcmp(coord_buf, "content") == 0)
                        cJSON_AddStringToObject(props_json, prop->key, coord_buf);
                    else
                        cJSON_AddNumberToObject(props_json, prop->key, prop->value.data.coord);
                    break;
                case VAL_TYPE_BOOL:
                    cJSON_AddBoolToObject(props_json, prop->key, prop->value.data.b);
                    break;
                case VAL_TYPE_ALIGN:
                    cJSON_AddStringToObject(props_json, prop->key, align_to_string(prop->value.data.align));
                    break;
                case VAL_TYPE_LAYOUT:
                    cJSON_AddStringToObject(props_json, prop->key, layout_to_string(prop->value.data.layout));
                    break;
                case VAL_TYPE_GRID_ALIGN:
                    cJSON_AddStringToObject(props_json, prop->key, grid_align_to_string(prop->value.data.grid_align));
                    break;
                case VAL_TYPE_INT_ARRAY: {
                    cJSON* arr_json = int_array_to_json_array(prop->value.data.int_array);
                    if (arr_json) cJSON_AddItemToObject(props_json, prop->key, arr_json);
                    break;
                }
                case VAL_TYPE_FLEX_ALIGN:
                    cJSON_AddStringToObject(props_json, prop->key, flex_align_to_string(prop->value.data.flex_align));
                    break;
                case VAL_TYPE_FLEX_FLOW:
                     cJSON_AddStringToObject(props_json, prop->key, flex_flow_to_string(prop->value.data.flex_flow));
                     break;
                case VAL_TYPE_SCALE_MODE:
                     cJSON_AddStringToObject(props_json, prop->key, scale_mode_to_string(prop->value.data.scale_mode));
                     break;
                // Add cases for other VAL_TYPEs if needed
                case VAL_TYPE_NONE: // Explicitly ignore NONE type
                case VAL_TYPE_COLOR: // Not typically used as direct properties
                case VAL_TYPE_FONT: // Not typically used as direct properties
                case VAL_TYPE_TEXTALIGN: // Text align is a style property
                default:
                     EMUL_LOG("WARN: Skipping unknown/unhandled property type %d for key '%s'\n", prop->value.type, prop->key);
                     break;
            }
        }
    }

    // Styles
    if (obj->style_count > 0) {
        cJSON* styles_json = cJSON_CreateObject();
         if (!styles_json) { cJSON_Delete(json_obj); return NULL; }
        cJSON_AddItemToObject(json_obj, "styles", styles_json);

        for (size_t i = 0; i < obj->style_count; ++i) {
            StyleEntry* entry = &obj->styles[i];
            if (!entry->prop_name) continue; // Skip if prop_name allocation failed

            const char* part_str = part_to_string(entry->part);
            const char* state_str = state_to_string(entry->state);

            cJSON* part_obj = cJSON_GetObjectItemCaseSensitive(styles_json, part_str);
            if (!part_obj) {
                part_obj = cJSON_CreateObject();
                if (!part_obj) { /* error handling */ continue; }
                cJSON_AddItemToObject(styles_json, part_str, part_obj);
             }

            cJSON* state_obj = cJSON_GetObjectItemCaseSensitive(part_obj, state_str);
            if (!state_obj) {
                 state_obj = cJSON_CreateObject();
                 if (!state_obj) { /* error handling */ continue; }
                 cJSON_AddItemToObject(part_obj, state_str, state_obj);
             }

            char coord_buf[20]; // Buffer for coord strings
            char color_buf[10]; // Buffer for color strings
            switch (entry->value.type) {
                case VAL_TYPE_COLOR:
                    color_to_hex_string(entry->value.data.color, color_buf, sizeof(color_buf));
                    cJSON_AddStringToObject(state_obj, entry->prop_name, color_buf);
                    break;
                case VAL_TYPE_COORD:
                    coord_to_string(entry->value.data.coord, coord_buf, sizeof(coord_buf));
                    // Store percentages and "content" as strings, numbers as numbers
                     if (strchr(coord_buf, '%') || strcmp(coord_buf, "content") == 0)
                        cJSON_AddStringToObject(state_obj, entry->prop_name, coord_buf);
                     else
                        cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.coord);
                    break;
                case VAL_TYPE_INT: // Often used for opa, border_width, padding etc.
                    cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.i);
                    break;
                case VAL_TYPE_FONT:
                    cJSON_AddStringToObject(state_obj, entry->prop_name, font_ptr_to_name(entry->value.data.font));
                    break;
                 case VAL_TYPE_TEXTALIGN:
                     cJSON_AddStringToObject(state_obj, entry->prop_name, text_align_to_string(entry->value.data.text_align));
                     break;
                 case VAL_TYPE_FLEX_FLOW: // Style property
                     cJSON_AddStringToObject(state_obj, entry->prop_name, flex_flow_to_string(entry->value.data.flex_flow));
                     break;
                 // Add cases for other VAL_TYPEs if needed for styles
                 case VAL_TYPE_NONE: // Explicitly ignore NONE type
                 case VAL_TYPE_STRING: // Not typically used for styles
                 case VAL_TYPE_BOOL:   // Not typically used for styles
                 case VAL_TYPE_ALIGN: // Not typically used for styles (basic align is property)
                 case VAL_TYPE_GRID_ALIGN: // Not a style property
                 case VAL_TYPE_LAYOUT: // Not a style property
                 case VAL_TYPE_FLEX_ALIGN: // Not a style property
                 case VAL_TYPE_SCALE_MODE: // Not a style property
                 case VAL_TYPE_INT_ARRAY: // Not a style property
                 default:
                      EMUL_LOG("WARN: Skipping unknown/unhandled style type %d for key '%s'\n", entry->value.type, entry->prop_name);
                      break;
            }
        }
    }

    // Children
    if (obj->child_count > 0) {
        cJSON* children_json = cJSON_CreateArray();
         if (!children_json) { cJSON_Delete(json_obj); return NULL; }
        cJSON_AddItemToObject(json_obj, "children", children_json);

        for (size_t i = 0; i < obj->child_count; ++i) {
            // Recursively call with the child pointer
            cJSON* child_json_obj = build_json_recursive(obj->children[i]);
            if (child_json_obj) {
                cJSON_AddItemToArray(children_json, child_json_obj);
            } else {
                EMUL_LOG("WARN: Failed to build JSON for child object %p\n", (void*)obj->children[i]);
            }
        }
    }

    return json_obj;
}

lv_obj_t *_maximize_client_area(lv_obj_t *obj) {
    lv_obj_set_style_margin_all(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);

    return obj;
}

lv_obj_t *_fill_parent(lv_obj_t *obj) {
    lv_obj_set_style_margin_all(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_size(obj, lv_pct(100), lv_pct(100));

    return obj;
}