
#include "emul_lvgl_internal.h" // Includes public header first
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h> // For PRIuPTR

// --- Debug Logging ---
// Simple logger (replace with a more robust solution if needed)
#ifdef EMUL_LVGL_DEBUG
#define EMUL_LOG(fmt, ...) printf("[EMUL_LVGL] " fmt, ##__VA_ARGS__)
#else
#define EMUL_LOG(fmt, ...) do {} while(0)
#endif

// --- cJSON Configuration ---
#ifndef JSON_print
#  ifdef EMUL_LVGL_PRETTY_JSON // Define this macro to enable pretty printing
#    define JSON_print cJSON_Print
#  else
#    define JSON_print cJSON_PrintUnformatted
#  endif
#endif

// --- Global State ---
lv_obj_t * g_screen_obj = NULL;
lv_obj_t ** g_all_objects = NULL;
size_t g_all_objects_count = 0;
size_t g_all_objects_capacity = 0;

FontMapEntry* g_font_map = NULL;
size_t g_font_map_count = 0;
size_t g_font_map_capacity = 0;

// --- Grow Array Macro ---
#define GROW_ARRAY(type, ptr, count, capacity) \
    if ((count) >= (capacity)) { \
        size_t old_capacity = (capacity); \
        (capacity) = ((capacity) == 0) ? 8 : (capacity) * 2; \
        type* new_ptr = (type*)realloc(ptr, (capacity) * sizeof(type)); \
        if (!new_ptr) { \
            EMUL_LOG("ERROR: Failed to realloc array for '%s'\n", #ptr); \
            (capacity) = old_capacity; \
            ptr = NULL; /* Indicate failure */ \
        } else { \
            ptr = new_ptr; \
            /* Initialize the new memory block if needed (especially for pointers) */ \
            /* memset((char*)ptr + old_capacity * sizeof(type), 0, (capacity - old_capacity) * sizeof(type)); */ \
        } \
    }

// --- Memory Management Helpers ---

void free_value(Value* value) {
    if (!value) return;
    if (value->type == VAL_TYPE_STRING && value->data.s) {
        free(value->data.s);
    } else if (value->type == VAL_TYPE_INT_ARRAY && value->data.int_array) {
        free(value->data.int_array);
    }
    // Add other types that need freeing (e.g., VAL_TYPE_PTR if owned)
    value->data.s = NULL; // Clear the first member as a safety measure
    value->type = VAL_TYPE_NONE;
}

void free_property(Property* prop) {
    if (!prop) return;
    if (prop->key) free(prop->key);
    free_value(&prop->value);
    prop->key = NULL;
}

void free_style_entry(StyleEntry* entry) {
     if (!entry) return;
    if (entry->prop_name) free(entry->prop_name);
    free_value(&entry->value);
    entry->prop_name = NULL;
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
    if (!g_all_objects || g_all_objects_count == 0) return;
    for (size_t i = 0; i < g_all_objects_count; ++i) {
        if (g_all_objects[i] == obj) {
            // Shift remaining elements down
            if (i < g_all_objects_count - 1) {
                memmove(&g_all_objects[i], &g_all_objects[i + 1], (g_all_objects_count - 1 - i) * sizeof(lv_obj_t *));
            }
            g_all_objects_count--;
             // Optional: Realloc smaller? Maybe not worth it frequently.
            return;
        }
    }
    EMUL_LOG("WARN: Object %p not found in global list for removal.\n", (void*)obj);
}

bool emul_obj_add_child(lv_obj_t *parent, lv_obj_t *child) {
    if (!parent || !child) return false;
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
    } else {
         EMUL_LOG("WARN: Child %p not found under parent %p for removal.\n", (void*)child_to_remove, (void*)parent);
    }
}

Property* find_property(lv_obj_t *obj, const char* key) {
    if (!obj || !key) return NULL;
    for (size_t i = 0; i < obj->prop_count; ++i) {
        // Check key before strcmp to handle potential NULL from failed strdup
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
        EMUL_LOG("Updating property '%s' on obj %p\n", key, (void*)obj);
        free_value(&existing->value); // Free old value
        existing->value = value;      // Assign new value
        return true;
    } else {
        // Add new property
        GROW_ARRAY(Property, obj->properties, obj->prop_count, obj->prop_capacity);
        if (!obj->properties) { free_value(&value); return false; } // Grow array failed

        obj->properties[obj->prop_count].key = strdup(key);
        if (!obj->properties[obj->prop_count].key) { // strdup failed
            EMUL_LOG("ERROR: strdup failed for property key '%s'\n", key);
            free_value(&value); // Free the value we were trying to add
            // Don't increment count
            return false;
        }
        obj->properties[obj->prop_count].value = value;
        obj->prop_count++;
        EMUL_LOG("Added property '%s' to obj %p\n", key, (void*)obj);
        return true;
    }
}

StyleEntry* find_style(lv_obj_t *obj, lv_part_t part, lv_state_t state, const char* prop_name) {
     if (!obj || !prop_name) return NULL;
     for (size_t i = 0; i < obj->style_count; ++i) {
         // Check prop_name before strcmp
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
         EMUL_LOG("Updating style '%s' [part:0x%X state:0x%X] on obj %p\n", prop_name, (unsigned int)part, (unsigned int)state, (void*)obj);
         free_value(&existing->value);
         existing->value = value;
         return true;
     } else {
         GROW_ARRAY(StyleEntry, obj->styles, obj->style_count, obj->style_capacity);
         if (!obj->styles) { free_value(&value); return false; } // Grow array failed

         obj->styles[obj->style_count].prop_name = strdup(prop_name);
         if (!obj->styles[obj->style_count].prop_name) {
             EMUL_LOG("ERROR: strdup failed for style prop_name '%s'\n", prop_name);
             free_value(&value);
             // Don't increment count
             return false;
         }
         obj->styles[obj->style_count].part = part;
         obj->styles[obj->style_count].state = state;
         obj->styles[obj->style_count].value = value;
         obj->style_count++;
         EMUL_LOG("Added style '%s' [part:0x%X state:0x%X] to obj %p\n", prop_name, (unsigned int)part, (unsigned int)state, (void*)obj);
         return true;
     }
}

// --- Value Creators ---
Value value_mk_string(const char* s) {
    Value v = {.type = VAL_TYPE_STRING, .data = {.s = NULL}};
    if (s) {
        v.data.s = strdup(s);
        if (!v.data.s) { EMUL_LOG("ERROR: strdup failed for string value\n"); v.type = VAL_TYPE_NONE; }
    }
    return v;
}
Value value_mk_int_array(const int32_t *array, size_t num_elems) {
    Value v = {.type = VAL_TYPE_INT_ARRAY, .data = {.int_array = NULL}};
    if (array && num_elems > 0) {
        // Allocate space for length + elements
        v.data.int_array = malloc((num_elems + 1) * sizeof(int32_t));
        if (v.data.int_array) {
            v.data.int_array[0] = (int32_t)num_elems; // Store length at index 0
            memcpy(&v.data.int_array[1], array, num_elems * sizeof(int32_t)); // Copy elements after length
        } else {
             EMUL_LOG("ERROR: malloc failed for int array\n");
             v.type = VAL_TYPE_NONE;
        }
    } else if (array && num_elems == 0) {
         // Handle empty array case: allocate just for the length field
        v.data.int_array = malloc(1 * sizeof(int32_t));
        if (v.data.int_array) {
             v.data.int_array[0] = 0;
        } else {
             EMUL_LOG("ERROR: malloc failed for empty int array\n");
             v.type = VAL_TYPE_NONE;
        }
    }
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
Value value_mk_opa(lv_opa_t val) { return (Value){.type = VAL_TYPE_OPA, .data = {.opa = val}}; }
Value value_mk_grad_dir(lv_grad_dir_t val) { return (Value){.type = VAL_TYPE_GRAD_DIR, .data = {.grad_dir = val}}; }
Value value_mk_ptr(void *p) { return (Value){.type = VAL_TYPE_PTR, .data = {.ptr = p}}; }

// --- Internal Object Creation Helper ---
static lv_obj_t * create_object_internal(lv_obj_t *parent, const char* type_name) {
    lv_obj_t *new_obj = (lv_obj_t *)calloc(1, sizeof(lv_obj_t));
    if (!new_obj) {
        EMUL_LOG("ERROR: Failed to allocate memory for new object type %s\n", type_name);
        return NULL;
    }

    new_obj->id = (uintptr_t)new_obj; // Use address as ID
    new_obj->type = type_name; // Assign static type string (pointer comparison is fine)
    // Parent will be set by emul_obj_add_child if parent is not NULL

    // Add to global list FIRST, so it's tracked even if parent add fails
    GROW_ARRAY(lv_obj_t *, g_all_objects, g_all_objects_count, g_all_objects_capacity);
    if (!g_all_objects) { // Grow array failed
        free(new_obj);
        EMUL_LOG("ERROR: Failed to grow global object list for new obj %s\n", type_name);
        return NULL;
    }
    g_all_objects[g_all_objects_count++] = new_obj;

    // Add to parent's children list (also sets new_obj->parent)
    if (parent) {
        if (!emul_obj_add_child(parent, new_obj)) {
            // Failed to add to parent, but it's already in global list. Remove it.
            remove_from_global_list(new_obj);
            free(new_obj); // Free the object itself
            EMUL_LOG("ERROR: Failed to add child %p to parent %p\n", (void*)new_obj, (void*)parent);
            return NULL;
        }
    } else {
        new_obj->parent = NULL; // Explicitly NULL if no parent
    }

    EMUL_LOG("Created object %p (type: %s), parent: %p, global_count: %zu\n",
              (void*)new_obj, type_name, (void*)parent, g_all_objects_count);

    return new_obj;
}

// --- Library Control ---

void emul_lvgl_init(void) {
    EMUL_LOG("Initializing LVGL Emulation Library\n");
    if (g_screen_obj != NULL || g_all_objects_count > 0 || g_font_map_count > 0) {
        EMUL_LOG("WARN: Already initialized or state not clean. Resetting state first.\n");
        emul_lvgl_reset(); // Reset if called again or state is dirty
    }
    // Clear any potential leftover pointers from a previous bad state
    g_screen_obj = NULL;
    g_all_objects = NULL;
    g_all_objects_count = 0;
    g_all_objects_capacity = 0;
    g_font_map = NULL;
    g_font_map_count = 0;
    g_font_map_capacity = 0;


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

    // Init font map (already done by reset/clear above)

    EMUL_LOG("Screen object %p created. Global count: %zu\n", (void*)g_screen_obj, g_all_objects_count);
}

void emul_lvgl_reset(void) {
    EMUL_LOG("Resetting LVGL Emulation state...\n");

    // Free all non-screen objects from the global list.
    // Iterate backwards to handle removals safely.
    if (g_all_objects) {
        for (size_t i = g_all_objects_count; i > 0; --i) {
            lv_obj_t *obj_to_check = g_all_objects[i - 1];
            if (obj_to_check != g_screen_obj) {
                EMUL_LOG("Reset: Deleting object %p (type: %s)\n", (void*)obj_to_check, obj_to_check->type);
                 // Call lv_obj_del, which handles recursion, freeing content, and removal from lists
                 lv_obj_del(obj_to_check);
                 // lv_obj_del should modify g_all_objects_count, but loop relies on initial count
            }
        }
    }
     // After the loop, g_all_objects should ideally only contain g_screen_obj if it existed.
     // Let's manually clean up the screen object's contents and reset the list.
    if (g_screen_obj) {
        EMUL_LOG("Reset: Clearing screen object %p contents\n", (void*)g_screen_obj);
        free_emul_object_contents(g_screen_obj); // Clear styles, props, children array

         // Reset global list to only contain the screen object
         if (g_all_objects && g_all_objects_capacity > 0) {
             g_all_objects[0] = g_screen_obj;
             g_all_objects_count = 1;
         } else {
             // This case should ideally not happen if init was successful
             EMUL_LOG("WARN: Global object list invalid during screen reset.\n");
             free(g_all_objects); // Free the potentially NULL or invalid pointer
             g_all_objects = NULL;
             g_all_objects_count = 0;
             g_all_objects_capacity = 0;
             // Try to re-allocate and add screen back
             GROW_ARRAY(lv_obj_t *, g_all_objects, g_all_objects_count, g_all_objects_capacity);
             if (g_all_objects) {
                 g_all_objects[g_all_objects_count++] = g_screen_obj;
             } else {
                 EMUL_LOG("ERROR: Failed to re-allocate global list after reset error. Screen object lost!\n");
                 free(g_screen_obj); // Cannot keep screen if list fails
                 g_screen_obj = NULL;
             }
         }
    } else {
        // Screen object doesn't exist, just clear the list fully
        free(g_all_objects);
        g_all_objects = NULL;
        g_all_objects_count = 0;
        g_all_objects_capacity = 0;
    }


    // Free font map entries
    for (size_t i = 0; i < g_font_map_count; ++i) {
        if (g_font_map[i].name) free(g_font_map[i].name);
    }
    free(g_font_map);
    g_font_map = NULL;
    g_font_map_count = 0;
    g_font_map_capacity = 0;

    EMUL_LOG("Reset complete. Screen object: %p, Global count: %zu\n", (void*)g_screen_obj, g_all_objects_count);
}

void emul_lvgl_deinit(void) {
    EMUL_LOG("Deinitializing LVGL Emulation Library...\n");
    emul_lvgl_reset(); // Clear all dynamic objects first

    // Free the screen object struct itself now
    if (g_screen_obj) {
        EMUL_LOG("Deinit: Freeing screen object %p\n", (void*)g_screen_obj);
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
            if ((g_font_map[i].name && strcmp(g_font_map[i].name, name) != 0) || !g_font_map[i].name) {
                EMUL_LOG("Updating font name for %p to '%s'\n", font_ptr, name);
                free(g_font_map[i].name); // Free old name if it existed
                g_font_map[i].name = strdup(name);
                 if (!g_font_map[i].name) {
                     EMUL_LOG("ERROR: strdup failed updating font name '%s'\n", name);
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
    EMUL_LOG("Registered font %p as '%s' (Font map count: %zu)\n", font_ptr, name, g_font_map_count);
}

// --- LVGL API Implementation ---

// ** Object Deletion/Cleanup **
void lv_obj_del(lv_obj_t *obj) {
    if (!obj) {
        EMUL_LOG("WARN: lv_obj_del(NULL) called.\n");
        return;
    }
     if (obj == g_screen_obj) {
         EMUL_LOG("WARN: Attempt to delete screen object (%p). Clearing content instead.\n", (void*)obj);
         lv_obj_clean(obj); // Just clean children, don't delete screen itself
         return;
     }

    EMUL_LOG("Deleting object %p (type: %s)... Global count before: %zu\n", (void*)obj, obj->type, g_all_objects_count);

    // 0. Check if object is still in the global list (prevents double free issues)
    bool found_in_global = false;
    size_t global_idx = (size_t)-1;
    for (size_t i = 0; i < g_all_objects_count; ++i) {
        if (g_all_objects[i] == obj) {
            found_in_global = true;
            global_idx = i;
            break;
        }
    }
    if (!found_in_global) {
        EMUL_LOG("WARN: Attempting to delete object %p which is not in the global list (already deleted?). Skipping free.\n", (void*)obj);
        // Still try to remove from parent if it has one, as parent might still hold dangling pointer
        if (obj->parent) {
            emul_obj_remove_child(obj->parent, obj);
        }
        return; // Don't proceed with freeing memory or recursive calls
    }

    // 1. Remove from parent's children list (if parent exists)
    if (obj->parent) {
        emul_obj_remove_child(obj->parent, obj);
    }

    // 2. Remove pointer from global list *before* freeing/recursion
    // remove_from_global_list uses memmove, safe to call here
    remove_from_global_list(obj); // This modifies g_all_objects_count

     // 3. Recursively delete children
     // Make a copy of the children pointers *before* freeing the object's contents
     size_t num_children = obj->child_count;
     lv_obj_t **children_copy = NULL;
     if(num_children > 0) {
        children_copy = malloc(num_children * sizeof(lv_obj_t *));
        if(children_copy) {
            memcpy(children_copy, obj->children, num_children * sizeof(lv_obj_t *));
            EMUL_LOG(" Copied %zu children pointers for obj %p recursive delete.\n", num_children, (void*)obj);
        } else {
             EMUL_LOG("ERROR: Failed to allocate children copy for delete of obj %p\n", (void*)obj);
             // Continue deletion of the object itself, children might leak if copy failed
             num_children = 0; // Prevent loop below
        }
     }

     // 4. Free current object's direct contents (props, styles, children *array*)
     // This invalidates obj->children pointer but children_copy still holds the actual child object pointers
     free_emul_object_contents(obj);

     // 5. Free the object struct itself
     EMUL_LOG(" Freeing obj %p struct memory.\n", (void*)obj);
     free(obj);

     // 6. Now recursively delete children using the copied pointers
     if(children_copy) {
         EMUL_LOG(" Recursively deleting %zu children of formerly obj %p...\n", num_children, (void*)obj);
         for(size_t i = 0; i < num_children; ++i) {
             lv_obj_t *child_ptr = children_copy[i];
             // lv_obj_del itself handles double-deletion checks via global list
             lv_obj_del(child_ptr); // Recursive call
         }
         free(children_copy);
     }

     EMUL_LOG("Deletion complete for obj %p. Global count after: %zu\n", (void*)obj, g_all_objects_count);
}

void lv_obj_clean(lv_obj_t *obj) {
     if (!obj) return;
     EMUL_LOG("Cleaning children of object %p (current child count: %zu)\n", (void*)obj, obj->child_count);

     // Make a copy of children pointers FIRST
     size_t num_children = obj->child_count;
     if (num_children == 0) return; // Nothing to clean

     lv_obj_t **children_to_delete = malloc(num_children * sizeof(lv_obj_t *));
     if (!children_to_delete) {
         EMUL_LOG("ERROR: Failed alloc for clean child copy on obj %p\n", (void*)obj);
         return; // Cannot proceed without copy
     }
     memcpy(children_to_delete, obj->children, num_children * sizeof(lv_obj_t *));

     // Delete each child using lv_obj_del with the pointer from the copy
     // lv_obj_del will modify obj->children and obj->child_count via emul_obj_remove_child
     for (size_t i = 0; i < num_children; ++i) {
         EMUL_LOG(" Cleaning: deleting child %p\n", (void*)children_to_delete[i]);
         lv_obj_del(children_to_delete[i]);
     }
     free(children_to_delete);

     // Sanity check: After deletion, the child count should be 0
     if (obj->child_count != 0) {
        EMUL_LOG("WARN: Child count not zero after clean (%zu) for obj %p. Forcibly clearing array.\n", obj->child_count, (void*)obj);
        obj->child_count = 0;
        // The children array pointer itself might still be valid if realloc happened,
        // but free_emul_object_contents or the next GROW_ARRAY should handle it.
        // Let's just nullify count and capacity. free()ing obj->children here might be unsafe if lv_obj_del realloc'd smaller.
        obj->child_capacity = 0; // Prevent reuse until grow
        // free(obj->children); // Avoid potential double-free or invalid pointer free
        // obj->children = NULL;
     }
     EMUL_LOG("Cleaning complete for obj %p. Final child count: %zu\n", (void*)obj, obj->child_count);
}

// ** Screen Access ** Returns pointer
lv_obj_t * lv_screen_active(void) {
    if (!g_screen_obj) {
        EMUL_LOG("WARN: lv_screen_active called before emul_lvgl_init or after failed init. Returning NULL.\n");
        return NULL;
    }
    return g_screen_obj; // Return the pointer stored globally
}

//Alias
// lv_obj_t * lv_scr_act(void) {
//     return lv_screen_active();
// }


// ** Hierarchy **
void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent_new) {
    if (!obj) return;
    if (obj == g_screen_obj) {
        EMUL_LOG("WARN: Cannot set parent of the screen object.\n");
        return;
    }

    lv_obj_t* parent_old = obj->parent;

    if(parent_old == parent_new) {
        EMUL_LOG("Object %p already child of %p. No change needed.\n", (void*)obj, (void*)parent_new);
        return; // No change needed
    }

    EMUL_LOG("Setting parent of obj %p from %p to %p\n", (void*)obj, (void*)parent_old, (void*)parent_new);

    // 1. Remove from old parent's children list
    if (parent_old) {
        emul_obj_remove_child(parent_old, obj);
    } else {
        // If it had no parent, it might be a detached object or a top-level one (other than screen)
        // No removal needed in this case.
    }

    // 2. Add to new parent's children list (sets obj->parent internally)
    if (parent_new) {
        if (!emul_obj_add_child(parent_new, obj)) {
             EMUL_LOG("ERROR: Failed to add obj %p as child of new parent %p! Object might be detached.\n", (void*)obj, (void*)parent_new);
             // Object is now detached, parent pointer is likely NULL from failed add_child.
        }
    } else {
         obj->parent = NULL; // Explicitly set parent to NULL if new parent is NULL
         EMUL_LOG(" Obj %p is now detached (parent set to NULL).\n", (void*)obj);
    }
    // No specific property added; hierarchy is the source of truth.
}

// --- Style ADD_STYLE Macro ---
// Macro to simplify adding styles
#define ADD_STYLE(obj_ptr, selector, prop_name_str, value_func) \
    do { \
        if (!(obj_ptr)) break; /* Check the pointer */ \
        /* Extract part and state from selector more robustly */ \
        lv_part_t part = (selector) & (LV_PART_MAIN | LV_PART_SCROLLBAR | LV_PART_INDICATOR | \
                                       LV_PART_KNOB | LV_PART_SELECTED | LV_PART_ITEMS | \
                                       LV_PART_CURSOR | LV_PART_CUSTOM_FIRST); /* Mask for known parts */ \
        /* Handle LV_PART_ANY if needed, though specific parts are preferred */ \
        if (((selector) & LV_PART_ANY) == LV_PART_ANY) part = LV_PART_ANY; \
        /* Extract state */ \
        lv_state_t state = (selector) & (LV_STATE_DEFAULT | LV_STATE_CHECKED | LV_STATE_FOCUSED | \
                                         LV_STATE_FOCUS_KEY | LV_STATE_EDITED | LV_STATE_HOVERED | \
                                         LV_STATE_PRESSED | LV_STATE_SCROLLED | LV_STATE_DISABLED); /* Mask for known states */ \
        /* Handle LV_STATE_ANY */ \
        if (((selector) & LV_STATE_ANY) == LV_STATE_ANY) state = LV_STATE_ANY; \
        else if (state == 0 && selector != 0) { /* If selector has part but no known state bits, assume DEFAULT */              state = LV_STATE_DEFAULT;         }         /* Add the style */ \
        emul_obj_add_style(obj_ptr, part, state, prop_name_str, value_func); \
    } while(0)


// --- Helper Value Creators Implementation ---
lv_color_t lv_color_hex(uint32_t c) {
    return lv_color_make((uint8_t)((c >> 16) & 0xFF), (uint8_t)((c >> 8) & 0xFF), (uint8_t)(c & 0xFF));
}
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b) {
    // Assumes lv_color_t has r, g, b fields. Adjust if using different color format internally.
    return (lv_color_t){.r = r, .g = g, .b = b};
}
lv_color_t lv_color_white(void) { return lv_color_make(255, 255, 255); }
lv_color_t lv_color_black(void) { return lv_color_make(0, 0, 0); }
int32_t lv_pct(int32_t v) { return LV_PCT(v); } // Use the macro defined in the header

// --- JSON Generation ---

// --- JSON Generation Helpers ---

const char* part_to_string(lv_part_t part) {
    // Map defined parts back to strings used in viewer JSON
    // This needs to be kept in sync with ui_builder's parse_part
    switch(part) {
        case LV_PART_MAIN: return "main"; // Use "main" instead of "default" for clarity? Let's stick to example: "default"
        case LV_PART_SCROLLBAR: return "scrollbar";
        case LV_PART_INDICATOR: return "indicator";
        case LV_PART_KNOB: return "knob";
        case LV_PART_SELECTED: return "selected";
        case LV_PART_ITEMS: return "items";
        case LV_PART_CURSOR: return "cursor";
        case LV_PART_ANY: return "part_any"; // Special case for ANY selector
        // Add others if used...
        default: {
            // Handle custom parts if needed
            if(part >= LV_PART_CUSTOM_FIRST) {
                static char custom_buf[32];
                snprintf(custom_buf, sizeof(custom_buf), "custom_part_%u", (unsigned int)(part - LV_PART_CUSTOM_FIRST));
                return custom_buf;
            }
            // Fallback for unknown values (shouldn't happen with masked ADD_STYLE)
            static char unknown_buf[32];
            snprintf(unknown_buf, sizeof(unknown_buf), "unknown_part_0x%X", (unsigned int)part);
             EMUL_LOG("WARN: Unknown part value 0x%X encountered during JSON generation.\n", (unsigned int)part);
            return unknown_buf;
        }
    }
}

const char* state_to_string(lv_state_t state) {
     // Map states to strings. Prioritize common overrides.
     // This needs to be kept in sync with ui_builder's parse_state
     if (state == LV_STATE_ANY) return "state_any"; // Special case for ANY selector

     // Build a string for combined states? Or just pick one? Example implies picking one.
     // Let's try building a combined string if multiple bits are set (more complex).
     // Simplified: return highest priority single state found.
     if (state & LV_STATE_DISABLED) return "disabled"; // Highest priority visual override usually
     if (state & LV_STATE_PRESSED) return "pressed";
     if (state & LV_STATE_CHECKED) return "checked"; // Checked before focused? Depends on widget.
     if (state & LV_STATE_FOCUSED) return "focused"; // Includes FOCUS_KEY? Assume yes for simplicity. LVGL combines them.
     if (state & LV_STATE_EDITED) return "edited";
     if (state & LV_STATE_HOVERED) return "hovered";
     if (state & LV_STATE_SCROLLED) return "scrolled";

     // If only default or no bits set
     if (state == LV_STATE_DEFAULT) return "default";

     // Fallback for unknown/combinations not handled above
     static char combined_buf[32];
     snprintf(combined_buf, sizeof(combined_buf), "state_0x%X", (unsigned int)state);
     EMUL_LOG("WARN: Unhandled state value 0x%X during JSON generation. Using fallback string.\n", (unsigned int)state);
     return combined_buf;
}

const char* align_to_string(lv_align_t align) {
    // Keep in sync with ui_builder parse_align
    switch(align) {
        case LV_ALIGN_DEFAULT: return "default"; // LV_ALIGN_DEFAULT is often 0 or LV_ALIGN_TOP_LEFT
        case LV_ALIGN_TOP_LEFT: return "top_left";
        case LV_ALIGN_TOP_MID: return "top_mid";
        case LV_ALIGN_TOP_RIGHT: return "top_right";
        case LV_ALIGN_LEFT_MID: return "left_mid";
        case LV_ALIGN_CENTER: return "center";
        case LV_ALIGN_RIGHT_MID: return "right_mid";
        case LV_ALIGN_BOTTOM_LEFT: return "bottom_left";
        case LV_ALIGN_BOTTOM_MID: return "bottom_mid";
        case LV_ALIGN_BOTTOM_RIGHT: return "bottom_right";
        // Add OUT alignments if needed
        default: return "default"; // Fallback
    }
}

const char* text_align_to_string(int32_t align) { // lv_text_align_t enum values
     // Keep in sync with ui_builder apply_single_style_prop
     switch(align) {
         // Assumes standard LV_TEXT_ALIGN_... values
         case 0: return "auto";   // LV_TEXT_ALIGN_AUTO (check actual value)
         case 1: return "left";   // LV_TEXT_ALIGN_LEFT
         case 2: return "center"; // LV_TEXT_ALIGN_CENTER
         case 3: return "right";  // LV_TEXT_ALIGN_RIGHT
         default: return "auto";
     }
 }

void color_to_hex_string(lv_color_t color, char* buf, size_t buf_size) {
    // Simple hex conversion #RRGGBB
    // Assumes color has r,g,b fields
    snprintf(buf, buf_size, "#%02X%02X%02X", color.r, color.g, color.b);
    // Handle alpha if needed: snprintf(buf, buf_size, "#%02X%02X%02X%02X", color.r, color.g, color.b, color.alpha);
}

const char* font_ptr_to_name(lv_font_t font_ptr) {
    if (!font_ptr) return "default"; // Or NULL? Let's return "default"

    // Check against known standard fonts first (if available)
    // This requires the generator or user to provide these symbols if they want nice names
    // extern const lv_font_t lv_font_montserrat_14; // Example declaration needed elsewhere
    // if (font_ptr == &lv_font_montserrat_14) return "montserrat_14";

    // Check registered fonts
    for (size_t i = 0; i < g_font_map_count; ++i) {
        if (g_font_map[i].ptr == font_ptr) {
            return g_font_map[i].name ? g_font_map[i].name : "unnamed_font";
        }
    }

    // Fallback if not registered and not a known standard font
    // Generate a stable name based on pointer address?
    static char unknown_buf[32];
    snprintf(unknown_buf, sizeof(unknown_buf), "font_ptr_%p", font_ptr);
    EMUL_LOG("WARN: Font pointer %p not registered. Using '%s'.\n", font_ptr, unknown_buf);
    return unknown_buf;
}

void coord_to_string(lv_coord_t coord, char* buf, size_t buf_size) {
    // Keep in sync with ui_builder parse_coord
    if (LV_COORD_IS_PCT(coord)) {
        snprintf(buf, buf_size, "%"PRId32"%%", LV_COORD_GET_PCT(coord));
    } else if (coord == LV_SIZE_CONTENT) {
        snprintf(buf, buf_size, "content");
    } else if (LV_COORD_IS_SPEC(coord)) { // Handle other special coords? Like Grid FR?
         // LV_GRID_FR(x) = (LV_COORD_MAX - 100 + x)
         if (coord >= (LV_COORD_MAX - 100) && coord < LV_COORD_MAX) {
             snprintf(buf, buf_size, "%"PRId32"fr", coord - (LV_COORD_MAX - 100));
         } else {
             snprintf(buf, buf_size, "spec_%"PRId32, LV_COORD_PLAIN(coord)); // Generic special
         }
    } else { // Plain coordinate
        snprintf(buf, buf_size, "%"PRId32, coord); // Use PRId32 for int32_t
    }
}

const char* layout_to_string(lv_layout_t layout) {
     // Keep in sync with ui_builder parse_layout
    switch(layout) {
        case LV_LAYOUT_NONE: return "none";
        case LV_LAYOUT_FLEX: return "flex";
        case LV_LAYOUT_GRID: return "grid";
        default: return "unknown";
    }
}

const char* grid_align_to_string(lv_grid_align_t align) {
    // Keep in sync with ui_builder parse_grid_align
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
    // Keep in sync with ui_builder parse_flex_align
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
    // Keep in sync with ui_builder parse_flex_flow
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
     // Keep in sync with ui_builder parse_scale_mode
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
const char* grad_dir_to_string(lv_grad_dir_t dir) {
    switch(dir) {
        case LV_GRAD_DIR_NONE: return "none";
        case LV_GRAD_DIR_VER: return "ver";
        case LV_GRAD_DIR_HOR: return "hor";
        case LV_GRAD_DIR_LINEAR: return "linear";
        case LV_GRAD_DIR_RADIAL: return "radial";
        case LV_GRAD_DIR_CONICAL: return "conical";
        default: return "unknown";
    }
}


cJSON* int_array_to_json_array(const int32_t* arr) {
    if (!arr) return NULL; // Return NULL for null pointer

    cJSON* json_arr = cJSON_CreateArray();
    if (!json_arr) {
        EMUL_LOG("ERROR: Failed to create cJSON array for int_array\n");
        return NULL;
    }

    int32_t len = arr[0]; // Length stored at index 0
    if (len < 0) {
         EMUL_LOG("WARN: Negative length (%d) found in int_array, treating as empty.\n", len);
         len = 0;
    }

    for (int32_t i = 0; i < len; i++) {
        // Access elements starting from index 1
        cJSON* num_item = cJSON_CreateNumber((double)arr[i + 1]); // cJSON uses doubles for numbers
        if (num_item) {
            cJSON_AddItemToArray(json_arr, num_item);
        } else {
            EMUL_LOG("ERROR: Failed to create cJSON number for int_array element %d\n", i);
            // Continue adding other elements if possible
        }
    }
    return json_arr;
}

// --- Recursive JSON Builder ---
cJSON* build_json_recursive(lv_obj_t *obj) {
    if (!obj) return NULL;

    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) return NULL;

    // Type
    cJSON_AddStringToObject(json_obj, "type", obj->type ? obj->type : "unknown");

    // Object ID (Pointer Address) - useful for debugging linkage
    char id_buf[32];
    snprintf(id_buf, sizeof(id_buf), "%p", (void*)obj);
    cJSON_AddStringToObject(json_obj, "id", id_buf);


    // Properties
    if (obj->prop_count > 0) {
        cJSON* props_json = cJSON_CreateObject();
        if (!props_json) { cJSON_Delete(json_obj); return NULL; }
        cJSON_AddItemToObject(json_obj, "properties", props_json);
        char buf[32]; // Shared buffer for string conversions

        for (size_t i = 0; i < obj->prop_count; ++i) {
            Property* prop = &obj->properties[i];
            if (!prop->key) continue; // Skip if key allocation failed

            switch (prop->value.type) {
                case VAL_TYPE_STRING:
                    cJSON_AddStringToObject(props_json, prop->key, prop->value.data.s ? prop->value.data.s : "");
                    break;
                case VAL_TYPE_INT: // Includes simple enums stored as int
                    cJSON_AddNumberToObject(props_json, prop->key, prop->value.data.i);
                    break;
                 case VAL_TYPE_OPA: // Opa is uint8_t but stored as int32_t in union maybe? Treat as number.
                    cJSON_AddNumberToObject(props_json, prop->key, prop->value.data.opa);
                    break;
                case VAL_TYPE_COORD:
                    coord_to_string(prop->value.data.coord, buf, sizeof(buf));
                    // Store percentages, content, fr as strings, numbers as numbers
                    if (strchr(buf, '%') || strcmp(buf, "content") == 0 || strstr(buf, "fr"))
                        cJSON_AddStringToObject(props_json, prop->key, buf);
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
                case VAL_TYPE_FLEX_ALIGN:
                    cJSON_AddStringToObject(props_json, prop->key, flex_align_to_string(prop->value.data.flex_align));
                    break;
                case VAL_TYPE_FLEX_FLOW:
                     cJSON_AddStringToObject(props_json, prop->key, flex_flow_to_string(prop->value.data.flex_flow));
                     break;
                case VAL_TYPE_SCALE_MODE:
                     cJSON_AddStringToObject(props_json, prop->key, scale_mode_to_string(prop->value.data.scale_mode));
                     break;
                case VAL_TYPE_GRAD_DIR:
                     cJSON_AddStringToObject(props_json, prop->key, grad_dir_to_string(prop->value.data.grad_dir));
                     break;
                case VAL_TYPE_INT_ARRAY: {
                    cJSON* arr_json = int_array_to_json_array(prop->value.data.int_array);
                    if (arr_json) cJSON_AddItemToObject(props_json, prop->key, arr_json);
                    // else: Error creating array logged in helper
                    break;
                }
                 case VAL_TYPE_PTR:
                    snprintf(buf, sizeof(buf), "ptr_%p", prop->value.data.ptr);
                    cJSON_AddStringToObject(props_json, prop->key, buf);
                    break;
                // Types usually handled by styles:
                case VAL_TYPE_COLOR:
                case VAL_TYPE_FONT:
                case VAL_TYPE_TEXTALIGN:
                     EMUL_LOG("WARN: Property '%s' has style-like type %d. Skipping in properties section.\n", prop->key, prop->value.type);
                    break;
                case VAL_TYPE_NONE: // Explicitly ignore NONE type
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

            // Get or create Part object
            cJSON* part_obj = cJSON_GetObjectItemCaseSensitive(styles_json, part_str);
            if (!part_obj) {
                part_obj = cJSON_CreateObject();
                if (!part_obj) { EMUL_LOG("ERROR: Failed create part obj %s\n", part_str); continue; }
                cJSON_AddItemToObject(styles_json, part_str, part_obj);
             } else if (!cJSON_IsObject(part_obj)) {
                 EMUL_LOG("ERROR: Existing item '%s' is not an object in styles.\n", part_str);
                 continue;
             }

             // Get or create State object within Part object
            cJSON* state_obj = cJSON_GetObjectItemCaseSensitive(part_obj, state_str);
            if (!state_obj) {
                 state_obj = cJSON_CreateObject();
                 if (!state_obj) { EMUL_LOG("ERROR: Failed create state obj %s\n", state_str); continue; }
                 cJSON_AddItemToObject(part_obj, state_str, state_obj);
             } else if (!cJSON_IsObject(state_obj)) {
                 EMUL_LOG("ERROR: Existing item '%s' is not an object in part '%s'.\n", state_str, part_str);
                 continue;
             }

            // Add the style property to the state object
            char buf[32]; // Shared buffer for conversions
            switch (entry->value.type) {
                case VAL_TYPE_COLOR:
                    color_to_hex_string(entry->value.data.color, buf, sizeof(buf));
                    cJSON_AddStringToObject(state_obj, entry->prop_name, buf);
                    break;
                case VAL_TYPE_COORD:
                    coord_to_string(entry->value.data.coord, buf, sizeof(buf));
                    // Store percentages, content, fr as strings, numbers as numbers
                     if (strchr(buf, '%') || strcmp(buf, "content") == 0 || strstr(buf, "fr"))
                        cJSON_AddStringToObject(state_obj, entry->prop_name, buf);
                     else
                        cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.coord);
                    break;
                case VAL_TYPE_INT: // Often used for opa, border_width, padding etc. Stored as int32
                    cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.i);
                    break;
                 case VAL_TYPE_OPA: // Stored as opa_t (uint8)
                    cJSON_AddNumberToObject(state_obj, entry->prop_name, entry->value.data.opa);
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
                 case VAL_TYPE_GRAD_DIR:
                     cJSON_AddStringToObject(state_obj, entry->prop_name, grad_dir_to_string(entry->value.data.grad_dir));
                     break;
                  case VAL_TYPE_PTR: // Pointer style (e.g., transition)
                     snprintf(buf, sizeof(buf), "ptr_%p", entry->value.data.ptr);
                     cJSON_AddStringToObject(state_obj, entry->prop_name, buf);
                     break;
                 // Add cases for other VAL_TYPEs if needed for styles
                 case VAL_TYPE_NONE: // Explicitly ignore NONE type
                 // Types usually handled by properties:
                 case VAL_TYPE_STRING:
                 case VAL_TYPE_BOOL:
                 case VAL_TYPE_ALIGN:
                 case VAL_TYPE_GRID_ALIGN:
                 case VAL_TYPE_LAYOUT:
                 case VAL_TYPE_FLEX_ALIGN:
                 case VAL_TYPE_SCALE_MODE:
                 case VAL_TYPE_INT_ARRAY:
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
            cJSON* child_json_obj = build_json_recursive(obj->children[i]);
            if (child_json_obj) {
                cJSON_AddItemToArray(children_json, child_json_obj);
            } else {
                EMUL_LOG("WARN: Failed to build JSON for child object %p (index %zu)\n", (void*)obj->children[i], i);
                // Add a placeholder null or skip? Let's skip.
            }
        }
    }

    return json_obj;
}


// --- Public JSON Generator ---
char* emul_lvgl_get_json(lv_obj_t *root_obj) {
    if (!root_obj) {
        EMUL_LOG("ERROR: emul_lvgl_get_json called with NULL root object pointer\n");
        return NULL;
    }
     if (root_obj != g_screen_obj) {
          EMUL_LOG("WARN: emul_lvgl_get_json called with object %p which is not the screen object %p. Output might be incomplete.\n", (void*)root_obj, (void*)g_screen_obj);
     }

    // Create a top-level container (optional, but good practice)
    cJSON *root_container = cJSON_CreateObject();
     if (!root_container) {
         EMUL_LOG("ERROR: Failed to create root cJSON container\n");
         return NULL;
     }

    // Call recursive builder with the root pointer
    EMUL_LOG("Starting JSON build from root %p...\n", (void*)root_obj);
    cJSON* ui_tree_json = build_json_recursive(root_obj);
    if (!ui_tree_json) {
        cJSON_Delete(root_container);
        EMUL_LOG("ERROR: Failed to build JSON structure recursively from root %p\n", (void*)root_obj);
        return NULL;
    }

    // Add the generated tree under a "root" key in the container
    cJSON_AddItemToObject(root_container, "root", ui_tree_json);

    // Convert the cJSON object to a string
    char* json_string = JSON_print(root_container);
    cJSON_Delete(root_container); // Delete the container and its children

    if (!json_string) {
        EMUL_LOG("ERROR: cJSON_Print failed\n");
    } else {
        EMUL_LOG("Generated JSON string successfully (root %p)\n", (void*)root_obj);
    }
    return json_string;
}

// --- Add Placeholder Functions ---
// (Generated Below)


// --- LVGL API Implementations ---

