
#include "lvgl_json_renderer.h"
#include <string.h> // For strcmp, strchr, strncpy, strlen etc.
#include <stdio.h>  // For snprintf, logging
#include <stdlib.h> // For strtoul

// LVGL functions used internally (ensure they are linked)
// extern lv_obj_t * lv_screen_active(void); // Declared in lvgl.h
// extern void *lv_malloc(size_t size); // Declared in lv_mem.h / lv_conf.h
// extern void lv_free(void *ptr);     // Declared in lv_mem.h / lv_conf.h
// extern char *lv_strdup(const char *str); // Declared in lv_mem.h / lv_conf.h

// --- Logging Helper ---
// Convert cJSON node to a compact string for logging (caller must free result)
char* json_node_to_string(cJSON *node) {
    if (!node) {
        // Use lv_strdup if linked, otherwise plain strdup (requires stdlib.h)
        #if defined(LV_USE_STDLIB_MALLOC) && LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
            return lv_strdup("NULL");
        #else
            // Fallback or define lv_strdup wrapper for standard malloc/strdup
             char *null_str = (char*)malloc(5); if(null_str) strcpy(null_str, "NULL"); return null_str;
        #endif
    }
    // PrintUnformatted is more compact for logs
    char *str = cJSON_PrintUnformatted(node);
    if (!str) {
        #if defined(LV_USE_STDLIB_MALLOC) && LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
            return lv_strdup("{\"error\":\"Failed to print JSON\"}");
        #else
             char *err_str = (char*)malloc(30); if(err_str) strcpy(err_str, "{\"error\":\"Failed to print JSON\"}"); return err_str;
        #endif
    }
    // Optional: Truncate very long strings if needed
    // const int max_len = 120;
    // if (strlen(str) > max_len) { str[max_len-3] = '.'; str[max_len-2] = '.'; str[max_len-1] = '.'; str[max_len] = '\0'; }
    return str; // cJSON_Print... allocates, caller must free using cJSON_free
}

// --- Invocation Table Data Structures ---
// Signature of the function pointers in the table entry
typedef bool (*invoke_fn_t)(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr);

// Structure for each entry in the invocation table
typedef struct {
    const char *name;       // LVGL function name (e.g., "lv_obj_set_width")
    invoke_fn_t invoke;   // Pointer to the C invocation wrapper function
    void *func_ptr;     // Pointer to the actual LVGL function
} invoke_table_entry_t;



// --- Configuration ---
// Add any compile-time configuration here if needed

// --- Pointer Registry Implementation ---
// --- Pointer Registry ---

#include <string.h>
#include <stdlib.h>

// Basic Hash Map Registry (Placeholder - requires implementation)
#define HASH_MAP_SIZE 256
typedef struct registry_entry {
    char *name;
    void *ptr;
    struct registry_entry *next;
} registry_entry_t;

static registry_entry_t* g_registry_map[HASH_MAP_SIZE] = {0};

static unsigned int hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c; /* djb2 */
    return hash % HASH_MAP_SIZE;
}

void lvgl_json_register_ptr(const char *name, void *ptr) {
    if (!name || !ptr) return;
    unsigned int index = hash(name);
    // Check if name already exists (update or handle error?)
    registry_entry_t *entry = g_registry_map[index];
    while(entry) {
        if(strcmp(entry->name, name) == 0) {
             LOG_WARN("Registry Warning: Name '%s' already registered. Updating pointer.", name);
             entry->ptr = ptr; // Update existing entry
             return;
        }
        entry = entry->next;
    }
    // Add new entry
    registry_entry_t *new_entry = (registry_entry_t *)LV_MALLOC(sizeof(registry_entry_t));
    if (!new_entry) { LOG_ERR("Registry Error: Failed to allocate memory"); return; }
    new_entry->name = lv_strdup(name);
    if (!new_entry->name) { LV_FREE(new_entry); LOG_ERR("Registry Error: Failed to duplicate name"); return; }
    new_entry->ptr = ptr;
    new_entry->next = g_registry_map[index];
    g_registry_map[index] = new_entry;
     LOG_INFO("Registered pointer '%s'", name);
}

void* lvgl_json_get_registered_ptr(const char *name) {
    if (!name) return NULL;
    unsigned int index = hash(name);
    registry_entry_t *entry = g_registry_map[index];
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry->ptr;
        }
        entry = entry->next;
    }
    return NULL;
}

void lvgl_json_registry_clear() {
    for(int i = 0; i < HASH_MAP_SIZE; ++i) {
        registry_entry_t *entry = g_registry_map[i];
        while(entry) {
             registry_entry_t *next = entry->next;
             LV_FREE(entry->name);
             LV_FREE(entry);
             entry = next;
        }
        g_registry_map[i] = NULL;
    }
     LOG_INFO("Pointer registry cleared.");
}



// --- Enum Unmarshaling ---
// --- Enum Unmarshaling ---

static bool unmarshal_enum_value(cJSON *json_value, const char *enum_type_name, int *dest) {
    if (!cJSON_IsString(json_value)) {
       // Allow integer values directly for enums? Maybe.
       if(cJSON_IsNumber(json_value)) { *dest = (int)json_value->valuedouble; return true; } 
       LOG_ERR("Enum Unmarshal Error: Expected string or number for %s, got %d", enum_type_name, json_value->type);
       return false;
    }
    const char *str_value = json_value->valuestring;
    if (!str_value) { return false; } // Should not happen if IsString passed

    if (strcmp(enum_type_name, "int") == 0) {
        if (strcmp(str_value, "LV_TREE_WALK_PRE_ORDER") == 0) { *dest = 0; return true; }
        else if (strcmp(str_value, "LV_TREE_WALK_POST_ORDER") == 0) { *dest = 1; return true; }
        LOG_ERR("Enum Unmarshal Error: Unknown value '%s' for enum type %s", str_value, enum_type_name);
        return false;
    }
    else if (strcmp(enum_type_name, "int") == 0) {
        if (strcmp(str_value, "LV_STR_SYMBOL_BULLET") == 0) { *dest = 0; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_AUDIO") == 0) { *dest = 1; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_VIDEO") == 0) { *dest = 2; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_LIST") == 0) { *dest = 3; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_OK") == 0) { *dest = 4; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_CLOSE") == 0) { *dest = 5; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_POWER") == 0) { *dest = 6; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_SETTINGS") == 0) { *dest = 7; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_HOME") == 0) { *dest = 8; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_DOWNLOAD") == 0) { *dest = 9; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_DRIVE") == 0) { *dest = 10; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_REFRESH") == 0) { *dest = 11; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_MUTE") == 0) { *dest = 12; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_VOLUME_MID") == 0) { *dest = 13; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_VOLUME_MAX") == 0) { *dest = 14; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_IMAGE") == 0) { *dest = 15; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_TINT") == 0) { *dest = 16; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_PREV") == 0) { *dest = 17; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_PLAY") == 0) { *dest = 18; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_PAUSE") == 0) { *dest = 19; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_STOP") == 0) { *dest = 20; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_NEXT") == 0) { *dest = 21; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_EJECT") == 0) { *dest = 22; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_LEFT") == 0) { *dest = 23; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_RIGHT") == 0) { *dest = 24; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_PLUS") == 0) { *dest = 25; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_MINUS") == 0) { *dest = 26; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_EYE_OPEN") == 0) { *dest = 27; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_EYE_CLOSE") == 0) { *dest = 28; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_WARNING") == 0) { *dest = 29; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_SHUFFLE") == 0) { *dest = 30; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_UP") == 0) { *dest = 31; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_DOWN") == 0) { *dest = 32; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_LOOP") == 0) { *dest = 33; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_DIRECTORY") == 0) { *dest = 34; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_UPLOAD") == 0) { *dest = 35; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_CALL") == 0) { *dest = 36; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_CUT") == 0) { *dest = 37; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_COPY") == 0) { *dest = 38; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_SAVE") == 0) { *dest = 39; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BARS") == 0) { *dest = 40; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_ENVELOPE") == 0) { *dest = 41; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_CHARGE") == 0) { *dest = 42; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_PASTE") == 0) { *dest = 43; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BELL") == 0) { *dest = 44; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_KEYBOARD") == 0) { *dest = 45; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_GPS") == 0) { *dest = 46; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_FILE") == 0) { *dest = 47; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_WIFI") == 0) { *dest = 48; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BATTERY_FULL") == 0) { *dest = 49; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BATTERY_3") == 0) { *dest = 50; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BATTERY_2") == 0) { *dest = 51; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BATTERY_1") == 0) { *dest = 52; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BATTERY_EMPTY") == 0) { *dest = 53; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_USB") == 0) { *dest = 54; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BLUETOOTH") == 0) { *dest = 55; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_TRASH") == 0) { *dest = 56; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_EDIT") == 0) { *dest = 57; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_BACKSPACE") == 0) { *dest = 58; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_SD_CARD") == 0) { *dest = 59; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_NEW_LINE") == 0) { *dest = 60; return true; }
        else if (strcmp(str_value, "LV_STR_SYMBOL_DUMMY") == 0) { *dest = 61; return true; }
        LOG_ERR("Enum Unmarshal Error: Unknown value '%s' for enum type %s", str_value, enum_type_name);
        return false;
    }
    else if (strcmp(enum_type_name, "int") == 0) {
        if (strcmp(str_value, "LV_STYLE_PROP_INV") == 0) { *dest = 0; return true; }
        else if (strcmp(str_value, "LV_STYLE_WIDTH") == 0) { *dest = 1; return true; }
        else if (strcmp(str_value, "LV_STYLE_HEIGHT") == 0) { *dest = 2; return true; }
        else if (strcmp(str_value, "LV_STYLE_LENGTH") == 0) { *dest = 3; return true; }
        else if (strcmp(str_value, "LV_STYLE_MIN_WIDTH") == 0) { *dest = 4; return true; }
        else if (strcmp(str_value, "LV_STYLE_MAX_WIDTH") == 0) { *dest = 5; return true; }
        else if (strcmp(str_value, "LV_STYLE_MIN_HEIGHT") == 0) { *dest = 6; return true; }
        else if (strcmp(str_value, "LV_STYLE_MAX_HEIGHT") == 0) { *dest = 7; return true; }
        else if (strcmp(str_value, "LV_STYLE_X") == 0) { *dest = 8; return true; }
        else if (strcmp(str_value, "LV_STYLE_Y") == 0) { *dest = 9; return true; }
        else if (strcmp(str_value, "LV_STYLE_ALIGN") == 0) { *dest = 10; return true; }
        else if (strcmp(str_value, "LV_STYLE_RADIUS") == 0) { *dest = 12; return true; }
        else if (strcmp(str_value, "LV_STYLE_RADIAL_OFFSET") == 0) { *dest = 13; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_RADIAL") == 0) { *dest = 14; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_TOP") == 0) { *dest = 16; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_BOTTOM") == 0) { *dest = 17; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_LEFT") == 0) { *dest = 18; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_RIGHT") == 0) { *dest = 19; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_ROW") == 0) { *dest = 20; return true; }
        else if (strcmp(str_value, "LV_STYLE_PAD_COLUMN") == 0) { *dest = 21; return true; }
        else if (strcmp(str_value, "LV_STYLE_LAYOUT") == 0) { *dest = 22; return true; }
        else if (strcmp(str_value, "LV_STYLE_MARGIN_TOP") == 0) { *dest = 24; return true; }
        else if (strcmp(str_value, "LV_STYLE_MARGIN_BOTTOM") == 0) { *dest = 25; return true; }
        else if (strcmp(str_value, "LV_STYLE_MARGIN_LEFT") == 0) { *dest = 26; return true; }
        else if (strcmp(str_value, "LV_STYLE_MARGIN_RIGHT") == 0) { *dest = 27; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_COLOR") == 0) { *dest = 28; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_OPA") == 0) { *dest = 29; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_GRAD_DIR") == 0) { *dest = 32; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_MAIN_STOP") == 0) { *dest = 33; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_GRAD_STOP") == 0) { *dest = 34; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_GRAD_COLOR") == 0) { *dest = 35; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_MAIN_OPA") == 0) { *dest = 36; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_GRAD_OPA") == 0) { *dest = 37; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_GRAD") == 0) { *dest = 38; return true; }
        else if (strcmp(str_value, "LV_STYLE_BASE_DIR") == 0) { *dest = 39; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_IMAGE_SRC") == 0) { *dest = 40; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_IMAGE_OPA") == 0) { *dest = 41; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_IMAGE_RECOLOR") == 0) { *dest = 42; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_IMAGE_RECOLOR_OPA") == 0) { *dest = 43; return true; }
        else if (strcmp(str_value, "LV_STYLE_BG_IMAGE_TILED") == 0) { *dest = 44; return true; }
        else if (strcmp(str_value, "LV_STYLE_CLIP_CORNER") == 0) { *dest = 45; return true; }
        else if (strcmp(str_value, "LV_STYLE_BORDER_WIDTH") == 0) { *dest = 48; return true; }
        else if (strcmp(str_value, "LV_STYLE_BORDER_COLOR") == 0) { *dest = 49; return true; }
        else if (strcmp(str_value, "LV_STYLE_BORDER_OPA") == 0) { *dest = 50; return true; }
        else if (strcmp(str_value, "LV_STYLE_BORDER_SIDE") == 0) { *dest = 52; return true; }
        else if (strcmp(str_value, "LV_STYLE_BORDER_POST") == 0) { *dest = 53; return true; }
        else if (strcmp(str_value, "LV_STYLE_OUTLINE_WIDTH") == 0) { *dest = 56; return true; }
        else if (strcmp(str_value, "LV_STYLE_OUTLINE_COLOR") == 0) { *dest = 57; return true; }
        else if (strcmp(str_value, "LV_STYLE_OUTLINE_OPA") == 0) { *dest = 58; return true; }
        else if (strcmp(str_value, "LV_STYLE_OUTLINE_PAD") == 0) { *dest = 59; return true; }
        else if (strcmp(str_value, "LV_STYLE_SHADOW_WIDTH") == 0) { *dest = 60; return true; }
        else if (strcmp(str_value, "LV_STYLE_SHADOW_COLOR") == 0) { *dest = 61; return true; }
        else if (strcmp(str_value, "LV_STYLE_SHADOW_OPA") == 0) { *dest = 62; return true; }
        else if (strcmp(str_value, "LV_STYLE_SHADOW_OFFSET_X") == 0) { *dest = 64; return true; }
        else if (strcmp(str_value, "LV_STYLE_SHADOW_OFFSET_Y") == 0) { *dest = 65; return true; }
        else if (strcmp(str_value, "LV_STYLE_SHADOW_SPREAD") == 0) { *dest = 66; return true; }
        else if (strcmp(str_value, "LV_STYLE_IMAGE_OPA") == 0) { *dest = 68; return true; }
        else if (strcmp(str_value, "LV_STYLE_IMAGE_RECOLOR") == 0) { *dest = 69; return true; }
        else if (strcmp(str_value, "LV_STYLE_IMAGE_RECOLOR_OPA") == 0) { *dest = 70; return true; }
        else if (strcmp(str_value, "LV_STYLE_LINE_WIDTH") == 0) { *dest = 72; return true; }
        else if (strcmp(str_value, "LV_STYLE_LINE_DASH_WIDTH") == 0) { *dest = 73; return true; }
        else if (strcmp(str_value, "LV_STYLE_LINE_DASH_GAP") == 0) { *dest = 74; return true; }
        else if (strcmp(str_value, "LV_STYLE_LINE_ROUNDED") == 0) { *dest = 75; return true; }
        else if (strcmp(str_value, "LV_STYLE_LINE_COLOR") == 0) { *dest = 76; return true; }
        else if (strcmp(str_value, "LV_STYLE_LINE_OPA") == 0) { *dest = 77; return true; }
        else if (strcmp(str_value, "LV_STYLE_ARC_WIDTH") == 0) { *dest = 80; return true; }
        else if (strcmp(str_value, "LV_STYLE_ARC_ROUNDED") == 0) { *dest = 81; return true; }
        else if (strcmp(str_value, "LV_STYLE_ARC_COLOR") == 0) { *dest = 82; return true; }
        else if (strcmp(str_value, "LV_STYLE_ARC_OPA") == 0) { *dest = 83; return true; }
        else if (strcmp(str_value, "LV_STYLE_ARC_IMAGE_SRC") == 0) { *dest = 84; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_COLOR") == 0) { *dest = 88; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_OPA") == 0) { *dest = 89; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_FONT") == 0) { *dest = 90; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_LETTER_SPACE") == 0) { *dest = 91; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_LINE_SPACE") == 0) { *dest = 92; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_DECOR") == 0) { *dest = 93; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_ALIGN") == 0) { *dest = 94; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_OUTLINE_STROKE_WIDTH") == 0) { *dest = 95; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_OUTLINE_STROKE_OPA") == 0) { *dest = 96; return true; }
        else if (strcmp(str_value, "LV_STYLE_TEXT_OUTLINE_STROKE_COLOR") == 0) { *dest = 97; return true; }
        else if (strcmp(str_value, "LV_STYLE_OPA") == 0) { *dest = 98; return true; }
        else if (strcmp(str_value, "LV_STYLE_OPA_LAYERED") == 0) { *dest = 99; return true; }
        else if (strcmp(str_value, "LV_STYLE_COLOR_FILTER_DSC") == 0) { *dest = 100; return true; }
        else if (strcmp(str_value, "LV_STYLE_COLOR_FILTER_OPA") == 0) { *dest = 101; return true; }
        else if (strcmp(str_value, "LV_STYLE_ANIM") == 0) { *dest = 102; return true; }
        else if (strcmp(str_value, "LV_STYLE_ANIM_DURATION") == 0) { *dest = 103; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSITION") == 0) { *dest = 104; return true; }
        else if (strcmp(str_value, "LV_STYLE_BLEND_MODE") == 0) { *dest = 105; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_WIDTH") == 0) { *dest = 106; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_HEIGHT") == 0) { *dest = 107; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSLATE_X") == 0) { *dest = 108; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSLATE_Y") == 0) { *dest = 109; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_SCALE_X") == 0) { *dest = 110; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_SCALE_Y") == 0) { *dest = 111; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_ROTATION") == 0) { *dest = 112; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_PIVOT_X") == 0) { *dest = 113; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_PIVOT_Y") == 0) { *dest = 114; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_SKEW_X") == 0) { *dest = 115; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSFORM_SKEW_Y") == 0) { *dest = 116; return true; }
        else if (strcmp(str_value, "LV_STYLE_BITMAP_MASK_SRC") == 0) { *dest = 117; return true; }
        else if (strcmp(str_value, "LV_STYLE_ROTARY_SENSITIVITY") == 0) { *dest = 118; return true; }
        else if (strcmp(str_value, "LV_STYLE_TRANSLATE_RADIAL") == 0) { *dest = 119; return true; }
        else if (strcmp(str_value, "LV_STYLE_RECOLOR") == 0) { *dest = 120; return true; }
        else if (strcmp(str_value, "LV_STYLE_RECOLOR_OPA") == 0) { *dest = 121; return true; }
        else if (strcmp(str_value, "LV_STYLE_FLEX_FLOW") == 0) { *dest = 122; return true; }
        else if (strcmp(str_value, "LV_STYLE_FLEX_MAIN_PLACE") == 0) { *dest = 123; return true; }
        else if (strcmp(str_value, "LV_STYLE_FLEX_CROSS_PLACE") == 0) { *dest = 124; return true; }
        else if (strcmp(str_value, "LV_STYLE_FLEX_TRACK_PLACE") == 0) { *dest = 125; return true; }
        else if (strcmp(str_value, "LV_STYLE_FLEX_GROW") == 0) { *dest = 126; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_COLUMN_ALIGN") == 0) { *dest = 127; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_ROW_ALIGN") == 0) { *dest = 128; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_ROW_DSC_ARRAY") == 0) { *dest = 129; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_COLUMN_DSC_ARRAY") == 0) { *dest = 130; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_CELL_COLUMN_POS") == 0) { *dest = 131; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_CELL_COLUMN_SPAN") == 0) { *dest = 132; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_CELL_X_ALIGN") == 0) { *dest = 133; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_CELL_ROW_POS") == 0) { *dest = 134; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_CELL_ROW_SPAN") == 0) { *dest = 135; return true; }
        else if (strcmp(str_value, "LV_STYLE_GRID_CELL_Y_ALIGN") == 0) { *dest = 136; return true; }
        else if (strcmp(str_value, "LV_STYLE_LAST_BUILT_IN_PROP") == 0) { *dest = 137; return true; }
        else if (strcmp(str_value, "LV_STYLE_NUM_BUILT_IN_PROPS") == 0) { *dest = 138; return true; }
        else if (strcmp(str_value, "LV_STYLE_PROP_ANY") == 0) { *dest = 255; return true; }
        else if (strcmp(str_value, "LV_STYLE_PROP_CONST") == 0) { *dest = 255; return true; }
        LOG_ERR("Enum Unmarshal Error: Unknown value '%s' for enum type %s", str_value, enum_type_name);
        return false;
    }
    else if (strcmp(enum_type_name, "int") == 0) {
        if (strcmp(str_value, "LV_PART_TEXTAREA_PLACEHOLDER") == 0) { *dest = 524288; return true; }
        LOG_ERR("Enum Unmarshal Error: Unknown value '%s' for enum type %s", str_value, enum_type_name);
        return false;
    }
    else {
        LOG_ERR("Enum Unmarshal Error: Unknown enum type '%s'", enum_type_name);
        return false;
    }
}



// --- Primitive Unmarshalers ---
// --- Primitive Unmarshalers ---

static bool unmarshal_int(cJSON *node, int *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (int)node->valuedouble;
    return true;
}

static bool unmarshal_int8(cJSON *node, int8_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (int8_t)node->valuedouble;
    return true;
}

static bool unmarshal_uint8(cJSON *node, uint8_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (uint8_t)node->valuedouble;
    return true;
}

static bool unmarshal_int16(cJSON *node, int16_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (int16_t)node->valuedouble;
    return true;
}

static bool unmarshal_uint16(cJSON *node, uint16_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (uint16_t)node->valuedouble;
    return true;
}

static bool unmarshal_int32(cJSON *node, int32_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (int32_t)node->valuedouble;
    return true;
}

static bool unmarshal_uint32(cJSON *node, uint32_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (uint32_t)node->valuedouble;
    return true;
}

static bool unmarshal_int64(cJSON *node, int64_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (int64_t)node->valuedouble;
    return true;
}

static bool unmarshal_uint64(cJSON *node, uint64_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (uint64_t)node->valuedouble;
    return true;
}

static bool unmarshal_size_t(cJSON *node, size_t *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (size_t)node->valuedouble;
    return true;
}

static bool unmarshal_float(cJSON *node, float *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (float)node->valuedouble;
    return true;
}

static bool unmarshal_double(cJSON *node, double *dest) {
    if (!cJSON_IsNumber(node)) return false;
    *dest = (double)node->valuedouble;
    return true;
}

static bool unmarshal_bool(cJSON *node, bool *dest) {
    if (!cJSON_IsBool(node)) return false;
    *dest = cJSON_IsTrue(node);
    return true;
}

static bool unmarshal_string_ptr(cJSON *node, char **dest) {
    if (!cJSON_IsString(node)) return false;
    *dest = node->valuestring;
    return true;
}

static bool unmarshal_char(cJSON *node, char *dest) {
    if (cJSON_IsString(node) && node->valuestring && node->valuestring[0] != '\0') { *dest = node->valuestring[0]; return true; }
    if (cJSON_IsNumber(node)) { *dest = (char)node->valuedouble; return true; }
    return false;
}



// --- Custom Unmarshalers ---
// --- Custom Unmarshalers ---

static bool unmarshal_color(cJSON *node, lv_color_t *dest) {
    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '#') return false;
    const char *hex_str = node->valuestring + 1;
    uint32_t hex_val = (uint32_t)strtoul(hex_str, NULL, 16);
    if (strlen(hex_str) == 6) { *dest = lv_color_hex(hex_val); return true; }
    // Add support for other hex lengths? (e.g. #RGB, #AARRGGBB)
    LOG_ERR("Color Unmarshal Error: Invalid hex format '%s'", node->valuestring);
    return false;
}

// Forward declaration
extern void* lvgl_json_get_registered_ptr(const char *name);

static bool unmarshal_registered_ptr(cJSON *node, void **dest) {
    if (!cJSON_IsString(node) || !node->valuestring || node->valuestring[0] != '@') return false;
    const char *name = node->valuestring + 1;
    *dest = lvgl_json_get_registered_ptr(name);
    if (!(*dest)) {
       LOG_ERR("Pointer Unmarshal Error: Registered pointer '@%s' not found.", name);
       return false;
    }
    return true;
}



// --- Forward declaration needed by invocation helpers ---
static const invoke_table_entry_t* find_invoke_entry(const char *name);
static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest);

// --- Invocation Helper Functions ---
// Forward declaration for the main unmarshaler
static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest);

// --- Invocation Helper Functions ---
// Specific Invoker for functions like lv_widget_create(lv_obj_t *parent)
// Signature: expects target_obj_ptr = parent, dest = lv_obj_t**, args_array = NULL
static bool invoke_widget_create(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_widget_create"); return false; }
    if (!dest) { LOG_ERR("Invoke Error: dest is NULL for invoke_widget_create (needed for result)"); return false; }
    if (args_array != NULL && cJSON_GetArraySize(args_array) > 0) {
       LOG_WARN_JSON(args_array, "Invoke Warning: invoke_widget_create expected 0 JSON args, got %d. Ignoring JSON args.", cJSON_GetArraySize(args_array));
    }

    lv_obj_t* parent = (lv_obj_t*)target_obj_ptr;
    // Define the specific function pointer type
    typedef lv_obj_t* (*specific_lv_create_func_type)(lv_obj_t*);
    specific_lv_create_func_type target_func = (specific_lv_create_func_type)func_ptr;

    // Call the target LVGL create function
    lv_obj_t* result = target_func(parent);

    // Store result widget pointer into *dest
    *(lv_obj_t**)dest = result;

    if (!result) {
        // The name of the actual function isn't known here easily, maybe log func_ptr address?
        LOG_WARN("Invoke Warning: invoke_widget_create (func ptr %p) returned NULL.", func_ptr);
        // Return true because the invoker itself succeeded, even if LVGL func failed.
        // The renderer should check the returned object pointer.
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL',)
// Representative LVGL func: lv_is_initialized (bool())
static bool invoke_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL"); return false; }
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'INT')
// Representative LVGL func: lv_color_format_has_alpha (bool(lv_color_format_t))
static bool invoke_BOOL_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_INT"); return false; }
    lv_color_format_t arg0;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_color_format_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color_format_t)
    if (!(unmarshal_value(json_arg0, "lv_color_format_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_format_t) for invoke_BOOL_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'INT', 'INT')
// Representative LVGL func: lv_color32_eq (bool(lv_color32_t, lv_color32_t))
static bool invoke_BOOL_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_INT_INT"); return false; }
    lv_color32_t arg0;
    lv_color32_t arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_color32_t, lv_color32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_BOOL_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color32_t)
    if (!(unmarshal_value(json_arg0, "lv_color32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color32_t) for invoke_BOOL_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_color32_t)
    if (!(unmarshal_value(json_arg1, "lv_color32_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_color32_t) for invoke_BOOL_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'const char *', 'const char *')
// Representative LVGL func: lv_streq (bool(char *, char *))
static bool invoke_BOOL_const_char_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_const_char_p_const_char_p"); return false; }
    char * arg0;
    char * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(char *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_const_char_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_BOOL_const_char_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_BOOL_const_char_p_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_BOOL_const_char_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_array_t *')
// Representative LVGL func: lv_array_is_empty (bool(lv_array_t *))
static bool invoke_BOOL_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_array_t_p"); return false; }
    lv_array_t * arg0;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_BOOL_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_array_t *', 'INT')
// Representative LVGL func: lv_array_resize (bool(lv_array_t *, uint32_t))
static bool invoke_BOOL_lv_array_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_array_t_p_INT"); return false; }
    lv_array_t * arg0;
    uint32_t arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_array_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_array_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_array_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_array_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_BOOL_lv_array_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_circle_buf_t *')
// Representative LVGL func: lv_circle_buf_is_empty (bool(lv_circle_buf_t *))
static bool invoke_BOOL_lv_circle_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_circle_buf_t_p"); return false; }
    lv_circle_buf_t * arg0;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_circle_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_circle_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_BOOL_lv_circle_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_color_t', 'lv_color_t')
// Representative LVGL func: lv_color_eq (bool(lv_color_t, lv_color_t))
static bool invoke_BOOL_lv_color_t_lv_color_t(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_color_t_lv_color_t"); return false; }
    lv_color_t arg0;
    lv_color_t arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_color_t, lv_color_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_color_t_lv_color_t"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_BOOL_lv_color_t_lv_color_t", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_color_t_lv_color_t"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_BOOL_lv_color_t_lv_color_t");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_color_t_lv_color_t"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg1, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_color_t) for invoke_BOOL_lv_color_t_lv_color_t");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_font_info_t *', 'lv_font_info_t *')
// Representative LVGL func: lv_font_info_is_equal (bool(lv_font_info_t *, lv_font_info_t *))
static bool invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p"); return false; }
    lv_font_info_t * arg0;
    lv_font_info_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_font_info_t *, lv_font_info_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_info_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_font_info_t *)
    if (!(unmarshal_value(json_arg0, "lv_font_info_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_font_info_t *) for invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_font_t *', 'lv_font_glyph_dsc_t *', 'INT', 'INT')
// Representative LVGL func: lv_font_get_glyph_dsc (bool(lv_font_t *, lv_font_glyph_dsc_t *, uint32_t, uint32_t))
static bool invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT"); return false; }
    lv_font_t * arg0;
    lv_font_glyph_dsc_t * arg1;
    uint32_t arg2;
    uint32_t arg3;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_font_t *, lv_font_glyph_dsc_t *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_font_glyph_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_font_glyph_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_font_glyph_dsc_t *) for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_ll_t *')
// Representative LVGL func: lv_ll_is_empty (bool(lv_ll_t *))
static bool invoke_BOOL_lv_ll_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_ll_t_p"); return false; }
    lv_ll_t * arg0;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_ll_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_ll_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_BOOL_lv_ll_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *')
// Representative LVGL func: lv_obj_refr_size (bool(lv_obj_t *))
static bool invoke_BOOL_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_BOOL_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_bg_image_tiled (bool(lv_obj_t *, lv_part_t))
static bool invoke_BOOL_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_BOOL_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'INT', 'INT')
// Representative LVGL func: lv_obj_has_style_prop (bool(lv_obj_t *, lv_style_selector_t, lv_style_prop_t))
static bool invoke_BOOL_lv_obj_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_style_selector_t arg1;
    lv_style_prop_t arg2;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_style_selector_t, lv_style_prop_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_BOOL_lv_obj_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg0, "lv_style_selector_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_selector_t) for invoke_BOOL_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_prop_t)
    if (!(unmarshal_value(json_arg1, "lv_style_prop_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_prop_t) for invoke_BOOL_lv_obj_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_table_has_cell_ctrl (bool(lv_obj_t *, uint32_t, uint32_t, lv_table_cell_ctrl_t))
static bool invoke_BOOL_lv_obj_t_p_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    uint32_t arg2;
    lv_table_cell_ctrl_t arg3;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, uint32_t, uint32_t, lv_table_cell_ctrl_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_BOOL_lv_obj_t_p_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_BOOL_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_BOOL_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_BOOL_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_table_cell_ctrl_t)
    if (!(unmarshal_value(json_arg2, "lv_table_cell_ctrl_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_table_cell_ctrl_t) for invoke_BOOL_lv_obj_t_p_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'const char *', 'INT')
// Representative LVGL func: lv_roller_set_selected_str (bool(lv_obj_t *, char *, lv_anim_enable_t))
static bool invoke_BOOL_lv_obj_t_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_const_char_p_INT"); return false; }
    lv_obj_t * arg0;
    char * arg1;
    lv_anim_enable_t arg2;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, char *, lv_anim_enable_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_BOOL_lv_obj_t_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_BOOL_lv_obj_t_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_obj_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_anim_enable_t)
    if (!(unmarshal_value(json_arg1, "lv_anim_enable_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_anim_enable_t) for invoke_BOOL_lv_obj_t_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'lv_area_t *')
// Representative LVGL func: lv_obj_area_is_visible (bool(lv_obj_t *, lv_area_t *))
static bool invoke_BOOL_lv_obj_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_lv_area_t_p"); return false; }
    lv_obj_t * arg0;
    lv_area_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_obj_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_BOOL_lv_obj_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'lv_event_dsc_t *')
// Representative LVGL func: lv_obj_remove_event_dsc (bool(lv_obj_t *, lv_event_dsc_t *))
static bool invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_event_dsc_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_event_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_event_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_event_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_event_dsc_t *) for invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'lv_obj_class_t *')
// Representative LVGL func: lv_obj_check_type (bool(lv_obj_t *, lv_obj_class_t *))
static bool invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_class_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_obj_class_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_class_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_class_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_class_t *) for invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'lv_obj_t *')
// Representative LVGL func: lv_menu_back_button_is_root (bool(lv_obj_t *, lv_obj_t *))
static bool invoke_BOOL_lv_obj_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_obj_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_lv_obj_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_BOOL_lv_obj_t_p_lv_obj_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'lv_point_t *')
// Representative LVGL func: lv_obj_hit_test (bool(lv_obj_t *, lv_point_t *))
static bool invoke_BOOL_lv_obj_t_p_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_lv_point_t_p"); return false; }
    lv_obj_t * arg0;
    lv_point_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_obj_t_p_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_BOOL_lv_obj_t_p_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_obj_t *', 'lv_style_t *', 'lv_style_t *', 'INT')
// Representative LVGL func: lv_obj_replace_style (bool(lv_obj_t *, lv_style_t *, lv_style_t *, lv_style_selector_t))
static bool invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_style_t * arg1;
    lv_style_t * arg2;
    lv_style_selector_t arg3;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_obj_t *, lv_style_t *, lv_style_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_t *)
    if (!(unmarshal_value(json_arg0, "lv_style_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_t *) for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_t *)
    if (!(unmarshal_value(json_arg1, "lv_style_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_t *) for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg2, "lv_style_selector_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_style_selector_t) for invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_rb_t *', 'INT', 'INT')
// Representative LVGL func: lv_rb_init (bool(lv_rb_t *, lv_rb_compare_t, size_t))
static bool invoke_BOOL_lv_rb_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_rb_t_p_INT_INT"); return false; }
    lv_rb_t * arg0;
    lv_rb_compare_t arg1;
    size_t arg2;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_rb_t *, lv_rb_compare_t, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_rb_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_BOOL_lv_rb_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_rb_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_rb_compare_t)
    if (!(unmarshal_value(json_arg0, "lv_rb_compare_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_rb_compare_t) for invoke_BOOL_lv_rb_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_rb_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_BOOL_lv_rb_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_rb_t *', 'POINTER')
// Representative LVGL func: lv_rb_drop (bool(lv_rb_t *, void *))
static bool invoke_BOOL_lv_rb_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_rb_t_p_POINTER"); return false; }
    lv_rb_t * arg0;
    void * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_rb_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_rb_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_rb_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_rb_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_BOOL_lv_rb_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_rb_t *', 'lv_rb_node_t *')
// Representative LVGL func: lv_rb_drop_node (bool(lv_rb_t *, lv_rb_node_t *))
static bool invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p"); return false; }
    lv_rb_t * arg0;
    lv_rb_node_t * arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_rb_t *, lv_rb_node_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_rb_node_t *)
    if (!(unmarshal_value(json_arg0, "lv_rb_node_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_rb_node_t *) for invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_style_t *')
// Representative LVGL func: lv_style_is_const (bool(lv_style_t *))
static bool invoke_BOOL_lv_style_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_style_t_p"); return false; }
    lv_style_t * arg0;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_style_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_style_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_BOOL_lv_style_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_style_t *', 'INT')
// Representative LVGL func: lv_style_remove_prop (bool(lv_style_t *, lv_style_prop_t))
static bool invoke_BOOL_lv_style_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_style_t_p_INT"); return false; }
    lv_style_t * arg0;
    lv_style_prop_t arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_style_t *, lv_style_prop_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_style_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_style_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_prop_t)
    if (!(unmarshal_value(json_arg0, "lv_style_prop_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_prop_t) for invoke_BOOL_lv_style_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_text_cmd_state_t *', 'INT')
// Representative LVGL func: lv_text_is_cmd (bool(lv_text_cmd_state_t *, uint32_t))
static bool invoke_BOOL_lv_text_cmd_state_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_text_cmd_state_t_p_INT"); return false; }
    lv_text_cmd_state_t * arg0;
    uint32_t arg1;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_text_cmd_state_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_text_cmd_state_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_text_cmd_state_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_BOOL_lv_text_cmd_state_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_text_cmd_state_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_BOOL_lv_text_cmd_state_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('BOOL', 'lv_tree_node_t *', 'INT', 'INT', 'INT', 'INT', 'POINTER')
// Representative LVGL func: lv_tree_walk (bool(lv_tree_node_t *, lv_tree_walk_mode_t, lv_tree_traverse_cb_t, lv_tree_before_cb_t, lv_tree_after_cb_t, void *))
static bool invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    lv_tree_node_t * arg0;
    lv_tree_walk_mode_t arg1;
    lv_tree_traverse_cb_t arg2;
    lv_tree_before_cb_t arg3;
    lv_tree_after_cb_t arg4;
    void * arg5;
    bool result;

    // Define specific function pointer type based on representative function
    typedef bool (*specific_lv_func_type)(lv_tree_node_t *, lv_tree_walk_mode_t, lv_tree_traverse_cb_t, lv_tree_before_cb_t, lv_tree_after_cb_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_tree_node_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 5 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (5 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 5) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 5 JSON args, got %d for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_tree_walk_mode_t)
    if (!(unmarshal_value(json_arg0, "lv_tree_walk_mode_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_tree_walk_mode_t) for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_tree_traverse_cb_t)
    if (!(unmarshal_value(json_arg1, "lv_tree_traverse_cb_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_tree_traverse_cb_t) for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_tree_before_cb_t)
    if (!(unmarshal_value(json_arg2, "lv_tree_before_cb_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_tree_before_cb_t) for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type lv_tree_after_cb_t)
    if (!(unmarshal_value(json_arg3, "lv_tree_after_cb_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type lv_tree_after_cb_t) for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 4 into C arg 5 (type void *)
    if (!(unmarshal_value(json_arg4, "void *", &arg5))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type void *) for invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3, arg4, arg5);

    // Store result if dest is provided
    if (dest) {
        *((bool *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT',)
// Representative LVGL func: lv_mem_test_core (lv_result_t())
static bool invoke_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT"); return false; }
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'INT')
// Representative LVGL func: lv_tick_elaps (uint32_t(uint32_t))
static bool invoke_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_INT"); return false; }
    uint32_t arg0;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'INT', 'INT')
// Representative LVGL func: lv_atan2 (uint16_t(int, int))
static bool invoke_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_INT_INT"); return false; }
    int arg0;
    int arg1;
    uint16_t result;

    // Define specific function pointer type based on representative function
    typedef uint16_t (*specific_lv_func_type)(int, int);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type int)
    if (!(unmarshal_value(json_arg0, "int", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int) for invoke_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type int)
    if (!(unmarshal_value(json_arg1, "int", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int) for invoke_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((uint16_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_color_16_16_mix (uint16_t(uint16_t, uint16_t, uint8_t))
static bool invoke_INT_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_INT_INT_INT"); return false; }
    uint16_t arg0;
    uint16_t arg1;
    uint8_t arg2;
    uint16_t result;

    // Define specific function pointer type based on representative function
    typedef uint16_t (*specific_lv_func_type)(uint16_t, uint16_t, uint8_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint16_t)
    if (!(unmarshal_value(json_arg0, "uint16_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint16_t) for invoke_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint16_t)
    if (!(unmarshal_value(json_arg1, "uint16_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint16_t) for invoke_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type uint8_t)
    if (!(unmarshal_value(json_arg2, "uint8_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint8_t) for invoke_INT_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((uint16_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'INT', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_color32_make (lv_color32_t(uint8_t, uint8_t, uint8_t, uint8_t))
static bool invoke_INT_INT_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_INT_INT_INT_INT"); return false; }
    uint8_t arg0;
    uint8_t arg1;
    uint8_t arg2;
    uint8_t arg3;
    lv_color32_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color32_t (*specific_lv_func_type)(uint8_t, uint8_t, uint8_t, uint8_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_INT_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_INT_INT_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint8_t)
    if (!(unmarshal_value(json_arg0, "uint8_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint8_t) for invoke_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint8_t)
    if (!(unmarshal_value(json_arg1, "uint8_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint8_t) for invoke_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type uint8_t)
    if (!(unmarshal_value(json_arg2, "uint8_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint8_t) for invoke_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 3 (type uint8_t)
    if (!(unmarshal_value(json_arg3, "uint8_t", &arg3))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type uint8_t) for invoke_INT_INT_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_color32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'INT', 'INT', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_cubic_bezier (int32_t(int32_t, int32_t, int32_t, int32_t, int32_t))
static bool invoke_INT_INT_INT_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    int32_t arg0;
    int32_t arg1;
    int32_t arg2;
    int32_t arg3;
    int32_t arg4;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(int32_t, int32_t, int32_t, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 5 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (5 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 5) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 5 JSON args, got %d for invoke_INT_INT_INT_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 4 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg4, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type int32_t) for invoke_INT_INT_INT_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3, arg4);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'INT', 'POINTER')
// Representative LVGL func: lv_async_call (lv_result_t(lv_async_cb_t, void *))
static bool invoke_INT_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_INT_POINTER"); return false; }
    lv_async_cb_t arg0;
    void * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_async_cb_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_async_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_async_cb_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_async_cb_t) for invoke_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_INT_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'POINTER')
// Representative LVGL func: lv_color24_luminance (uint8_t(uint8_t *))
static bool invoke_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_POINTER"); return false; }
    uint8_t * arg0;
    uint8_t result;

    // Define specific function pointer type based on representative function
    typedef uint8_t (*specific_lv_func_type)(uint8_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint8_t *)
    if (!(unmarshal_value(json_arg0, "uint8_t *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint8_t *) for invoke_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint8_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'POINTER', 'INT')
// Representative LVGL func: lv_mem_add_pool (lv_mem_pool_t(void *, size_t))
static bool invoke_INT_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_POINTER_INT"); return false; }
    void * arg0;
    size_t arg1;
    lv_mem_pool_t result;

    // Define specific function pointer type based on representative function
    typedef lv_mem_pool_t (*specific_lv_func_type)(void *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_INT_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_mem_pool_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'POINTER', 'POINTER', 'INT')
// Representative LVGL func: lv_memcmp (int(void *, void *, size_t))
static bool invoke_INT_POINTER_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_POINTER_POINTER_INT"); return false; }
    void * arg0;
    void * arg1;
    size_t arg2;
    int result;

    // Define specific function pointer type based on representative function
    typedef int (*specific_lv_func_type)(void *, void *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_POINTER_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_POINTER_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_INT_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_INT_POINTER_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((int *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'POINTER', 'lv_image_header_t *')
// Representative LVGL func: lv_image_decoder_get_info (lv_result_t(void *, lv_image_header_t *))
static bool invoke_INT_POINTER_lv_image_header_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_POINTER_lv_image_header_t_p"); return false; }
    void * arg0;
    lv_image_header_t * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(void *, lv_image_header_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_POINTER_lv_image_header_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_POINTER_lv_image_header_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_POINTER_lv_image_header_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_POINTER_lv_image_header_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_POINTER_lv_image_header_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_image_header_t *)
    if (!(unmarshal_value(json_arg1, "lv_image_header_t *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_image_header_t *) for invoke_INT_POINTER_lv_image_header_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *')
// Representative LVGL func: lv_strlen (size_t(char *))
static bool invoke_INT_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p"); return false; }
    char * arg0;
    size_t result;

    // Define specific function pointer type based on representative function
    typedef size_t (*specific_lv_func_type)(char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((size_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *', 'INT')
// Representative LVGL func: lv_strnlen (size_t(char *, size_t))
static bool invoke_INT_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p_INT"); return false; }
    char * arg0;
    size_t arg1;
    size_t result;

    // Define specific function pointer type based on representative function
    typedef size_t (*specific_lv_func_type)(char *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_INT_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((size_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *', 'INT', 'const char *', 'UNKNOWN')
// Representative LVGL func: lv_vsnprintf (int(char *, size_t, char *, va_list))
static bool invoke_INT_const_char_p_INT_const_char_p_UNKNOWN(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN"); return false; }
    char * arg0;
    size_t arg1;
    char * arg2;
    va_list arg3;
    int result;

    // Define specific function pointer type based on representative function
    typedef int (*specific_lv_func_type)(char *, size_t, char *, va_list);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg2, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type char *) for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN"); return false; }
    // Unmarshal JSON arg 3 into C arg 3 (type va_list)
    if (!(unmarshal_value(json_arg3, "va_list", &arg3))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type va_list) for invoke_INT_const_char_p_INT_const_char_p_UNKNOWN");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((int *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *', 'INT', 'lv_font_t *', 'INT')
// Representative LVGL func: lv_text_get_width (int32_t(char *, uint32_t, lv_font_t *, int32_t))
static bool invoke_INT_const_char_p_INT_lv_font_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p_INT_lv_font_t_p_INT"); return false; }
    char * arg0;
    uint32_t arg1;
    lv_font_t * arg2;
    int32_t arg3;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(char *, uint32_t, lv_font_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p_INT_lv_font_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_INT_const_char_p_INT_lv_font_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p_INT_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p_INT_lv_font_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_const_char_p_INT_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_INT_const_char_p_INT_lv_font_t_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_const_char_p_INT_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type lv_font_t *)
    if (!(unmarshal_value(json_arg2, "lv_font_t *", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_font_t *) for invoke_INT_const_char_p_INT_lv_font_t_p_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_INT_const_char_p_INT_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_INT_const_char_p_INT_lv_font_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *', 'INT', 'lv_font_t *', 'INT', 'INT')
// Representative LVGL func: lv_text_get_width_with_flags (int32_t(char *, uint32_t, lv_font_t *, int32_t, lv_text_flag_t))
static bool invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    char * arg0;
    uint32_t arg1;
    lv_font_t * arg2;
    int32_t arg3;
    lv_text_flag_t arg4;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(char *, uint32_t, lv_font_t *, int32_t, lv_text_flag_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 5 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (5 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 5) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 5 JSON args, got %d for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type lv_font_t *)
    if (!(unmarshal_value(json_arg2, "lv_font_t *", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_font_t *) for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 4 into C arg 4 (type lv_text_flag_t)
    if (!(unmarshal_value(json_arg4, "lv_text_flag_t", &arg4))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type lv_text_flag_t) for invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3, arg4);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *', 'const char *')
// Representative LVGL func: lv_strcmp (int(char *, char *))
static bool invoke_INT_const_char_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p_const_char_p"); return false; }
    char * arg0;
    char * arg1;
    int result;

    // Define specific function pointer type based on representative function
    typedef int (*specific_lv_func_type)(char *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_const_char_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_INT_const_char_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((int *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'const char *', 'const char *', 'INT')
// Representative LVGL func: lv_strlcpy (size_t(char *, char *, size_t))
static bool invoke_INT_const_char_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_const_char_p_const_char_p_INT"); return false; }
    char * arg0;
    char * arg1;
    size_t arg2;
    size_t result;

    // Define specific function pointer type based on representative function
    typedef size_t (*specific_lv_func_type)(char *, char *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_const_char_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_const_char_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_const_char_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_INT_const_char_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_INT_const_char_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((size_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_area_t *')
// Representative LVGL func: lv_area_get_width (int32_t(lv_area_t *))
static bool invoke_INT_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_area_t_p"); return false; }
    lv_area_t * arg0;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_area_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_array_t *')
// Representative LVGL func: lv_array_size (uint32_t(lv_array_t *))
static bool invoke_INT_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_array_t_p"); return false; }
    lv_array_t * arg0;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_array_t *', 'INT')
// Representative LVGL func: lv_array_remove (lv_result_t(lv_array_t *, uint32_t))
static bool invoke_INT_lv_array_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_array_t_p_INT"); return false; }
    lv_array_t * arg0;
    uint32_t arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_array_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_array_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_array_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_array_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_array_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_array_t *', 'INT', 'INT')
// Representative LVGL func: lv_array_erase (lv_result_t(lv_array_t *, uint32_t, uint32_t))
static bool invoke_INT_lv_array_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_array_t_p_INT_INT"); return false; }
    lv_array_t * arg0;
    uint32_t arg1;
    uint32_t arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_array_t *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_array_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_array_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_array_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_array_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_array_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_INT_lv_array_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_array_t *', 'INT', 'POINTER')
// Representative LVGL func: lv_array_assign (lv_result_t(lv_array_t *, uint32_t, void *))
static bool invoke_INT_lv_array_t_p_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_array_t_p_INT_POINTER"); return false; }
    lv_array_t * arg0;
    uint32_t arg1;
    void * arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_array_t *, uint32_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_array_t_p_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_array_t_p_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_array_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_array_t_p_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_array_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_INT_lv_array_t_p_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_array_t *', 'POINTER')
// Representative LVGL func: lv_array_push_back (lv_result_t(lv_array_t *, void *))
static bool invoke_INT_lv_array_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_array_t_p_POINTER"); return false; }
    lv_array_t * arg0;
    void * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_array_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_array_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_array_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_array_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_lv_array_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_array_t *', 'lv_array_t *')
// Representative LVGL func: lv_array_concat (lv_result_t(lv_array_t *, lv_array_t *))
static bool invoke_INT_lv_array_t_p_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_array_t_p_lv_array_t_p"); return false; }
    lv_array_t * arg0;
    lv_array_t * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_array_t *, lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_array_t_p_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_array_t_p_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_array_t_p_lv_array_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_array_t *)
    if (!(unmarshal_value(json_arg0, "lv_array_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_array_t *) for invoke_INT_lv_array_t_p_lv_array_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_circle_buf_t *')
// Representative LVGL func: lv_circle_buf_size (uint32_t(lv_circle_buf_t *))
static bool invoke_INT_lv_circle_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_circle_buf_t_p"); return false; }
    lv_circle_buf_t * arg0;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_circle_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_circle_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_circle_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_circle_buf_t *', 'INT')
// Representative LVGL func: lv_circle_buf_resize (lv_result_t(lv_circle_buf_t *, uint32_t))
static bool invoke_INT_lv_circle_buf_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_circle_buf_t_p_INT"); return false; }
    lv_circle_buf_t * arg0;
    uint32_t arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_circle_buf_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_circle_buf_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_circle_buf_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_circle_buf_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_circle_buf_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_circle_buf_t *', 'INT', 'INT', 'POINTER')
// Representative LVGL func: lv_circle_buf_fill (uint32_t(lv_circle_buf_t *, uint32_t, lv_circle_buf_fill_cb_t, void *))
static bool invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER"); return false; }
    lv_circle_buf_t * arg0;
    uint32_t arg1;
    lv_circle_buf_fill_cb_t arg2;
    void * arg3;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_circle_buf_t *, uint32_t, lv_circle_buf_fill_cb_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_circle_buf_fill_cb_t)
    if (!(unmarshal_value(json_arg1, "lv_circle_buf_fill_cb_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_circle_buf_fill_cb_t) for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_circle_buf_t *', 'INT', 'POINTER')
// Representative LVGL func: lv_circle_buf_peek_at (lv_result_t(lv_circle_buf_t *, uint32_t, void *))
static bool invoke_INT_lv_circle_buf_t_p_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_circle_buf_t_p_INT_POINTER"); return false; }
    lv_circle_buf_t * arg0;
    uint32_t arg1;
    void * arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_circle_buf_t *, uint32_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_circle_buf_t_p_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_circle_buf_t_p_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_circle_buf_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_circle_buf_t_p_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_circle_buf_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_INT_lv_circle_buf_t_p_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_circle_buf_t *', 'POINTER')
// Representative LVGL func: lv_circle_buf_read (lv_result_t(lv_circle_buf_t *, void *))
static bool invoke_INT_lv_circle_buf_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_circle_buf_t_p_POINTER"); return false; }
    lv_circle_buf_t * arg0;
    void * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_circle_buf_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_circle_buf_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_circle_buf_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_circle_buf_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_lv_circle_buf_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_color_t')
// Representative LVGL func: lv_color_to_int (uint32_t(lv_color_t))
static bool invoke_INT_lv_color_t(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_color_t"); return false; }
    lv_color_t arg0;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_color_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_color_t"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_color_t", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_color_t"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_INT_lv_color_t");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_color_t', 'INT')
// Representative LVGL func: lv_color_to_32 (lv_color32_t(lv_color_t, lv_opa_t))
static bool invoke_INT_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_color_t_INT"); return false; }
    lv_color_t arg0;
    lv_opa_t arg1;
    lv_color32_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color32_t (*specific_lv_func_type)(lv_color_t, lv_opa_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_INT_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_opa_t)
    if (!(unmarshal_value(json_arg1, "lv_opa_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_opa_t) for invoke_INT_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_color32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_font_t *')
// Representative LVGL func: lv_font_get_line_height (int32_t(lv_font_t *))
static bool invoke_INT_lv_font_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_font_t_p"); return false; }
    lv_font_t * arg0;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(lv_font_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_font_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_font_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_font_t *', 'INT', 'INT')
// Representative LVGL func: lv_font_get_glyph_width (uint16_t(lv_font_t *, uint32_t, uint32_t))
static bool invoke_INT_lv_font_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_font_t_p_INT_INT"); return false; }
    lv_font_t * arg0;
    uint32_t arg1;
    uint32_t arg2;
    uint16_t result;

    // Define specific function pointer type based on representative function
    typedef uint16_t (*specific_lv_func_type)(lv_font_t *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_font_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_font_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_font_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_font_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_INT_lv_font_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((uint16_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_dir_t *')
// Representative LVGL func: lv_fs_dir_close (lv_fs_res_t(lv_fs_dir_t *))
static bool invoke_INT_lv_fs_dir_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_dir_t_p"); return false; }
    lv_fs_dir_t * arg0;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_dir_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_dir_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_dir_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_fs_dir_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_dir_t *', 'const char *')
// Representative LVGL func: lv_fs_dir_open (lv_fs_res_t(lv_fs_dir_t *, char *))
static bool invoke_INT_lv_fs_dir_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_dir_t_p_const_char_p"); return false; }
    lv_fs_dir_t * arg0;
    char * arg1;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_dir_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_dir_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_dir_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_fs_dir_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_fs_dir_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_lv_fs_dir_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_dir_t *', 'const char *', 'INT')
// Representative LVGL func: lv_fs_dir_read (lv_fs_res_t(lv_fs_dir_t *, char *, uint32_t))
static bool invoke_INT_lv_fs_dir_t_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_dir_t_p_const_char_p_INT"); return false; }
    lv_fs_dir_t * arg0;
    char * arg1;
    uint32_t arg2;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_dir_t *, char *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_dir_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_dir_t_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_fs_dir_t_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_fs_dir_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_lv_fs_dir_t_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_fs_dir_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_INT_lv_fs_dir_t_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_file_t *')
// Representative LVGL func: lv_fs_close (lv_fs_res_t(lv_fs_file_t *))
static bool invoke_INT_lv_fs_file_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_file_t_p"); return false; }
    lv_fs_file_t * arg0;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_file_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_file_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_file_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_fs_file_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_file_t *', 'INT', 'INT')
// Representative LVGL func: lv_fs_seek (lv_fs_res_t(lv_fs_file_t *, uint32_t, lv_fs_whence_t))
static bool invoke_INT_lv_fs_file_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_file_t_p_INT_INT"); return false; }
    lv_fs_file_t * arg0;
    uint32_t arg1;
    lv_fs_whence_t arg2;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_file_t *, uint32_t, lv_fs_whence_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_file_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_file_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_fs_file_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_fs_file_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_INT_lv_fs_file_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_fs_file_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_fs_whence_t)
    if (!(unmarshal_value(json_arg1, "lv_fs_whence_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_fs_whence_t) for invoke_INT_lv_fs_file_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_file_t *', 'POINTER')
// Representative LVGL func: lv_fs_tell (lv_fs_res_t(lv_fs_file_t *, uint32_t *))
static bool invoke_INT_lv_fs_file_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_file_t_p_POINTER"); return false; }
    lv_fs_file_t * arg0;
    uint32_t * arg1;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_file_t *, uint32_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_file_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_file_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_fs_file_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_fs_file_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t *)
    if (!(unmarshal_value(json_arg0, "uint32_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t *) for invoke_INT_lv_fs_file_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_file_t *', 'POINTER', 'INT', 'POINTER')
// Representative LVGL func: lv_fs_read (lv_fs_res_t(lv_fs_file_t *, void *, uint32_t, uint32_t *))
static bool invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER"); return false; }
    lv_fs_file_t * arg0;
    void * arg1;
    uint32_t arg2;
    uint32_t * arg3;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_file_t *, void *, uint32_t, uint32_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_file_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t *)
    if (!(unmarshal_value(json_arg2, "uint32_t *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t *) for invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_fs_file_t *', 'const char *', 'INT')
// Representative LVGL func: lv_fs_open (lv_fs_res_t(lv_fs_file_t *, char *, lv_fs_mode_t))
static bool invoke_INT_lv_fs_file_t_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_fs_file_t_p_const_char_p_INT"); return false; }
    lv_fs_file_t * arg0;
    char * arg1;
    lv_fs_mode_t arg2;
    lv_fs_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_res_t (*specific_lv_func_type)(lv_fs_file_t *, char *, lv_fs_mode_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_file_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_fs_file_t_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_fs_file_t_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_fs_file_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_lv_fs_file_t_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_fs_file_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_fs_mode_t)
    if (!(unmarshal_value(json_arg1, "lv_fs_mode_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_fs_mode_t) for invoke_INT_lv_fs_file_t_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_image_decoder_dsc_t *', 'POINTER', 'lv_image_decoder_args_t *')
// Representative LVGL func: lv_image_decoder_open (lv_result_t(lv_image_decoder_dsc_t *, void *, lv_image_decoder_args_t *))
static bool invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p"); return false; }
    lv_image_decoder_dsc_t * arg0;
    void * arg1;
    lv_image_decoder_args_t * arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_image_decoder_dsc_t *, void *, lv_image_decoder_args_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_image_decoder_args_t *)
    if (!(unmarshal_value(json_arg1, "lv_image_decoder_args_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_image_decoder_args_t *) for invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_image_decoder_dsc_t *', 'lv_area_t *', 'lv_area_t *')
// Representative LVGL func: lv_image_decoder_get_area (lv_result_t(lv_image_decoder_dsc_t *, lv_area_t *, lv_area_t *))
static bool invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    lv_image_decoder_dsc_t * arg0;
    lv_area_t * arg1;
    lv_area_t * arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_image_decoder_dsc_t *, lv_area_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_area_t *)
    if (!(unmarshal_value(json_arg1, "lv_area_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_area_t *) for invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_image_decoder_t *', 'lv_image_decoder_dsc_t *')
// Representative LVGL func: lv_bin_decoder_open (lv_result_t(lv_image_decoder_t *, lv_image_decoder_dsc_t *))
static bool invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_decoder_dsc_t * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_image_decoder_t *, lv_image_decoder_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_decoder_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_image_decoder_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_decoder_dsc_t *) for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_image_decoder_t *', 'lv_image_decoder_dsc_t *', 'lv_area_t *', 'lv_area_t *')
// Representative LVGL func: lv_bin_decoder_get_area (lv_result_t(lv_image_decoder_t *, lv_image_decoder_dsc_t *, lv_area_t *, lv_area_t *))
static bool invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_decoder_dsc_t * arg1;
    lv_area_t * arg2;
    lv_area_t * arg3;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_image_decoder_t *, lv_image_decoder_dsc_t *, lv_area_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_decoder_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_image_decoder_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_decoder_dsc_t *) for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_area_t *)
    if (!(unmarshal_value(json_arg1, "lv_area_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_area_t *) for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_area_t *)
    if (!(unmarshal_value(json_arg2, "lv_area_t *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_area_t *) for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_image_decoder_t *', 'lv_image_decoder_dsc_t *', 'lv_image_header_t *')
// Representative LVGL func: lv_bin_decoder_info (lv_result_t(lv_image_decoder_t *, lv_image_decoder_dsc_t *, lv_image_header_t *))
static bool invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_decoder_dsc_t * arg1;
    lv_image_header_t * arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_image_decoder_t *, lv_image_decoder_dsc_t *, lv_image_header_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_decoder_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_image_decoder_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_decoder_dsc_t *) for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_image_header_t *)
    if (!(unmarshal_value(json_arg1, "lv_image_header_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_image_header_t *) for invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_ll_t *')
// Representative LVGL func: lv_ll_get_len (uint32_t(lv_ll_t *))
static bool invoke_INT_lv_ll_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_ll_t_p"); return false; }
    lv_ll_t * arg0;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_ll_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_ll_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_ll_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_class_t *', 'lv_event_t *')
// Representative LVGL func: lv_obj_event_base (lv_result_t(lv_obj_class_t *, lv_event_t *))
static bool invoke_INT_lv_obj_class_t_p_lv_event_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_class_t_p_lv_event_t_p"); return false; }
    lv_obj_class_t * arg0;
    lv_event_t * arg1;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_obj_class_t *, lv_event_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_class_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_class_t_p_lv_event_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_class_t_p_lv_event_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_class_t_p_lv_event_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_event_t *)
    if (!(unmarshal_value(json_arg0, "lv_event_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_event_t *) for invoke_INT_lv_obj_class_t_p_lv_event_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_child_count (uint32_t(lv_obj_t *))
static bool invoke_INT_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_width (int32_t(lv_obj_t *, lv_part_t))
static bool invoke_INT_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_INT_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'INT', 'INT')
// Representative LVGL func: lv_obj_get_style_prop (lv_style_value_t(lv_obj_t *, lv_part_t, lv_style_prop_t))
static bool invoke_INT_lv_obj_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_style_prop_t arg2;
    lv_style_value_t result;

    // Define specific function pointer type based on representative function
    typedef lv_style_value_t (*specific_lv_func_type)(lv_obj_t *, lv_part_t, lv_style_prop_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_obj_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_INT_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_prop_t)
    if (!(unmarshal_value(json_arg1, "lv_style_prop_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_prop_t) for invoke_INT_lv_obj_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_style_value_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'INT', 'POINTER')
// Representative LVGL func: lv_obj_send_event (lv_result_t(lv_obj_t *, lv_event_code_t, void *))
static bool invoke_INT_lv_obj_t_p_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_INT_POINTER"); return false; }
    lv_obj_t * arg0;
    lv_event_code_t arg1;
    void * arg2;
    lv_result_t result;

    // Define specific function pointer type based on representative function
    typedef lv_result_t (*specific_lv_func_type)(lv_obj_t *, lv_event_code_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_obj_t_p_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_event_code_t)
    if (!(unmarshal_value(json_arg0, "lv_event_code_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_event_code_t) for invoke_INT_lv_obj_t_p_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_obj_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_INT_lv_obj_t_p_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_result_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'INT', 'const char *')
// Representative LVGL func: lv_obj_calculate_style_text_align (lv_text_align_t(lv_obj_t *, lv_part_t, char *))
static bool invoke_INT_lv_obj_t_p_INT_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_INT_const_char_p"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    char * arg2;
    lv_text_align_t result;

    // Define specific function pointer type based on representative function
    typedef lv_text_align_t (*specific_lv_func_type)(lv_obj_t *, lv_part_t, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_INT_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_obj_t_p_INT_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_INT_lv_obj_t_p_INT_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_obj_t_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_INT_lv_obj_t_p_INT_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_text_align_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'INT', 'lv_style_value_t *', 'INT')
// Representative LVGL func: lv_obj_get_local_style_prop (lv_style_res_t(lv_obj_t *, lv_style_prop_t, lv_style_value_t *, lv_style_selector_t))
static bool invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_style_prop_t arg1;
    lv_style_value_t * arg2;
    lv_style_selector_t arg3;
    lv_style_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_style_res_t (*specific_lv_func_type)(lv_obj_t *, lv_style_prop_t, lv_style_value_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_prop_t)
    if (!(unmarshal_value(json_arg0, "lv_style_prop_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_prop_t) for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_value_t *)
    if (!(unmarshal_value(json_arg1, "lv_style_value_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_value_t *) for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg2, "lv_style_selector_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_style_selector_t) for invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_style_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'const char *')
// Representative LVGL func: lv_dropdown_get_option_index (int32_t(lv_obj_t *, char *))
static bool invoke_INT_lv_obj_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_const_char_p"); return false; }
    lv_obj_t * arg0;
    char * arg1;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(lv_obj_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_INT_lv_obj_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'lv_chart_cursor_t *')
// Representative LVGL func: lv_chart_get_cursor_point (lv_point_t(lv_obj_t *, lv_chart_cursor_t *))
static bool invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_cursor_t * arg1;
    lv_point_t result;

    // Define specific function pointer type based on representative function
    typedef lv_point_t (*specific_lv_func_type)(lv_obj_t *, lv_chart_cursor_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_cursor_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_cursor_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_cursor_t *) for invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_point_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'lv_chart_series_t *')
// Representative LVGL func: lv_chart_get_x_start_point (uint32_t(lv_obj_t *, lv_chart_series_t *))
static bool invoke_INT_lv_obj_t_p_lv_chart_series_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_t_p_lv_chart_series_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_INT_lv_obj_t_p_lv_chart_series_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'lv_obj_class_t *')
// Representative LVGL func: lv_obj_get_child_count_by_type (uint32_t(lv_obj_t *, lv_obj_class_t *))
static bool invoke_INT_lv_obj_t_p_lv_obj_class_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_lv_obj_class_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_class_t * arg1;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_obj_t *, lv_obj_class_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_lv_obj_class_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_t_p_lv_obj_class_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_lv_obj_class_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_class_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_class_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_class_t *) for invoke_INT_lv_obj_t_p_lv_obj_class_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'lv_point_t *', 'BOOL')
// Representative LVGL func: lv_label_get_letter_on (uint32_t(lv_obj_t *, lv_point_t *, bool))
static bool invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL"); return false; }
    lv_obj_t * arg0;
    lv_point_t * arg1;
    bool arg2;
    uint32_t result;

    // Define specific function pointer type based on representative function
    typedef uint32_t (*specific_lv_func_type)(lv_obj_t *, lv_point_t *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type bool)
    if (!(unmarshal_value(json_arg1, "bool", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type bool) for invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((uint32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_obj_t *', 'lv_span_t *')
// Representative LVGL func: lv_spangroup_get_span_coords (lv_span_coords_t(lv_obj_t *, lv_span_t *))
static bool invoke_INT_lv_obj_t_p_lv_span_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_obj_t_p_lv_span_t_p"); return false; }
    lv_obj_t * arg0;
    lv_span_t * arg1;
    lv_span_coords_t result;

    // Define specific function pointer type based on representative function
    typedef lv_span_coords_t (*specific_lv_func_type)(lv_obj_t *, lv_span_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_obj_t_p_lv_span_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_INT_lv_obj_t_p_lv_span_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_obj_t_p_lv_span_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_span_t *)
    if (!(unmarshal_value(json_arg0, "lv_span_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_span_t *) for invoke_INT_lv_obj_t_p_lv_span_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_span_coords_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_point_precise_t *')
// Representative LVGL func: lv_point_from_precise (lv_point_t(lv_point_precise_t *))
static bool invoke_INT_lv_point_precise_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_point_precise_t_p"); return false; }
    lv_point_precise_t * arg0;
    lv_point_t result;

    // Define specific function pointer type based on representative function
    typedef lv_point_t (*specific_lv_func_type)(lv_point_precise_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_precise_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_point_precise_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_point_precise_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_point_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_point_t *')
// Representative LVGL func: lv_point_to_precise (lv_point_precise_t(lv_point_t *))
static bool invoke_INT_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_point_t_p"); return false; }
    lv_point_t * arg0;
    lv_point_precise_t result;

    // Define specific function pointer type based on representative function
    typedef lv_point_precise_t (*specific_lv_func_type)(lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_point_precise_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_style_t *', 'INT', 'lv_style_value_t *')
// Representative LVGL func: lv_style_get_prop (lv_style_res_t(lv_style_t *, lv_style_prop_t, lv_style_value_t *))
static bool invoke_INT_lv_style_t_p_INT_lv_style_value_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p"); return false; }
    lv_style_t * arg0;
    lv_style_prop_t arg1;
    lv_style_value_t * arg2;
    lv_style_res_t result;

    // Define specific function pointer type based on representative function
    typedef lv_style_res_t (*specific_lv_func_type)(lv_style_t *, lv_style_prop_t, lv_style_value_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_prop_t)
    if (!(unmarshal_value(json_arg0, "lv_style_prop_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_prop_t) for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_value_t *)
    if (!(unmarshal_value(json_arg1, "lv_style_value_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_value_t *) for invoke_INT_lv_style_t_p_INT_lv_style_value_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_style_res_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('INT', 'lv_subject_t *')
// Representative LVGL func: lv_subject_get_int (int32_t(lv_subject_t *))
static bool invoke_INT_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_INT_lv_subject_t_p"); return false; }
    lv_subject_t * arg0;
    int32_t result;

    // Define specific function pointer type based on representative function
    typedef int32_t (*specific_lv_func_type)(lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_INT_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_INT_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((int32_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'INT')
// Representative LVGL func: lv_malloc (void *(size_t))
static bool invoke_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_INT"); return false; }
    size_t arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type size_t)
    if (!(unmarshal_value(json_arg0, "size_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type size_t) for invoke_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'INT', 'INT')
// Representative LVGL func: lv_calloc (void *(size_t, size_t))
static bool invoke_POINTER_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_INT_INT"); return false; }
    size_t arg0;
    size_t arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(size_t, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_POINTER_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type size_t)
    if (!(unmarshal_value(json_arg0, "size_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type size_t) for invoke_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_POINTER_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'POINTER', 'INT')
// Representative LVGL func: lv_realloc (void *(void *, size_t))
static bool invoke_POINTER_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_POINTER_INT"); return false; }
    void * arg0;
    size_t arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(void *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_POINTER_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_POINTER_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'POINTER', 'POINTER', 'INT')
// Representative LVGL func: lv_memcpy (void *(void *, void *, size_t))
static bool invoke_POINTER_POINTER_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_POINTER_POINTER_INT"); return false; }
    void * arg0;
    void * arg1;
    size_t arg2;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(void *, void *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_POINTER_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_POINTER_POINTER_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_POINTER_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_POINTER_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_POINTER_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_POINTER_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_POINTER_POINTER_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'POINTER', 'POINTER', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_utils_bsearch (void *(void *, void *, size_t, size_t, int))
static bool invoke_POINTER_POINTER_POINTER_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    void * arg0;
    void * arg1;
    size_t arg2;
    size_t arg3;
    int arg4;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(void *, void *, size_t, size_t, int);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 5 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (5 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 5) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 5 JSON args, got %d for invoke_POINTER_POINTER_POINTER_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_POINTER_POINTER_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_POINTER_POINTER_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_POINTER_POINTER_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 3 (type size_t)
    if (!(unmarshal_value(json_arg3, "size_t", &arg3))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type size_t) for invoke_POINTER_POINTER_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_POINTER_POINTER_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 4 into C arg 4 (type int)
    if (!(unmarshal_value(json_arg4, "int", &arg4))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type int) for invoke_POINTER_POINTER_POINTER_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3, arg4);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_array_t *')
// Representative LVGL func: lv_array_front (void *(lv_array_t *))
static bool invoke_POINTER_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_array_t_p"); return false; }
    lv_array_t * arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_POINTER_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_array_t *', 'INT')
// Representative LVGL func: lv_array_at (void *(lv_array_t *, uint32_t))
static bool invoke_POINTER_lv_array_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_array_t_p_INT"); return false; }
    lv_array_t * arg0;
    uint32_t arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_array_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_array_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_array_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_array_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_POINTER_lv_array_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_circle_buf_t *')
// Representative LVGL func: lv_circle_buf_head (void *(lv_circle_buf_t *))
static bool invoke_POINTER_lv_circle_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_circle_buf_t_p"); return false; }
    lv_circle_buf_t * arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_circle_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_circle_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_POINTER_lv_circle_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_font_glyph_dsc_t *', 'lv_draw_buf_t *')
// Representative LVGL func: lv_font_get_glyph_bitmap (void *(lv_font_glyph_dsc_t *, lv_draw_buf_t *))
static bool invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p"); return false; }
    lv_font_glyph_dsc_t * arg0;
    lv_draw_buf_t * arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_font_glyph_dsc_t *, lv_draw_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_glyph_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_draw_buf_t *)
    if (!(unmarshal_value(json_arg0, "lv_draw_buf_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_draw_buf_t *) for invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_ll_t *')
// Representative LVGL func: lv_ll_ins_head (void *(lv_ll_t *))
static bool invoke_POINTER_lv_ll_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_ll_t_p"); return false; }
    lv_ll_t * arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_ll_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_ll_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_POINTER_lv_ll_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_ll_t *', 'POINTER')
// Representative LVGL func: lv_ll_ins_prev (void *(lv_ll_t *, void *))
static bool invoke_POINTER_lv_ll_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_ll_t_p_POINTER"); return false; }
    lv_ll_t * arg0;
    void * arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_ll_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_ll_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_ll_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_ll_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_POINTER_lv_ll_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_user_data (void *(lv_obj_t *))
static bool invoke_POINTER_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_POINTER_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_bg_image_src (void *(lv_obj_t *, lv_part_t))
static bool invoke_POINTER_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_POINTER_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_obj_t *', 'INT', 'INT')
// Representative LVGL func: lv_table_get_cell_user_data (void *(lv_obj_t *, uint16_t, uint16_t))
static bool invoke_POINTER_lv_obj_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_obj_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    uint16_t arg1;
    uint16_t arg2;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_obj_t *, uint16_t, uint16_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_obj_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_POINTER_lv_obj_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint16_t)
    if (!(unmarshal_value(json_arg0, "uint16_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint16_t) for invoke_POINTER_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_POINTER_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint16_t)
    if (!(unmarshal_value(json_arg1, "uint16_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint16_t) for invoke_POINTER_lv_obj_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_obj_t *', 'lv_chart_series_t *')
// Representative LVGL func: lv_chart_get_series_y_array (int32_t *(lv_obj_t *, lv_chart_series_t *))
static bool invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    int32_t * result;

    // Define specific function pointer type based on representative function
    typedef int32_t * (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((int32_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_observer_t *')
// Representative LVGL func: lv_observer_get_target (void *(lv_observer_t *))
static bool invoke_POINTER_lv_observer_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_observer_t_p"); return false; }
    lv_observer_t * arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_observer_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_observer_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_observer_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_POINTER_lv_observer_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_rb_t *', 'POINTER')
// Representative LVGL func: lv_rb_remove (void *(lv_rb_t *, void *))
static bool invoke_POINTER_lv_rb_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_rb_t_p_POINTER"); return false; }
    lv_rb_t * arg0;
    void * arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_rb_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_rb_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_rb_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_rb_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_POINTER_lv_rb_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_rb_t *', 'lv_rb_node_t *')
// Representative LVGL func: lv_rb_remove_node (void *(lv_rb_t *, lv_rb_node_t *))
static bool invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p"); return false; }
    lv_rb_t * arg0;
    lv_rb_node_t * arg1;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_rb_t *, lv_rb_node_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_rb_node_t *)
    if (!(unmarshal_value(json_arg0, "lv_rb_node_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_rb_node_t *) for invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('POINTER', 'lv_subject_t *')
// Representative LVGL func: lv_subject_get_pointer (void *(lv_subject_t *))
static bool invoke_POINTER_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_POINTER_lv_subject_t_p"); return false; }
    lv_subject_t * arg0;
    void * result;

    // Define specific function pointer type based on representative function
    typedef void * (*specific_lv_func_type)(lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_POINTER_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_POINTER_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((void * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *',)
// Representative LVGL func: lv_version_info (char *())
static bool invoke_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p"); return false; }
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'const char *')
// Representative LVGL func: lv_strdup (char *(char *))
static bool invoke_const_char_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_const_char_p"); return false; }
    char * arg0;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_const_char_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_const_char_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'const char *', 'INT')
// Representative LVGL func: lv_strndup (char *(char *, size_t))
static bool invoke_const_char_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_const_char_p_INT"); return false; }
    char * arg0;
    size_t arg1;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(char *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_const_char_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_const_char_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_const_char_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'const char *', 'const char *')
// Representative LVGL func: lv_strcpy (char *(char *, char *))
static bool invoke_const_char_p_const_char_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_const_char_p_const_char_p"); return false; }
    char * arg0;
    char * arg1;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(char *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_const_char_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_const_char_p_const_char_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_const_char_p_const_char_p_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_const_char_p_const_char_p_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_const_char_p_const_char_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'const char *', 'const char *', 'INT')
// Representative LVGL func: lv_strncpy (char *(char *, char *, size_t))
static bool invoke_const_char_p_const_char_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_const_char_p_const_char_p_INT"); return false; }
    char * arg0;
    char * arg1;
    size_t arg2;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(char *, char *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_const_char_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_const_char_p_const_char_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_const_char_p_const_char_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_const_char_p_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_const_char_p_const_char_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_const_char_p_const_char_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_const_char_p_const_char_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'lv_obj_t *')
// Representative LVGL func: lv_label_get_text (char *(lv_obj_t *))
static bool invoke_const_char_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_const_char_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_buttonmatrix_get_button_text (char *(lv_obj_t *, uint32_t))
static bool invoke_const_char_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(lv_obj_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_const_char_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_const_char_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'lv_obj_t *', 'INT', 'INT')
// Representative LVGL func: lv_table_get_cell_value (char *(lv_obj_t *, uint32_t, uint32_t))
static bool invoke_const_char_p_lv_obj_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_lv_obj_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    uint32_t arg2;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(lv_obj_t *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_lv_obj_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_const_char_p_lv_obj_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_const_char_p_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_const_char_p_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_const_char_p_lv_obj_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'lv_obj_t *', 'lv_obj_t *')
// Representative LVGL func: lv_list_get_button_text (char *(lv_obj_t *, lv_obj_t *))
static bool invoke_const_char_p_lv_obj_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_const_char_p_lv_obj_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_const_char_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_const_char_p_lv_obj_t_p_lv_obj_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'lv_span_t *')
// Representative LVGL func: lv_span_get_text (char *(lv_span_t *))
static bool invoke_const_char_p_lv_span_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_lv_span_t_p"); return false; }
    lv_span_t * arg0;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(lv_span_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_span_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_lv_span_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_const_char_p_lv_span_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('const char *', 'lv_subject_t *')
// Representative LVGL func: lv_subject_get_string (char *(lv_subject_t *))
static bool invoke_const_char_p_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_const_char_p_lv_subject_t_p"); return false; }
    lv_subject_t * arg0;
    char * result;

    // Define specific function pointer type based on representative function
    typedef char * (*specific_lv_func_type)(lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_const_char_p_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_const_char_p_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((char * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_anim_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_anim (lv_anim_t *(lv_obj_t *, lv_part_t))
static bool invoke_lv_anim_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_anim_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_anim_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_anim_t * (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_anim_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_anim_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_anim_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_lv_anim_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_anim_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_cache_entry_t *', 'lv_image_decoder_t *', 'lv_image_cache_data_t *', 'lv_draw_buf_t *', 'POINTER')
// Representative LVGL func: lv_image_decoder_add_to_cache (lv_cache_entry_t *(lv_image_decoder_t *, lv_image_cache_data_t *, lv_draw_buf_t *, void *))
static bool invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_cache_data_t * arg1;
    lv_draw_buf_t * arg2;
    void * arg3;
    lv_cache_entry_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_cache_entry_t * (*specific_lv_func_type)(lv_image_decoder_t *, lv_image_cache_data_t *, lv_draw_buf_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_cache_data_t *)
    if (!(unmarshal_value(json_arg0, "lv_image_cache_data_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_cache_data_t *) for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_buf_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_buf_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_buf_t *) for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_cache_entry_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_chart_cursor_t *', 'lv_obj_t *', 'lv_color_t', 'INT')
// Representative LVGL func: lv_chart_add_cursor (lv_chart_cursor_t *(lv_obj_t *, lv_color_t, lv_dir_t))
static bool invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    lv_obj_t * arg0;
    lv_color_t arg1;
    lv_dir_t arg2;
    lv_chart_cursor_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_chart_cursor_t * (*specific_lv_func_type)(lv_obj_t *, lv_color_t, lv_dir_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_dir_t)
    if (!(unmarshal_value(json_arg1, "lv_dir_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_dir_t) for invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_chart_cursor_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_chart_series_t *', 'lv_obj_t *', 'lv_chart_series_t *')
// Representative LVGL func: lv_chart_get_series_next (lv_chart_series_t *(lv_obj_t *, lv_chart_series_t *))
static bool invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    lv_chart_series_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_chart_series_t * (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_chart_series_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_chart_series_t *', 'lv_obj_t *', 'lv_color_t', 'INT')
// Representative LVGL func: lv_chart_add_series (lv_chart_series_t *(lv_obj_t *, lv_color_t, lv_chart_axis_t))
static bool invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    lv_obj_t * arg0;
    lv_color_t arg1;
    lv_chart_axis_t arg2;
    lv_chart_series_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_chart_series_t * (*specific_lv_func_type)(lv_obj_t *, lv_color_t, lv_chart_axis_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_chart_axis_t)
    if (!(unmarshal_value(json_arg1, "lv_chart_axis_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_chart_axis_t) for invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_chart_series_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_circle_buf_t *', 'INT', 'INT')
// Representative LVGL func: lv_circle_buf_create (lv_circle_buf_t *(uint32_t, uint32_t))
static bool invoke_lv_circle_buf_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_circle_buf_t_p_INT_INT"); return false; }
    uint32_t arg0;
    uint32_t arg1;
    lv_circle_buf_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_circle_buf_t * (*specific_lv_func_type)(uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_circle_buf_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_circle_buf_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_circle_buf_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_lv_circle_buf_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_circle_buf_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_lv_circle_buf_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_circle_buf_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_circle_buf_t *', 'POINTER', 'INT', 'INT')
// Representative LVGL func: lv_circle_buf_create_from_buf (lv_circle_buf_t *(void *, uint32_t, uint32_t))
static bool invoke_lv_circle_buf_t_p_POINTER_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_circle_buf_t_p_POINTER_INT_INT"); return false; }
    void * arg0;
    uint32_t arg1;
    uint32_t arg2;
    lv_circle_buf_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_circle_buf_t * (*specific_lv_func_type)(void *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_circle_buf_t_p_POINTER_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_circle_buf_t_p_POINTER_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_circle_buf_t_p_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_lv_circle_buf_t_p_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_circle_buf_t_p_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_lv_circle_buf_t_p_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_circle_buf_t_p_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_lv_circle_buf_t_p_POINTER_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_circle_buf_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_circle_buf_t *', 'lv_array_t *')
// Representative LVGL func: lv_circle_buf_create_from_array (lv_circle_buf_t *(lv_array_t *))
static bool invoke_lv_circle_buf_t_p_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_circle_buf_t_p_lv_array_t_p"); return false; }
    lv_array_t * arg0;
    lv_circle_buf_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_circle_buf_t * (*specific_lv_func_type)(lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_circle_buf_t_p_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_circle_buf_t_p_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_circle_buf_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_filter_dsc_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_color_filter_dsc (lv_color_filter_dsc_t *(lv_obj_t *, lv_part_t))
static bool invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_color_filter_dsc_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_color_filter_dsc_t * (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_filter_dsc_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t',)
// Representative LVGL func: lv_color_white (lv_color_t())
static bool invoke_lv_color_t(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t"); return false; }
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_color_t", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'INT')
// Representative LVGL func: lv_color_hex (lv_color_t(uint32_t))
static bool invoke_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_INT"); return false; }
    uint32_t arg0;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'INT', 'INT')
// Representative LVGL func: lv_palette_lighten (lv_color_t(lv_palette_t, uint8_t))
static bool invoke_lv_color_t_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_INT_INT"); return false; }
    lv_palette_t arg0;
    uint8_t arg1;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(lv_palette_t, uint8_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_color_t_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_palette_t)
    if (!(unmarshal_value(json_arg0, "lv_palette_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_palette_t) for invoke_lv_color_t_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_color_t_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint8_t)
    if (!(unmarshal_value(json_arg1, "uint8_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint8_t) for invoke_lv_color_t_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_color_make (lv_color_t(uint8_t, uint8_t, uint8_t))
static bool invoke_lv_color_t_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_INT_INT_INT"); return false; }
    uint8_t arg0;
    uint8_t arg1;
    uint8_t arg2;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(uint8_t, uint8_t, uint8_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_color_t_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint8_t)
    if (!(unmarshal_value(json_arg0, "uint8_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint8_t) for invoke_lv_color_t_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_color_t_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint8_t)
    if (!(unmarshal_value(json_arg1, "uint8_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint8_t) for invoke_lv_color_t_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_color_t_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type uint8_t)
    if (!(unmarshal_value(json_arg2, "uint8_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint8_t) for invoke_lv_color_t_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'lv_color_t', 'INT')
// Representative LVGL func: lv_color_lighten (lv_color_t(lv_color_t, lv_opa_t))
static bool invoke_lv_color_t_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_lv_color_t_INT"); return false; }
    lv_color_t arg0;
    lv_opa_t arg1;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(lv_color_t, lv_opa_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_color_t_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_lv_color_t_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_color_t_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_opa_t)
    if (!(unmarshal_value(json_arg1, "lv_opa_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_opa_t) for invoke_lv_color_t_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'lv_color_t', 'lv_color_t', 'INT')
// Representative LVGL func: lv_color_mix (lv_color_t(lv_color_t, lv_color_t, uint8_t))
static bool invoke_lv_color_t_lv_color_t_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_lv_color_t_lv_color_t_INT"); return false; }
    lv_color_t arg0;
    lv_color_t arg1;
    uint8_t arg2;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(lv_color_t, lv_color_t, uint8_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_lv_color_t_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_color_t_lv_color_t_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_lv_color_t_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_lv_color_t_lv_color_t_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_color_t_lv_color_t_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg1, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_color_t) for invoke_lv_color_t_lv_color_t_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_color_t_lv_color_t_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type uint8_t)
    if (!(unmarshal_value(json_arg2, "uint8_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint8_t) for invoke_lv_color_t_lv_color_t_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_bg_color (lv_color_t(lv_obj_t *, lv_part_t))
static bool invoke_lv_color_t_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_color_t_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_lv_color_t_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'lv_obj_t *', 'lv_chart_series_t *')
// Representative LVGL func: lv_chart_get_series_color (lv_color_t(lv_obj_t *, lv_chart_series_t *))
static bool invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_color_t', 'lv_subject_t *')
// Representative LVGL func: lv_subject_get_color (lv_color_t(lv_subject_t *))
static bool invoke_lv_color_t_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_color_t_lv_subject_t_p"); return false; }
    lv_subject_t * arg0;
    lv_color_t result;

    // Define specific function pointer type based on representative function
    typedef lv_color_t (*specific_lv_func_type)(lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_color_t_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_color_t_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_color_t *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_display_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_display (lv_display_t *(lv_obj_t *))
static bool invoke_lv_display_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_display_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_display_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_display_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_display_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_display_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_display_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_draw_buf_t *', 'lv_image_decoder_dsc_t *', 'lv_draw_buf_t *')
// Representative LVGL func: lv_image_decoder_post_process (lv_draw_buf_t *(lv_image_decoder_dsc_t *, lv_draw_buf_t *))
static bool invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p"); return false; }
    lv_image_decoder_dsc_t * arg0;
    lv_draw_buf_t * arg1;
    lv_draw_buf_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_draw_buf_t * (*specific_lv_func_type)(lv_image_decoder_dsc_t *, lv_draw_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_draw_buf_t *)
    if (!(unmarshal_value(json_arg0, "lv_draw_buf_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_draw_buf_t *) for invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_draw_buf_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_draw_buf_t *', 'lv_obj_t *')
// Representative LVGL func: lv_canvas_get_draw_buf (lv_draw_buf_t *(lv_obj_t *))
static bool invoke_lv_draw_buf_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_draw_buf_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_draw_buf_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_draw_buf_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_draw_buf_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_draw_buf_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_draw_buf_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_event_dsc_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_event_dsc (lv_event_dsc_t *(lv_obj_t *, uint32_t))
static bool invoke_lv_event_dsc_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    lv_event_dsc_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_event_dsc_t * (*specific_lv_func_type)(lv_obj_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_event_dsc_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_event_dsc_t *', 'lv_obj_t *', 'INT', 'INT', 'POINTER')
// Representative LVGL func: lv_obj_add_event_cb (lv_event_dsc_t *(lv_obj_t *, lv_event_cb_t, lv_event_code_t, void *))
static bool invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER"); return false; }
    lv_obj_t * arg0;
    lv_event_cb_t arg1;
    lv_event_code_t arg2;
    void * arg3;
    lv_event_dsc_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_event_dsc_t * (*specific_lv_func_type)(lv_obj_t *, lv_event_cb_t, lv_event_code_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_event_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_event_cb_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_event_cb_t) for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_event_code_t)
    if (!(unmarshal_value(json_arg1, "lv_event_code_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_event_code_t) for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_event_dsc_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_font_t *',)
// Representative LVGL func: lv_font_get_default (lv_font_t *())
static bool invoke_lv_font_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_font_t_p"); return false; }
    lv_font_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_font_t * (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_font_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_font_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((lv_font_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_font_t *', 'const char *')
// Representative LVGL func: lv_binfont_create (lv_font_t *(char *))
static bool invoke_lv_font_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_font_t_p_const_char_p"); return false; }
    char * arg0;
    lv_font_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_font_t * (*specific_lv_func_type)(char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_font_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_font_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_font_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_lv_font_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_font_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_font_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_text_font (lv_font_t *(lv_obj_t *, lv_part_t))
static bool invoke_lv_font_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_font_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_font_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_font_t * (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_font_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_font_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_font_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_lv_font_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_font_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_fs_drv_t *', 'INT')
// Representative LVGL func: lv_fs_get_drv (lv_fs_drv_t *(char))
static bool invoke_lv_fs_drv_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_fs_drv_t_p_INT"); return false; }
    char arg0;
    lv_fs_drv_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_drv_t * (*specific_lv_func_type)(char);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_fs_drv_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_fs_drv_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_fs_drv_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char)
    if (!(unmarshal_value(json_arg0, "char", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char) for invoke_lv_fs_drv_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_drv_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_fs_drv_t *', 'const char *')
// Representative LVGL func: lv_fs_drv_create_managed (lv_fs_drv_t *(char *))
static bool invoke_lv_fs_drv_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_fs_drv_t_p_const_char_p"); return false; }
    char * arg0;
    lv_fs_drv_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_fs_drv_t * (*specific_lv_func_type)(char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_fs_drv_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_fs_drv_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_fs_drv_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_lv_fs_drv_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_fs_drv_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_grad_dsc_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_bg_grad (lv_grad_dsc_t *(lv_obj_t *, lv_part_t))
static bool invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_grad_dsc_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_grad_dsc_t * (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_grad_dsc_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_group_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_group (lv_group_t *(lv_obj_t *))
static bool invoke_lv_group_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_group_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_group_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_group_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_group_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_group_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_group_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_image_decoder_t *',)
// Representative LVGL func: lv_image_decoder_create (lv_image_decoder_t *())
static bool invoke_lv_image_decoder_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_image_decoder_t_p"); return false; }
    lv_image_decoder_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_image_decoder_t * (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_image_decoder_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_image_decoder_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((lv_image_decoder_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_image_decoder_t *', 'lv_image_decoder_t *')
// Representative LVGL func: lv_image_decoder_get_next (lv_image_decoder_t *(lv_image_decoder_t *))
static bool invoke_lv_image_decoder_t_p_lv_image_decoder_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_image_decoder_t_p_lv_image_decoder_t_p"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_decoder_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_image_decoder_t * (*specific_lv_func_type)(lv_image_decoder_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_image_decoder_t_p_lv_image_decoder_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_image_decoder_t_p_lv_image_decoder_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_image_decoder_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_image_dsc_t *', 'lv_obj_t *')
// Representative LVGL func: lv_image_get_bitmap_map_src (lv_image_dsc_t *(lv_obj_t *))
static bool invoke_lv_image_dsc_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_image_dsc_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_image_dsc_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_image_dsc_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_image_dsc_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_image_dsc_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_image_dsc_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_iter_t *',)
// Representative LVGL func: lv_image_cache_iter_create (lv_iter_t *())
static bool invoke_lv_iter_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_iter_t_p"); return false; }
    lv_iter_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_iter_t * (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_iter_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_iter_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((lv_iter_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_layer_t *', 'const char *')
// Representative LVGL func: lv_layer_create_managed (lv_layer_t *(char *))
static bool invoke_lv_layer_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_layer_t_p_const_char_p"); return false; }
    char * arg0;
    lv_layer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_layer_t * (*specific_lv_func_type)(char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_layer_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_layer_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_layer_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_lv_layer_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_layer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_matrix_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_transform (lv_matrix_t *(lv_obj_t *))
static bool invoke_lv_matrix_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_matrix_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_matrix_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_matrix_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_matrix_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_matrix_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_matrix_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_class_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_class (lv_obj_class_t *(lv_obj_t *))
static bool invoke_lv_obj_class_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_class_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_class_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_class_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_class_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_obj_class_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_class_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *',)
// Representative LVGL func: lv_screen_active (lv_obj_t *())
static bool invoke_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p"); return false; }
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func();

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_get_screen (lv_obj_t *(lv_obj_t *))
static bool invoke_lv_obj_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_child (lv_obj_t *(lv_obj_t *, int32_t))
static bool invoke_lv_obj_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    int32_t arg1;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_lv_obj_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_tileview_add_tile (lv_obj_t *(lv_obj_t *, uint8_t, uint8_t, lv_dir_t))
static bool invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    uint8_t arg1;
    uint8_t arg2;
    lv_dir_t arg3;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, uint8_t, uint8_t, lv_dir_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint8_t)
    if (!(unmarshal_value(json_arg0, "uint8_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint8_t) for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint8_t)
    if (!(unmarshal_value(json_arg1, "uint8_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint8_t) for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_dir_t)
    if (!(unmarshal_value(json_arg2, "lv_dir_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_dir_t) for invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'INT', 'lv_obj_class_t *')
// Representative LVGL func: lv_obj_get_child_by_type (lv_obj_t *(lv_obj_t *, int32_t, lv_obj_class_t *))
static bool invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p"); return false; }
    lv_obj_t * arg0;
    int32_t arg1;
    lv_obj_class_t * arg2;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, int32_t, lv_obj_class_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_obj_class_t *)
    if (!(unmarshal_value(json_arg1, "lv_obj_class_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_obj_class_t *) for invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'POINTER')
// Representative LVGL func: lv_msgbox_add_header_button (lv_obj_t *(lv_obj_t *, void *))
static bool invoke_lv_obj_t_p_lv_obj_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_POINTER"); return false; }
    lv_obj_t * arg0;
    void * arg1;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_lv_obj_t_p_lv_obj_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'POINTER', 'INT')
// Representative LVGL func: lv_win_add_button (lv_obj_t *(lv_obj_t *, void *, int32_t))
static bool invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT"); return false; }
    lv_obj_t * arg0;
    void * arg1;
    int32_t arg2;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, void *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'POINTER', 'const char *')
// Representative LVGL func: lv_list_add_button (lv_obj_t *(lv_obj_t *, void *, char *))
static bool invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p"); return false; }
    lv_obj_t * arg0;
    void * arg1;
    char * arg2;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, void *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_obj_t *', 'const char *')
// Representative LVGL func: lv_list_add_text (lv_obj_t *(lv_obj_t *, char *))
static bool invoke_lv_obj_t_p_lv_obj_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    lv_obj_t * arg0;
    char * arg1;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_obj_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_obj_t_p_lv_obj_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_lv_obj_t_p_lv_obj_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_obj_t *', 'lv_observer_t *')
// Representative LVGL func: lv_observer_get_target_obj (lv_obj_t *(lv_observer_t *))
static bool invoke_lv_obj_t_p_lv_observer_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_obj_t_p_lv_observer_t_p"); return false; }
    lv_observer_t * arg0;
    lv_obj_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_obj_t * (*specific_lv_func_type)(lv_observer_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_observer_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_obj_t_p_lv_observer_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_obj_t_p_lv_observer_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_obj_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_observer_t *', 'lv_obj_t *', 'lv_subject_t *')
// Representative LVGL func: lv_obj_bind_checked (lv_observer_t *(lv_obj_t *, lv_subject_t *))
static bool invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p"); return false; }
    lv_obj_t * arg0;
    lv_subject_t * arg1;
    lv_observer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_observer_t * (*specific_lv_func_type)(lv_obj_t *, lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_subject_t *)
    if (!(unmarshal_value(json_arg0, "lv_subject_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_subject_t *) for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_observer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_observer_t *', 'lv_obj_t *', 'lv_subject_t *', 'INT', 'INT')
// Representative LVGL func: lv_obj_bind_flag_if_eq (lv_observer_t *(lv_obj_t *, lv_subject_t *, lv_obj_flag_t, int32_t))
static bool invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_subject_t * arg1;
    lv_obj_flag_t arg2;
    int32_t arg3;
    lv_observer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_observer_t * (*specific_lv_func_type)(lv_obj_t *, lv_subject_t *, lv_obj_flag_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_subject_t *)
    if (!(unmarshal_value(json_arg0, "lv_subject_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_subject_t *) for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_obj_flag_t)
    if (!(unmarshal_value(json_arg1, "lv_obj_flag_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_obj_flag_t) for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_observer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_observer_t *', 'lv_obj_t *', 'lv_subject_t *', 'const char *')
// Representative LVGL func: lv_label_bind_text (lv_observer_t *(lv_obj_t *, lv_subject_t *, char *))
static bool invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p"); return false; }
    lv_obj_t * arg0;
    lv_subject_t * arg1;
    char * arg2;
    lv_observer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_observer_t * (*specific_lv_func_type)(lv_obj_t *, lv_subject_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_subject_t *)
    if (!(unmarshal_value(json_arg0, "lv_subject_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_subject_t *) for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_observer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_observer_t *', 'lv_subject_t *', 'INT', 'POINTER')
// Representative LVGL func: lv_subject_add_observer (lv_observer_t *(lv_subject_t *, lv_observer_cb_t, void *))
static bool invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER"); return false; }
    lv_subject_t * arg0;
    lv_observer_cb_t arg1;
    void * arg2;
    lv_observer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_observer_t * (*specific_lv_func_type)(lv_subject_t *, lv_observer_cb_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_observer_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_observer_cb_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_observer_cb_t) for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2);

    // Store result if dest is provided
    if (dest) {
        *((lv_observer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_observer_t *', 'lv_subject_t *', 'INT', 'POINTER', 'POINTER')
// Representative LVGL func: lv_subject_add_observer_with_target (lv_observer_t *(lv_subject_t *, lv_observer_cb_t, void *, void *))
static bool invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER"); return false; }
    lv_subject_t * arg0;
    lv_observer_cb_t arg1;
    void * arg2;
    void * arg3;
    lv_observer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_observer_t * (*specific_lv_func_type)(lv_subject_t *, lv_observer_cb_t, void *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_observer_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_observer_cb_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_observer_cb_t) for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_observer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_observer_t *', 'lv_subject_t *', 'INT', 'lv_obj_t *', 'POINTER')
// Representative LVGL func: lv_subject_add_observer_obj (lv_observer_t *(lv_subject_t *, lv_observer_cb_t, lv_obj_t *, void *))
static bool invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER"); return false; }
    lv_subject_t * arg0;
    lv_observer_cb_t arg1;
    lv_obj_t * arg2;
    void * arg3;
    lv_observer_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_observer_t * (*specific_lv_func_type)(lv_subject_t *, lv_observer_cb_t, lv_obj_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_observer_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_observer_cb_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_observer_cb_t) for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg1, "lv_obj_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_obj_t *) for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1, arg2, arg3);

    // Store result if dest is provided
    if (dest) {
        *((lv_observer_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_point_precise_t *', 'lv_obj_t *')
// Representative LVGL func: lv_line_get_points (lv_point_precise_t *(lv_obj_t *))
static bool invoke_lv_point_precise_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_point_precise_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_point_precise_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_point_precise_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_point_precise_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_point_precise_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_point_precise_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_rb_node_t *', 'lv_rb_node_t *')
// Representative LVGL func: lv_rb_minimum_from (lv_rb_node_t *(lv_rb_node_t *))
static bool invoke_lv_rb_node_t_p_lv_rb_node_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_rb_node_t_p_lv_rb_node_t_p"); return false; }
    lv_rb_node_t * arg0;
    lv_rb_node_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_rb_node_t * (*specific_lv_func_type)(lv_rb_node_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_node_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_rb_node_t_p_lv_rb_node_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_rb_node_t_p_lv_rb_node_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_rb_node_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_rb_node_t *', 'lv_rb_t *')
// Representative LVGL func: lv_rb_minimum (lv_rb_node_t *(lv_rb_t *))
static bool invoke_lv_rb_node_t_p_lv_rb_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_rb_node_t_p_lv_rb_t_p"); return false; }
    lv_rb_t * arg0;
    lv_rb_node_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_rb_node_t * (*specific_lv_func_type)(lv_rb_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_rb_node_t_p_lv_rb_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_rb_node_t_p_lv_rb_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_rb_node_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_rb_node_t *', 'lv_rb_t *', 'POINTER')
// Representative LVGL func: lv_rb_insert (lv_rb_node_t *(lv_rb_t *, void *))
static bool invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER"); return false; }
    lv_rb_t * arg0;
    void * arg1;
    lv_rb_node_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_rb_node_t * (*specific_lv_func_type)(lv_rb_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_rb_node_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_scale_section_t *', 'lv_obj_t *')
// Representative LVGL func: lv_scale_add_section (lv_scale_section_t *(lv_obj_t *))
static bool invoke_lv_scale_section_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_scale_section_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_scale_section_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_scale_section_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_scale_section_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_scale_section_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_scale_section_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_span_t *', 'lv_obj_t *')
// Representative LVGL func: lv_spangroup_add_span (lv_span_t *(lv_obj_t *))
static bool invoke_lv_span_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_span_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_span_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_span_t * (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_span_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_span_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_span_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_span_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_spangroup_get_child (lv_span_t *(lv_obj_t *, int32_t))
static bool invoke_lv_span_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_span_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    int32_t arg1;
    lv_span_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_span_t * (*specific_lv_func_type)(lv_obj_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_span_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_span_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_span_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_lv_span_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_span_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_span_t *', 'lv_obj_t *', 'lv_point_t *')
// Representative LVGL func: lv_spangroup_get_span_by_point (lv_span_t *(lv_obj_t *, lv_point_t *))
static bool invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p"); return false; }
    lv_obj_t * arg0;
    lv_point_t * arg1;
    lv_span_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_span_t * (*specific_lv_func_type)(lv_obj_t *, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_span_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_style_t *', 'const char *')
// Representative LVGL func: lv_style_create_managed (lv_style_t *(char *))
static bool invoke_lv_style_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_style_t_p_const_char_p"); return false; }
    char * arg0;
    lv_style_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_style_t * (*specific_lv_func_type)(char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_style_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_style_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_style_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_lv_style_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_style_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_style_t *', 'lv_span_t *')
// Representative LVGL func: lv_span_get_style (lv_style_t *(lv_span_t *))
static bool invoke_lv_style_t_p_lv_span_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_style_t_p_lv_span_t_p"); return false; }
    lv_span_t * arg0;
    lv_style_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_style_t * (*specific_lv_func_type)(lv_span_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_span_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_style_t_p_lv_span_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_lv_style_t_p_lv_span_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    result = target_func(arg0);

    // Store result if dest is provided
    if (dest) {
        *((lv_style_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_style_transition_dsc_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_get_style_transition (lv_style_transition_dsc_t *(lv_obj_t *, lv_part_t))
static bool invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_style_transition_dsc_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_style_transition_dsc_t * (*specific_lv_func_type)(lv_obj_t *, lv_part_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_style_transition_dsc_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_subject_t *', 'lv_subject_t *', 'INT')
// Representative LVGL func: lv_subject_get_group_element (lv_subject_t *(lv_subject_t *, int32_t))
static bool invoke_lv_subject_t_p_lv_subject_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_subject_t_p_lv_subject_t_p_INT"); return false; }
    lv_subject_t * arg0;
    int32_t arg1;
    lv_subject_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_subject_t * (*specific_lv_func_type)(lv_subject_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_subject_t_p_lv_subject_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_subject_t_p_lv_subject_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_subject_t_p_lv_subject_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_lv_subject_t_p_lv_subject_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_subject_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('lv_tree_node_t *', 'lv_tree_class_t *', 'lv_tree_node_t *')
// Representative LVGL func: lv_tree_node_create (lv_tree_node_t *(lv_tree_class_t *, lv_tree_node_t *))
static bool invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p"); return false; }
    lv_tree_class_t * arg0;
    lv_tree_node_t * arg1;
    lv_tree_node_t * result;

    // Define specific function pointer type based on representative function
    typedef lv_tree_node_t * (*specific_lv_func_type)(lv_tree_class_t *, lv_tree_node_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_tree_class_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_tree_node_t *)
    if (!(unmarshal_value(json_arg0, "lv_tree_node_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_tree_node_t *) for invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p");
        return false;
    }

    // Call the target LVGL function
    result = target_func(arg0, arg1);

    // Store result if dest is provided
    if (dest) {
        *((lv_tree_node_t * *)dest) = result;
    }

    return true;
}

// Generic Invoker for signature category: ('void',)
// Representative LVGL func: lv_init (void())
static bool invoke_void(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void"); return false; }

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(void);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func();


    return true;
}

// Generic Invoker for signature category: ('void', 'BOOL')
// Representative LVGL func: lv_obj_enable_style_refresh (void(bool))
static bool invoke_void_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_BOOL"); return false; }
    bool arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type bool)
    if (!(unmarshal_value(json_arg0, "bool", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type bool) for invoke_void_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'INT')
// Representative LVGL func: lv_mem_remove_pool (void(lv_mem_pool_t))
static bool invoke_void_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_INT"); return false; }
    lv_mem_pool_t arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_mem_pool_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_mem_pool_t)
    if (!(unmarshal_value(json_arg0, "lv_mem_pool_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_mem_pool_t) for invoke_void_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'INT', 'BOOL')
// Representative LVGL func: lv_image_cache_resize (void(uint32_t, bool))
static bool invoke_void_INT_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_INT_BOOL"); return false; }
    uint32_t arg0;
    bool arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(uint32_t, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_INT_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_INT_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_INT_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_INT_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_INT_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type bool)
    if (!(unmarshal_value(json_arg1, "bool", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type bool) for invoke_void_INT_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'INT', 'lv_sqrt_res_t *', 'INT')
// Representative LVGL func: lv_sqrt (void(uint32_t, lv_sqrt_res_t *, uint32_t))
static bool invoke_void_INT_lv_sqrt_res_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_INT_lv_sqrt_res_t_p_INT"); return false; }
    uint32_t arg0;
    lv_sqrt_res_t * arg1;
    uint32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(uint32_t, lv_sqrt_res_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_INT_lv_sqrt_res_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_INT_lv_sqrt_res_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_INT_lv_sqrt_res_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_INT_lv_sqrt_res_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_INT_lv_sqrt_res_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type lv_sqrt_res_t *)
    if (!(unmarshal_value(json_arg1, "lv_sqrt_res_t *", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_sqrt_res_t *) for invoke_void_INT_lv_sqrt_res_t_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_INT_lv_sqrt_res_t_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_void_INT_lv_sqrt_res_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'POINTER')
// Representative LVGL func: lv_obj_null_on_delete (void(lv_obj_t **))
static bool invoke_void_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_POINTER"); return false; }
    lv_obj_t ** arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t **);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type lv_obj_t **)
    if (!(unmarshal_value(json_arg0, "lv_obj_t **", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t **) for invoke_void_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'POINTER', 'INT')
// Representative LVGL func: lv_memzero (void(void *, size_t))
static bool invoke_void_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_POINTER_INT"); return false; }
    void * arg0;
    size_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(void *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_void_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'POINTER', 'INT', 'INT')
// Representative LVGL func: lv_memset (void(void *, uint8_t, size_t))
static bool invoke_void_POINTER_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_POINTER_INT_INT"); return false; }
    void * arg0;
    uint8_t arg1;
    size_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(void *, uint8_t, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_POINTER_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_POINTER_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 0 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg0))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 1 (type uint8_t)
    if (!(unmarshal_value(json_arg1, "uint8_t", &arg1))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint8_t) for invoke_void_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_void_POINTER_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_anim_t *')
// Representative LVGL func: lv_obj_delete_anim_completed_cb (void(lv_anim_t *))
static bool invoke_void_lv_anim_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_anim_t_p"); return false; }
    lv_anim_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_anim_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_anim_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_anim_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_anim_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_area_t *', 'INT')
// Representative LVGL func: lv_area_set_width (void(lv_area_t *, int32_t))
static bool invoke_void_lv_area_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_area_t_p_INT"); return false; }
    lv_area_t * arg0;
    int32_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_area_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_area_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_area_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_area_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_area_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_area_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_area_t *', 'INT', 'INT')
// Representative LVGL func: lv_area_increase (void(lv_area_t *, int32_t, int32_t))
static bool invoke_void_lv_area_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_area_t_p_INT_INT"); return false; }
    lv_area_t * arg0;
    int32_t arg1;
    int32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_area_t *, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_area_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_area_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_area_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_area_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_area_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_area_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_area_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_area_t *', 'INT', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_area_set (void(lv_area_t *, int32_t, int32_t, int32_t, int32_t))
static bool invoke_void_lv_area_t_p_INT_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_area_t_p_INT_INT_INT_INT"); return false; }
    lv_area_t * arg0;
    int32_t arg1;
    int32_t arg2;
    int32_t arg3;
    int32_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_area_t *, int32_t, int32_t, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_area_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_area_t_p_INT_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_area_t_p_INT_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_area_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_area_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_area_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_area_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_area_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_area_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_area_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_void_lv_area_t_p_INT_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_area_t *', 'lv_area_t *')
// Representative LVGL func: lv_area_copy (void(lv_area_t *, lv_area_t *))
static bool invoke_void_lv_area_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_area_t_p_lv_area_t_p"); return false; }
    lv_area_t * arg0;
    lv_area_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_area_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_area_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_area_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_area_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_void_lv_area_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_area_t *', 'lv_area_t *', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_area_align (void(lv_area_t *, lv_area_t *, lv_align_t, int32_t, int32_t))
static bool invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT"); return false; }
    lv_area_t * arg0;
    lv_area_t * arg1;
    lv_align_t arg2;
    int32_t arg3;
    int32_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_area_t *, lv_area_t *, lv_align_t, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_area_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_align_t)
    if (!(unmarshal_value(json_arg1, "lv_align_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_align_t) for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_array_t *')
// Representative LVGL func: lv_array_deinit (void(lv_array_t *))
static bool invoke_void_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_array_t_p"); return false; }
    lv_array_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_array_t *', 'INT', 'INT')
// Representative LVGL func: lv_array_init (void(lv_array_t *, uint32_t, uint32_t))
static bool invoke_void_lv_array_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_array_t_p_INT_INT"); return false; }
    lv_array_t * arg0;
    uint32_t arg1;
    uint32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_array_t *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_array_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_array_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_array_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_lv_array_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_array_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_array_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_array_t *', 'POINTER', 'INT', 'INT')
// Representative LVGL func: lv_array_init_from_buf (void(lv_array_t *, void *, uint32_t, uint32_t))
static bool invoke_void_lv_array_t_p_POINTER_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_array_t_p_POINTER_INT_INT"); return false; }
    lv_array_t * arg0;
    void * arg1;
    uint32_t arg2;
    uint32_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_array_t *, void *, uint32_t, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_array_t_p_POINTER_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_array_t_p_POINTER_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_array_t_p_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_array_t_p_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_array_t_p_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_array_t_p_POINTER_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_array_t_p_POINTER_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_void_lv_array_t_p_POINTER_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_array_t *', 'lv_array_t *')
// Representative LVGL func: lv_array_copy (void(lv_array_t *, lv_array_t *))
static bool invoke_void_lv_array_t_p_lv_array_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_array_t_p_lv_array_t_p"); return false; }
    lv_array_t * arg0;
    lv_array_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_array_t *, lv_array_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_array_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_array_t_p_lv_array_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_array_t_p_lv_array_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_array_t_p_lv_array_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_array_t *)
    if (!(unmarshal_value(json_arg0, "lv_array_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_array_t *) for invoke_void_lv_array_t_p_lv_array_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_circle_buf_t *')
// Representative LVGL func: lv_circle_buf_destroy (void(lv_circle_buf_t *))
static bool invoke_void_lv_circle_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_circle_buf_t_p"); return false; }
    lv_circle_buf_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_circle_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_circle_buf_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_circle_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_circle_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_color16_t *', 'INT')
// Representative LVGL func: lv_color16_premultiply (void(lv_color16_t *, lv_opa_t))
static bool invoke_void_lv_color16_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_color16_t_p_INT"); return false; }
    lv_color16_t * arg0;
    lv_opa_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_color16_t *, lv_opa_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_color16_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_color16_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_color16_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_color16_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_opa_t)
    if (!(unmarshal_value(json_arg0, "lv_opa_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_opa_t) for invoke_void_lv_color16_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_color32_t *')
// Representative LVGL func: lv_color_premultiply (void(lv_color32_t *))
static bool invoke_void_lv_color32_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_color32_t_p"); return false; }
    lv_color32_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_color32_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_color32_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_color32_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_color32_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_color_filter_dsc_t *', 'INT')
// Representative LVGL func: lv_color_filter_dsc_init (void(lv_color_filter_dsc_t *, lv_color_filter_cb_t))
static bool invoke_void_lv_color_filter_dsc_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_color_filter_dsc_t_p_INT"); return false; }
    lv_color_filter_dsc_t * arg0;
    lv_color_filter_cb_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_color_filter_dsc_t *, lv_color_filter_cb_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_color_filter_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_color_filter_dsc_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_color_filter_dsc_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_color_filter_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_filter_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_color_filter_cb_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_filter_cb_t) for invoke_void_lv_color_filter_dsc_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_display_t *')
// Representative LVGL func: lv_refr_now (void(lv_display_t *))
static bool invoke_void_lv_display_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_display_t_p"); return false; }
    lv_display_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_display_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_display_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_display_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_display_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_event_t *')
// Representative LVGL func: lv_keyboard_def_event_cb (void(lv_event_t *))
static bool invoke_void_lv_event_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_event_t_p"); return false; }
    lv_event_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_event_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_event_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_event_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_event_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_font_glyph_dsc_t *')
// Representative LVGL func: lv_font_glyph_release_draw_data (void(lv_font_glyph_dsc_t *))
static bool invoke_void_lv_font_glyph_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_font_glyph_dsc_t_p"); return false; }
    lv_font_glyph_dsc_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_font_glyph_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_glyph_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_font_glyph_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_font_glyph_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_font_t *')
// Representative LVGL func: lv_binfont_destroy (void(lv_font_t *))
static bool invoke_void_lv_font_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_font_t_p"); return false; }
    lv_font_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_font_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_font_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_font_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_font_t *', 'INT')
// Representative LVGL func: lv_font_set_kerning (void(lv_font_t *, lv_font_kerning_t))
static bool invoke_void_lv_font_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_font_t_p_INT"); return false; }
    lv_font_t * arg0;
    lv_font_kerning_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_font_t *, lv_font_kerning_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_font_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_font_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_font_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_font_kerning_t)
    if (!(unmarshal_value(json_arg0, "lv_font_kerning_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_font_kerning_t) for invoke_void_lv_font_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_fs_drv_t *')
// Representative LVGL func: lv_fs_drv_init (void(lv_fs_drv_t *))
static bool invoke_void_lv_fs_drv_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_fs_drv_t_p"); return false; }
    lv_fs_drv_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_fs_drv_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_drv_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_fs_drv_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_fs_drv_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_fs_path_ex_t *', 'INT', 'POINTER', 'INT')
// Representative LVGL func: lv_fs_make_path_from_buffer (void(lv_fs_path_ex_t *, char, void *, uint32_t))
static bool invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT"); return false; }
    lv_fs_path_ex_t * arg0;
    char arg1;
    void * arg2;
    uint32_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_fs_path_ex_t *, char, void *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_fs_path_ex_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char)
    if (!(unmarshal_value(json_arg0, "char", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char) for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_grad_dsc_t *')
// Representative LVGL func: lv_grad_horizontal_init (void(lv_grad_dsc_t *))
static bool invoke_void_lv_grad_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_grad_dsc_t_p"); return false; }
    lv_grad_dsc_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_grad_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_grad_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_grad_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_grad_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_grad_dsc_t *', 'lv_color_t *', 'lv_opa_t *', 'POINTER', 'INT')
// Representative LVGL func: lv_grad_init_stops (void(lv_grad_dsc_t *, lv_color_t *, lv_opa_t *, uint8_t *, int))
static bool invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT"); return false; }
    lv_grad_dsc_t * arg0;
    lv_color_t * arg1;
    lv_opa_t * arg2;
    uint8_t * arg3;
    int arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_grad_dsc_t *, lv_color_t *, lv_opa_t *, uint8_t *, int);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_grad_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_t *)
    if (!(unmarshal_value(json_arg0, "lv_color_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t *) for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_opa_t *)
    if (!(unmarshal_value(json_arg1, "lv_opa_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_opa_t *) for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint8_t *)
    if (!(unmarshal_value(json_arg2, "uint8_t *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint8_t *) for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int)
    if (!(unmarshal_value(json_arg3, "int", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int) for invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_image_decoder_dsc_t *')
// Representative LVGL func: lv_image_decoder_close (void(lv_image_decoder_dsc_t *))
static bool invoke_void_lv_image_decoder_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_image_decoder_dsc_t_p"); return false; }
    lv_image_decoder_dsc_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_image_decoder_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_image_decoder_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_image_decoder_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_image_decoder_t *')
// Representative LVGL func: lv_image_decoder_delete (void(lv_image_decoder_t *))
static bool invoke_void_lv_image_decoder_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_image_decoder_t_p"); return false; }
    lv_image_decoder_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_image_decoder_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_image_decoder_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_image_decoder_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_image_decoder_t *', 'INT')
// Representative LVGL func: lv_image_decoder_set_info_cb (void(lv_image_decoder_t *, lv_image_decoder_info_f_t))
static bool invoke_void_lv_image_decoder_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_image_decoder_t_p_INT"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_decoder_info_f_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_image_decoder_t *, lv_image_decoder_info_f_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_image_decoder_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_image_decoder_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_image_decoder_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_decoder_info_f_t)
    if (!(unmarshal_value(json_arg0, "lv_image_decoder_info_f_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_decoder_info_f_t) for invoke_void_lv_image_decoder_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_image_decoder_t *', 'lv_image_decoder_dsc_t *')
// Representative LVGL func: lv_bin_decoder_close (void(lv_image_decoder_t *, lv_image_decoder_dsc_t *))
static bool invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p"); return false; }
    lv_image_decoder_t * arg0;
    lv_image_decoder_dsc_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_image_decoder_t *, lv_image_decoder_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_decoder_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_decoder_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_image_decoder_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_decoder_dsc_t *) for invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_image_dsc_t *')
// Representative LVGL func: lv_image_buf_free (void(lv_image_dsc_t *))
static bool invoke_void_lv_image_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_image_dsc_t_p"); return false; }
    lv_image_dsc_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_image_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_image_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_image_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_image_dsc_t *', 'INT', 'INT')
// Representative LVGL func: lv_image_buf_set_palette (void(lv_image_dsc_t *, uint8_t, lv_color32_t))
static bool invoke_void_lv_image_dsc_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_image_dsc_t_p_INT_INT"); return false; }
    lv_image_dsc_t * arg0;
    uint8_t arg1;
    lv_color32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_image_dsc_t *, uint8_t, lv_color32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_image_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_image_dsc_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_image_dsc_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_image_dsc_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint8_t)
    if (!(unmarshal_value(json_arg0, "uint8_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint8_t) for invoke_void_lv_image_dsc_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_image_dsc_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_color32_t)
    if (!(unmarshal_value(json_arg1, "lv_color32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_color32_t) for invoke_void_lv_image_dsc_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_layer_t *')
// Representative LVGL func: lv_layer_init (void(lv_layer_t *))
static bool invoke_void_lv_layer_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_layer_t_p"); return false; }
    lv_layer_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_layer_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_layer_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_layer_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_layer_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_layer_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_redraw (void(lv_layer_t *, lv_obj_t *))
static bool invoke_void_lv_layer_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_layer_t_p_lv_obj_t_p"); return false; }
    lv_layer_t * arg0;
    lv_obj_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_layer_t *, lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_layer_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_layer_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_layer_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_layer_t_p_lv_obj_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_layer_t_p_lv_obj_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_ll_t *')
// Representative LVGL func: lv_ll_clear (void(lv_ll_t *))
static bool invoke_void_lv_ll_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_ll_t_p"); return false; }
    lv_ll_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_ll_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_ll_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_ll_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_ll_t *', 'INT')
// Representative LVGL func: lv_ll_init (void(lv_ll_t *, uint32_t))
static bool invoke_void_lv_ll_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_ll_t_p_INT"); return false; }
    lv_ll_t * arg0;
    uint32_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_ll_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_ll_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_ll_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_ll_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_lv_ll_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_ll_t *', 'POINTER')
// Representative LVGL func: lv_ll_remove (void(lv_ll_t *, void *))
static bool invoke_void_lv_ll_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_ll_t_p_POINTER"); return false; }
    lv_ll_t * arg0;
    void * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_ll_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_ll_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_ll_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_ll_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_ll_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_ll_t *', 'POINTER', 'POINTER')
// Representative LVGL func: lv_ll_move_before (void(lv_ll_t *, void *, void *))
static bool invoke_void_lv_ll_t_p_POINTER_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_ll_t_p_POINTER_POINTER"); return false; }
    lv_ll_t * arg0;
    void * arg1;
    void * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_ll_t *, void *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_ll_t_p_POINTER_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_ll_t_p_POINTER_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_ll_t_p_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_ll_t_p_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_ll_t_p_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_void_lv_ll_t_p_POINTER_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_ll_t *', 'lv_ll_t *', 'POINTER', 'BOOL')
// Representative LVGL func: lv_ll_chg_list (void(lv_ll_t *, lv_ll_t *, void *, bool))
static bool invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL"); return false; }
    lv_ll_t * arg0;
    lv_ll_t * arg1;
    void * arg2;
    bool arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_ll_t *, lv_ll_t *, void *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_ll_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_ll_t *)
    if (!(unmarshal_value(json_arg0, "lv_ll_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_ll_t *) for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type bool)
    if (!(unmarshal_value(json_arg2, "bool", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type bool) for invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_mem_monitor_t *')
// Representative LVGL func: lv_mem_monitor_core (void(lv_mem_monitor_t *))
static bool invoke_void_lv_mem_monitor_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_mem_monitor_t_p"); return false; }
    lv_mem_monitor_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_mem_monitor_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_mem_monitor_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_mem_monitor_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_mem_monitor_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *')
// Representative LVGL func: lv_screen_load (void(lv_obj_t *))
static bool invoke_void_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'BOOL')
// Representative LVGL func: lv_image_set_antialias (void(lv_obj_t *, bool))
static bool invoke_void_lv_obj_t_p_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_BOOL"); return false; }
    lv_obj_t * arg0;
    bool arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type bool)
    if (!(unmarshal_value(json_arg0, "bool", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type bool) for invoke_void_lv_obj_t_p_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'BOOL', 'INT')
// Representative LVGL func: lv_obj_set_style_bg_image_tiled (void(lv_obj_t *, bool, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_BOOL_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_BOOL_INT"); return false; }
    lv_obj_t * arg0;
    bool arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, bool, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_BOOL_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_BOOL_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_BOOL_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type bool)
    if (!(unmarshal_value(json_arg0, "bool", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type bool) for invoke_void_lv_obj_t_p_BOOL_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_BOOL_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_BOOL_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_obj_set_flex_flow (void(lv_obj_t *, lv_flex_flow_t))
static bool invoke_void_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_flex_flow_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_flex_flow_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_flex_flow_t)
    if (!(unmarshal_value(json_arg0, "lv_flex_flow_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_flex_flow_t) for invoke_void_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'BOOL')
// Representative LVGL func: lv_obj_set_flag (void(lv_obj_t *, lv_obj_flag_t, bool))
static bool invoke_void_lv_obj_t_p_INT_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_BOOL"); return false; }
    lv_obj_t * arg0;
    lv_obj_flag_t arg1;
    bool arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_flag_t, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_flag_t)
    if (!(unmarshal_value(json_arg0, "lv_obj_flag_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_flag_t) for invoke_void_lv_obj_t_p_INT_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type bool)
    if (!(unmarshal_value(json_arg1, "bool", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type bool) for invoke_void_lv_obj_t_p_INT_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT')
// Representative LVGL func: lv_obj_set_grid_align (void(lv_obj_t *, lv_grid_align_t, lv_grid_align_t))
static bool invoke_void_lv_obj_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_grid_align_t arg1;
    lv_grid_align_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_grid_align_t, lv_grid_align_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_grid_align_t)
    if (!(unmarshal_value(json_arg0, "lv_grid_align_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_grid_align_t) for invoke_void_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_grid_align_t)
    if (!(unmarshal_value(json_arg1, "lv_grid_align_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_grid_align_t) for invoke_void_lv_obj_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'BOOL')
// Representative LVGL func: lv_obj_move_children_by (void(lv_obj_t *, int32_t, int32_t, bool))
static bool invoke_void_lv_obj_t_p_INT_INT_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_BOOL"); return false; }
    lv_obj_t * arg0;
    int32_t arg1;
    int32_t arg2;
    bool arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, int32_t, int32_t, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_BOOL");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type bool)
    if (!(unmarshal_value(json_arg2, "bool", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type bool) for invoke_void_lv_obj_t_p_INT_INT_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_obj_set_flex_align (void(lv_obj_t *, lv_flex_align_t, lv_flex_align_t, lv_flex_align_t))
static bool invoke_void_lv_obj_t_p_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_flex_align_t arg1;
    lv_flex_align_t arg2;
    lv_flex_align_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_flex_align_t, lv_flex_align_t, lv_flex_align_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_flex_align_t)
    if (!(unmarshal_value(json_arg0, "lv_flex_align_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_flex_align_t) for invoke_void_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_flex_align_t)
    if (!(unmarshal_value(json_arg1, "lv_flex_align_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_flex_align_t) for invoke_void_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_flex_align_t)
    if (!(unmarshal_value(json_arg2, "lv_flex_align_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_flex_align_t) for invoke_void_lv_obj_t_p_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'INT', 'BOOL')
// Representative LVGL func: lv_screen_load_anim (void(lv_obj_t *, lv_screen_load_anim_t, uint32_t, uint32_t, bool))
static bool invoke_void_lv_obj_t_p_INT_INT_INT_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL"); return false; }
    lv_obj_t * arg0;
    lv_screen_load_anim_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    bool arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_screen_load_anim_t, uint32_t, uint32_t, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_screen_load_anim_t)
    if (!(unmarshal_value(json_arg0, "lv_screen_load_anim_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_screen_load_anim_t) for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type bool)
    if (!(unmarshal_value(json_arg3, "bool", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type bool) for invoke_void_lv_obj_t_p_INT_INT_INT_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'INT', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_obj_set_grid_cell (void(lv_obj_t *, lv_grid_align_t, int32_t, int32_t, lv_grid_align_t, int32_t, int32_t))
static bool invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_grid_align_t arg1;
    int32_t arg2;
    int32_t arg3;
    lv_grid_align_t arg4;
    int32_t arg5;
    int32_t arg6;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_grid_align_t, int32_t, int32_t, lv_grid_align_t, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 6 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (6 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 6) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 6 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_grid_align_t)
    if (!(unmarshal_value(json_arg0, "lv_grid_align_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_grid_align_t) for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type lv_grid_align_t)
    if (!(unmarshal_value(json_arg3, "lv_grid_align_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type lv_grid_align_t) for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 4 into C arg 5 (type int32_t)
    if (!(unmarshal_value(json_arg4, "int32_t", &arg5))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg5 = cJSON_GetArrayItem(args_array, 5);
    if (!json_arg5) { LOG_ERR("Invoke Error: Failed to get JSON arg 5 for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 5 into C arg 6 (type int32_t)
    if (!(unmarshal_value(json_arg5, "int32_t", &arg6))) {
        LOG_ERR_JSON(json_arg5, "Invoke Error: Failed to unmarshal JSON arg 5 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4, arg5, arg6);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'POINTER')
// Representative LVGL func: lv_table_set_cell_user_data (void(lv_obj_t *, uint16_t, uint16_t, void *))
static bool invoke_void_lv_obj_t_p_INT_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_POINTER"); return false; }
    lv_obj_t * arg0;
    uint16_t arg1;
    uint16_t arg2;
    void * arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, uint16_t, uint16_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint16_t)
    if (!(unmarshal_value(json_arg0, "uint16_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint16_t) for invoke_void_lv_obj_t_p_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint16_t)
    if (!(unmarshal_value(json_arg1, "uint16_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint16_t) for invoke_void_lv_obj_t_p_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_void_lv_obj_t_p_INT_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'const char *')
// Representative LVGL func: lv_table_set_cell_value (void(lv_obj_t *, uint32_t, uint32_t, char *))
static bool invoke_void_lv_obj_t_p_INT_INT_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_const_char_p"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    uint32_t arg2;
    char * arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, uint32_t, uint32_t, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_lv_obj_t_p_INT_INT_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_INT_INT_const_char_p");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type char *)
    if (!(unmarshal_value(json_arg2, "char *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type char *) for invoke_void_lv_obj_t_p_INT_INT_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'INT', 'lv_color_t', 'INT')
// Representative LVGL func: lv_canvas_set_px (void(lv_obj_t *, int32_t, int32_t, lv_color_t, lv_opa_t))
static bool invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT"); return false; }
    lv_obj_t * arg0;
    int32_t arg1;
    int32_t arg2;
    lv_color_t arg3;
    lv_opa_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, int32_t, int32_t, lv_color_t, lv_opa_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_color_t)
    if (!(unmarshal_value(json_arg2, "lv_color_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_color_t) for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type lv_opa_t)
    if (!(unmarshal_value(json_arg3, "lv_opa_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type lv_opa_t) for invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'POINTER')
// Representative LVGL func: lv_obj_tree_walk (void(lv_obj_t *, lv_obj_tree_walk_cb_t, void *))
static bool invoke_void_lv_obj_t_p_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_POINTER"); return false; }
    lv_obj_t * arg0;
    lv_obj_tree_walk_cb_t arg1;
    void * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_tree_walk_cb_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_tree_walk_cb_t)
    if (!(unmarshal_value(json_arg0, "lv_obj_tree_walk_cb_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_tree_walk_cb_t) for invoke_void_lv_obj_t_p_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_void_lv_obj_t_p_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'POINTER', 'POINTER', 'POINTER')
// Representative LVGL func: lv_imagebutton_set_src (void(lv_obj_t *, lv_imagebutton_state_t, void *, void *, void *))
static bool invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER"); return false; }
    lv_obj_t * arg0;
    lv_imagebutton_state_t arg1;
    void * arg2;
    void * arg3;
    void * arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_imagebutton_state_t, void *, void *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_imagebutton_state_t)
    if (!(unmarshal_value(json_arg0, "lv_imagebutton_state_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_imagebutton_state_t) for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type void *)
    if (!(unmarshal_value(json_arg1, "void *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type void *) for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type void *)
    if (!(unmarshal_value(json_arg2, "void *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type void *) for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type void *)
    if (!(unmarshal_value(json_arg3, "void *", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type void *) for invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'const char *')
// Representative LVGL func: lv_label_ins_text (void(lv_obj_t *, uint32_t, char *))
static bool invoke_void_lv_obj_t_p_INT_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_const_char_p"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    char * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, uint32_t, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_lv_obj_t_p_INT_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_void_lv_obj_t_p_INT_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'lv_draw_arc_dsc_t *')
// Representative LVGL func: lv_obj_init_draw_arc_dsc (void(lv_obj_t *, lv_part_t, lv_draw_arc_dsc_t *))
static bool invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_draw_arc_dsc_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_part_t, lv_draw_arc_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_arc_dsc_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_arc_dsc_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_arc_dsc_t *) for invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'lv_draw_image_dsc_t *')
// Representative LVGL func: lv_obj_init_draw_image_dsc (void(lv_obj_t *, lv_part_t, lv_draw_image_dsc_t *))
static bool invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_draw_image_dsc_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_part_t, lv_draw_image_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_image_dsc_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_image_dsc_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_image_dsc_t *) for invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'lv_draw_label_dsc_t *')
// Representative LVGL func: lv_obj_init_draw_label_dsc (void(lv_obj_t *, lv_part_t, lv_draw_label_dsc_t *))
static bool invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_draw_label_dsc_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_part_t, lv_draw_label_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_label_dsc_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_label_dsc_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_label_dsc_t *) for invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'lv_draw_line_dsc_t *')
// Representative LVGL func: lv_obj_init_draw_line_dsc (void(lv_obj_t *, lv_part_t, lv_draw_line_dsc_t *))
static bool invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_draw_line_dsc_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_part_t, lv_draw_line_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_line_dsc_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_line_dsc_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_line_dsc_t *) for invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'lv_draw_rect_dsc_t *')
// Representative LVGL func: lv_obj_init_draw_rect_dsc (void(lv_obj_t *, lv_part_t, lv_draw_rect_dsc_t *))
static bool invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_part_t arg1;
    lv_draw_rect_dsc_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_part_t, lv_draw_rect_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_rect_dsc_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_rect_dsc_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_rect_dsc_t *) for invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'INT', 'lv_point_t *')
// Representative LVGL func: lv_label_get_letter_pos (void(lv_obj_t *, uint32_t, lv_point_t *))
static bool invoke_void_lv_obj_t_p_INT_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_INT_lv_point_t_p"); return false; }
    lv_obj_t * arg0;
    uint32_t arg1;
    lv_point_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, uint32_t, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_INT_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_INT_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_INT_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type uint32_t)
    if (!(unmarshal_value(json_arg0, "uint32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type uint32_t) for invoke_void_lv_obj_t_p_INT_lv_point_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_INT_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_point_t *)
    if (!(unmarshal_value(json_arg1, "lv_point_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_point_t *) for invoke_void_lv_obj_t_p_INT_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'POINTER')
// Representative LVGL func: lv_obj_set_user_data (void(lv_obj_t *, void *))
static bool invoke_void_lv_obj_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_POINTER"); return false; }
    lv_obj_t * arg0;
    void * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_obj_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'POINTER', 'INT')
// Representative LVGL func: lv_obj_set_style_bg_image_src (void(lv_obj_t *, void *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_POINTER_INT"); return false; }
    lv_obj_t * arg0;
    void * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, void *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_obj_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'POINTER', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_canvas_set_buffer (void(lv_obj_t *, void *, int32_t, int32_t, lv_color_format_t))
static bool invoke_void_lv_obj_t_p_POINTER_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    void * arg1;
    int32_t arg2;
    int32_t arg3;
    lv_color_format_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, void *, int32_t, int32_t, lv_color_format_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type lv_color_format_t)
    if (!(unmarshal_value(json_arg3, "lv_color_format_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type lv_color_format_t) for invoke_void_lv_obj_t_p_POINTER_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'POINTER', 'POINTER')
// Representative LVGL func: lv_obj_set_grid_dsc_array (void(lv_obj_t *, int32_t *, int32_t *))
static bool invoke_void_lv_obj_t_p_POINTER_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_POINTER_POINTER"); return false; }
    lv_obj_t * arg0;
    int32_t * arg1;
    int32_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, int32_t *, int32_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_POINTER_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_POINTER_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t *)
    if (!(unmarshal_value(json_arg0, "int32_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t *) for invoke_void_lv_obj_t_p_POINTER_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_POINTER_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t *)
    if (!(unmarshal_value(json_arg1, "int32_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t *) for invoke_void_lv_obj_t_p_POINTER_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'const char *')
// Representative LVGL func: lv_label_set_text (void(lv_obj_t *, char *))
static bool invoke_void_lv_obj_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_const_char_p"); return false; }
    lv_obj_t * arg0;
    char * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_void_lv_obj_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'const char *', 'INT')
// Representative LVGL func: lv_dropdown_add_option (void(lv_obj_t *, char *, uint32_t))
static bool invoke_void_lv_obj_t_p_const_char_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_const_char_p_INT"); return false; }
    lv_obj_t * arg0;
    char * arg1;
    uint32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, char *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_const_char_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_const_char_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_void_lv_obj_t_p_const_char_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_const_char_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_const_char_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_anim_t *', 'INT')
// Representative LVGL func: lv_obj_set_style_anim (void(lv_obj_t *, lv_anim_t *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_anim_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_anim_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_anim_t * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_anim_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_anim_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_anim_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_anim_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_anim_t *)
    if (!(unmarshal_value(json_arg0, "lv_anim_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_anim_t *) for invoke_void_lv_obj_t_p_lv_anim_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_anim_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_anim_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_area_t *')
// Representative LVGL func: lv_obj_get_coords (void(lv_obj_t *, lv_area_t *))
static bool invoke_void_lv_obj_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_area_t_p"); return false; }
    lv_obj_t * arg0;
    lv_area_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_void_lv_obj_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_area_t *', 'INT')
// Representative LVGL func: lv_obj_get_transformed_area (void(lv_obj_t *, lv_area_t *, lv_obj_point_transform_flag_t))
static bool invoke_void_lv_obj_t_p_lv_area_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_area_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_area_t * arg1;
    lv_obj_point_transform_flag_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_area_t *, lv_obj_point_transform_flag_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_area_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_area_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_area_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_void_lv_obj_t_p_lv_area_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_area_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_obj_point_transform_flag_t)
    if (!(unmarshal_value(json_arg1, "lv_obj_point_transform_flag_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_obj_point_transform_flag_t) for invoke_void_lv_obj_t_p_lv_area_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_area_t *', 'lv_area_t *')
// Representative LVGL func: lv_obj_get_scrollbar_area (void(lv_obj_t *, lv_area_t *, lv_area_t *))
static bool invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    lv_obj_t * arg0;
    lv_area_t * arg1;
    lv_area_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_area_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_area_t *)
    if (!(unmarshal_value(json_arg1, "lv_area_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_area_t *) for invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_area_t *', 'lv_draw_buf_t *', 'lv_area_t *')
// Representative LVGL func: lv_canvas_copy_buf (void(lv_obj_t *, lv_area_t *, lv_draw_buf_t *, lv_area_t *))
static bool invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p"); return false; }
    lv_obj_t * arg0;
    lv_area_t * arg1;
    lv_draw_buf_t * arg2;
    lv_area_t * arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_area_t *, lv_draw_buf_t *, lv_area_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_area_t *)
    if (!(unmarshal_value(json_arg0, "lv_area_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_area_t *) for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_draw_buf_t *)
    if (!(unmarshal_value(json_arg1, "lv_draw_buf_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_draw_buf_t *) for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_area_t *)
    if (!(unmarshal_value(json_arg2, "lv_area_t *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_area_t *) for invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_buttonmatrix_ctrl_t *')
// Representative LVGL func: lv_buttonmatrix_set_ctrl_map (void(lv_obj_t *, lv_buttonmatrix_ctrl_t *))
static bool invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p"); return false; }
    lv_obj_t * arg0;
    lv_buttonmatrix_ctrl_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_buttonmatrix_ctrl_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_buttonmatrix_ctrl_t *)
    if (!(unmarshal_value(json_arg0, "lv_buttonmatrix_ctrl_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_buttonmatrix_ctrl_t *) for invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_cursor_t *', 'lv_chart_series_t *', 'INT')
// Representative LVGL func: lv_chart_set_cursor_point (void(lv_obj_t *, lv_chart_cursor_t *, lv_chart_series_t *, uint32_t))
static bool invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_chart_cursor_t * arg1;
    lv_chart_series_t * arg2;
    uint32_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_cursor_t *, lv_chart_series_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_cursor_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_cursor_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_cursor_t *) for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg1, "lv_chart_series_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_cursor_t *', 'lv_point_t *')
// Representative LVGL func: lv_chart_set_cursor_pos (void(lv_obj_t *, lv_chart_cursor_t *, lv_point_t *))
static bool invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_cursor_t * arg1;
    lv_point_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_cursor_t *, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_cursor_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_cursor_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_cursor_t *) for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_point_t *)
    if (!(unmarshal_value(json_arg1, "lv_point_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_point_t *) for invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *')
// Representative LVGL func: lv_chart_remove_series (void(lv_obj_t *, lv_chart_series_t *))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'BOOL')
// Representative LVGL func: lv_chart_hide_series (void(lv_obj_t *, lv_chart_series_t *, bool))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    bool arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type bool)
    if (!(unmarshal_value(json_arg1, "bool", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type bool) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'INT')
// Representative LVGL func: lv_chart_set_x_start_point (void(lv_obj_t *, lv_chart_series_t *, uint32_t))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    uint32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'INT', 'INT')
// Representative LVGL func: lv_chart_set_next_value2 (void(lv_obj_t *, lv_chart_series_t *, int32_t, int32_t))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    int32_t arg2;
    int32_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_chart_set_series_value_by_id2 (void(lv_obj_t *, lv_chart_series_t *, uint32_t, int32_t, int32_t))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    uint32_t arg2;
    int32_t arg3;
    int32_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, uint32_t, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'INT', 'lv_point_t *')
// Representative LVGL func: lv_chart_get_point_pos_by_id (void(lv_obj_t *, lv_chart_series_t *, uint32_t, lv_point_t *))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    uint32_t arg2;
    lv_point_t * arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, uint32_t, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_point_t *)
    if (!(unmarshal_value(json_arg2, "lv_point_t *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_point_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'POINTER')
// Representative LVGL func: lv_chart_set_series_ext_y_array (void(lv_obj_t *, lv_chart_series_t *, int32_t *))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    int32_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, int32_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t *)
    if (!(unmarshal_value(json_arg1, "int32_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'POINTER', 'INT')
// Representative LVGL func: lv_chart_set_series_values (void(lv_obj_t *, lv_chart_series_t *, int32_t *, size_t))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    int32_t * arg2;
    size_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, int32_t *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t *)
    if (!(unmarshal_value(json_arg1, "int32_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'POINTER', 'POINTER', 'INT')
// Representative LVGL func: lv_chart_set_series_values2 (void(lv_obj_t *, lv_chart_series_t *, int32_t *, int32_t *, size_t))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    int32_t * arg2;
    int32_t * arg3;
    size_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, int32_t *, int32_t *, size_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t *)
    if (!(unmarshal_value(json_arg1, "int32_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t *)
    if (!(unmarshal_value(json_arg2, "int32_t *", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type size_t)
    if (!(unmarshal_value(json_arg3, "size_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type size_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_chart_series_t *', 'lv_color_t')
// Representative LVGL func: lv_chart_set_series_color (void(lv_obj_t *, lv_chart_series_t *, lv_color_t))
static bool invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t"); return false; }
    lv_obj_t * arg0;
    lv_chart_series_t * arg1;
    lv_color_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_chart_series_t *, lv_color_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_chart_series_t *)
    if (!(unmarshal_value(json_arg0, "lv_chart_series_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_chart_series_t *) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_color_t)
    if (!(unmarshal_value(json_arg1, "lv_color_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_color_t) for invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_color_filter_dsc_t *', 'INT')
// Representative LVGL func: lv_obj_set_style_color_filter_dsc (void(lv_obj_t *, lv_color_filter_dsc_t *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_color_filter_dsc_t * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_color_filter_dsc_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_filter_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_color_filter_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_filter_dsc_t *) for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_color_t', 'INT')
// Representative LVGL func: lv_obj_set_style_bg_color (void(lv_obj_t *, lv_color_t, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_color_t_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_color_t_INT"); return false; }
    lv_obj_t * arg0;
    lv_color_t arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_color_t, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_color_t_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_color_t_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_void_lv_obj_t_p_lv_color_t_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_color_t_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_color_t_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_draw_buf_t *')
// Representative LVGL func: lv_canvas_set_draw_buf (void(lv_obj_t *, lv_draw_buf_t *))
static bool invoke_void_lv_obj_t_p_lv_draw_buf_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_draw_buf_t_p"); return false; }
    lv_obj_t * arg0;
    lv_draw_buf_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_draw_buf_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_draw_buf_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_draw_buf_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_draw_buf_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_draw_buf_t *)
    if (!(unmarshal_value(json_arg0, "lv_draw_buf_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_draw_buf_t *) for invoke_void_lv_obj_t_p_lv_draw_buf_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_font_t *', 'INT')
// Representative LVGL func: lv_obj_set_style_text_font (void(lv_obj_t *, lv_font_t *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_font_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_font_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_font_t * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_font_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_font_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_font_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_font_t *)
    if (!(unmarshal_value(json_arg0, "lv_font_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_font_t *) for invoke_void_lv_obj_t_p_lv_font_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_font_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_font_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_grad_dsc_t *', 'INT')
// Representative LVGL func: lv_obj_set_style_bg_grad (void(lv_obj_t *, lv_grad_dsc_t *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_grad_dsc_t * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_grad_dsc_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_grad_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_grad_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_grad_dsc_t *) for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_image_dsc_t *')
// Representative LVGL func: lv_image_set_bitmap_map_src (void(lv_obj_t *, lv_image_dsc_t *))
static bool invoke_void_lv_obj_t_p_lv_image_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_image_dsc_t_p"); return false; }
    lv_obj_t * arg0;
    lv_image_dsc_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_image_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_image_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_image_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_image_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_image_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_image_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_image_dsc_t *) for invoke_void_lv_obj_t_p_lv_image_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_layer_t *')
// Representative LVGL func: lv_canvas_init_layer (void(lv_obj_t *, lv_layer_t *))
static bool invoke_void_lv_obj_t_p_lv_layer_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_layer_t_p"); return false; }
    lv_obj_t * arg0;
    lv_layer_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_layer_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_layer_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_layer_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_layer_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_layer_t *)
    if (!(unmarshal_value(json_arg0, "lv_layer_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_layer_t *) for invoke_void_lv_obj_t_p_lv_layer_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_matrix_t *')
// Representative LVGL func: lv_obj_set_transform (void(lv_obj_t *, lv_matrix_t *))
static bool invoke_void_lv_obj_t_p_lv_matrix_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_matrix_t_p"); return false; }
    lv_obj_t * arg0;
    lv_matrix_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_matrix_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_matrix_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_matrix_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_matrix_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_matrix_t *)
    if (!(unmarshal_value(json_arg0, "lv_matrix_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_matrix_t *) for invoke_void_lv_obj_t_p_lv_matrix_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_obj_t *')
// Representative LVGL func: lv_obj_set_parent (void(lv_obj_t *, lv_obj_t *))
static bool invoke_void_lv_obj_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_obj_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_obj_t *', 'INT')
// Representative LVGL func: lv_arc_align_obj_to_angle (void(lv_obj_t *, lv_obj_t *, int32_t))
static bool invoke_void_lv_obj_t_p_lv_obj_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    int32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_obj_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_obj_t *', 'INT', 'INT')
// Representative LVGL func: lv_scale_set_line_needle_value (void(lv_obj_t *, lv_obj_t *, int32_t, int32_t))
static bool invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    int32_t arg2;
    int32_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_obj_t *', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_obj_align_to (void(lv_obj_t *, lv_obj_t *, lv_align_t, int32_t, int32_t))
static bool invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    lv_align_t arg2;
    int32_t arg3;
    int32_t arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *, lv_align_t, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_align_t)
    if (!(unmarshal_value(json_arg1, "lv_align_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_align_t) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_obj_t *', 'const char *')
// Representative LVGL func: lv_list_set_button_text (void(lv_obj_t *, lv_obj_t *, char *))
static bool invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    char * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_obj_t *', 'lv_obj_t *')
// Representative LVGL func: lv_menu_set_load_page_event (void(lv_obj_t *, lv_obj_t *, lv_obj_t *))
static bool invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    lv_obj_t * arg0;
    lv_obj_t * arg1;
    lv_obj_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_obj_t *, lv_obj_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg0, "lv_obj_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_obj_t *)
    if (!(unmarshal_value(json_arg1, "lv_obj_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_obj_t *) for invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_point_precise_t *', 'INT')
// Representative LVGL func: lv_line_set_points (void(lv_obj_t *, lv_point_precise_t *, uint32_t))
static bool invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_point_precise_t * arg1;
    uint32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_point_precise_t *, uint32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_precise_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_precise_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_precise_t *) for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type uint32_t)
    if (!(unmarshal_value(json_arg1, "uint32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type uint32_t) for invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_point_t *')
// Representative LVGL func: lv_obj_get_scroll_end (void(lv_obj_t *, lv_point_t *))
static bool invoke_void_lv_obj_t_p_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_point_t_p"); return false; }
    lv_obj_t * arg0;
    lv_point_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_void_lv_obj_t_p_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_point_t *', 'INT')
// Representative LVGL func: lv_obj_transform_point (void(lv_obj_t *, lv_point_t *, lv_obj_point_transform_flag_t))
static bool invoke_void_lv_obj_t_p_lv_point_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_point_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_point_t * arg1;
    lv_obj_point_transform_flag_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_point_t *, lv_obj_point_transform_flag_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_point_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_point_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_point_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_void_lv_obj_t_p_lv_point_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_point_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_obj_point_transform_flag_t)
    if (!(unmarshal_value(json_arg1, "lv_obj_point_transform_flag_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_obj_point_transform_flag_t) for invoke_void_lv_obj_t_p_lv_point_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_point_t *', 'INT', 'INT')
// Representative LVGL func: lv_obj_transform_point_array (void(lv_obj_t *, lv_point_t *, size_t, lv_obj_point_transform_flag_t))
static bool invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_point_t * arg1;
    size_t arg2;
    lv_obj_point_transform_flag_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_point_t *, size_t, lv_obj_point_transform_flag_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type size_t)
    if (!(unmarshal_value(json_arg1, "size_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type size_t) for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type lv_obj_point_transform_flag_t)
    if (!(unmarshal_value(json_arg2, "lv_obj_point_transform_flag_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type lv_obj_point_transform_flag_t) for invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_scale_section_t *', 'INT', 'INT')
// Representative LVGL func: lv_scale_set_section_range (void(lv_obj_t *, lv_scale_section_t *, int32_t, int32_t))
static bool invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT"); return false; }
    lv_obj_t * arg0;
    lv_scale_section_t * arg1;
    int32_t arg2;
    int32_t arg3;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_scale_section_t *, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 3 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (3 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 3) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 3 JSON args, got %d for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_scale_section_t *)
    if (!(unmarshal_value(json_arg0, "lv_scale_section_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_scale_section_t *) for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_scale_section_t *', 'lv_style_t *')
// Representative LVGL func: lv_scale_set_section_style_main (void(lv_obj_t *, lv_scale_section_t *, lv_style_t *))
static bool invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p"); return false; }
    lv_obj_t * arg0;
    lv_scale_section_t * arg1;
    lv_style_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_scale_section_t *, lv_style_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_scale_section_t *)
    if (!(unmarshal_value(json_arg0, "lv_scale_section_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_scale_section_t *) for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_t *)
    if (!(unmarshal_value(json_arg1, "lv_style_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_t *) for invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_span_t *')
// Representative LVGL func: lv_spangroup_delete_span (void(lv_obj_t *, lv_span_t *))
static bool invoke_void_lv_obj_t_p_lv_span_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_span_t_p"); return false; }
    lv_obj_t * arg0;
    lv_span_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_span_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_span_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_span_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_span_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_span_t *)
    if (!(unmarshal_value(json_arg0, "lv_span_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_span_t *) for invoke_void_lv_obj_t_p_lv_span_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_span_t *', 'const char *')
// Representative LVGL func: lv_spangroup_set_span_text (void(lv_obj_t *, lv_span_t *, char *))
static bool invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p"); return false; }
    lv_obj_t * arg0;
    lv_span_t * arg1;
    char * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_span_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_span_t *)
    if (!(unmarshal_value(json_arg0, "lv_span_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_span_t *) for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_span_t *', 'lv_style_t *')
// Representative LVGL func: lv_spangroup_set_span_style (void(lv_obj_t *, lv_span_t *, lv_style_t *))
static bool invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p"); return false; }
    lv_obj_t * arg0;
    lv_span_t * arg1;
    lv_style_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_span_t *, lv_style_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_span_t *)
    if (!(unmarshal_value(json_arg0, "lv_span_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_span_t *) for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_t *)
    if (!(unmarshal_value(json_arg1, "lv_style_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_t *) for invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_style_t *', 'INT')
// Representative LVGL func: lv_obj_add_style (void(lv_obj_t *, lv_style_t *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_style_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_style_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_style_t * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_style_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_style_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_style_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_t *)
    if (!(unmarshal_value(json_arg0, "lv_style_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_t *) for invoke_void_lv_obj_t_p_lv_style_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_style_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_style_transition_dsc_t *', 'INT')
// Representative LVGL func: lv_obj_set_style_transition (void(lv_obj_t *, lv_style_transition_dsc_t *, lv_style_selector_t))
static bool invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT"); return false; }
    lv_obj_t * arg0;
    lv_style_transition_dsc_t * arg1;
    lv_style_selector_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_style_transition_dsc_t *, lv_style_selector_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_transition_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_style_transition_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_transition_dsc_t *) for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_selector_t)
    if (!(unmarshal_value(json_arg1, "lv_style_selector_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_selector_t) for invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_obj_t *', 'lv_subject_t *')
// Representative LVGL func: lv_obj_remove_from_subject (void(lv_obj_t *, lv_subject_t *))
static bool invoke_void_lv_obj_t_p_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_obj_t_p_lv_subject_t_p"); return false; }
    lv_obj_t * arg0;
    lv_subject_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_obj_t *, lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_obj_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_obj_t_p_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_obj_t_p_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_obj_t_p_lv_subject_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_subject_t *)
    if (!(unmarshal_value(json_arg0, "lv_subject_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_subject_t *) for invoke_void_lv_obj_t_p_lv_subject_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_observer_t *')
// Representative LVGL func: lv_observer_remove (void(lv_observer_t *))
static bool invoke_void_lv_observer_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_observer_t_p"); return false; }
    lv_observer_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_observer_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_observer_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_observer_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_observer_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_precise_t *', 'INT', 'INT')
// Representative LVGL func: lv_point_precise_set (void(lv_point_precise_t *, lv_value_precise_t, lv_value_precise_t))
static bool invoke_void_lv_point_precise_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_precise_t_p_INT_INT"); return false; }
    lv_point_precise_t * arg0;
    lv_value_precise_t arg1;
    lv_value_precise_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_precise_t *, lv_value_precise_t, lv_value_precise_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_precise_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_precise_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_point_precise_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_precise_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_value_precise_t)
    if (!(unmarshal_value(json_arg0, "lv_value_precise_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_value_precise_t) for invoke_void_lv_point_precise_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_point_precise_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_value_precise_t)
    if (!(unmarshal_value(json_arg1, "lv_value_precise_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_value_precise_t) for invoke_void_lv_point_precise_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_precise_t *', 'lv_point_precise_t *')
// Representative LVGL func: lv_point_precise_swap (void(lv_point_precise_t *, lv_point_precise_t *))
static bool invoke_void_lv_point_precise_t_p_lv_point_precise_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_precise_t_p_lv_point_precise_t_p"); return false; }
    lv_point_precise_t * arg0;
    lv_point_precise_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_precise_t *, lv_point_precise_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_precise_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_precise_t_p_lv_point_precise_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_point_precise_t_p_lv_point_precise_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_precise_t_p_lv_point_precise_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_precise_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_precise_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_precise_t *) for invoke_void_lv_point_precise_t_p_lv_point_precise_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_t *', 'INT', 'INT')
// Representative LVGL func: lv_point_set (void(lv_point_t *, int32_t, int32_t))
static bool invoke_void_lv_point_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_t_p_INT_INT"); return false; }
    lv_point_t * arg0;
    int32_t arg1;
    int32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_t *, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_point_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_point_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_t *', 'INT', 'INT', 'INT', 'INT', 'lv_point_t *', 'BOOL')
// Representative LVGL func: lv_point_array_transform (void(lv_point_t *, size_t, int32_t, int32_t, int32_t, lv_point_t *, bool))
static bool invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    lv_point_t * arg0;
    size_t arg1;
    int32_t arg2;
    int32_t arg3;
    int32_t arg4;
    lv_point_t * arg5;
    bool arg6;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_t *, size_t, int32_t, int32_t, int32_t, lv_point_t *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 6 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (6 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 6) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 6 JSON args, got %d for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type size_t)
    if (!(unmarshal_value(json_arg0, "size_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type size_t) for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 4 into C arg 5 (type lv_point_t *)
    if (!(unmarshal_value(json_arg4, "lv_point_t *", &arg5))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type lv_point_t *) for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg5 = cJSON_GetArrayItem(args_array, 5);
    if (!json_arg5) { LOG_ERR("Invoke Error: Failed to get JSON arg 5 for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 5 into C arg 6 (type bool)
    if (!(unmarshal_value(json_arg5, "bool", &arg6))) {
        LOG_ERR_JSON(json_arg5, "Invoke Error: Failed to unmarshal JSON arg 5 (expected C type bool) for invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4, arg5, arg6);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_t *', 'INT', 'INT', 'INT', 'lv_point_t *', 'BOOL')
// Representative LVGL func: lv_point_transform (void(lv_point_t *, int32_t, int32_t, int32_t, lv_point_t *, bool))
static bool invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    lv_point_t * arg0;
    int32_t arg1;
    int32_t arg2;
    int32_t arg3;
    lv_point_t * arg4;
    bool arg5;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_t *, int32_t, int32_t, int32_t, lv_point_t *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 5 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (5 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 5) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 5 JSON args, got %d for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type lv_point_t *)
    if (!(unmarshal_value(json_arg3, "lv_point_t *", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type lv_point_t *) for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 4 into C arg 5 (type bool)
    if (!(unmarshal_value(json_arg4, "bool", &arg5))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type bool) for invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4, arg5);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_t *', 'const char *', 'lv_font_t *', 'INT', 'INT', 'INT', 'INT')
// Representative LVGL func: lv_text_get_size (void(lv_point_t *, char *, lv_font_t *, int32_t, int32_t, int32_t, lv_text_flag_t))
static bool invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    lv_point_t * arg0;
    char * arg1;
    lv_font_t * arg2;
    int32_t arg3;
    int32_t arg4;
    int32_t arg5;
    lv_text_flag_t arg6;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_t *, char *, lv_font_t *, int32_t, int32_t, int32_t, lv_text_flag_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 6 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (6 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 6) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 6 JSON args, got %d for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_font_t *)
    if (!(unmarshal_value(json_arg1, "lv_font_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_font_t *) for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type int32_t)
    if (!(unmarshal_value(json_arg2, "int32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type int32_t) for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type int32_t)
    if (!(unmarshal_value(json_arg3, "int32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type int32_t) for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 4 into C arg 5 (type int32_t)
    if (!(unmarshal_value(json_arg4, "int32_t", &arg5))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type int32_t) for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT");
        return false;
    }
    cJSON *json_arg5 = cJSON_GetArrayItem(args_array, 5);
    if (!json_arg5) { LOG_ERR("Invoke Error: Failed to get JSON arg 5 for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT"); return false; }
    // Unmarshal JSON arg 5 into C arg 6 (type lv_text_flag_t)
    if (!(unmarshal_value(json_arg5, "lv_text_flag_t", &arg6))) {
        LOG_ERR_JSON(json_arg5, "Invoke Error: Failed to unmarshal JSON arg 5 (expected C type lv_text_flag_t) for invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4, arg5, arg6);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_point_t *', 'lv_point_t *')
// Representative LVGL func: lv_point_swap (void(lv_point_t *, lv_point_t *))
static bool invoke_void_lv_point_t_p_lv_point_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_point_t_p_lv_point_t_p"); return false; }
    lv_point_t * arg0;
    lv_point_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_point_t *, lv_point_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_point_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_point_t_p_lv_point_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_point_t_p_lv_point_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_point_t_p_lv_point_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_point_t *)
    if (!(unmarshal_value(json_arg0, "lv_point_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_point_t *) for invoke_void_lv_point_t_p_lv_point_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_rb_t *')
// Representative LVGL func: lv_rb_destroy (void(lv_rb_t *))
static bool invoke_void_lv_rb_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_rb_t_p"); return false; }
    lv_rb_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_rb_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_rb_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_rb_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_rb_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_scale_section_t *', 'INT', 'INT')
// Representative LVGL func: lv_scale_section_set_range (void(lv_scale_section_t *, int32_t, int32_t))
static bool invoke_void_lv_scale_section_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_scale_section_t_p_INT_INT"); return false; }
    lv_scale_section_t * arg0;
    int32_t arg1;
    int32_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_scale_section_t *, int32_t, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_scale_section_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_scale_section_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_scale_section_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_scale_section_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_scale_section_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_scale_section_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type int32_t)
    if (!(unmarshal_value(json_arg1, "int32_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type int32_t) for invoke_void_lv_scale_section_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_scale_section_t *', 'INT', 'lv_style_t *')
// Representative LVGL func: lv_scale_section_set_style (void(lv_scale_section_t *, lv_part_t, lv_style_t *))
static bool invoke_void_lv_scale_section_t_p_INT_lv_style_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p"); return false; }
    lv_scale_section_t * arg0;
    lv_part_t arg1;
    lv_style_t * arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_scale_section_t *, lv_part_t, lv_style_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_scale_section_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_part_t)
    if (!(unmarshal_value(json_arg0, "lv_part_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_part_t) for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_t *)
    if (!(unmarshal_value(json_arg1, "lv_style_t *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_t *) for invoke_void_lv_scale_section_t_p_INT_lv_style_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_span_t *', 'const char *')
// Representative LVGL func: lv_span_set_text (void(lv_span_t *, char *))
static bool invoke_void_lv_span_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_span_t_p_const_char_p"); return false; }
    lv_span_t * arg0;
    char * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_span_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_span_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_span_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_span_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_span_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_void_lv_span_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *')
// Representative LVGL func: lv_style_init (void(lv_style_t *))
static bool invoke_void_lv_style_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p"); return false; }
    lv_style_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_style_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'BOOL')
// Representative LVGL func: lv_style_set_bg_image_tiled (void(lv_style_t *, bool))
static bool invoke_void_lv_style_t_p_BOOL(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_BOOL"); return false; }
    lv_style_t * arg0;
    bool arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, bool);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_BOOL"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_BOOL", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_BOOL"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type bool)
    if (!(unmarshal_value(json_arg0, "bool", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type bool) for invoke_void_lv_style_t_p_BOOL");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'INT')
// Representative LVGL func: lv_style_set_width (void(lv_style_t *, int32_t))
static bool invoke_void_lv_style_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_INT"); return false; }
    lv_style_t * arg0;
    int32_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_style_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'INT', 'INT')
// Representative LVGL func: lv_style_set_prop (void(lv_style_t *, lv_style_prop_t, lv_style_value_t))
static bool invoke_void_lv_style_t_p_INT_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_INT_INT"); return false; }
    lv_style_t * arg0;
    lv_style_prop_t arg1;
    lv_style_value_t arg2;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_style_prop_t, lv_style_value_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 2 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (2 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_INT_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 2) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 2 JSON args, got %d for invoke_void_lv_style_t_p_INT_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_prop_t)
    if (!(unmarshal_value(json_arg0, "lv_style_prop_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_prop_t) for invoke_void_lv_style_t_p_INT_INT");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_style_t_p_INT_INT"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_style_value_t)
    if (!(unmarshal_value(json_arg1, "lv_style_value_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_style_value_t) for invoke_void_lv_style_t_p_INT_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'POINTER')
// Representative LVGL func: lv_style_set_bg_image_src (void(lv_style_t *, void *))
static bool invoke_void_lv_style_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_POINTER"); return false; }
    lv_style_t * arg0;
    void * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_style_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_anim_t *')
// Representative LVGL func: lv_style_set_anim (void(lv_style_t *, lv_anim_t *))
static bool invoke_void_lv_style_t_p_lv_anim_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_anim_t_p"); return false; }
    lv_style_t * arg0;
    lv_anim_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_anim_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_anim_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_anim_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_anim_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_anim_t *)
    if (!(unmarshal_value(json_arg0, "lv_anim_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_anim_t *) for invoke_void_lv_style_t_p_lv_anim_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_color_filter_dsc_t *')
// Representative LVGL func: lv_style_set_color_filter_dsc (void(lv_style_t *, lv_color_filter_dsc_t *))
static bool invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p"); return false; }
    lv_style_t * arg0;
    lv_color_filter_dsc_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_color_filter_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_filter_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_color_filter_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_filter_dsc_t *) for invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_color_t')
// Representative LVGL func: lv_style_set_bg_color (void(lv_style_t *, lv_color_t))
static bool invoke_void_lv_style_t_p_lv_color_t(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_color_t"); return false; }
    lv_style_t * arg0;
    lv_color_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_color_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_color_t"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_color_t", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_color_t"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_void_lv_style_t_p_lv_color_t");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_font_t *')
// Representative LVGL func: lv_style_set_text_font (void(lv_style_t *, lv_font_t *))
static bool invoke_void_lv_style_t_p_lv_font_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_font_t_p"); return false; }
    lv_style_t * arg0;
    lv_font_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_font_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_font_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_font_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_font_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_font_t *)
    if (!(unmarshal_value(json_arg0, "lv_font_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_font_t *) for invoke_void_lv_style_t_p_lv_font_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_grad_dsc_t *')
// Representative LVGL func: lv_style_set_bg_grad (void(lv_style_t *, lv_grad_dsc_t *))
static bool invoke_void_lv_style_t_p_lv_grad_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_grad_dsc_t_p"); return false; }
    lv_style_t * arg0;
    lv_grad_dsc_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_grad_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_grad_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_grad_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_grad_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_grad_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_grad_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_grad_dsc_t *) for invoke_void_lv_style_t_p_lv_grad_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_style_t *')
// Representative LVGL func: lv_style_copy (void(lv_style_t *, lv_style_t *))
static bool invoke_void_lv_style_t_p_lv_style_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_style_t_p"); return false; }
    lv_style_t * arg0;
    lv_style_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_style_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_style_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_style_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_style_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_t *)
    if (!(unmarshal_value(json_arg0, "lv_style_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_t *) for invoke_void_lv_style_t_p_lv_style_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_t *', 'lv_style_transition_dsc_t *')
// Representative LVGL func: lv_style_set_transition (void(lv_style_t *, lv_style_transition_dsc_t *))
static bool invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p"); return false; }
    lv_style_t * arg0;
    lv_style_transition_dsc_t * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_t *, lv_style_transition_dsc_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_transition_dsc_t *)
    if (!(unmarshal_value(json_arg0, "lv_style_transition_dsc_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_transition_dsc_t *) for invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_style_transition_dsc_t *', 'lv_style_prop_t *', 'INT', 'INT', 'INT', 'POINTER')
// Representative LVGL func: lv_style_transition_dsc_init (void(lv_style_transition_dsc_t *, lv_style_prop_t *, lv_anim_path_cb_t, uint32_t, uint32_t, void *))
static bool invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    lv_style_transition_dsc_t * arg0;
    lv_style_prop_t * arg1;
    lv_anim_path_cb_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    void * arg5;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_style_transition_dsc_t *, lv_style_prop_t *, lv_anim_path_cb_t, uint32_t, uint32_t, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_style_transition_dsc_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 5 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (5 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 5) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 5 JSON args, got %d for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_style_prop_t *)
    if (!(unmarshal_value(json_arg0, "lv_style_prop_t *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_style_prop_t *) for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type lv_anim_path_cb_t)
    if (!(unmarshal_value(json_arg1, "lv_anim_path_cb_t", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type lv_anim_path_cb_t) for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type uint32_t)
    if (!(unmarshal_value(json_arg2, "uint32_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type uint32_t) for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type uint32_t)
    if (!(unmarshal_value(json_arg3, "uint32_t", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type uint32_t) for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER");
        return false;
    }
    cJSON *json_arg4 = cJSON_GetArrayItem(args_array, 4);
    if (!json_arg4) { LOG_ERR("Invoke Error: Failed to get JSON arg 4 for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER"); return false; }
    // Unmarshal JSON arg 4 into C arg 5 (type void *)
    if (!(unmarshal_value(json_arg4, "void *", &arg5))) {
        LOG_ERR_JSON(json_arg4, "Invoke Error: Failed to unmarshal JSON arg 4 (expected C type void *) for invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4, arg5);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_subject_t *')
// Representative LVGL func: lv_subject_deinit (void(lv_subject_t *))
static bool invoke_void_lv_subject_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_subject_t_p"); return false; }
    lv_subject_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_subject_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_subject_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_subject_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_subject_t *', 'INT')
// Representative LVGL func: lv_subject_init_int (void(lv_subject_t *, int32_t))
static bool invoke_void_lv_subject_t_p_INT(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_subject_t_p_INT"); return false; }
    lv_subject_t * arg0;
    int32_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_subject_t *, int32_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_subject_t_p_INT"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_subject_t_p_INT", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_subject_t_p_INT"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type int32_t)
    if (!(unmarshal_value(json_arg0, "int32_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type int32_t) for invoke_void_lv_subject_t_p_INT");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_subject_t *', 'POINTER')
// Representative LVGL func: lv_subject_init_pointer (void(lv_subject_t *, void *))
static bool invoke_void_lv_subject_t_p_POINTER(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_subject_t_p_POINTER"); return false; }
    lv_subject_t * arg0;
    void * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_subject_t *, void *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_subject_t_p_POINTER"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_subject_t_p_POINTER", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_subject_t_p_POINTER"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type void *)
    if (!(unmarshal_value(json_arg0, "void *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type void *) for invoke_void_lv_subject_t_p_POINTER");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_subject_t *', 'const char *')
// Representative LVGL func: lv_subject_copy_string (void(lv_subject_t *, char *))
static bool invoke_void_lv_subject_t_p_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_subject_t_p_const_char_p"); return false; }
    lv_subject_t * arg0;
    char * arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_subject_t *, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_subject_t_p_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_subject_t_p_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_subject_t_p_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_void_lv_subject_t_p_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_subject_t *', 'const char *', 'const char *', 'INT', 'const char *')
// Representative LVGL func: lv_subject_init_string (void(lv_subject_t *, char *, char *, size_t, char *))
static bool invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p"); return false; }
    lv_subject_t * arg0;
    char * arg1;
    char * arg2;
    size_t arg3;
    char * arg4;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_subject_t *, char *, char *, size_t, char *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 4 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (4 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 4) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 4 JSON args, got %d for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type char *)
    if (!(unmarshal_value(json_arg0, "char *", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type char *) for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p");
        return false;
    }
    cJSON *json_arg1 = cJSON_GetArrayItem(args_array, 1);
    if (!json_arg1) { LOG_ERR("Invoke Error: Failed to get JSON arg 1 for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 1 into C arg 2 (type char *)
    if (!(unmarshal_value(json_arg1, "char *", &arg2))) {
        LOG_ERR_JSON(json_arg1, "Invoke Error: Failed to unmarshal JSON arg 1 (expected C type char *) for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p");
        return false;
    }
    cJSON *json_arg2 = cJSON_GetArrayItem(args_array, 2);
    if (!json_arg2) { LOG_ERR("Invoke Error: Failed to get JSON arg 2 for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 2 into C arg 3 (type size_t)
    if (!(unmarshal_value(json_arg2, "size_t", &arg3))) {
        LOG_ERR_JSON(json_arg2, "Invoke Error: Failed to unmarshal JSON arg 2 (expected C type size_t) for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p");
        return false;
    }
    cJSON *json_arg3 = cJSON_GetArrayItem(args_array, 3);
    if (!json_arg3) { LOG_ERR("Invoke Error: Failed to get JSON arg 3 for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p"); return false; }
    // Unmarshal JSON arg 3 into C arg 4 (type char *)
    if (!(unmarshal_value(json_arg3, "char *", &arg4))) {
        LOG_ERR_JSON(json_arg3, "Invoke Error: Failed to unmarshal JSON arg 3 (expected C type char *) for invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1, arg2, arg3, arg4);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_subject_t *', 'lv_color_t')
// Representative LVGL func: lv_subject_init_color (void(lv_subject_t *, lv_color_t))
static bool invoke_void_lv_subject_t_p_lv_color_t(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_subject_t_p_lv_color_t"); return false; }
    lv_subject_t * arg0;
    lv_color_t arg1;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_subject_t *, lv_color_t);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_subject_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 1 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (1 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_subject_t_p_lv_color_t"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 1) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 1 JSON args, got %d for invoke_void_lv_subject_t_p_lv_color_t", arg_count); return false; }

    // Unmarshal arguments from JSON
    cJSON *json_arg0 = cJSON_GetArrayItem(args_array, 0);
    if (!json_arg0) { LOG_ERR("Invoke Error: Failed to get JSON arg 0 for invoke_void_lv_subject_t_p_lv_color_t"); return false; }
    // Unmarshal JSON arg 0 into C arg 1 (type lv_color_t)
    if (!(unmarshal_value(json_arg0, "lv_color_t", &arg1))) {
        LOG_ERR_JSON(json_arg0, "Invoke Error: Failed to unmarshal JSON arg 0 (expected C type lv_color_t) for invoke_void_lv_subject_t_p_lv_color_t");
        return false;
    }

    // Call the target LVGL function
    target_func(arg0, arg1);


    return true;
}

// Generic Invoker for signature category: ('void', 'lv_tree_node_t *')
// Representative LVGL func: lv_tree_node_delete (void(lv_tree_node_t *))
static bool invoke_void_lv_tree_node_t_p(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {
    if (!func_ptr) { LOG_ERR("Invoke Error: func_ptr is NULL for invoke_void_lv_tree_node_t_p"); return false; }
    lv_tree_node_t * arg0;

    // Define specific function pointer type based on representative function
    typedef void (*specific_lv_func_type)(lv_tree_node_t *);
    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;

    // Assign target_obj_ptr to first C argument (arg0)
    arg0 = (lv_tree_node_t *)target_obj_ptr;
    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)
    // Add validation if target MUST be non-NULL for this signature group if needed.
    // Expecting 0 arguments from JSON array
    if (!cJSON_IsArray(args_array)) {
       if (0 == 0 && args_array == NULL) { /* Okay */ }
       else { LOG_ERR_JSON(args_array, "Invoke Error: args_array is not a valid array for invoke_void_lv_tree_node_t_p"); return false; }
    }
    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);
    if (arg_count != 0) { LOG_ERR_JSON(args_array, "Invoke Error: Expected 0 JSON args, got %d for invoke_void_lv_tree_node_t_p", arg_count); return false; }

    // Unmarshal arguments from JSON

    // Call the target LVGL function
    target_func(arg0);


    return true;
}



// --- Invocation Table ---
// The global invocation table
static const invoke_table_entry_t g_invoke_table[] = {
    {"lv_arc_align_obj_to_angle", &invoke_void_lv_obj_t_p_lv_obj_t_p_INT, (void*)&lv_arc_align_obj_to_angle},
    {"lv_arc_bind_value", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p, (void*)&lv_arc_bind_value},
    {"lv_arc_create", &invoke_widget_create, (void*)&lv_arc_create},
    {"lv_arc_get_angle_end", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_angle_end},
    {"lv_arc_get_angle_start", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_angle_start},
    {"lv_arc_get_bg_angle_end", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_bg_angle_end},
    {"lv_arc_get_bg_angle_start", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_bg_angle_start},
    {"lv_arc_get_knob_offset", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_knob_offset},
    {"lv_arc_get_max_value", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_max_value},
    {"lv_arc_get_min_value", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_min_value},
    {"lv_arc_get_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_mode},
    {"lv_arc_get_rotation", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_rotation},
    {"lv_arc_get_value", &invoke_INT_lv_obj_t_p, (void*)&lv_arc_get_value},
    {"lv_arc_rotate_obj_to_angle", &invoke_void_lv_obj_t_p_lv_obj_t_p_INT, (void*)&lv_arc_rotate_obj_to_angle},
    {"lv_arc_set_angles", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_arc_set_angles},
    {"lv_arc_set_bg_angles", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_arc_set_bg_angles},
    {"lv_arc_set_bg_end_angle", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_bg_end_angle},
    {"lv_arc_set_bg_start_angle", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_bg_start_angle},
    {"lv_arc_set_change_rate", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_change_rate},
    {"lv_arc_set_end_angle", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_end_angle},
    {"lv_arc_set_knob_offset", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_knob_offset},
    {"lv_arc_set_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_mode},
    {"lv_arc_set_range", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_arc_set_range},
    {"lv_arc_set_rotation", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_rotation},
    {"lv_arc_set_start_angle", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_start_angle},
    {"lv_arc_set_value", &invoke_void_lv_obj_t_p_INT, (void*)&lv_arc_set_value},
    {"lv_area_align", &invoke_void_lv_area_t_p_lv_area_t_p_INT_INT_INT, (void*)&lv_area_align},
    {"lv_area_copy", &invoke_void_lv_area_t_p_lv_area_t_p, (void*)&lv_area_copy},
    {"lv_area_get_height", &invoke_INT_lv_area_t_p, (void*)&lv_area_get_height},
    {"lv_area_get_size", &invoke_INT_lv_area_t_p, (void*)&lv_area_get_size},
    {"lv_area_get_width", &invoke_INT_lv_area_t_p, (void*)&lv_area_get_width},
    {"lv_area_increase", &invoke_void_lv_area_t_p_INT_INT, (void*)&lv_area_increase},
    {"lv_area_move", &invoke_void_lv_area_t_p_INT_INT, (void*)&lv_area_move},
    {"lv_area_set", &invoke_void_lv_area_t_p_INT_INT_INT_INT, (void*)&lv_area_set},
    {"lv_area_set_height", &invoke_void_lv_area_t_p_INT, (void*)&lv_area_set_height},
    {"lv_area_set_width", &invoke_void_lv_area_t_p_INT, (void*)&lv_area_set_width},
    {"lv_array_assign", &invoke_INT_lv_array_t_p_INT_POINTER, (void*)&lv_array_assign},
    {"lv_array_at", &invoke_POINTER_lv_array_t_p_INT, (void*)&lv_array_at},
    {"lv_array_back", &invoke_POINTER_lv_array_t_p, (void*)&lv_array_back},
    {"lv_array_capacity", &invoke_INT_lv_array_t_p, (void*)&lv_array_capacity},
    {"lv_array_clear", &invoke_void_lv_array_t_p, (void*)&lv_array_clear},
    {"lv_array_concat", &invoke_INT_lv_array_t_p_lv_array_t_p, (void*)&lv_array_concat},
    {"lv_array_copy", &invoke_void_lv_array_t_p_lv_array_t_p, (void*)&lv_array_copy},
    {"lv_array_deinit", &invoke_void_lv_array_t_p, (void*)&lv_array_deinit},
    {"lv_array_erase", &invoke_INT_lv_array_t_p_INT_INT, (void*)&lv_array_erase},
    {"lv_array_front", &invoke_POINTER_lv_array_t_p, (void*)&lv_array_front},
    {"lv_array_init", &invoke_void_lv_array_t_p_INT_INT, (void*)&lv_array_init},
    {"lv_array_init_from_buf", &invoke_void_lv_array_t_p_POINTER_INT_INT, (void*)&lv_array_init_from_buf},
    {"lv_array_is_empty", &invoke_BOOL_lv_array_t_p, (void*)&lv_array_is_empty},
    {"lv_array_is_full", &invoke_BOOL_lv_array_t_p, (void*)&lv_array_is_full},
    {"lv_array_push_back", &invoke_INT_lv_array_t_p_POINTER, (void*)&lv_array_push_back},
    {"lv_array_remove", &invoke_INT_lv_array_t_p_INT, (void*)&lv_array_remove},
    {"lv_array_resize", &invoke_BOOL_lv_array_t_p_INT, (void*)&lv_array_resize},
    {"lv_array_shrink", &invoke_void_lv_array_t_p, (void*)&lv_array_shrink},
    {"lv_array_size", &invoke_INT_lv_array_t_p, (void*)&lv_array_size},
    {"lv_async_call", &invoke_INT_INT_POINTER, (void*)&lv_async_call},
    {"lv_async_call_cancel", &invoke_INT_INT_POINTER, (void*)&lv_async_call_cancel},
    {"lv_atan2", &invoke_INT_INT_INT, (void*)&lv_atan2},
    {"lv_bar_create", &invoke_widget_create, (void*)&lv_bar_create},
    {"lv_bar_get_max_value", &invoke_INT_lv_obj_t_p, (void*)&lv_bar_get_max_value},
    {"lv_bar_get_min_value", &invoke_INT_lv_obj_t_p, (void*)&lv_bar_get_min_value},
    {"lv_bar_get_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_bar_get_mode},
    {"lv_bar_get_orientation", &invoke_INT_lv_obj_t_p, (void*)&lv_bar_get_orientation},
    {"lv_bar_get_start_value", &invoke_INT_lv_obj_t_p, (void*)&lv_bar_get_start_value},
    {"lv_bar_get_value", &invoke_INT_lv_obj_t_p, (void*)&lv_bar_get_value},
    {"lv_bar_is_symmetrical", &invoke_BOOL_lv_obj_t_p, (void*)&lv_bar_is_symmetrical},
    {"lv_bar_set_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_bar_set_mode},
    {"lv_bar_set_orientation", &invoke_void_lv_obj_t_p_INT, (void*)&lv_bar_set_orientation},
    {"lv_bar_set_range", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_bar_set_range},
    {"lv_bar_set_start_value", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_bar_set_start_value},
    {"lv_bar_set_value", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_bar_set_value},
    {"lv_bezier3", &invoke_INT_INT_INT_INT_INT_INT, (void*)&lv_bezier3},
    {"lv_bin_decoder_close", &invoke_void_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p, (void*)&lv_bin_decoder_close},
    {"lv_bin_decoder_get_area", &invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p, (void*)&lv_bin_decoder_get_area},
    {"lv_bin_decoder_info", &invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p_lv_image_header_t_p, (void*)&lv_bin_decoder_info},
    {"lv_bin_decoder_init", &invoke_void, (void*)&lv_bin_decoder_init},
    {"lv_bin_decoder_open", &invoke_INT_lv_image_decoder_t_p_lv_image_decoder_dsc_t_p, (void*)&lv_bin_decoder_open},
    {"lv_binfont_create", &invoke_lv_font_t_p_const_char_p, (void*)&lv_binfont_create},
    {"lv_binfont_destroy", &invoke_void_lv_font_t_p, (void*)&lv_binfont_destroy},
    {"lv_button_create", &invoke_widget_create, (void*)&lv_button_create},
    {"lv_buttonmatrix_clear_button_ctrl", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_buttonmatrix_clear_button_ctrl},
    {"lv_buttonmatrix_clear_button_ctrl_all", &invoke_void_lv_obj_t_p_INT, (void*)&lv_buttonmatrix_clear_button_ctrl_all},
    {"lv_buttonmatrix_create", &invoke_widget_create, (void*)&lv_buttonmatrix_create},
    {"lv_buttonmatrix_get_button_text", &invoke_const_char_p_lv_obj_t_p_INT, (void*)&lv_buttonmatrix_get_button_text},
    {"lv_buttonmatrix_get_map", &invoke_POINTER_lv_obj_t_p, (void*)&lv_buttonmatrix_get_map},
    {"lv_buttonmatrix_get_one_checked", &invoke_BOOL_lv_obj_t_p, (void*)&lv_buttonmatrix_get_one_checked},
    {"lv_buttonmatrix_get_selected_button", &invoke_INT_lv_obj_t_p, (void*)&lv_buttonmatrix_get_selected_button},
    {"lv_buttonmatrix_has_button_ctrl", &invoke_BOOL_lv_obj_t_p_INT_INT, (void*)&lv_buttonmatrix_has_button_ctrl},
    {"lv_buttonmatrix_set_button_ctrl", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_buttonmatrix_set_button_ctrl},
    {"lv_buttonmatrix_set_button_ctrl_all", &invoke_void_lv_obj_t_p_INT, (void*)&lv_buttonmatrix_set_button_ctrl_all},
    {"lv_buttonmatrix_set_button_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_buttonmatrix_set_button_width},
    {"lv_buttonmatrix_set_ctrl_map", &invoke_void_lv_obj_t_p_lv_buttonmatrix_ctrl_t_p, (void*)&lv_buttonmatrix_set_ctrl_map},
    {"lv_buttonmatrix_set_one_checked", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_buttonmatrix_set_one_checked},
    {"lv_buttonmatrix_set_selected_button", &invoke_void_lv_obj_t_p_INT, (void*)&lv_buttonmatrix_set_selected_button},
    {"lv_calloc", &invoke_POINTER_INT_INT, (void*)&lv_calloc},
    {"lv_canvas_buf_size", &invoke_INT_INT_INT_INT_INT, (void*)&lv_canvas_buf_size},
    {"lv_canvas_copy_buf", &invoke_void_lv_obj_t_p_lv_area_t_p_lv_draw_buf_t_p_lv_area_t_p, (void*)&lv_canvas_copy_buf},
    {"lv_canvas_create", &invoke_widget_create, (void*)&lv_canvas_create},
    {"lv_canvas_fill_bg", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_canvas_fill_bg},
    {"lv_canvas_finish_layer", &invoke_void_lv_obj_t_p_lv_layer_t_p, (void*)&lv_canvas_finish_layer},
    {"lv_canvas_get_buf", &invoke_POINTER_lv_obj_t_p, (void*)&lv_canvas_get_buf},
    {"lv_canvas_get_draw_buf", &invoke_lv_draw_buf_t_p_lv_obj_t_p, (void*)&lv_canvas_get_draw_buf},
    {"lv_canvas_get_image", &invoke_lv_image_dsc_t_p_lv_obj_t_p, (void*)&lv_canvas_get_image},
    {"lv_canvas_get_px", &invoke_INT_lv_obj_t_p_INT_INT, (void*)&lv_canvas_get_px},
    {"lv_canvas_init_layer", &invoke_void_lv_obj_t_p_lv_layer_t_p, (void*)&lv_canvas_init_layer},
    {"lv_canvas_set_buffer", &invoke_void_lv_obj_t_p_POINTER_INT_INT_INT, (void*)&lv_canvas_set_buffer},
    {"lv_canvas_set_draw_buf", &invoke_void_lv_obj_t_p_lv_draw_buf_t_p, (void*)&lv_canvas_set_draw_buf},
    {"lv_canvas_set_palette", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_canvas_set_palette},
    {"lv_canvas_set_px", &invoke_void_lv_obj_t_p_INT_INT_lv_color_t_INT, (void*)&lv_canvas_set_px},
    {"lv_chart_add_cursor", &invoke_lv_chart_cursor_t_p_lv_obj_t_p_lv_color_t_INT, (void*)&lv_chart_add_cursor},
    {"lv_chart_add_series", &invoke_lv_chart_series_t_p_lv_obj_t_p_lv_color_t_INT, (void*)&lv_chart_add_series},
    {"lv_chart_create", &invoke_widget_create, (void*)&lv_chart_create},
    {"lv_chart_get_cursor_point", &invoke_INT_lv_obj_t_p_lv_chart_cursor_t_p, (void*)&lv_chart_get_cursor_point},
    {"lv_chart_get_first_point_center_offset", &invoke_INT_lv_obj_t_p, (void*)&lv_chart_get_first_point_center_offset},
    {"lv_chart_get_point_count", &invoke_INT_lv_obj_t_p, (void*)&lv_chart_get_point_count},
    {"lv_chart_get_point_pos_by_id", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_lv_point_t_p, (void*)&lv_chart_get_point_pos_by_id},
    {"lv_chart_get_pressed_point", &invoke_INT_lv_obj_t_p, (void*)&lv_chart_get_pressed_point},
    {"lv_chart_get_series_color", &invoke_lv_color_t_lv_obj_t_p_lv_chart_series_t_p, (void*)&lv_chart_get_series_color},
    {"lv_chart_get_series_next", &invoke_lv_chart_series_t_p_lv_obj_t_p_lv_chart_series_t_p, (void*)&lv_chart_get_series_next},
    {"lv_chart_get_series_x_array", &invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p, (void*)&lv_chart_get_series_x_array},
    {"lv_chart_get_series_y_array", &invoke_POINTER_lv_obj_t_p_lv_chart_series_t_p, (void*)&lv_chart_get_series_y_array},
    {"lv_chart_get_type", &invoke_INT_lv_obj_t_p, (void*)&lv_chart_get_type},
    {"lv_chart_get_x_start_point", &invoke_INT_lv_obj_t_p_lv_chart_series_t_p, (void*)&lv_chart_get_x_start_point},
    {"lv_chart_hide_series", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_BOOL, (void*)&lv_chart_hide_series},
    {"lv_chart_refresh", &invoke_void_lv_obj_t_p, (void*)&lv_chart_refresh},
    {"lv_chart_remove_series", &invoke_void_lv_obj_t_p_lv_chart_series_t_p, (void*)&lv_chart_remove_series},
    {"lv_chart_set_all_values", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT, (void*)&lv_chart_set_all_values},
    {"lv_chart_set_axis_range", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_chart_set_axis_range},
    {"lv_chart_set_cursor_point", &invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_chart_series_t_p_INT, (void*)&lv_chart_set_cursor_point},
    {"lv_chart_set_cursor_pos", &invoke_void_lv_obj_t_p_lv_chart_cursor_t_p_lv_point_t_p, (void*)&lv_chart_set_cursor_pos},
    {"lv_chart_set_div_line_count", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_chart_set_div_line_count},
    {"lv_chart_set_next_value", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT, (void*)&lv_chart_set_next_value},
    {"lv_chart_set_next_value2", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT, (void*)&lv_chart_set_next_value2},
    {"lv_chart_set_point_count", &invoke_void_lv_obj_t_p_INT, (void*)&lv_chart_set_point_count},
    {"lv_chart_set_series_color", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_lv_color_t, (void*)&lv_chart_set_series_color},
    {"lv_chart_set_series_ext_x_array", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER, (void*)&lv_chart_set_series_ext_x_array},
    {"lv_chart_set_series_ext_y_array", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER, (void*)&lv_chart_set_series_ext_y_array},
    {"lv_chart_set_series_value_by_id", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT, (void*)&lv_chart_set_series_value_by_id},
    {"lv_chart_set_series_value_by_id2", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT_INT_INT, (void*)&lv_chart_set_series_value_by_id2},
    {"lv_chart_set_series_values", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_INT, (void*)&lv_chart_set_series_values},
    {"lv_chart_set_series_values2", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_POINTER_POINTER_INT, (void*)&lv_chart_set_series_values2},
    {"lv_chart_set_type", &invoke_void_lv_obj_t_p_INT, (void*)&lv_chart_set_type},
    {"lv_chart_set_update_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_chart_set_update_mode},
    {"lv_chart_set_x_start_point", &invoke_void_lv_obj_t_p_lv_chart_series_t_p_INT, (void*)&lv_chart_set_x_start_point},
    {"lv_checkbox_create", &invoke_widget_create, (void*)&lv_checkbox_create},
    {"lv_checkbox_get_text", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_checkbox_get_text},
    {"lv_checkbox_set_text", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_checkbox_set_text},
    {"lv_checkbox_set_text_static", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_checkbox_set_text_static},
    {"lv_circle_buf_capacity", &invoke_INT_lv_circle_buf_t_p, (void*)&lv_circle_buf_capacity},
    {"lv_circle_buf_create", &invoke_lv_circle_buf_t_p_INT_INT, (void*)&lv_circle_buf_create},
    {"lv_circle_buf_create_from_array", &invoke_lv_circle_buf_t_p_lv_array_t_p, (void*)&lv_circle_buf_create_from_array},
    {"lv_circle_buf_create_from_buf", &invoke_lv_circle_buf_t_p_POINTER_INT_INT, (void*)&lv_circle_buf_create_from_buf},
    {"lv_circle_buf_destroy", &invoke_void_lv_circle_buf_t_p, (void*)&lv_circle_buf_destroy},
    {"lv_circle_buf_fill", &invoke_INT_lv_circle_buf_t_p_INT_INT_POINTER, (void*)&lv_circle_buf_fill},
    {"lv_circle_buf_head", &invoke_POINTER_lv_circle_buf_t_p, (void*)&lv_circle_buf_head},
    {"lv_circle_buf_is_empty", &invoke_BOOL_lv_circle_buf_t_p, (void*)&lv_circle_buf_is_empty},
    {"lv_circle_buf_is_full", &invoke_BOOL_lv_circle_buf_t_p, (void*)&lv_circle_buf_is_full},
    {"lv_circle_buf_peek", &invoke_INT_lv_circle_buf_t_p_POINTER, (void*)&lv_circle_buf_peek},
    {"lv_circle_buf_peek_at", &invoke_INT_lv_circle_buf_t_p_INT_POINTER, (void*)&lv_circle_buf_peek_at},
    {"lv_circle_buf_read", &invoke_INT_lv_circle_buf_t_p_POINTER, (void*)&lv_circle_buf_read},
    {"lv_circle_buf_remain", &invoke_INT_lv_circle_buf_t_p, (void*)&lv_circle_buf_remain},
    {"lv_circle_buf_reset", &invoke_void_lv_circle_buf_t_p, (void*)&lv_circle_buf_reset},
    {"lv_circle_buf_resize", &invoke_INT_lv_circle_buf_t_p_INT, (void*)&lv_circle_buf_resize},
    {"lv_circle_buf_size", &invoke_INT_lv_circle_buf_t_p, (void*)&lv_circle_buf_size},
    {"lv_circle_buf_skip", &invoke_INT_lv_circle_buf_t_p, (void*)&lv_circle_buf_skip},
    {"lv_circle_buf_tail", &invoke_POINTER_lv_circle_buf_t_p, (void*)&lv_circle_buf_tail},
    {"lv_circle_buf_write", &invoke_INT_lv_circle_buf_t_p_POINTER, (void*)&lv_circle_buf_write},
    {"lv_clamp_height", &invoke_INT_INT_INT_INT_INT, (void*)&lv_clamp_height},
    {"lv_clamp_width", &invoke_INT_INT_INT_INT_INT, (void*)&lv_clamp_width},
    {"lv_color16_luminance", &invoke_INT_INT, (void*)&lv_color16_luminance},
    {"lv_color16_premultiply", &invoke_void_lv_color16_t_p_INT, (void*)&lv_color16_premultiply},
    {"lv_color24_luminance", &invoke_INT_POINTER, (void*)&lv_color24_luminance},
    {"lv_color32_eq", &invoke_BOOL_INT_INT, (void*)&lv_color32_eq},
    {"lv_color32_luminance", &invoke_INT_INT, (void*)&lv_color32_luminance},
    {"lv_color32_make", &invoke_INT_INT_INT_INT_INT, (void*)&lv_color32_make},
    {"lv_color_16_16_mix", &invoke_INT_INT_INT_INT, (void*)&lv_color_16_16_mix},
    {"lv_color_black", &invoke_lv_color_t, (void*)&lv_color_black},
    {"lv_color_darken", &invoke_lv_color_t_lv_color_t_INT, (void*)&lv_color_darken},
    {"lv_color_eq", &invoke_BOOL_lv_color_t_lv_color_t, (void*)&lv_color_eq},
    {"lv_color_filter_dsc_init", &invoke_void_lv_color_filter_dsc_t_p_INT, (void*)&lv_color_filter_dsc_init},
    {"lv_color_format_get_bpp", &invoke_INT_INT, (void*)&lv_color_format_get_bpp},
    {"lv_color_format_get_size", &invoke_INT_INT, (void*)&lv_color_format_get_size},
    {"lv_color_format_has_alpha", &invoke_BOOL_INT, (void*)&lv_color_format_has_alpha},
    {"lv_color_hex", &invoke_lv_color_t_INT, (void*)&lv_color_hex},
    {"lv_color_hex3", &invoke_lv_color_t_INT, (void*)&lv_color_hex3},
    {"lv_color_hsv_to_rgb", &invoke_lv_color_t_INT_INT_INT, (void*)&lv_color_hsv_to_rgb},
    {"lv_color_lighten", &invoke_lv_color_t_lv_color_t_INT, (void*)&lv_color_lighten},
    {"lv_color_luminance", &invoke_INT_lv_color_t, (void*)&lv_color_luminance},
    {"lv_color_make", &invoke_lv_color_t_INT_INT_INT, (void*)&lv_color_make},
    {"lv_color_mix", &invoke_lv_color_t_lv_color_t_lv_color_t_INT, (void*)&lv_color_mix},
    {"lv_color_mix32", &invoke_INT_INT_INT, (void*)&lv_color_mix32},
    {"lv_color_mix32_premultiplied", &invoke_INT_INT_INT, (void*)&lv_color_mix32_premultiplied},
    {"lv_color_over32", &invoke_INT_INT_INT, (void*)&lv_color_over32},
    {"lv_color_premultiply", &invoke_void_lv_color32_t_p, (void*)&lv_color_premultiply},
    {"lv_color_rgb_to_hsv", &invoke_INT_INT_INT_INT, (void*)&lv_color_rgb_to_hsv},
    {"lv_color_to_32", &invoke_INT_lv_color_t_INT, (void*)&lv_color_to_32},
    {"lv_color_to_hsv", &invoke_INT_lv_color_t, (void*)&lv_color_to_hsv},
    {"lv_color_to_int", &invoke_INT_lv_color_t, (void*)&lv_color_to_int},
    {"lv_color_to_u16", &invoke_INT_lv_color_t, (void*)&lv_color_to_u16},
    {"lv_color_to_u32", &invoke_INT_lv_color_t, (void*)&lv_color_to_u32},
    {"lv_color_white", &invoke_lv_color_t, (void*)&lv_color_white},
    {"lv_cubic_bezier", &invoke_INT_INT_INT_INT_INT_INT, (void*)&lv_cubic_bezier},
    {"lv_deinit", &invoke_void, (void*)&lv_deinit},
    {"lv_delay_ms", &invoke_void_INT, (void*)&lv_delay_ms},
    {"lv_delay_set_cb", &invoke_void_INT, (void*)&lv_delay_set_cb},
    {"lv_dpx", &invoke_INT_INT, (void*)&lv_dpx},
    {"lv_dropdown_add_option", &invoke_void_lv_obj_t_p_const_char_p_INT, (void*)&lv_dropdown_add_option},
    {"lv_dropdown_bind_value", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p, (void*)&lv_dropdown_bind_value},
    {"lv_dropdown_clear_options", &invoke_void_lv_obj_t_p, (void*)&lv_dropdown_clear_options},
    {"lv_dropdown_close", &invoke_void_lv_obj_t_p, (void*)&lv_dropdown_close},
    {"lv_dropdown_create", &invoke_widget_create, (void*)&lv_dropdown_create},
    {"lv_dropdown_get_dir", &invoke_INT_lv_obj_t_p, (void*)&lv_dropdown_get_dir},
    {"lv_dropdown_get_list", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_dropdown_get_list},
    {"lv_dropdown_get_option_count", &invoke_INT_lv_obj_t_p, (void*)&lv_dropdown_get_option_count},
    {"lv_dropdown_get_option_index", &invoke_INT_lv_obj_t_p_const_char_p, (void*)&lv_dropdown_get_option_index},
    {"lv_dropdown_get_options", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_dropdown_get_options},
    {"lv_dropdown_get_selected", &invoke_INT_lv_obj_t_p, (void*)&lv_dropdown_get_selected},
    {"lv_dropdown_get_selected_highlight", &invoke_BOOL_lv_obj_t_p, (void*)&lv_dropdown_get_selected_highlight},
    {"lv_dropdown_get_selected_str", &invoke_void_lv_obj_t_p_const_char_p_INT, (void*)&lv_dropdown_get_selected_str},
    {"lv_dropdown_get_symbol", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_dropdown_get_symbol},
    {"lv_dropdown_get_text", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_dropdown_get_text},
    {"lv_dropdown_is_open", &invoke_BOOL_lv_obj_t_p, (void*)&lv_dropdown_is_open},
    {"lv_dropdown_open", &invoke_void_lv_obj_t_p, (void*)&lv_dropdown_open},
    {"lv_dropdown_set_dir", &invoke_void_lv_obj_t_p_INT, (void*)&lv_dropdown_set_dir},
    {"lv_dropdown_set_options", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_dropdown_set_options},
    {"lv_dropdown_set_options_static", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_dropdown_set_options_static},
    {"lv_dropdown_set_selected", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_dropdown_set_selected},
    {"lv_dropdown_set_selected_highlight", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_dropdown_set_selected_highlight},
    {"lv_dropdown_set_symbol", &invoke_void_lv_obj_t_p_POINTER, (void*)&lv_dropdown_set_symbol},
    {"lv_dropdown_set_text", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_dropdown_set_text},
    {"lv_flex_init", &invoke_void, (void*)&lv_flex_init},
    {"lv_font_get_bitmap_fmt_txt", &invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p, (void*)&lv_font_get_bitmap_fmt_txt},
    {"lv_font_get_default", &invoke_lv_font_t_p, (void*)&lv_font_get_default},
    {"lv_font_get_glyph_bitmap", &invoke_POINTER_lv_font_glyph_dsc_t_p_lv_draw_buf_t_p, (void*)&lv_font_get_glyph_bitmap},
    {"lv_font_get_glyph_dsc", &invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT, (void*)&lv_font_get_glyph_dsc},
    {"lv_font_get_glyph_dsc_fmt_txt", &invoke_BOOL_lv_font_t_p_lv_font_glyph_dsc_t_p_INT_INT, (void*)&lv_font_get_glyph_dsc_fmt_txt},
    {"lv_font_get_glyph_width", &invoke_INT_lv_font_t_p_INT_INT, (void*)&lv_font_get_glyph_width},
    {"lv_font_get_line_height", &invoke_INT_lv_font_t_p, (void*)&lv_font_get_line_height},
    {"lv_font_glyph_release_draw_data", &invoke_void_lv_font_glyph_dsc_t_p, (void*)&lv_font_glyph_release_draw_data},
    {"lv_font_info_is_equal", &invoke_BOOL_lv_font_info_t_p_lv_font_info_t_p, (void*)&lv_font_info_is_equal},
    {"lv_font_set_kerning", &invoke_void_lv_font_t_p_INT, (void*)&lv_font_set_kerning},
    {"lv_free", &invoke_void, (void*)&lv_free},
    {"lv_free_core", &invoke_void, (void*)&lv_free_core},
    {"lv_fs_close", &invoke_INT_lv_fs_file_t_p, (void*)&lv_fs_close},
    {"lv_fs_dir_close", &invoke_INT_lv_fs_dir_t_p, (void*)&lv_fs_dir_close},
    {"lv_fs_dir_open", &invoke_INT_lv_fs_dir_t_p_const_char_p, (void*)&lv_fs_dir_open},
    {"lv_fs_dir_read", &invoke_INT_lv_fs_dir_t_p_const_char_p_INT, (void*)&lv_fs_dir_read},
    {"lv_fs_drv_create_managed", &invoke_lv_fs_drv_t_p_const_char_p, (void*)&lv_fs_drv_create_managed},
    {"lv_fs_drv_init", &invoke_void_lv_fs_drv_t_p, (void*)&lv_fs_drv_init},
    {"lv_fs_drv_register", &invoke_void_lv_fs_drv_t_p, (void*)&lv_fs_drv_register},
    {"lv_fs_get_drv", &invoke_lv_fs_drv_t_p_INT, (void*)&lv_fs_get_drv},
    {"lv_fs_get_ext", &invoke_const_char_p_const_char_p, (void*)&lv_fs_get_ext},
    {"lv_fs_get_last", &invoke_const_char_p_const_char_p, (void*)&lv_fs_get_last},
    {"lv_fs_get_letters", &invoke_const_char_p_const_char_p, (void*)&lv_fs_get_letters},
    {"lv_fs_is_ready", &invoke_BOOL_INT, (void*)&lv_fs_is_ready},
    {"lv_fs_make_path_from_buffer", &invoke_void_lv_fs_path_ex_t_p_INT_POINTER_INT, (void*)&lv_fs_make_path_from_buffer},
    {"lv_fs_open", &invoke_INT_lv_fs_file_t_p_const_char_p_INT, (void*)&lv_fs_open},
    {"lv_fs_read", &invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER, (void*)&lv_fs_read},
    {"lv_fs_seek", &invoke_INT_lv_fs_file_t_p_INT_INT, (void*)&lv_fs_seek},
    {"lv_fs_tell", &invoke_INT_lv_fs_file_t_p_POINTER, (void*)&lv_fs_tell},
    {"lv_fs_up", &invoke_const_char_p_const_char_p, (void*)&lv_fs_up},
    {"lv_fs_write", &invoke_INT_lv_fs_file_t_p_POINTER_INT_POINTER, (void*)&lv_fs_write},
    {"lv_grad_horizontal_init", &invoke_void_lv_grad_dsc_t_p, (void*)&lv_grad_horizontal_init},
    {"lv_grad_init_stops", &invoke_void_lv_grad_dsc_t_p_lv_color_t_p_lv_opa_t_p_POINTER_INT, (void*)&lv_grad_init_stops},
    {"lv_grad_vertical_init", &invoke_void_lv_grad_dsc_t_p, (void*)&lv_grad_vertical_init},
    {"lv_grid_fr", &invoke_INT_INT, (void*)&lv_grid_fr},
    {"lv_grid_init", &invoke_void, (void*)&lv_grid_init},
    {"lv_image_buf_free", &invoke_void_lv_image_dsc_t_p, (void*)&lv_image_buf_free},
    {"lv_image_buf_set_palette", &invoke_void_lv_image_dsc_t_p_INT_INT, (void*)&lv_image_buf_set_palette},
    {"lv_image_cache_drop", &invoke_void, (void*)&lv_image_cache_drop},
    {"lv_image_cache_dump", &invoke_void, (void*)&lv_image_cache_dump},
    {"lv_image_cache_init", &invoke_INT_INT, (void*)&lv_image_cache_init},
    {"lv_image_cache_is_enabled", &invoke_BOOL, (void*)&lv_image_cache_is_enabled},
    {"lv_image_cache_iter_create", &invoke_lv_iter_t_p, (void*)&lv_image_cache_iter_create},
    {"lv_image_cache_resize", &invoke_void_INT_BOOL, (void*)&lv_image_cache_resize},
    {"lv_image_create", &invoke_widget_create, (void*)&lv_image_create},
    {"lv_image_decoder_add_to_cache", &invoke_lv_cache_entry_t_p_lv_image_decoder_t_p_lv_image_cache_data_t_p_lv_draw_buf_t_p_POINTER, (void*)&lv_image_decoder_add_to_cache},
    {"lv_image_decoder_close", &invoke_void_lv_image_decoder_dsc_t_p, (void*)&lv_image_decoder_close},
    {"lv_image_decoder_create", &invoke_lv_image_decoder_t_p, (void*)&lv_image_decoder_create},
    {"lv_image_decoder_delete", &invoke_void_lv_image_decoder_t_p, (void*)&lv_image_decoder_delete},
    {"lv_image_decoder_get_area", &invoke_INT_lv_image_decoder_dsc_t_p_lv_area_t_p_lv_area_t_p, (void*)&lv_image_decoder_get_area},
    {"lv_image_decoder_get_info", &invoke_INT_POINTER_lv_image_header_t_p, (void*)&lv_image_decoder_get_info},
    {"lv_image_decoder_get_next", &invoke_lv_image_decoder_t_p_lv_image_decoder_t_p, (void*)&lv_image_decoder_get_next},
    {"lv_image_decoder_open", &invoke_INT_lv_image_decoder_dsc_t_p_POINTER_lv_image_decoder_args_t_p, (void*)&lv_image_decoder_open},
    {"lv_image_decoder_post_process", &invoke_lv_draw_buf_t_p_lv_image_decoder_dsc_t_p_lv_draw_buf_t_p, (void*)&lv_image_decoder_post_process},
    {"lv_image_decoder_set_close_cb", &invoke_void_lv_image_decoder_t_p_INT, (void*)&lv_image_decoder_set_close_cb},
    {"lv_image_decoder_set_get_area_cb", &invoke_void_lv_image_decoder_t_p_INT, (void*)&lv_image_decoder_set_get_area_cb},
    {"lv_image_decoder_set_info_cb", &invoke_void_lv_image_decoder_t_p_INT, (void*)&lv_image_decoder_set_info_cb},
    {"lv_image_decoder_set_open_cb", &invoke_void_lv_image_decoder_t_p_INT, (void*)&lv_image_decoder_set_open_cb},
    {"lv_image_get_antialias", &invoke_BOOL_lv_obj_t_p, (void*)&lv_image_get_antialias},
    {"lv_image_get_bitmap_map_src", &invoke_lv_image_dsc_t_p_lv_obj_t_p, (void*)&lv_image_get_bitmap_map_src},
    {"lv_image_get_blend_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_blend_mode},
    {"lv_image_get_inner_align", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_inner_align},
    {"lv_image_get_offset_x", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_offset_x},
    {"lv_image_get_offset_y", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_offset_y},
    {"lv_image_get_pivot", &invoke_void_lv_obj_t_p_lv_point_t_p, (void*)&lv_image_get_pivot},
    {"lv_image_get_rotation", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_rotation},
    {"lv_image_get_scale", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_scale},
    {"lv_image_get_scale_x", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_scale_x},
    {"lv_image_get_scale_y", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_scale_y},
    {"lv_image_get_src", &invoke_POINTER_lv_obj_t_p, (void*)&lv_image_get_src},
    {"lv_image_get_src_height", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_src_height},
    {"lv_image_get_src_width", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_src_width},
    {"lv_image_get_transformed_height", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_transformed_height},
    {"lv_image_get_transformed_width", &invoke_INT_lv_obj_t_p, (void*)&lv_image_get_transformed_width},
    {"lv_image_set_antialias", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_image_set_antialias},
    {"lv_image_set_bitmap_map_src", &invoke_void_lv_obj_t_p_lv_image_dsc_t_p, (void*)&lv_image_set_bitmap_map_src},
    {"lv_image_set_blend_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_blend_mode},
    {"lv_image_set_inner_align", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_inner_align},
    {"lv_image_set_offset_x", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_offset_x},
    {"lv_image_set_offset_y", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_offset_y},
    {"lv_image_set_pivot", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_image_set_pivot},
    {"lv_image_set_rotation", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_rotation},
    {"lv_image_set_scale", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_scale},
    {"lv_image_set_scale_x", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_scale_x},
    {"lv_image_set_scale_y", &invoke_void_lv_obj_t_p_INT, (void*)&lv_image_set_scale_y},
    {"lv_image_set_src", &invoke_void_lv_obj_t_p_POINTER, (void*)&lv_image_set_src},
    {"lv_image_src_get_type", &invoke_INT, (void*)&lv_image_src_get_type},
    {"lv_imagebutton_create", &invoke_widget_create, (void*)&lv_imagebutton_create},
    {"lv_imagebutton_get_src_left", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_imagebutton_get_src_left},
    {"lv_imagebutton_get_src_middle", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_imagebutton_get_src_middle},
    {"lv_imagebutton_get_src_right", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_imagebutton_get_src_right},
    {"lv_imagebutton_set_src", &invoke_void_lv_obj_t_p_INT_POINTER_POINTER_POINTER, (void*)&lv_imagebutton_set_src},
    {"lv_imagebutton_set_state", &invoke_void_lv_obj_t_p_INT, (void*)&lv_imagebutton_set_state},
    {"lv_init", &invoke_void, (void*)&lv_init},
    {"lv_is_initialized", &invoke_BOOL, (void*)&lv_is_initialized},
    {"lv_keyboard_create", &invoke_widget_create, (void*)&lv_keyboard_create},
    {"lv_keyboard_def_event_cb", &invoke_void_lv_event_t_p, (void*)&lv_keyboard_def_event_cb},
    {"lv_keyboard_get_button_text", &invoke_const_char_p_lv_obj_t_p_INT, (void*)&lv_keyboard_get_button_text},
    {"lv_keyboard_get_map_array", &invoke_POINTER_lv_obj_t_p, (void*)&lv_keyboard_get_map_array},
    {"lv_keyboard_get_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_keyboard_get_mode},
    {"lv_keyboard_get_popovers", &invoke_BOOL_lv_obj_t_p, (void*)&lv_keyboard_get_popovers},
    {"lv_keyboard_get_selected_button", &invoke_INT_lv_obj_t_p, (void*)&lv_keyboard_get_selected_button},
    {"lv_keyboard_get_textarea", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_keyboard_get_textarea},
    {"lv_keyboard_set_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_keyboard_set_mode},
    {"lv_keyboard_set_popovers", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_keyboard_set_popovers},
    {"lv_keyboard_set_textarea", &invoke_void_lv_obj_t_p_lv_obj_t_p, (void*)&lv_keyboard_set_textarea},
    {"lv_label_bind_text", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_const_char_p, (void*)&lv_label_bind_text},
    {"lv_label_create", &invoke_widget_create, (void*)&lv_label_create},
    {"lv_label_cut_text", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_label_cut_text},
    {"lv_label_get_letter_on", &invoke_INT_lv_obj_t_p_lv_point_t_p_BOOL, (void*)&lv_label_get_letter_on},
    {"lv_label_get_letter_pos", &invoke_void_lv_obj_t_p_INT_lv_point_t_p, (void*)&lv_label_get_letter_pos},
    {"lv_label_get_long_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_label_get_long_mode},
    {"lv_label_get_recolor", &invoke_BOOL_lv_obj_t_p, (void*)&lv_label_get_recolor},
    {"lv_label_get_text", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_label_get_text},
    {"lv_label_get_text_selection_end", &invoke_INT_lv_obj_t_p, (void*)&lv_label_get_text_selection_end},
    {"lv_label_get_text_selection_start", &invoke_INT_lv_obj_t_p, (void*)&lv_label_get_text_selection_start},
    {"lv_label_ins_text", &invoke_void_lv_obj_t_p_INT_const_char_p, (void*)&lv_label_ins_text},
    {"lv_label_is_char_under_pos", &invoke_BOOL_lv_obj_t_p_lv_point_t_p, (void*)&lv_label_is_char_under_pos},
    {"lv_label_set_long_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_label_set_long_mode},
    {"lv_label_set_recolor", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_label_set_recolor},
    {"lv_label_set_text", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_label_set_text},
    {"lv_label_set_text_selection_end", &invoke_void_lv_obj_t_p_INT, (void*)&lv_label_set_text_selection_end},
    {"lv_label_set_text_selection_start", &invoke_void_lv_obj_t_p_INT, (void*)&lv_label_set_text_selection_start},
    {"lv_label_set_text_static", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_label_set_text_static},
    {"lv_layer_bottom", &invoke_lv_obj_t_p, (void*)&lv_layer_bottom},
    {"lv_layer_create_managed", &invoke_lv_layer_t_p_const_char_p, (void*)&lv_layer_create_managed},
    {"lv_layer_init", &invoke_void_lv_layer_t_p, (void*)&lv_layer_init},
    {"lv_layer_reset", &invoke_void_lv_layer_t_p, (void*)&lv_layer_reset},
    {"lv_layer_sys", &invoke_lv_obj_t_p, (void*)&lv_layer_sys},
    {"lv_layer_top", &invoke_lv_obj_t_p, (void*)&lv_layer_top},
    {"lv_layout_register", &invoke_INT_INT_POINTER, (void*)&lv_layout_register},
    {"lv_line_create", &invoke_widget_create, (void*)&lv_line_create},
    {"lv_line_get_point_count", &invoke_INT_lv_obj_t_p, (void*)&lv_line_get_point_count},
    {"lv_line_get_points", &invoke_lv_point_precise_t_p_lv_obj_t_p, (void*)&lv_line_get_points},
    {"lv_line_get_points_mutable", &invoke_lv_point_precise_t_p_lv_obj_t_p, (void*)&lv_line_get_points_mutable},
    {"lv_line_get_y_invert", &invoke_BOOL_lv_obj_t_p, (void*)&lv_line_get_y_invert},
    {"lv_line_is_point_array_mutable", &invoke_BOOL_lv_obj_t_p, (void*)&lv_line_is_point_array_mutable},
    {"lv_line_set_points", &invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT, (void*)&lv_line_set_points},
    {"lv_line_set_points_mutable", &invoke_void_lv_obj_t_p_lv_point_precise_t_p_INT, (void*)&lv_line_set_points_mutable},
    {"lv_line_set_y_invert", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_line_set_y_invert},
    {"lv_list_add_button", &invoke_lv_obj_t_p_lv_obj_t_p_POINTER_const_char_p, (void*)&lv_list_add_button},
    {"lv_list_add_text", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_list_add_text},
    {"lv_list_create", &invoke_widget_create, (void*)&lv_list_create},
    {"lv_list_get_button_text", &invoke_const_char_p_lv_obj_t_p_lv_obj_t_p, (void*)&lv_list_get_button_text},
    {"lv_list_set_button_text", &invoke_void_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_list_set_button_text},
    {"lv_ll_chg_list", &invoke_void_lv_ll_t_p_lv_ll_t_p_POINTER_BOOL, (void*)&lv_ll_chg_list},
    {"lv_ll_clear", &invoke_void_lv_ll_t_p, (void*)&lv_ll_clear},
    {"lv_ll_get_head", &invoke_POINTER_lv_ll_t_p, (void*)&lv_ll_get_head},
    {"lv_ll_get_len", &invoke_INT_lv_ll_t_p, (void*)&lv_ll_get_len},
    {"lv_ll_get_next", &invoke_POINTER_lv_ll_t_p_POINTER, (void*)&lv_ll_get_next},
    {"lv_ll_get_prev", &invoke_POINTER_lv_ll_t_p_POINTER, (void*)&lv_ll_get_prev},
    {"lv_ll_get_tail", &invoke_POINTER_lv_ll_t_p, (void*)&lv_ll_get_tail},
    {"lv_ll_init", &invoke_void_lv_ll_t_p_INT, (void*)&lv_ll_init},
    {"lv_ll_ins_head", &invoke_POINTER_lv_ll_t_p, (void*)&lv_ll_ins_head},
    {"lv_ll_ins_prev", &invoke_POINTER_lv_ll_t_p_POINTER, (void*)&lv_ll_ins_prev},
    {"lv_ll_ins_tail", &invoke_POINTER_lv_ll_t_p, (void*)&lv_ll_ins_tail},
    {"lv_ll_is_empty", &invoke_BOOL_lv_ll_t_p, (void*)&lv_ll_is_empty},
    {"lv_ll_move_before", &invoke_void_lv_ll_t_p_POINTER_POINTER, (void*)&lv_ll_move_before},
    {"lv_ll_remove", &invoke_void_lv_ll_t_p_POINTER, (void*)&lv_ll_remove},
    {"lv_malloc", &invoke_POINTER_INT, (void*)&lv_malloc},
    {"lv_malloc_core", &invoke_POINTER_INT, (void*)&lv_malloc_core},
    {"lv_malloc_zeroed", &invoke_POINTER_INT, (void*)&lv_malloc_zeroed},
    {"lv_map", &invoke_INT_INT_INT_INT_INT_INT, (void*)&lv_map},
    {"lv_mem_add_pool", &invoke_INT_POINTER_INT, (void*)&lv_mem_add_pool},
    {"lv_mem_deinit", &invoke_void, (void*)&lv_mem_deinit},
    {"lv_mem_init", &invoke_void, (void*)&lv_mem_init},
    {"lv_mem_monitor", &invoke_void_lv_mem_monitor_t_p, (void*)&lv_mem_monitor},
    {"lv_mem_monitor_core", &invoke_void_lv_mem_monitor_t_p, (void*)&lv_mem_monitor_core},
    {"lv_mem_remove_pool", &invoke_void_INT, (void*)&lv_mem_remove_pool},
    {"lv_mem_test", &invoke_INT, (void*)&lv_mem_test},
    {"lv_mem_test_core", &invoke_INT, (void*)&lv_mem_test_core},
    {"lv_memcmp", &invoke_INT_POINTER_POINTER_INT, (void*)&lv_memcmp},
    {"lv_memcpy", &invoke_POINTER_POINTER_POINTER_INT, (void*)&lv_memcpy},
    {"lv_memmove", &invoke_POINTER_POINTER_POINTER_INT, (void*)&lv_memmove},
    {"lv_memset", &invoke_void_POINTER_INT_INT, (void*)&lv_memset},
    {"lv_memzero", &invoke_void_POINTER_INT, (void*)&lv_memzero},
    {"lv_menu_back_button_is_root", &invoke_BOOL_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_back_button_is_root},
    {"lv_menu_clear_history", &invoke_void_lv_obj_t_p, (void*)&lv_menu_clear_history},
    {"lv_menu_cont_create", &invoke_widget_create, (void*)&lv_menu_cont_create},
    {"lv_menu_create", &invoke_widget_create, (void*)&lv_menu_create},
    {"lv_menu_get_cur_main_page", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_get_cur_main_page},
    {"lv_menu_get_cur_sidebar_page", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_get_cur_sidebar_page},
    {"lv_menu_get_main_header", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_get_main_header},
    {"lv_menu_get_main_header_back_button", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_get_main_header_back_button},
    {"lv_menu_get_sidebar_header", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_get_sidebar_header},
    {"lv_menu_get_sidebar_header_back_button", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_get_sidebar_header_back_button},
    {"lv_menu_page_create", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_menu_page_create},
    {"lv_menu_section_create", &invoke_widget_create, (void*)&lv_menu_section_create},
    {"lv_menu_separator_create", &invoke_widget_create, (void*)&lv_menu_separator_create},
    {"lv_menu_set_load_page_event", &invoke_void_lv_obj_t_p_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_set_load_page_event},
    {"lv_menu_set_mode_header", &invoke_void_lv_obj_t_p_INT, (void*)&lv_menu_set_mode_header},
    {"lv_menu_set_mode_root_back_button", &invoke_void_lv_obj_t_p_INT, (void*)&lv_menu_set_mode_root_back_button},
    {"lv_menu_set_page", &invoke_void_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_set_page},
    {"lv_menu_set_page_title", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_menu_set_page_title},
    {"lv_menu_set_page_title_static", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_menu_set_page_title_static},
    {"lv_menu_set_sidebar_page", &invoke_void_lv_obj_t_p_lv_obj_t_p, (void*)&lv_menu_set_sidebar_page},
    {"lv_msgbox_add_close_button", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_msgbox_add_close_button},
    {"lv_msgbox_add_footer_button", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_msgbox_add_footer_button},
    {"lv_msgbox_add_header_button", &invoke_lv_obj_t_p_lv_obj_t_p_POINTER, (void*)&lv_msgbox_add_header_button},
    {"lv_msgbox_add_text", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_msgbox_add_text},
    {"lv_msgbox_add_title", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_msgbox_add_title},
    {"lv_msgbox_close", &invoke_void_lv_obj_t_p, (void*)&lv_msgbox_close},
    {"lv_msgbox_close_async", &invoke_void_lv_obj_t_p, (void*)&lv_msgbox_close_async},
    {"lv_msgbox_create", &invoke_widget_create, (void*)&lv_msgbox_create},
    {"lv_msgbox_get_content", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_msgbox_get_content},
    {"lv_msgbox_get_footer", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_msgbox_get_footer},
    {"lv_msgbox_get_header", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_msgbox_get_header},
    {"lv_msgbox_get_title", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_msgbox_get_title},
    {"lv_obj_add_event_cb", &invoke_lv_event_dsc_t_p_lv_obj_t_p_INT_INT_POINTER, (void*)&lv_obj_add_event_cb},
    {"lv_obj_add_flag", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_add_flag},
    {"lv_obj_add_state", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_add_state},
    {"lv_obj_add_style", &invoke_void_lv_obj_t_p_lv_style_t_p_INT, (void*)&lv_obj_add_style},
    {"lv_obj_align", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_align},
    {"lv_obj_align_to", &invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_align_to},
    {"lv_obj_allocate_spec_attr", &invoke_void_lv_obj_t_p, (void*)&lv_obj_allocate_spec_attr},
    {"lv_obj_area_is_visible", &invoke_BOOL_lv_obj_t_p_lv_area_t_p, (void*)&lv_obj_area_is_visible},
    {"lv_obj_bind_checked", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p, (void*)&lv_obj_bind_checked},
    {"lv_obj_bind_flag_if_eq", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_flag_if_eq},
    {"lv_obj_bind_flag_if_ge", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_flag_if_ge},
    {"lv_obj_bind_flag_if_gt", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_flag_if_gt},
    {"lv_obj_bind_flag_if_le", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_flag_if_le},
    {"lv_obj_bind_flag_if_lt", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_flag_if_lt},
    {"lv_obj_bind_flag_if_not_eq", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_flag_if_not_eq},
    {"lv_obj_bind_state_if_eq", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_state_if_eq},
    {"lv_obj_bind_state_if_ge", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_state_if_ge},
    {"lv_obj_bind_state_if_gt", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_state_if_gt},
    {"lv_obj_bind_state_if_le", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_state_if_le},
    {"lv_obj_bind_state_if_lt", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_state_if_lt},
    {"lv_obj_bind_state_if_not_eq", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p_INT_INT, (void*)&lv_obj_bind_state_if_not_eq},
    {"lv_obj_calculate_ext_draw_size", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_calculate_ext_draw_size},
    {"lv_obj_calculate_style_text_align", &invoke_INT_lv_obj_t_p_INT_const_char_p, (void*)&lv_obj_calculate_style_text_align},
    {"lv_obj_center", &invoke_void_lv_obj_t_p, (void*)&lv_obj_center},
    {"lv_obj_check_type", &invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p, (void*)&lv_obj_check_type},
    {"lv_obj_clean", &invoke_void_lv_obj_t_p, (void*)&lv_obj_clean},
    {"lv_obj_create", &invoke_widget_create, (void*)&lv_obj_create},
    {"lv_obj_delete", &invoke_void_lv_obj_t_p, (void*)&lv_obj_delete},
    {"lv_obj_delete_anim_completed_cb", &invoke_void_lv_anim_t_p, (void*)&lv_obj_delete_anim_completed_cb},
    {"lv_obj_delete_async", &invoke_void_lv_obj_t_p, (void*)&lv_obj_delete_async},
    {"lv_obj_delete_delayed", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_delete_delayed},
    {"lv_obj_dump_tree", &invoke_void_lv_obj_t_p, (void*)&lv_obj_dump_tree},
    {"lv_obj_enable_style_refresh", &invoke_void_BOOL, (void*)&lv_obj_enable_style_refresh},
    {"lv_obj_event_base", &invoke_INT_lv_obj_class_t_p_lv_event_t_p, (void*)&lv_obj_event_base},
    {"lv_obj_fade_in", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_fade_in},
    {"lv_obj_fade_out", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_fade_out},
    {"lv_obj_get_child", &invoke_lv_obj_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_child},
    {"lv_obj_get_child_by_type", &invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p, (void*)&lv_obj_get_child_by_type},
    {"lv_obj_get_child_count", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_child_count},
    {"lv_obj_get_child_count_by_type", &invoke_INT_lv_obj_t_p_lv_obj_class_t_p, (void*)&lv_obj_get_child_count_by_type},
    {"lv_obj_get_class", &invoke_lv_obj_class_t_p_lv_obj_t_p, (void*)&lv_obj_get_class},
    {"lv_obj_get_click_area", &invoke_void_lv_obj_t_p_lv_area_t_p, (void*)&lv_obj_get_click_area},
    {"lv_obj_get_content_coords", &invoke_void_lv_obj_t_p_lv_area_t_p, (void*)&lv_obj_get_content_coords},
    {"lv_obj_get_content_height", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_content_height},
    {"lv_obj_get_content_width", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_content_width},
    {"lv_obj_get_coords", &invoke_void_lv_obj_t_p_lv_area_t_p, (void*)&lv_obj_get_coords},
    {"lv_obj_get_display", &invoke_lv_display_t_p_lv_obj_t_p, (void*)&lv_obj_get_display},
    {"lv_obj_get_event_count", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_event_count},
    {"lv_obj_get_event_dsc", &invoke_lv_event_dsc_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_event_dsc},
    {"lv_obj_get_group", &invoke_lv_group_t_p_lv_obj_t_p, (void*)&lv_obj_get_group},
    {"lv_obj_get_height", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_height},
    {"lv_obj_get_index", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_index},
    {"lv_obj_get_index_by_type", &invoke_INT_lv_obj_t_p_lv_obj_class_t_p, (void*)&lv_obj_get_index_by_type},
    {"lv_obj_get_local_style_prop", &invoke_INT_lv_obj_t_p_INT_lv_style_value_t_p_INT, (void*)&lv_obj_get_local_style_prop},
    {"lv_obj_get_parent", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_obj_get_parent},
    {"lv_obj_get_screen", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_obj_get_screen},
    {"lv_obj_get_scroll_bottom", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_bottom},
    {"lv_obj_get_scroll_dir", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_dir},
    {"lv_obj_get_scroll_end", &invoke_void_lv_obj_t_p_lv_point_t_p, (void*)&lv_obj_get_scroll_end},
    {"lv_obj_get_scroll_left", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_left},
    {"lv_obj_get_scroll_right", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_right},
    {"lv_obj_get_scroll_snap_x", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_snap_x},
    {"lv_obj_get_scroll_snap_y", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_snap_y},
    {"lv_obj_get_scroll_top", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_top},
    {"lv_obj_get_scroll_x", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_x},
    {"lv_obj_get_scroll_y", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scroll_y},
    {"lv_obj_get_scrollbar_area", &invoke_void_lv_obj_t_p_lv_area_t_p_lv_area_t_p, (void*)&lv_obj_get_scrollbar_area},
    {"lv_obj_get_scrollbar_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_scrollbar_mode},
    {"lv_obj_get_self_height", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_self_height},
    {"lv_obj_get_self_width", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_self_width},
    {"lv_obj_get_sibling", &invoke_lv_obj_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_sibling},
    {"lv_obj_get_sibling_by_type", &invoke_lv_obj_t_p_lv_obj_t_p_INT_lv_obj_class_t_p, (void*)&lv_obj_get_sibling_by_type},
    {"lv_obj_get_state", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_state},
    {"lv_obj_get_style_align", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_align},
    {"lv_obj_get_style_anim", &invoke_lv_anim_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_style_anim},
    {"lv_obj_get_style_anim_duration", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_anim_duration},
    {"lv_obj_get_style_arc_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_arc_color},
    {"lv_obj_get_style_arc_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_arc_color_filtered},
    {"lv_obj_get_style_arc_image_src", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_obj_get_style_arc_image_src},
    {"lv_obj_get_style_arc_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_arc_opa},
    {"lv_obj_get_style_arc_rounded", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_get_style_arc_rounded},
    {"lv_obj_get_style_arc_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_arc_width},
    {"lv_obj_get_style_base_dir", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_base_dir},
    {"lv_obj_get_style_bg_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_color},
    {"lv_obj_get_style_bg_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_color_filtered},
    {"lv_obj_get_style_bg_grad", &invoke_lv_grad_dsc_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_grad},
    {"lv_obj_get_style_bg_grad_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_grad_color},
    {"lv_obj_get_style_bg_grad_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_grad_color_filtered},
    {"lv_obj_get_style_bg_grad_dir", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_grad_dir},
    {"lv_obj_get_style_bg_grad_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_grad_opa},
    {"lv_obj_get_style_bg_grad_stop", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_grad_stop},
    {"lv_obj_get_style_bg_image_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_image_opa},
    {"lv_obj_get_style_bg_image_recolor", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_image_recolor},
    {"lv_obj_get_style_bg_image_recolor_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_image_recolor_filtered},
    {"lv_obj_get_style_bg_image_recolor_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_image_recolor_opa},
    {"lv_obj_get_style_bg_image_src", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_image_src},
    {"lv_obj_get_style_bg_image_tiled", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_image_tiled},
    {"lv_obj_get_style_bg_main_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_main_opa},
    {"lv_obj_get_style_bg_main_stop", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_main_stop},
    {"lv_obj_get_style_bg_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bg_opa},
    {"lv_obj_get_style_bitmap_mask_src", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_obj_get_style_bitmap_mask_src},
    {"lv_obj_get_style_blend_mode", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_blend_mode},
    {"lv_obj_get_style_border_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_border_color},
    {"lv_obj_get_style_border_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_border_color_filtered},
    {"lv_obj_get_style_border_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_border_opa},
    {"lv_obj_get_style_border_post", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_get_style_border_post},
    {"lv_obj_get_style_border_side", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_border_side},
    {"lv_obj_get_style_border_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_border_width},
    {"lv_obj_get_style_clip_corner", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_get_style_clip_corner},
    {"lv_obj_get_style_color_filter_dsc", &invoke_lv_color_filter_dsc_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_style_color_filter_dsc},
    {"lv_obj_get_style_color_filter_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_color_filter_opa},
    {"lv_obj_get_style_flex_cross_place", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_flex_cross_place},
    {"lv_obj_get_style_flex_flow", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_flex_flow},
    {"lv_obj_get_style_flex_grow", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_flex_grow},
    {"lv_obj_get_style_flex_main_place", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_flex_main_place},
    {"lv_obj_get_style_flex_track_place", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_flex_track_place},
    {"lv_obj_get_style_grid_cell_column_pos", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_cell_column_pos},
    {"lv_obj_get_style_grid_cell_column_span", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_cell_column_span},
    {"lv_obj_get_style_grid_cell_row_pos", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_cell_row_pos},
    {"lv_obj_get_style_grid_cell_row_span", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_cell_row_span},
    {"lv_obj_get_style_grid_cell_x_align", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_cell_x_align},
    {"lv_obj_get_style_grid_cell_y_align", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_cell_y_align},
    {"lv_obj_get_style_grid_column_align", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_column_align},
    {"lv_obj_get_style_grid_column_dsc_array", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_column_dsc_array},
    {"lv_obj_get_style_grid_row_align", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_row_align},
    {"lv_obj_get_style_grid_row_dsc_array", &invoke_POINTER_lv_obj_t_p_INT, (void*)&lv_obj_get_style_grid_row_dsc_array},
    {"lv_obj_get_style_height", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_height},
    {"lv_obj_get_style_image_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_image_opa},
    {"lv_obj_get_style_image_recolor", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_image_recolor},
    {"lv_obj_get_style_image_recolor_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_image_recolor_filtered},
    {"lv_obj_get_style_image_recolor_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_image_recolor_opa},
    {"lv_obj_get_style_layout", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_layout},
    {"lv_obj_get_style_length", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_length},
    {"lv_obj_get_style_line_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_color},
    {"lv_obj_get_style_line_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_color_filtered},
    {"lv_obj_get_style_line_dash_gap", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_dash_gap},
    {"lv_obj_get_style_line_dash_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_dash_width},
    {"lv_obj_get_style_line_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_opa},
    {"lv_obj_get_style_line_rounded", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_rounded},
    {"lv_obj_get_style_line_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_line_width},
    {"lv_obj_get_style_margin_bottom", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_margin_bottom},
    {"lv_obj_get_style_margin_left", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_margin_left},
    {"lv_obj_get_style_margin_right", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_margin_right},
    {"lv_obj_get_style_margin_top", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_margin_top},
    {"lv_obj_get_style_max_height", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_max_height},
    {"lv_obj_get_style_max_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_max_width},
    {"lv_obj_get_style_min_height", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_min_height},
    {"lv_obj_get_style_min_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_min_width},
    {"lv_obj_get_style_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_opa},
    {"lv_obj_get_style_opa_layered", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_opa_layered},
    {"lv_obj_get_style_opa_recursive", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_opa_recursive},
    {"lv_obj_get_style_outline_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_outline_color},
    {"lv_obj_get_style_outline_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_outline_color_filtered},
    {"lv_obj_get_style_outline_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_outline_opa},
    {"lv_obj_get_style_outline_pad", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_outline_pad},
    {"lv_obj_get_style_outline_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_outline_width},
    {"lv_obj_get_style_pad_bottom", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_bottom},
    {"lv_obj_get_style_pad_column", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_column},
    {"lv_obj_get_style_pad_left", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_left},
    {"lv_obj_get_style_pad_radial", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_radial},
    {"lv_obj_get_style_pad_right", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_right},
    {"lv_obj_get_style_pad_row", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_row},
    {"lv_obj_get_style_pad_top", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_pad_top},
    {"lv_obj_get_style_prop", &invoke_INT_lv_obj_t_p_INT_INT, (void*)&lv_obj_get_style_prop},
    {"lv_obj_get_style_radial_offset", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_radial_offset},
    {"lv_obj_get_style_radius", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_radius},
    {"lv_obj_get_style_recolor", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_recolor},
    {"lv_obj_get_style_recolor_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_recolor_opa},
    {"lv_obj_get_style_recolor_recursive", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_recolor_recursive},
    {"lv_obj_get_style_rotary_sensitivity", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_rotary_sensitivity},
    {"lv_obj_get_style_shadow_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_color},
    {"lv_obj_get_style_shadow_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_color_filtered},
    {"lv_obj_get_style_shadow_offset_x", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_offset_x},
    {"lv_obj_get_style_shadow_offset_y", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_offset_y},
    {"lv_obj_get_style_shadow_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_opa},
    {"lv_obj_get_style_shadow_spread", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_spread},
    {"lv_obj_get_style_shadow_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_shadow_width},
    {"lv_obj_get_style_space_bottom", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_space_bottom},
    {"lv_obj_get_style_space_left", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_space_left},
    {"lv_obj_get_style_space_right", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_space_right},
    {"lv_obj_get_style_space_top", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_space_top},
    {"lv_obj_get_style_text_align", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_align},
    {"lv_obj_get_style_text_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_color},
    {"lv_obj_get_style_text_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_color_filtered},
    {"lv_obj_get_style_text_decor", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_decor},
    {"lv_obj_get_style_text_font", &invoke_lv_font_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_font},
    {"lv_obj_get_style_text_letter_space", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_letter_space},
    {"lv_obj_get_style_text_line_space", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_line_space},
    {"lv_obj_get_style_text_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_opa},
    {"lv_obj_get_style_text_outline_stroke_color", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_outline_stroke_color},
    {"lv_obj_get_style_text_outline_stroke_color_filtered", &invoke_lv_color_t_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_outline_stroke_color_filtered},
    {"lv_obj_get_style_text_outline_stroke_opa", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_outline_stroke_opa},
    {"lv_obj_get_style_text_outline_stroke_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_text_outline_stroke_width},
    {"lv_obj_get_style_transform_height", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_height},
    {"lv_obj_get_style_transform_pivot_x", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_pivot_x},
    {"lv_obj_get_style_transform_pivot_y", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_pivot_y},
    {"lv_obj_get_style_transform_rotation", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_rotation},
    {"lv_obj_get_style_transform_scale_x", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_scale_x},
    {"lv_obj_get_style_transform_scale_x_safe", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_scale_x_safe},
    {"lv_obj_get_style_transform_scale_y", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_scale_y},
    {"lv_obj_get_style_transform_scale_y_safe", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_scale_y_safe},
    {"lv_obj_get_style_transform_skew_x", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_skew_x},
    {"lv_obj_get_style_transform_skew_y", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_skew_y},
    {"lv_obj_get_style_transform_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transform_width},
    {"lv_obj_get_style_transition", &invoke_lv_style_transition_dsc_t_p_lv_obj_t_p_INT, (void*)&lv_obj_get_style_transition},
    {"lv_obj_get_style_translate_radial", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_translate_radial},
    {"lv_obj_get_style_translate_x", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_translate_x},
    {"lv_obj_get_style_translate_y", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_translate_y},
    {"lv_obj_get_style_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_width},
    {"lv_obj_get_style_x", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_x},
    {"lv_obj_get_style_y", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_get_style_y},
    {"lv_obj_get_transform", &invoke_lv_matrix_t_p_lv_obj_t_p, (void*)&lv_obj_get_transform},
    {"lv_obj_get_transformed_area", &invoke_void_lv_obj_t_p_lv_area_t_p_INT, (void*)&lv_obj_get_transformed_area},
    {"lv_obj_get_user_data", &invoke_POINTER_lv_obj_t_p, (void*)&lv_obj_get_user_data},
    {"lv_obj_get_width", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_width},
    {"lv_obj_get_x", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_x},
    {"lv_obj_get_x2", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_x2},
    {"lv_obj_get_x_aligned", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_x_aligned},
    {"lv_obj_get_y", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_y},
    {"lv_obj_get_y2", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_y2},
    {"lv_obj_get_y_aligned", &invoke_INT_lv_obj_t_p, (void*)&lv_obj_get_y_aligned},
    {"lv_obj_has_class", &invoke_BOOL_lv_obj_t_p_lv_obj_class_t_p, (void*)&lv_obj_has_class},
    {"lv_obj_has_flag", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_has_flag},
    {"lv_obj_has_flag_any", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_has_flag_any},
    {"lv_obj_has_state", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_has_state},
    {"lv_obj_has_style_prop", &invoke_BOOL_lv_obj_t_p_INT_INT, (void*)&lv_obj_has_style_prop},
    {"lv_obj_hit_test", &invoke_BOOL_lv_obj_t_p_lv_point_t_p, (void*)&lv_obj_hit_test},
    {"lv_obj_init_draw_arc_dsc", &invoke_void_lv_obj_t_p_INT_lv_draw_arc_dsc_t_p, (void*)&lv_obj_init_draw_arc_dsc},
    {"lv_obj_init_draw_image_dsc", &invoke_void_lv_obj_t_p_INT_lv_draw_image_dsc_t_p, (void*)&lv_obj_init_draw_image_dsc},
    {"lv_obj_init_draw_label_dsc", &invoke_void_lv_obj_t_p_INT_lv_draw_label_dsc_t_p, (void*)&lv_obj_init_draw_label_dsc},
    {"lv_obj_init_draw_line_dsc", &invoke_void_lv_obj_t_p_INT_lv_draw_line_dsc_t_p, (void*)&lv_obj_init_draw_line_dsc},
    {"lv_obj_init_draw_rect_dsc", &invoke_void_lv_obj_t_p_INT_lv_draw_rect_dsc_t_p, (void*)&lv_obj_init_draw_rect_dsc},
    {"lv_obj_invalidate", &invoke_void_lv_obj_t_p, (void*)&lv_obj_invalidate},
    {"lv_obj_invalidate_area", &invoke_void_lv_obj_t_p_lv_area_t_p, (void*)&lv_obj_invalidate_area},
    {"lv_obj_is_editable", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_is_editable},
    {"lv_obj_is_group_def", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_is_group_def},
    {"lv_obj_is_layout_positioned", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_is_layout_positioned},
    {"lv_obj_is_scrolling", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_is_scrolling},
    {"lv_obj_is_valid", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_is_valid},
    {"lv_obj_is_visible", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_is_visible},
    {"lv_obj_mark_layout_as_dirty", &invoke_void_lv_obj_t_p, (void*)&lv_obj_mark_layout_as_dirty},
    {"lv_obj_move_background", &invoke_void_lv_obj_t_p, (void*)&lv_obj_move_background},
    {"lv_obj_move_children_by", &invoke_void_lv_obj_t_p_INT_INT_BOOL, (void*)&lv_obj_move_children_by},
    {"lv_obj_move_foreground", &invoke_void_lv_obj_t_p, (void*)&lv_obj_move_foreground},
    {"lv_obj_move_to", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_move_to},
    {"lv_obj_move_to_index", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_move_to_index},
    {"lv_obj_null_on_delete", &invoke_void_POINTER, (void*)&lv_obj_null_on_delete},
    {"lv_obj_readjust_scroll", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_readjust_scroll},
    {"lv_obj_redraw", &invoke_void_lv_layer_t_p_lv_obj_t_p, (void*)&lv_obj_redraw},
    {"lv_obj_refr_pos", &invoke_void_lv_obj_t_p, (void*)&lv_obj_refr_pos},
    {"lv_obj_refr_size", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_refr_size},
    {"lv_obj_refresh_ext_draw_size", &invoke_void_lv_obj_t_p, (void*)&lv_obj_refresh_ext_draw_size},
    {"lv_obj_refresh_self_size", &invoke_BOOL_lv_obj_t_p, (void*)&lv_obj_refresh_self_size},
    {"lv_obj_refresh_style", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_refresh_style},
    {"lv_obj_remove_event", &invoke_BOOL_lv_obj_t_p_INT, (void*)&lv_obj_remove_event},
    {"lv_obj_remove_event_cb", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_obj_remove_event_cb},
    {"lv_obj_remove_event_cb_with_user_data", &invoke_INT_lv_obj_t_p_INT_POINTER, (void*)&lv_obj_remove_event_cb_with_user_data},
    {"lv_obj_remove_event_dsc", &invoke_BOOL_lv_obj_t_p_lv_event_dsc_t_p, (void*)&lv_obj_remove_event_dsc},
    {"lv_obj_remove_flag", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_remove_flag},
    {"lv_obj_remove_from_subject", &invoke_void_lv_obj_t_p_lv_subject_t_p, (void*)&lv_obj_remove_from_subject},
    {"lv_obj_remove_local_style_prop", &invoke_BOOL_lv_obj_t_p_INT_INT, (void*)&lv_obj_remove_local_style_prop},
    {"lv_obj_remove_state", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_remove_state},
    {"lv_obj_remove_style", &invoke_void_lv_obj_t_p_lv_style_t_p_INT, (void*)&lv_obj_remove_style},
    {"lv_obj_remove_style_all", &invoke_void_lv_obj_t_p, (void*)&lv_obj_remove_style_all},
    {"lv_obj_replace_style", &invoke_BOOL_lv_obj_t_p_lv_style_t_p_lv_style_t_p_INT, (void*)&lv_obj_replace_style},
    {"lv_obj_report_style_change", &invoke_void_lv_style_t_p, (void*)&lv_obj_report_style_change},
    {"lv_obj_reset_transform", &invoke_void_lv_obj_t_p, (void*)&lv_obj_reset_transform},
    {"lv_obj_scroll_by", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_scroll_by},
    {"lv_obj_scroll_by_bounded", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_scroll_by_bounded},
    {"lv_obj_scroll_to", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_scroll_to},
    {"lv_obj_scroll_to_view", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_scroll_to_view},
    {"lv_obj_scroll_to_view_recursive", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_scroll_to_view_recursive},
    {"lv_obj_scroll_to_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_scroll_to_x},
    {"lv_obj_scroll_to_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_scroll_to_y},
    {"lv_obj_scrollbar_invalidate", &invoke_void_lv_obj_t_p, (void*)&lv_obj_scrollbar_invalidate},
    {"lv_obj_send_event", &invoke_INT_lv_obj_t_p_INT_POINTER, (void*)&lv_obj_send_event},
    {"lv_obj_set_align", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_align},
    {"lv_obj_set_content_height", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_content_height},
    {"lv_obj_set_content_width", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_content_width},
    {"lv_obj_set_ext_click_area", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_ext_click_area},
    {"lv_obj_set_flag", &invoke_void_lv_obj_t_p_INT_BOOL, (void*)&lv_obj_set_flag},
    {"lv_obj_set_flex_align", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_set_flex_align},
    {"lv_obj_set_flex_flow", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_flex_flow},
    {"lv_obj_set_flex_grow", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_flex_grow},
    {"lv_obj_set_grid_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_grid_align},
    {"lv_obj_set_grid_cell", &invoke_void_lv_obj_t_p_INT_INT_INT_INT_INT_INT, (void*)&lv_obj_set_grid_cell},
    {"lv_obj_set_grid_dsc_array", &invoke_void_lv_obj_t_p_POINTER_POINTER, (void*)&lv_obj_set_grid_dsc_array},
    {"lv_obj_set_height", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_height},
    {"lv_obj_set_layout", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_layout},
    {"lv_obj_set_local_style_prop", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_set_local_style_prop},
    {"lv_obj_set_parent", &invoke_void_lv_obj_t_p_lv_obj_t_p, (void*)&lv_obj_set_parent},
    {"lv_obj_set_pos", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_pos},
    {"lv_obj_set_scroll_dir", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_scroll_dir},
    {"lv_obj_set_scroll_snap_x", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_scroll_snap_x},
    {"lv_obj_set_scroll_snap_y", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_scroll_snap_y},
    {"lv_obj_set_scrollbar_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_scrollbar_mode},
    {"lv_obj_set_size", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_size},
    {"lv_obj_set_state", &invoke_void_lv_obj_t_p_INT_BOOL, (void*)&lv_obj_set_state},
    {"lv_obj_set_style_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_align},
    {"lv_obj_set_style_anim", &invoke_void_lv_obj_t_p_lv_anim_t_p_INT, (void*)&lv_obj_set_style_anim},
    {"lv_obj_set_style_anim_duration", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_anim_duration},
    {"lv_obj_set_style_arc_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_arc_color},
    {"lv_obj_set_style_arc_image_src", &invoke_void_lv_obj_t_p_POINTER_INT, (void*)&lv_obj_set_style_arc_image_src},
    {"lv_obj_set_style_arc_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_arc_opa},
    {"lv_obj_set_style_arc_rounded", &invoke_void_lv_obj_t_p_BOOL_INT, (void*)&lv_obj_set_style_arc_rounded},
    {"lv_obj_set_style_arc_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_arc_width},
    {"lv_obj_set_style_base_dir", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_base_dir},
    {"lv_obj_set_style_bg_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_bg_color},
    {"lv_obj_set_style_bg_grad", &invoke_void_lv_obj_t_p_lv_grad_dsc_t_p_INT, (void*)&lv_obj_set_style_bg_grad},
    {"lv_obj_set_style_bg_grad_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_bg_grad_color},
    {"lv_obj_set_style_bg_grad_dir", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_grad_dir},
    {"lv_obj_set_style_bg_grad_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_grad_opa},
    {"lv_obj_set_style_bg_grad_stop", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_grad_stop},
    {"lv_obj_set_style_bg_image_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_image_opa},
    {"lv_obj_set_style_bg_image_recolor", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_bg_image_recolor},
    {"lv_obj_set_style_bg_image_recolor_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_image_recolor_opa},
    {"lv_obj_set_style_bg_image_src", &invoke_void_lv_obj_t_p_POINTER_INT, (void*)&lv_obj_set_style_bg_image_src},
    {"lv_obj_set_style_bg_image_tiled", &invoke_void_lv_obj_t_p_BOOL_INT, (void*)&lv_obj_set_style_bg_image_tiled},
    {"lv_obj_set_style_bg_main_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_main_opa},
    {"lv_obj_set_style_bg_main_stop", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_main_stop},
    {"lv_obj_set_style_bg_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_bg_opa},
    {"lv_obj_set_style_bitmap_mask_src", &invoke_void_lv_obj_t_p_POINTER_INT, (void*)&lv_obj_set_style_bitmap_mask_src},
    {"lv_obj_set_style_blend_mode", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_blend_mode},
    {"lv_obj_set_style_border_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_border_color},
    {"lv_obj_set_style_border_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_border_opa},
    {"lv_obj_set_style_border_post", &invoke_void_lv_obj_t_p_BOOL_INT, (void*)&lv_obj_set_style_border_post},
    {"lv_obj_set_style_border_side", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_border_side},
    {"lv_obj_set_style_border_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_border_width},
    {"lv_obj_set_style_clip_corner", &invoke_void_lv_obj_t_p_BOOL_INT, (void*)&lv_obj_set_style_clip_corner},
    {"lv_obj_set_style_color_filter_dsc", &invoke_void_lv_obj_t_p_lv_color_filter_dsc_t_p_INT, (void*)&lv_obj_set_style_color_filter_dsc},
    {"lv_obj_set_style_color_filter_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_color_filter_opa},
    {"lv_obj_set_style_flex_cross_place", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_flex_cross_place},
    {"lv_obj_set_style_flex_flow", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_flex_flow},
    {"lv_obj_set_style_flex_grow", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_flex_grow},
    {"lv_obj_set_style_flex_main_place", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_flex_main_place},
    {"lv_obj_set_style_flex_track_place", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_flex_track_place},
    {"lv_obj_set_style_grid_cell_column_pos", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_cell_column_pos},
    {"lv_obj_set_style_grid_cell_column_span", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_cell_column_span},
    {"lv_obj_set_style_grid_cell_row_pos", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_cell_row_pos},
    {"lv_obj_set_style_grid_cell_row_span", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_cell_row_span},
    {"lv_obj_set_style_grid_cell_x_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_cell_x_align},
    {"lv_obj_set_style_grid_cell_y_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_cell_y_align},
    {"lv_obj_set_style_grid_column_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_column_align},
    {"lv_obj_set_style_grid_column_dsc_array", &invoke_void_lv_obj_t_p_POINTER_INT, (void*)&lv_obj_set_style_grid_column_dsc_array},
    {"lv_obj_set_style_grid_row_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_grid_row_align},
    {"lv_obj_set_style_grid_row_dsc_array", &invoke_void_lv_obj_t_p_POINTER_INT, (void*)&lv_obj_set_style_grid_row_dsc_array},
    {"lv_obj_set_style_height", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_height},
    {"lv_obj_set_style_image_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_image_opa},
    {"lv_obj_set_style_image_recolor", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_image_recolor},
    {"lv_obj_set_style_image_recolor_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_image_recolor_opa},
    {"lv_obj_set_style_layout", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_layout},
    {"lv_obj_set_style_length", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_length},
    {"lv_obj_set_style_line_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_line_color},
    {"lv_obj_set_style_line_dash_gap", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_line_dash_gap},
    {"lv_obj_set_style_line_dash_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_line_dash_width},
    {"lv_obj_set_style_line_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_line_opa},
    {"lv_obj_set_style_line_rounded", &invoke_void_lv_obj_t_p_BOOL_INT, (void*)&lv_obj_set_style_line_rounded},
    {"lv_obj_set_style_line_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_line_width},
    {"lv_obj_set_style_margin_all", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_all},
    {"lv_obj_set_style_margin_bottom", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_bottom},
    {"lv_obj_set_style_margin_hor", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_hor},
    {"lv_obj_set_style_margin_left", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_left},
    {"lv_obj_set_style_margin_right", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_right},
    {"lv_obj_set_style_margin_top", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_top},
    {"lv_obj_set_style_margin_ver", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_margin_ver},
    {"lv_obj_set_style_max_height", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_max_height},
    {"lv_obj_set_style_max_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_max_width},
    {"lv_obj_set_style_min_height", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_min_height},
    {"lv_obj_set_style_min_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_min_width},
    {"lv_obj_set_style_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_opa},
    {"lv_obj_set_style_opa_layered", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_opa_layered},
    {"lv_obj_set_style_outline_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_outline_color},
    {"lv_obj_set_style_outline_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_outline_opa},
    {"lv_obj_set_style_outline_pad", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_outline_pad},
    {"lv_obj_set_style_outline_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_outline_width},
    {"lv_obj_set_style_pad_all", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_all},
    {"lv_obj_set_style_pad_bottom", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_bottom},
    {"lv_obj_set_style_pad_column", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_column},
    {"lv_obj_set_style_pad_gap", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_gap},
    {"lv_obj_set_style_pad_hor", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_hor},
    {"lv_obj_set_style_pad_left", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_left},
    {"lv_obj_set_style_pad_radial", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_radial},
    {"lv_obj_set_style_pad_right", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_right},
    {"lv_obj_set_style_pad_row", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_row},
    {"lv_obj_set_style_pad_top", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_top},
    {"lv_obj_set_style_pad_ver", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_pad_ver},
    {"lv_obj_set_style_radial_offset", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_radial_offset},
    {"lv_obj_set_style_radius", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_radius},
    {"lv_obj_set_style_recolor", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_recolor},
    {"lv_obj_set_style_recolor_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_recolor_opa},
    {"lv_obj_set_style_rotary_sensitivity", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_rotary_sensitivity},
    {"lv_obj_set_style_shadow_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_shadow_color},
    {"lv_obj_set_style_shadow_offset_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_shadow_offset_x},
    {"lv_obj_set_style_shadow_offset_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_shadow_offset_y},
    {"lv_obj_set_style_shadow_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_shadow_opa},
    {"lv_obj_set_style_shadow_spread", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_shadow_spread},
    {"lv_obj_set_style_shadow_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_shadow_width},
    {"lv_obj_set_style_size", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_obj_set_style_size},
    {"lv_obj_set_style_text_align", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_align},
    {"lv_obj_set_style_text_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_text_color},
    {"lv_obj_set_style_text_decor", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_decor},
    {"lv_obj_set_style_text_font", &invoke_void_lv_obj_t_p_lv_font_t_p_INT, (void*)&lv_obj_set_style_text_font},
    {"lv_obj_set_style_text_letter_space", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_letter_space},
    {"lv_obj_set_style_text_line_space", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_line_space},
    {"lv_obj_set_style_text_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_opa},
    {"lv_obj_set_style_text_outline_stroke_color", &invoke_void_lv_obj_t_p_lv_color_t_INT, (void*)&lv_obj_set_style_text_outline_stroke_color},
    {"lv_obj_set_style_text_outline_stroke_opa", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_outline_stroke_opa},
    {"lv_obj_set_style_text_outline_stroke_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_text_outline_stroke_width},
    {"lv_obj_set_style_transform_height", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_height},
    {"lv_obj_set_style_transform_pivot_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_pivot_x},
    {"lv_obj_set_style_transform_pivot_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_pivot_y},
    {"lv_obj_set_style_transform_rotation", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_rotation},
    {"lv_obj_set_style_transform_scale", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_scale},
    {"lv_obj_set_style_transform_scale_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_scale_x},
    {"lv_obj_set_style_transform_scale_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_scale_y},
    {"lv_obj_set_style_transform_skew_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_skew_x},
    {"lv_obj_set_style_transform_skew_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_skew_y},
    {"lv_obj_set_style_transform_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_transform_width},
    {"lv_obj_set_style_transition", &invoke_void_lv_obj_t_p_lv_style_transition_dsc_t_p_INT, (void*)&lv_obj_set_style_transition},
    {"lv_obj_set_style_translate_radial", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_translate_radial},
    {"lv_obj_set_style_translate_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_translate_x},
    {"lv_obj_set_style_translate_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_translate_y},
    {"lv_obj_set_style_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_width},
    {"lv_obj_set_style_x", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_x},
    {"lv_obj_set_style_y", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_obj_set_style_y},
    {"lv_obj_set_transform", &invoke_void_lv_obj_t_p_lv_matrix_t_p, (void*)&lv_obj_set_transform},
    {"lv_obj_set_user_data", &invoke_void_lv_obj_t_p_POINTER, (void*)&lv_obj_set_user_data},
    {"lv_obj_set_width", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_width},
    {"lv_obj_set_x", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_x},
    {"lv_obj_set_y", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_set_y},
    {"lv_obj_stop_scroll_anim", &invoke_void_lv_obj_t_p, (void*)&lv_obj_stop_scroll_anim},
    {"lv_obj_style_apply_color_filter", &invoke_INT_lv_obj_t_p_INT_INT, (void*)&lv_obj_style_apply_color_filter},
    {"lv_obj_style_apply_recolor", &invoke_INT_lv_obj_t_p_INT_INT, (void*)&lv_obj_style_apply_recolor},
    {"lv_obj_style_get_selector_part", &invoke_INT_INT, (void*)&lv_obj_style_get_selector_part},
    {"lv_obj_style_get_selector_state", &invoke_INT_INT, (void*)&lv_obj_style_get_selector_state},
    {"lv_obj_swap", &invoke_void_lv_obj_t_p_lv_obj_t_p, (void*)&lv_obj_swap},
    {"lv_obj_transform_point", &invoke_void_lv_obj_t_p_lv_point_t_p_INT, (void*)&lv_obj_transform_point},
    {"lv_obj_transform_point_array", &invoke_void_lv_obj_t_p_lv_point_t_p_INT_INT, (void*)&lv_obj_transform_point_array},
    {"lv_obj_tree_walk", &invoke_void_lv_obj_t_p_INT_POINTER, (void*)&lv_obj_tree_walk},
    {"lv_obj_update_layout", &invoke_void_lv_obj_t_p, (void*)&lv_obj_update_layout},
    {"lv_obj_update_snap", &invoke_void_lv_obj_t_p_INT, (void*)&lv_obj_update_snap},
    {"lv_observer_get_target", &invoke_POINTER_lv_observer_t_p, (void*)&lv_observer_get_target},
    {"lv_observer_get_target_obj", &invoke_lv_obj_t_p_lv_observer_t_p, (void*)&lv_observer_get_target_obj},
    {"lv_observer_get_user_data", &invoke_POINTER_lv_observer_t_p, (void*)&lv_observer_get_user_data},
    {"lv_observer_remove", &invoke_void_lv_observer_t_p, (void*)&lv_observer_remove},
    {"lv_palette_darken", &invoke_lv_color_t_INT_INT, (void*)&lv_palette_darken},
    {"lv_palette_lighten", &invoke_lv_color_t_INT_INT, (void*)&lv_palette_lighten},
    {"lv_palette_main", &invoke_lv_color_t_INT, (void*)&lv_palette_main},
    {"lv_pct", &invoke_INT_INT, (void*)&lv_pct},
    {"lv_pct_to_px", &invoke_INT_INT_INT, (void*)&lv_pct_to_px},
    {"lv_point_array_transform", &invoke_void_lv_point_t_p_INT_INT_INT_INT_lv_point_t_p_BOOL, (void*)&lv_point_array_transform},
    {"lv_point_from_precise", &invoke_INT_lv_point_precise_t_p, (void*)&lv_point_from_precise},
    {"lv_point_precise_set", &invoke_void_lv_point_precise_t_p_INT_INT, (void*)&lv_point_precise_set},
    {"lv_point_precise_swap", &invoke_void_lv_point_precise_t_p_lv_point_precise_t_p, (void*)&lv_point_precise_swap},
    {"lv_point_set", &invoke_void_lv_point_t_p_INT_INT, (void*)&lv_point_set},
    {"lv_point_swap", &invoke_void_lv_point_t_p_lv_point_t_p, (void*)&lv_point_swap},
    {"lv_point_to_precise", &invoke_INT_lv_point_t_p, (void*)&lv_point_to_precise},
    {"lv_point_transform", &invoke_void_lv_point_t_p_INT_INT_INT_lv_point_t_p_BOOL, (void*)&lv_point_transform},
    {"lv_pow", &invoke_INT_INT_INT, (void*)&lv_pow},
    {"lv_rand", &invoke_INT_INT_INT, (void*)&lv_rand},
    {"lv_rand_set_seed", &invoke_void_INT, (void*)&lv_rand_set_seed},
    {"lv_rb_destroy", &invoke_void_lv_rb_t_p, (void*)&lv_rb_destroy},
    {"lv_rb_drop", &invoke_BOOL_lv_rb_t_p_POINTER, (void*)&lv_rb_drop},
    {"lv_rb_drop_node", &invoke_BOOL_lv_rb_t_p_lv_rb_node_t_p, (void*)&lv_rb_drop_node},
    {"lv_rb_find", &invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER, (void*)&lv_rb_find},
    {"lv_rb_init", &invoke_BOOL_lv_rb_t_p_INT_INT, (void*)&lv_rb_init},
    {"lv_rb_insert", &invoke_lv_rb_node_t_p_lv_rb_t_p_POINTER, (void*)&lv_rb_insert},
    {"lv_rb_maximum", &invoke_lv_rb_node_t_p_lv_rb_t_p, (void*)&lv_rb_maximum},
    {"lv_rb_maximum_from", &invoke_lv_rb_node_t_p_lv_rb_node_t_p, (void*)&lv_rb_maximum_from},
    {"lv_rb_minimum", &invoke_lv_rb_node_t_p_lv_rb_t_p, (void*)&lv_rb_minimum},
    {"lv_rb_minimum_from", &invoke_lv_rb_node_t_p_lv_rb_node_t_p, (void*)&lv_rb_minimum_from},
    {"lv_rb_remove", &invoke_POINTER_lv_rb_t_p_POINTER, (void*)&lv_rb_remove},
    {"lv_rb_remove_node", &invoke_POINTER_lv_rb_t_p_lv_rb_node_t_p, (void*)&lv_rb_remove_node},
    {"lv_realloc", &invoke_POINTER_POINTER_INT, (void*)&lv_realloc},
    {"lv_realloc_core", &invoke_POINTER_POINTER_INT, (void*)&lv_realloc_core},
    {"lv_reallocf", &invoke_POINTER_POINTER_INT, (void*)&lv_reallocf},
    {"lv_refr_now", &invoke_void_lv_display_t_p, (void*)&lv_refr_now},
    {"lv_roller_bind_value", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p, (void*)&lv_roller_bind_value},
    {"lv_roller_create", &invoke_widget_create, (void*)&lv_roller_create},
    {"lv_roller_get_option_count", &invoke_INT_lv_obj_t_p, (void*)&lv_roller_get_option_count},
    {"lv_roller_get_options", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_roller_get_options},
    {"lv_roller_get_selected", &invoke_INT_lv_obj_t_p, (void*)&lv_roller_get_selected},
    {"lv_roller_get_selected_str", &invoke_void_lv_obj_t_p_const_char_p_INT, (void*)&lv_roller_get_selected_str},
    {"lv_roller_set_options", &invoke_void_lv_obj_t_p_const_char_p_INT, (void*)&lv_roller_set_options},
    {"lv_roller_set_selected", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_roller_set_selected},
    {"lv_roller_set_selected_str", &invoke_BOOL_lv_obj_t_p_const_char_p_INT, (void*)&lv_roller_set_selected_str},
    {"lv_roller_set_visible_row_count", &invoke_void_lv_obj_t_p_INT, (void*)&lv_roller_set_visible_row_count},
    {"lv_scale_add_section", &invoke_lv_scale_section_t_p_lv_obj_t_p, (void*)&lv_scale_add_section},
    {"lv_scale_create", &invoke_widget_create, (void*)&lv_scale_create},
    {"lv_scale_get_angle_range", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_angle_range},
    {"lv_scale_get_label_show", &invoke_BOOL_lv_obj_t_p, (void*)&lv_scale_get_label_show},
    {"lv_scale_get_major_tick_every", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_major_tick_every},
    {"lv_scale_get_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_mode},
    {"lv_scale_get_range_max_value", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_range_max_value},
    {"lv_scale_get_range_min_value", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_range_min_value},
    {"lv_scale_get_rotation", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_rotation},
    {"lv_scale_get_total_tick_count", &invoke_INT_lv_obj_t_p, (void*)&lv_scale_get_total_tick_count},
    {"lv_scale_section_set_range", &invoke_void_lv_scale_section_t_p_INT_INT, (void*)&lv_scale_section_set_range},
    {"lv_scale_section_set_style", &invoke_void_lv_scale_section_t_p_INT_lv_style_t_p, (void*)&lv_scale_section_set_style},
    {"lv_scale_set_angle_range", &invoke_void_lv_obj_t_p_INT, (void*)&lv_scale_set_angle_range},
    {"lv_scale_set_draw_ticks_on_top", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_scale_set_draw_ticks_on_top},
    {"lv_scale_set_image_needle_value", &invoke_void_lv_obj_t_p_lv_obj_t_p_INT, (void*)&lv_scale_set_image_needle_value},
    {"lv_scale_set_label_show", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_scale_set_label_show},
    {"lv_scale_set_line_needle_value", &invoke_void_lv_obj_t_p_lv_obj_t_p_INT_INT, (void*)&lv_scale_set_line_needle_value},
    {"lv_scale_set_major_tick_every", &invoke_void_lv_obj_t_p_INT, (void*)&lv_scale_set_major_tick_every},
    {"lv_scale_set_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_scale_set_mode},
    {"lv_scale_set_post_draw", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_scale_set_post_draw},
    {"lv_scale_set_range", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_scale_set_range},
    {"lv_scale_set_rotation", &invoke_void_lv_obj_t_p_INT, (void*)&lv_scale_set_rotation},
    {"lv_scale_set_section_range", &invoke_void_lv_obj_t_p_lv_scale_section_t_p_INT_INT, (void*)&lv_scale_set_section_range},
    {"lv_scale_set_section_style_indicator", &invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p, (void*)&lv_scale_set_section_style_indicator},
    {"lv_scale_set_section_style_items", &invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p, (void*)&lv_scale_set_section_style_items},
    {"lv_scale_set_section_style_main", &invoke_void_lv_obj_t_p_lv_scale_section_t_p_lv_style_t_p, (void*)&lv_scale_set_section_style_main},
    {"lv_scale_set_total_tick_count", &invoke_void_lv_obj_t_p_INT, (void*)&lv_scale_set_total_tick_count},
    {"lv_screen_active", &invoke_lv_obj_t_p, (void*)&lv_screen_active},
    {"lv_screen_load", &invoke_void_lv_obj_t_p, (void*)&lv_screen_load},
    {"lv_screen_load_anim", &invoke_void_lv_obj_t_p_INT_INT_INT_BOOL, (void*)&lv_screen_load_anim},
    {"lv_slider_bind_value", &invoke_lv_observer_t_p_lv_obj_t_p_lv_subject_t_p, (void*)&lv_slider_bind_value},
    {"lv_slider_create", &invoke_widget_create, (void*)&lv_slider_create},
    {"lv_slider_get_left_value", &invoke_INT_lv_obj_t_p, (void*)&lv_slider_get_left_value},
    {"lv_slider_get_max_value", &invoke_INT_lv_obj_t_p, (void*)&lv_slider_get_max_value},
    {"lv_slider_get_min_value", &invoke_INT_lv_obj_t_p, (void*)&lv_slider_get_min_value},
    {"lv_slider_get_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_slider_get_mode},
    {"lv_slider_get_orientation", &invoke_INT_lv_obj_t_p, (void*)&lv_slider_get_orientation},
    {"lv_slider_get_value", &invoke_INT_lv_obj_t_p, (void*)&lv_slider_get_value},
    {"lv_slider_is_dragged", &invoke_BOOL_lv_obj_t_p, (void*)&lv_slider_is_dragged},
    {"lv_slider_is_symmetrical", &invoke_BOOL_lv_obj_t_p, (void*)&lv_slider_is_symmetrical},
    {"lv_slider_set_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_slider_set_mode},
    {"lv_slider_set_orientation", &invoke_void_lv_obj_t_p_INT, (void*)&lv_slider_set_orientation},
    {"lv_slider_set_range", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_slider_set_range},
    {"lv_slider_set_start_value", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_slider_set_start_value},
    {"lv_slider_set_value", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_slider_set_value},
    {"lv_span_get_style", &invoke_lv_style_t_p_lv_span_t_p, (void*)&lv_span_get_style},
    {"lv_span_get_text", &invoke_const_char_p_lv_span_t_p, (void*)&lv_span_get_text},
    {"lv_span_set_text", &invoke_void_lv_span_t_p_const_char_p, (void*)&lv_span_set_text},
    {"lv_span_set_text_static", &invoke_void_lv_span_t_p_const_char_p, (void*)&lv_span_set_text_static},
    {"lv_span_set_text_static", &invoke_void_lv_span_t_p_const_char_p, (void*)&lv_span_set_text_static},
    {"lv_span_stack_deinit", &invoke_void, (void*)&lv_span_stack_deinit},
    {"lv_span_stack_init", &invoke_void, (void*)&lv_span_stack_init},
    {"lv_spangroup_add_span", &invoke_lv_span_t_p_lv_obj_t_p, (void*)&lv_spangroup_add_span},
    {"lv_spangroup_create", &invoke_widget_create, (void*)&lv_spangroup_create},
    {"lv_spangroup_delete_span", &invoke_void_lv_obj_t_p_lv_span_t_p, (void*)&lv_spangroup_delete_span},
    {"lv_spangroup_get_align", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_align},
    {"lv_spangroup_get_child", &invoke_lv_span_t_p_lv_obj_t_p_INT, (void*)&lv_spangroup_get_child},
    {"lv_spangroup_get_expand_height", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_spangroup_get_expand_height},
    {"lv_spangroup_get_expand_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_spangroup_get_expand_width},
    {"lv_spangroup_get_indent", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_indent},
    {"lv_spangroup_get_max_line_height", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_max_line_height},
    {"lv_spangroup_get_max_lines", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_max_lines},
    {"lv_spangroup_get_mode", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_mode},
    {"lv_spangroup_get_overflow", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_overflow},
    {"lv_spangroup_get_span_by_point", &invoke_lv_span_t_p_lv_obj_t_p_lv_point_t_p, (void*)&lv_spangroup_get_span_by_point},
    {"lv_spangroup_get_span_coords", &invoke_INT_lv_obj_t_p_lv_span_t_p, (void*)&lv_spangroup_get_span_coords},
    {"lv_spangroup_get_span_count", &invoke_INT_lv_obj_t_p, (void*)&lv_spangroup_get_span_count},
    {"lv_spangroup_refresh", &invoke_void_lv_obj_t_p, (void*)&lv_spangroup_refresh},
    {"lv_spangroup_set_align", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spangroup_set_align},
    {"lv_spangroup_set_indent", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spangroup_set_indent},
    {"lv_spangroup_set_max_lines", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spangroup_set_max_lines},
    {"lv_spangroup_set_mode", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spangroup_set_mode},
    {"lv_spangroup_set_overflow", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spangroup_set_overflow},
    {"lv_spangroup_set_span_style", &invoke_void_lv_obj_t_p_lv_span_t_p_lv_style_t_p, (void*)&lv_spangroup_set_span_style},
    {"lv_spangroup_set_span_text", &invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p, (void*)&lv_spangroup_set_span_text},
    {"lv_spangroup_set_span_text_static", &invoke_void_lv_obj_t_p_lv_span_t_p_const_char_p, (void*)&lv_spangroup_set_span_text_static},
    {"lv_spinbox_create", &invoke_widget_create, (void*)&lv_spinbox_create},
    {"lv_spinbox_decrement", &invoke_void_lv_obj_t_p, (void*)&lv_spinbox_decrement},
    {"lv_spinbox_get_rollover", &invoke_BOOL_lv_obj_t_p, (void*)&lv_spinbox_get_rollover},
    {"lv_spinbox_get_step", &invoke_INT_lv_obj_t_p, (void*)&lv_spinbox_get_step},
    {"lv_spinbox_get_value", &invoke_INT_lv_obj_t_p, (void*)&lv_spinbox_get_value},
    {"lv_spinbox_increment", &invoke_void_lv_obj_t_p, (void*)&lv_spinbox_increment},
    {"lv_spinbox_set_cursor_pos", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spinbox_set_cursor_pos},
    {"lv_spinbox_set_digit_format", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_spinbox_set_digit_format},
    {"lv_spinbox_set_digit_step_direction", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spinbox_set_digit_step_direction},
    {"lv_spinbox_set_range", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_spinbox_set_range},
    {"lv_spinbox_set_rollover", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_spinbox_set_rollover},
    {"lv_spinbox_set_step", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spinbox_set_step},
    {"lv_spinbox_set_value", &invoke_void_lv_obj_t_p_INT, (void*)&lv_spinbox_set_value},
    {"lv_spinbox_step_next", &invoke_void_lv_obj_t_p, (void*)&lv_spinbox_step_next},
    {"lv_spinbox_step_prev", &invoke_void_lv_obj_t_p, (void*)&lv_spinbox_step_prev},
    {"lv_spinner_create", &invoke_widget_create, (void*)&lv_spinner_create},
    {"lv_spinner_set_anim_params", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_spinner_set_anim_params},
    {"lv_sqr", &invoke_INT_INT, (void*)&lv_sqr},
    {"lv_sqrt", &invoke_void_INT_lv_sqrt_res_t_p_INT, (void*)&lv_sqrt},
    {"lv_sqrt32", &invoke_INT_INT, (void*)&lv_sqrt32},
    {"lv_strcat", &invoke_const_char_p_const_char_p_const_char_p, (void*)&lv_strcat},
    {"lv_strchr", &invoke_const_char_p_const_char_p_INT, (void*)&lv_strchr},
    {"lv_strcmp", &invoke_INT_const_char_p_const_char_p, (void*)&lv_strcmp},
    {"lv_strcpy", &invoke_const_char_p_const_char_p_const_char_p, (void*)&lv_strcpy},
    {"lv_strdup", &invoke_const_char_p_const_char_p, (void*)&lv_strdup},
    {"lv_streq", &invoke_BOOL_const_char_p_const_char_p, (void*)&lv_streq},
    {"lv_strlcpy", &invoke_INT_const_char_p_const_char_p_INT, (void*)&lv_strlcpy},
    {"lv_strlen", &invoke_INT_const_char_p, (void*)&lv_strlen},
    {"lv_strncat", &invoke_const_char_p_const_char_p_const_char_p_INT, (void*)&lv_strncat},
    {"lv_strncmp", &invoke_INT_const_char_p_const_char_p_INT, (void*)&lv_strncmp},
    {"lv_strncpy", &invoke_const_char_p_const_char_p_const_char_p_INT, (void*)&lv_strncpy},
    {"lv_strndup", &invoke_const_char_p_const_char_p_INT, (void*)&lv_strndup},
    {"lv_strnlen", &invoke_INT_const_char_p_INT, (void*)&lv_strnlen},
    {"lv_style_copy", &invoke_void_lv_style_t_p_lv_style_t_p, (void*)&lv_style_copy},
    {"lv_style_create_managed", &invoke_lv_style_t_p_const_char_p, (void*)&lv_style_create_managed},
    {"lv_style_get_num_custom_props", &invoke_INT, (void*)&lv_style_get_num_custom_props},
    {"lv_style_get_prop", &invoke_INT_lv_style_t_p_INT_lv_style_value_t_p, (void*)&lv_style_get_prop},
    {"lv_style_get_prop_group", &invoke_INT_INT, (void*)&lv_style_get_prop_group},
    {"lv_style_get_prop_inlined", &invoke_INT_lv_style_t_p_INT_lv_style_value_t_p, (void*)&lv_style_get_prop_inlined},
    {"lv_style_init", &invoke_void_lv_style_t_p, (void*)&lv_style_init},
    {"lv_style_is_const", &invoke_BOOL_lv_style_t_p, (void*)&lv_style_is_const},
    {"lv_style_is_empty", &invoke_BOOL_lv_style_t_p, (void*)&lv_style_is_empty},
    {"lv_style_prop_get_default", &invoke_INT_INT, (void*)&lv_style_prop_get_default},
    {"lv_style_prop_has_flag", &invoke_BOOL_INT_INT, (void*)&lv_style_prop_has_flag},
    {"lv_style_prop_lookup_flags", &invoke_INT_INT, (void*)&lv_style_prop_lookup_flags},
    {"lv_style_register_prop", &invoke_INT_INT, (void*)&lv_style_register_prop},
    {"lv_style_remove_prop", &invoke_BOOL_lv_style_t_p_INT, (void*)&lv_style_remove_prop},
    {"lv_style_reset", &invoke_void_lv_style_t_p, (void*)&lv_style_reset},
    {"lv_style_set_align", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_align},
    {"lv_style_set_anim", &invoke_void_lv_style_t_p_lv_anim_t_p, (void*)&lv_style_set_anim},
    {"lv_style_set_anim_duration", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_anim_duration},
    {"lv_style_set_arc_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_arc_color},
    {"lv_style_set_arc_image_src", &invoke_void_lv_style_t_p_POINTER, (void*)&lv_style_set_arc_image_src},
    {"lv_style_set_arc_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_arc_opa},
    {"lv_style_set_arc_rounded", &invoke_void_lv_style_t_p_BOOL, (void*)&lv_style_set_arc_rounded},
    {"lv_style_set_arc_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_arc_width},
    {"lv_style_set_base_dir", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_base_dir},
    {"lv_style_set_bg_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_bg_color},
    {"lv_style_set_bg_grad", &invoke_void_lv_style_t_p_lv_grad_dsc_t_p, (void*)&lv_style_set_bg_grad},
    {"lv_style_set_bg_grad_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_bg_grad_color},
    {"lv_style_set_bg_grad_dir", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_grad_dir},
    {"lv_style_set_bg_grad_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_grad_opa},
    {"lv_style_set_bg_grad_stop", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_grad_stop},
    {"lv_style_set_bg_image_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_image_opa},
    {"lv_style_set_bg_image_recolor", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_bg_image_recolor},
    {"lv_style_set_bg_image_recolor_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_image_recolor_opa},
    {"lv_style_set_bg_image_src", &invoke_void_lv_style_t_p_POINTER, (void*)&lv_style_set_bg_image_src},
    {"lv_style_set_bg_image_tiled", &invoke_void_lv_style_t_p_BOOL, (void*)&lv_style_set_bg_image_tiled},
    {"lv_style_set_bg_main_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_main_opa},
    {"lv_style_set_bg_main_stop", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_main_stop},
    {"lv_style_set_bg_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_bg_opa},
    {"lv_style_set_bitmap_mask_src", &invoke_void_lv_style_t_p_POINTER, (void*)&lv_style_set_bitmap_mask_src},
    {"lv_style_set_blend_mode", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_blend_mode},
    {"lv_style_set_border_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_border_color},
    {"lv_style_set_border_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_border_opa},
    {"lv_style_set_border_post", &invoke_void_lv_style_t_p_BOOL, (void*)&lv_style_set_border_post},
    {"lv_style_set_border_side", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_border_side},
    {"lv_style_set_border_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_border_width},
    {"lv_style_set_clip_corner", &invoke_void_lv_style_t_p_BOOL, (void*)&lv_style_set_clip_corner},
    {"lv_style_set_color_filter_dsc", &invoke_void_lv_style_t_p_lv_color_filter_dsc_t_p, (void*)&lv_style_set_color_filter_dsc},
    {"lv_style_set_color_filter_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_color_filter_opa},
    {"lv_style_set_flex_cross_place", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_flex_cross_place},
    {"lv_style_set_flex_flow", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_flex_flow},
    {"lv_style_set_flex_grow", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_flex_grow},
    {"lv_style_set_flex_main_place", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_flex_main_place},
    {"lv_style_set_flex_track_place", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_flex_track_place},
    {"lv_style_set_grid_cell_column_pos", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_cell_column_pos},
    {"lv_style_set_grid_cell_column_span", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_cell_column_span},
    {"lv_style_set_grid_cell_row_pos", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_cell_row_pos},
    {"lv_style_set_grid_cell_row_span", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_cell_row_span},
    {"lv_style_set_grid_cell_x_align", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_cell_x_align},
    {"lv_style_set_grid_cell_y_align", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_cell_y_align},
    {"lv_style_set_grid_column_align", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_column_align},
    {"lv_style_set_grid_column_dsc_array", &invoke_void_lv_style_t_p_POINTER, (void*)&lv_style_set_grid_column_dsc_array},
    {"lv_style_set_grid_row_align", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_grid_row_align},
    {"lv_style_set_grid_row_dsc_array", &invoke_void_lv_style_t_p_POINTER, (void*)&lv_style_set_grid_row_dsc_array},
    {"lv_style_set_height", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_height},
    {"lv_style_set_image_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_image_opa},
    {"lv_style_set_image_recolor", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_image_recolor},
    {"lv_style_set_image_recolor_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_image_recolor_opa},
    {"lv_style_set_layout", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_layout},
    {"lv_style_set_length", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_length},
    {"lv_style_set_line_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_line_color},
    {"lv_style_set_line_dash_gap", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_line_dash_gap},
    {"lv_style_set_line_dash_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_line_dash_width},
    {"lv_style_set_line_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_line_opa},
    {"lv_style_set_line_rounded", &invoke_void_lv_style_t_p_BOOL, (void*)&lv_style_set_line_rounded},
    {"lv_style_set_line_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_line_width},
    {"lv_style_set_margin_all", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_all},
    {"lv_style_set_margin_bottom", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_bottom},
    {"lv_style_set_margin_hor", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_hor},
    {"lv_style_set_margin_left", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_left},
    {"lv_style_set_margin_right", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_right},
    {"lv_style_set_margin_top", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_top},
    {"lv_style_set_margin_ver", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_margin_ver},
    {"lv_style_set_max_height", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_max_height},
    {"lv_style_set_max_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_max_width},
    {"lv_style_set_min_height", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_min_height},
    {"lv_style_set_min_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_min_width},
    {"lv_style_set_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_opa},
    {"lv_style_set_opa_layered", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_opa_layered},
    {"lv_style_set_outline_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_outline_color},
    {"lv_style_set_outline_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_outline_opa},
    {"lv_style_set_outline_pad", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_outline_pad},
    {"lv_style_set_outline_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_outline_width},
    {"lv_style_set_pad_all", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_all},
    {"lv_style_set_pad_bottom", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_bottom},
    {"lv_style_set_pad_column", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_column},
    {"lv_style_set_pad_gap", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_gap},
    {"lv_style_set_pad_hor", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_hor},
    {"lv_style_set_pad_left", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_left},
    {"lv_style_set_pad_radial", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_radial},
    {"lv_style_set_pad_right", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_right},
    {"lv_style_set_pad_row", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_row},
    {"lv_style_set_pad_top", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_top},
    {"lv_style_set_pad_ver", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_pad_ver},
    {"lv_style_set_prop", &invoke_void_lv_style_t_p_INT_INT, (void*)&lv_style_set_prop},
    {"lv_style_set_radial_offset", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_radial_offset},
    {"lv_style_set_radius", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_radius},
    {"lv_style_set_recolor", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_recolor},
    {"lv_style_set_recolor_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_recolor_opa},
    {"lv_style_set_rotary_sensitivity", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_rotary_sensitivity},
    {"lv_style_set_shadow_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_shadow_color},
    {"lv_style_set_shadow_offset_x", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_shadow_offset_x},
    {"lv_style_set_shadow_offset_y", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_shadow_offset_y},
    {"lv_style_set_shadow_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_shadow_opa},
    {"lv_style_set_shadow_spread", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_shadow_spread},
    {"lv_style_set_shadow_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_shadow_width},
    {"lv_style_set_size", &invoke_void_lv_style_t_p_INT_INT, (void*)&lv_style_set_size},
    {"lv_style_set_text_align", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_align},
    {"lv_style_set_text_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_text_color},
    {"lv_style_set_text_decor", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_decor},
    {"lv_style_set_text_font", &invoke_void_lv_style_t_p_lv_font_t_p, (void*)&lv_style_set_text_font},
    {"lv_style_set_text_letter_space", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_letter_space},
    {"lv_style_set_text_line_space", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_line_space},
    {"lv_style_set_text_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_opa},
    {"lv_style_set_text_outline_stroke_color", &invoke_void_lv_style_t_p_lv_color_t, (void*)&lv_style_set_text_outline_stroke_color},
    {"lv_style_set_text_outline_stroke_opa", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_outline_stroke_opa},
    {"lv_style_set_text_outline_stroke_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_text_outline_stroke_width},
    {"lv_style_set_transform_height", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_height},
    {"lv_style_set_transform_pivot_x", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_pivot_x},
    {"lv_style_set_transform_pivot_y", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_pivot_y},
    {"lv_style_set_transform_rotation", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_rotation},
    {"lv_style_set_transform_scale", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_scale},
    {"lv_style_set_transform_scale_x", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_scale_x},
    {"lv_style_set_transform_scale_y", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_scale_y},
    {"lv_style_set_transform_skew_x", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_skew_x},
    {"lv_style_set_transform_skew_y", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_skew_y},
    {"lv_style_set_transform_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_transform_width},
    {"lv_style_set_transition", &invoke_void_lv_style_t_p_lv_style_transition_dsc_t_p, (void*)&lv_style_set_transition},
    {"lv_style_set_translate_radial", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_translate_radial},
    {"lv_style_set_translate_x", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_translate_x},
    {"lv_style_set_translate_y", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_translate_y},
    {"lv_style_set_width", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_width},
    {"lv_style_set_x", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_x},
    {"lv_style_set_y", &invoke_void_lv_style_t_p_INT, (void*)&lv_style_set_y},
    {"lv_style_transition_dsc_init", &invoke_void_lv_style_transition_dsc_t_p_lv_style_prop_t_p_INT_INT_INT_POINTER, (void*)&lv_style_transition_dsc_init},
    {"lv_subject_add_observer", &invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER, (void*)&lv_subject_add_observer},
    {"lv_subject_add_observer_obj", &invoke_lv_observer_t_p_lv_subject_t_p_INT_lv_obj_t_p_POINTER, (void*)&lv_subject_add_observer_obj},
    {"lv_subject_add_observer_with_target", &invoke_lv_observer_t_p_lv_subject_t_p_INT_POINTER_POINTER, (void*)&lv_subject_add_observer_with_target},
    {"lv_subject_copy_string", &invoke_void_lv_subject_t_p_const_char_p, (void*)&lv_subject_copy_string},
    {"lv_subject_deinit", &invoke_void_lv_subject_t_p, (void*)&lv_subject_deinit},
    {"lv_subject_get_color", &invoke_lv_color_t_lv_subject_t_p, (void*)&lv_subject_get_color},
    {"lv_subject_get_group_element", &invoke_lv_subject_t_p_lv_subject_t_p_INT, (void*)&lv_subject_get_group_element},
    {"lv_subject_get_int", &invoke_INT_lv_subject_t_p, (void*)&lv_subject_get_int},
    {"lv_subject_get_pointer", &invoke_POINTER_lv_subject_t_p, (void*)&lv_subject_get_pointer},
    {"lv_subject_get_previous_color", &invoke_lv_color_t_lv_subject_t_p, (void*)&lv_subject_get_previous_color},
    {"lv_subject_get_previous_int", &invoke_INT_lv_subject_t_p, (void*)&lv_subject_get_previous_int},
    {"lv_subject_get_previous_pointer", &invoke_POINTER_lv_subject_t_p, (void*)&lv_subject_get_previous_pointer},
    {"lv_subject_get_previous_string", &invoke_const_char_p_lv_subject_t_p, (void*)&lv_subject_get_previous_string},
    {"lv_subject_get_string", &invoke_const_char_p_lv_subject_t_p, (void*)&lv_subject_get_string},
    {"lv_subject_init_color", &invoke_void_lv_subject_t_p_lv_color_t, (void*)&lv_subject_init_color},
    {"lv_subject_init_int", &invoke_void_lv_subject_t_p_INT, (void*)&lv_subject_init_int},
    {"lv_subject_init_pointer", &invoke_void_lv_subject_t_p_POINTER, (void*)&lv_subject_init_pointer},
    {"lv_subject_init_string", &invoke_void_lv_subject_t_p_const_char_p_const_char_p_INT_const_char_p, (void*)&lv_subject_init_string},
    {"lv_subject_notify", &invoke_void_lv_subject_t_p, (void*)&lv_subject_notify},
    {"lv_subject_set_color", &invoke_void_lv_subject_t_p_lv_color_t, (void*)&lv_subject_set_color},
    {"lv_subject_set_int", &invoke_void_lv_subject_t_p_INT, (void*)&lv_subject_set_int},
    {"lv_subject_set_pointer", &invoke_void_lv_subject_t_p_POINTER, (void*)&lv_subject_set_pointer},
    {"lv_switch_create", &invoke_widget_create, (void*)&lv_switch_create},
    {"lv_switch_get_orientation", &invoke_INT_lv_obj_t_p, (void*)&lv_switch_get_orientation},
    {"lv_switch_set_orientation", &invoke_void_lv_obj_t_p_INT, (void*)&lv_switch_set_orientation},
    {"lv_table_clear_cell_ctrl", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_table_clear_cell_ctrl},
    {"lv_table_create", &invoke_widget_create, (void*)&lv_table_create},
    {"lv_table_get_cell_user_data", &invoke_POINTER_lv_obj_t_p_INT_INT, (void*)&lv_table_get_cell_user_data},
    {"lv_table_get_cell_value", &invoke_const_char_p_lv_obj_t_p_INT_INT, (void*)&lv_table_get_cell_value},
    {"lv_table_get_column_count", &invoke_INT_lv_obj_t_p, (void*)&lv_table_get_column_count},
    {"lv_table_get_column_width", &invoke_INT_lv_obj_t_p_INT, (void*)&lv_table_get_column_width},
    {"lv_table_get_row_count", &invoke_INT_lv_obj_t_p, (void*)&lv_table_get_row_count},
    {"lv_table_get_selected_cell", &invoke_void_lv_obj_t_p_POINTER_POINTER, (void*)&lv_table_get_selected_cell},
    {"lv_table_has_cell_ctrl", &invoke_BOOL_lv_obj_t_p_INT_INT_INT, (void*)&lv_table_has_cell_ctrl},
    {"lv_table_set_cell_ctrl", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_table_set_cell_ctrl},
    {"lv_table_set_cell_user_data", &invoke_void_lv_obj_t_p_INT_INT_POINTER, (void*)&lv_table_set_cell_user_data},
    {"lv_table_set_cell_value", &invoke_void_lv_obj_t_p_INT_INT_const_char_p, (void*)&lv_table_set_cell_value},
    {"lv_table_set_column_count", &invoke_void_lv_obj_t_p_INT, (void*)&lv_table_set_column_count},
    {"lv_table_set_column_width", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_table_set_column_width},
    {"lv_table_set_row_count", &invoke_void_lv_obj_t_p_INT, (void*)&lv_table_set_row_count},
    {"lv_table_set_selected_cell", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_table_set_selected_cell},
    {"lv_tabview_add_tab", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_tabview_add_tab},
    {"lv_tabview_create", &invoke_widget_create, (void*)&lv_tabview_create},
    {"lv_tabview_get_content", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_tabview_get_content},
    {"lv_tabview_get_tab_active", &invoke_INT_lv_obj_t_p, (void*)&lv_tabview_get_tab_active},
    {"lv_tabview_get_tab_bar", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_tabview_get_tab_bar},
    {"lv_tabview_get_tab_count", &invoke_INT_lv_obj_t_p, (void*)&lv_tabview_get_tab_count},
    {"lv_tabview_rename_tab", &invoke_void_lv_obj_t_p_INT_const_char_p, (void*)&lv_tabview_rename_tab},
    {"lv_tabview_set_active", &invoke_void_lv_obj_t_p_INT_INT, (void*)&lv_tabview_set_active},
    {"lv_tabview_set_tab_bar_position", &invoke_void_lv_obj_t_p_INT, (void*)&lv_tabview_set_tab_bar_position},
    {"lv_tabview_set_tab_bar_size", &invoke_void_lv_obj_t_p_INT, (void*)&lv_tabview_set_tab_bar_size},
    {"lv_task_handler", &invoke_INT, (void*)&lv_task_handler},
    {"lv_text_get_size", &invoke_void_lv_point_t_p_const_char_p_lv_font_t_p_INT_INT_INT_INT, (void*)&lv_text_get_size},
    {"lv_text_get_width", &invoke_INT_const_char_p_INT_lv_font_t_p_INT, (void*)&lv_text_get_width},
    {"lv_text_get_width_with_flags", &invoke_INT_const_char_p_INT_lv_font_t_p_INT_INT, (void*)&lv_text_get_width_with_flags},
    {"lv_text_is_cmd", &invoke_BOOL_lv_text_cmd_state_t_p_INT, (void*)&lv_text_is_cmd},
    {"lv_textarea_add_char", &invoke_void_lv_obj_t_p_INT, (void*)&lv_textarea_add_char},
    {"lv_textarea_add_text", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_textarea_add_text},
    {"lv_textarea_clear_selection", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_clear_selection},
    {"lv_textarea_create", &invoke_widget_create, (void*)&lv_textarea_create},
    {"lv_textarea_cursor_down", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_cursor_down},
    {"lv_textarea_cursor_left", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_cursor_left},
    {"lv_textarea_cursor_right", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_cursor_right},
    {"lv_textarea_cursor_up", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_cursor_up},
    {"lv_textarea_delete_char", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_delete_char},
    {"lv_textarea_delete_char_forward", &invoke_void_lv_obj_t_p, (void*)&lv_textarea_delete_char_forward},
    {"lv_textarea_get_accepted_chars", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_textarea_get_accepted_chars},
    {"lv_textarea_get_current_char", &invoke_INT_lv_obj_t_p, (void*)&lv_textarea_get_current_char},
    {"lv_textarea_get_cursor_click_pos", &invoke_BOOL_lv_obj_t_p, (void*)&lv_textarea_get_cursor_click_pos},
    {"lv_textarea_get_cursor_pos", &invoke_INT_lv_obj_t_p, (void*)&lv_textarea_get_cursor_pos},
    {"lv_textarea_get_label", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_textarea_get_label},
    {"lv_textarea_get_max_length", &invoke_INT_lv_obj_t_p, (void*)&lv_textarea_get_max_length},
    {"lv_textarea_get_one_line", &invoke_BOOL_lv_obj_t_p, (void*)&lv_textarea_get_one_line},
    {"lv_textarea_get_password_bullet", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_textarea_get_password_bullet},
    {"lv_textarea_get_password_mode", &invoke_BOOL_lv_obj_t_p, (void*)&lv_textarea_get_password_mode},
    {"lv_textarea_get_password_show_time", &invoke_INT_lv_obj_t_p, (void*)&lv_textarea_get_password_show_time},
    {"lv_textarea_get_placeholder_text", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_textarea_get_placeholder_text},
    {"lv_textarea_get_text", &invoke_const_char_p_lv_obj_t_p, (void*)&lv_textarea_get_text},
    {"lv_textarea_get_text_selection", &invoke_BOOL_lv_obj_t_p, (void*)&lv_textarea_get_text_selection},
    {"lv_textarea_set_accepted_chars", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_textarea_set_accepted_chars},
    {"lv_textarea_set_align", &invoke_void_lv_obj_t_p_INT, (void*)&lv_textarea_set_align},
    {"lv_textarea_set_cursor_click_pos", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_textarea_set_cursor_click_pos},
    {"lv_textarea_set_cursor_pos", &invoke_void_lv_obj_t_p_INT, (void*)&lv_textarea_set_cursor_pos},
    {"lv_textarea_set_insert_replace", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_textarea_set_insert_replace},
    {"lv_textarea_set_max_length", &invoke_void_lv_obj_t_p_INT, (void*)&lv_textarea_set_max_length},
    {"lv_textarea_set_one_line", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_textarea_set_one_line},
    {"lv_textarea_set_password_bullet", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_textarea_set_password_bullet},
    {"lv_textarea_set_password_mode", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_textarea_set_password_mode},
    {"lv_textarea_set_password_show_time", &invoke_void_lv_obj_t_p_INT, (void*)&lv_textarea_set_password_show_time},
    {"lv_textarea_set_placeholder_text", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_textarea_set_placeholder_text},
    {"lv_textarea_set_text", &invoke_void_lv_obj_t_p_const_char_p, (void*)&lv_textarea_set_text},
    {"lv_textarea_set_text_selection", &invoke_void_lv_obj_t_p_BOOL, (void*)&lv_textarea_set_text_selection},
    {"lv_textarea_text_is_selected", &invoke_BOOL_lv_obj_t_p, (void*)&lv_textarea_text_is_selected},
    {"lv_tick_elaps", &invoke_INT_INT, (void*)&lv_tick_elaps},
    {"lv_tick_get", &invoke_INT, (void*)&lv_tick_get},
    {"lv_tick_inc", &invoke_void_INT, (void*)&lv_tick_inc},
    {"lv_tick_set_cb", &invoke_void_INT, (void*)&lv_tick_set_cb},
    {"lv_tileview_add_tile", &invoke_lv_obj_t_p_lv_obj_t_p_INT_INT_INT, (void*)&lv_tileview_add_tile},
    {"lv_tileview_create", &invoke_widget_create, (void*)&lv_tileview_create},
    {"lv_tileview_get_tile_active", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_tileview_get_tile_active},
    {"lv_tileview_set_tile", &invoke_void_lv_obj_t_p_lv_obj_t_p_INT, (void*)&lv_tileview_set_tile},
    {"lv_tileview_set_tile_by_index", &invoke_void_lv_obj_t_p_INT_INT_INT, (void*)&lv_tileview_set_tile_by_index},
    {"lv_tree_node_create", &invoke_lv_tree_node_t_p_lv_tree_class_t_p_lv_tree_node_t_p, (void*)&lv_tree_node_create},
    {"lv_tree_node_delete", &invoke_void_lv_tree_node_t_p, (void*)&lv_tree_node_delete},
    {"lv_tree_walk", &invoke_BOOL_lv_tree_node_t_p_INT_INT_INT_INT_POINTER, (void*)&lv_tree_walk},
    {"lv_trigo_cos", &invoke_INT_INT, (void*)&lv_trigo_cos},
    {"lv_trigo_sin", &invoke_INT_INT, (void*)&lv_trigo_sin},
    {"lv_utils_bsearch", &invoke_POINTER_POINTER_POINTER_INT_INT_INT, (void*)&lv_utils_bsearch},
    {"lv_version_info", &invoke_const_char_p, (void*)&lv_version_info},
    {"lv_version_major", &invoke_INT, (void*)&lv_version_major},
    {"lv_version_minor", &invoke_INT, (void*)&lv_version_minor},
    {"lv_version_patch", &invoke_INT, (void*)&lv_version_patch},
    {"lv_vsnprintf", &invoke_INT_const_char_p_INT_const_char_p_UNKNOWN, (void*)&lv_vsnprintf},
    {"lv_win_add_button", &invoke_lv_obj_t_p_lv_obj_t_p_POINTER_INT, (void*)&lv_win_add_button},
    {"lv_win_add_title", &invoke_lv_obj_t_p_lv_obj_t_p_const_char_p, (void*)&lv_win_add_title},
    {"lv_win_create", &invoke_widget_create, (void*)&lv_win_create},
    {"lv_win_get_content", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_win_get_content},
    {"lv_win_get_header", &invoke_lv_obj_t_p_lv_obj_t_p, (void*)&lv_win_get_header},
    {"lv_zalloc", &invoke_POINTER_INT, (void*)&lv_zalloc},
    {NULL, NULL, NULL} // Sentinel
};

#define INVOKE_TABLE_SIZE 1298



// --- Function Lookup Implementation ---
// --- Function Lookup ---

// Finds an entry in the invocation table by function name.
static const invoke_table_entry_t* find_invoke_entry(const char *name) {
    if (!name) return NULL;
    for (int i = 0; g_invoke_table[i].name != NULL; ++i) {
        // Direct string comparison
        if (strcmp(g_invoke_table[i].name, name) == 0) {
            return &g_invoke_table[i];
        }
    }
    return NULL;
}



// --- Main Value Unmarshaler Implementation ---
// --- Main Value Unmarshaler ---

// Forward declarations
static const invoke_table_entry_t* find_invoke_entry(const char *name);
static bool unmarshal_enum_value(cJSON *json_value, const char *enum_type_name, int *dest);
static bool unmarshal_color(cJSON *node, lv_color_t *dest);
static bool unmarshal_registered_ptr(cJSON *node, void **dest);
static bool unmarshal_int(cJSON *node, int *dest);
static bool unmarshal_int8(cJSON *node, int8_t *dest);
static bool unmarshal_float(cJSON *node, float *dest);
static bool unmarshal_double(cJSON *node, double *dest);
static bool unmarshal_bool(cJSON *node, bool *dest);
static bool unmarshal_string_ptr(cJSON *node, char **dest);
static bool unmarshal_char(cJSON *node, char *dest);
static bool unmarshal_size_t(cJSON *node, size_t *dest);
static bool unmarshal_int32(cJSON *node, int32_t *dest);
static bool unmarshal_uint8(cJSON *node, uint8_t *dest);

static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest) {
    if (!json_value || !expected_c_type || !dest) return false;

    // 1. Handle nested function calls { "call": "func_name", "args": [...] }
    if (cJSON_IsObject(json_value)) {
        cJSON *call_item = cJSON_GetObjectItemCaseSensitive(json_value, "call");
        cJSON *args_item = cJSON_GetObjectItemCaseSensitive(json_value, "args");
        if (call_item && cJSON_IsString(call_item) && args_item && cJSON_IsArray(args_item)) {
            const char *func_name = call_item->valuestring;
            const invoke_table_entry_t* entry = find_invoke_entry(func_name);
            if (!entry) {
                LOG_ERR_JSON(json_value, "Unmarshal Error: Nested call function '%s' not found in invoke table.", func_name);
                return false;
            }
            // Invoke the nested function, result goes into 'dest'
            // Note: target_obj_ptr is NULL for nested calls (unless context implies otherwise)
            if (!entry->invoke(NULL, dest, args_item, entry->func_ptr)) {
                 LOG_ERR_JSON(json_value, "Unmarshal Error: Nested call to '%s' failed.", func_name);
                 return false;
            }
            // TODO: Check if the return type of the nested call matches expected_c_type?
            // This requires getting return type info from the invoke_table_entry or function signature.
            return true; // Nested call successful
        }
    }

    // 2. Handle custom prefixes in strings
    if (cJSON_IsString(json_value) && json_value->valuestring) {
        const char *str_val = json_value->valuestring;
        if (str_val[0] == '#') {
           if (strcmp(expected_c_type, "lv_color_t") == 0) {
               return unmarshal_color(json_value, (lv_color_t*)dest);
           } else { /* Fall through if type mismatch */ }
        }
        if (str_val[0] == '@') {
           if (strchr(expected_c_type, '*')) {
                return unmarshal_registered_ptr(json_value, (void**)dest);
           } else { /* Fall through if type mismatch */ }
        }
        // Fall through to check if it's an enum name or regular string
    }

    // 3. Dispatch based on expected C type
    // Using strcmp for type matching. Could be optimized (e.g., hash map if many types).
    if (strcmp(expected_c_type, "int") == 0) {
        return unmarshal_int(json_value, (int*)dest);
    }
    else if (strcmp(expected_c_type, "int8_t") == 0) {
        return unmarshal_int8(json_value, (int8_t*)dest);
    }
    else if (strcmp(expected_c_type, "uint8_t") == 0) {
        return unmarshal_uint8(json_value, (uint8_t*)dest);
    }
    else if (strcmp(expected_c_type, "int16_t") == 0) {
        return unmarshal_int16(json_value, (int16_t*)dest);
    }
    else if (strcmp(expected_c_type, "uint16_t") == 0) {
        return unmarshal_uint16(json_value, (uint16_t*)dest);
    }
    else if (strcmp(expected_c_type, "int32_t") == 0) {
        return unmarshal_int32(json_value, (int32_t*)dest);
    }
    else if (strcmp(expected_c_type, "uint32_t") == 0) {
        return unmarshal_uint32(json_value, (uint32_t*)dest);
    }
    else if (strcmp(expected_c_type, "int64_t") == 0) {
        return unmarshal_int64(json_value, (int64_t*)dest);
    }
    else if (strcmp(expected_c_type, "uint64_t") == 0) {
        return unmarshal_uint64(json_value, (uint64_t*)dest);
    }
    else if (strcmp(expected_c_type, "size_t") == 0) {
        return unmarshal_size_t(json_value, (size_t*)dest);
    }
    else if (strcmp(expected_c_type, "lv_coord_t") == 0) {
        return unmarshal_int32(json_value, (lv_coord_t*)dest);
    }
    else if (strcmp(expected_c_type, "lv_opa_t") == 0) {
        return unmarshal_uint8(json_value, (lv_opa_t*)dest);
    }
    else if (strcmp(expected_c_type, "float") == 0) {
        return unmarshal_float(json_value, (float*)dest);
    }
    else if (strcmp(expected_c_type, "double") == 0) {
        return unmarshal_double(json_value, (double*)dest);
    }
    else if (strcmp(expected_c_type, "bool") == 0) {
        return unmarshal_bool(json_value, (bool*)dest);
    }
    else if (strcmp(expected_c_type, "_Bool") == 0) {
        return unmarshal_bool(json_value, (_Bool*)dest);
    }
    else if (strcmp(expected_c_type, "const char *") == 0) {
        return unmarshal_string_ptr(json_value, (char **)dest);
    }
    else if (strcmp(expected_c_type, "char *") == 0) {
        return unmarshal_string_ptr(json_value, (char **)dest);
    }
    else if (strcmp(expected_c_type, "char") == 0) {
        return unmarshal_char(json_value, (char*)dest);
    }
    else if (strcmp(expected_c_type, "lv_color_t") == 0) {
        return unmarshal_color(json_value, (lv_color_t*)dest);
    }
    else if (strncmp(expected_c_type, "lv_", 3) == 0 && strstr(expected_c_type, "_t")) {
       // Assuming an enum type based on naming convention
       return unmarshal_enum_value(json_value, expected_c_type, (int*)dest);
    }
    else {
        // Type not handled or is a generic pointer expected via '@' (which failed above)
        LOG_ERR_JSON(json_value, "Unmarshal Error: Unsupported expected C type '%s' or invalid value format.", expected_c_type);
        // Attempt string unmarshal as a last resort? Only if pointer expected.
        if (strchr(expected_c_type, '*') && cJSON_IsString(json_value)) {
           LOG_WARN("Attempting basic string unmarshal for unexpected type %s", expected_c_type);
           return unmarshal_string_ptr(json_value, (char **)dest);
        }
        return false;
    }
}



// --- Custom Managed Object Creators Implementation ---
// --- Custom Managed Object Creators ---

#ifndef LV_MALLOC
#include <stdlib.h>
#define LV_MALLOC malloc
#define LV_FREE free
#endif

extern void lvgl_json_register_ptr(const char *name, void *ptr);
extern void lv_style_init(lv_style_t *);
extern void lv_fs_drv_init(lv_fs_drv_t *);
extern void lv_layer_init(lv_layer_t *);

// Creator for lv_style_t using lv_style_init
lv_style_t* lv_style_create_managed(const char *name) {
    if (!name) {
        LOG_ERR("lv_style_create_managed: Name cannot be NULL.");
        return NULL;
    }
    LOG_INFO("Creating managed lv_style_t with name '%s'", name);
    lv_style_t *new_obj = (lv_style_t*)LV_MALLOC(sizeof(lv_style_t));
    if (!new_obj) {
        LOG_ERR("lv_style_create_managed: Failed to allocate memory for lv_style_t.");
        return NULL;
    }
    lv_style_init(new_obj);
    lvgl_json_register_ptr(name, (void*)new_obj);
    return new_obj;
}

// Creator for lv_fs_drv_t using lv_fs_drv_init
lv_fs_drv_t* lv_fs_drv_create_managed(const char *name) {
    if (!name) {
        LOG_ERR("lv_fs_drv_create_managed: Name cannot be NULL.");
        return NULL;
    }
    LOG_INFO("Creating managed lv_fs_drv_t with name '%s'", name);
    lv_fs_drv_t *new_obj = (lv_fs_drv_t*)LV_MALLOC(sizeof(lv_fs_drv_t));
    if (!new_obj) {
        LOG_ERR("lv_fs_drv_create_managed: Failed to allocate memory for lv_fs_drv_t.");
        return NULL;
    }
    lv_fs_drv_init(new_obj);
    lvgl_json_register_ptr(name, (void*)new_obj);
    return new_obj;
}

// Creator for lv_layer_t using lv_layer_init
lv_layer_t* lv_layer_create_managed(const char *name) {
    if (!name) {
        LOG_ERR("lv_layer_create_managed: Name cannot be NULL.");
        return NULL;
    }
    LOG_INFO("Creating managed lv_layer_t with name '%s'", name);
    lv_layer_t *new_obj = (lv_layer_t*)LV_MALLOC(sizeof(lv_layer_t));
    if (!new_obj) {
        LOG_ERR("lv_layer_create_managed: Failed to allocate memory for lv_layer_t.");
        return NULL;
    }
    lv_layer_init(new_obj);
    lvgl_json_register_ptr(name, (void*)new_obj);
    return new_obj;
}



// --- JSON UI Renderer Implementation ---
// --- JSON UI Renderer ---

#include <stdio.h> // For debug prints

// Forward declarations
static bool render_json_node(cJSON *node, lv_obj_t *parent);
static const invoke_table_entry_t* find_invoke_entry(const char *name);
static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest);
extern void* lvgl_json_get_registered_ptr(const char *name);
extern void lvgl_json_register_ptr(const char *name, void *ptr);
extern lv_style_t* lv_style_create_managed(const char *name);
extern lv_fs_drv_t* lv_fs_drv_create_managed(const char *name);
extern lv_layer_t* lv_layer_create_managed(const char *name);

static bool render_json_node(cJSON *node, lv_obj_t *parent) {
    if (!cJSON_IsObject(node)) {
        LOG_ERR("Render Error: Expected JSON object for UI node.");
        return false;
    }

    // 1. Determine Object Type and ID
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(node, "type");
    const char *type_str = "obj"; // Default type
    if (type_item && cJSON_IsString(type_item)) {
        type_str = type_item->valuestring;
    }
    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(node, "id");
    const char *id_str = NULL;
    if (id_item && cJSON_IsString(id_item)) {
        if (id_item->valuestring[0] == '@') {
           id_str = id_item->valuestring + 1; // Get name part after '@'
        } else {
           LOG_WARN("Render Warning: 'id' property '%s' should start with '@' for registration. Ignoring registration.", id_item->valuestring);
           id_str = NULL; // Don't register if format is wrong
        }
    }

    // 2. Create the LVGL Object / Resource
    void *created_entity = NULL; // Can be lv_obj_t* or lv_style_t* etc.
    bool is_widget = true; // Is it an lv_obj_t based widget?

    // Check for custom creators (e.g., for styles)
    if (strcmp(type_str, "style") == 0) {
        if (!id_str) {
            LOG_ERR("Render Error: Type 'style' requires an 'id' property starting with '@'.");
            return false;
        }
        // Call custom creator which handles allocation, init, and registration
        created_entity = (void*)lv_style_create_managed(id_str);
        if (!created_entity) return false; // Error logged in creator
        is_widget = false; // Mark as non-widget (doesn't take parent, no children)
    }
    else if (strcmp(type_str, "fs_drv") == 0) {
        if (!id_str) {
            LOG_ERR("Render Error: Type 'fs_drv' requires an 'id' property starting with '@'.");
            return false;
        }
        // Call custom creator which handles allocation, init, and registration
        created_entity = (void*)lv_fs_drv_create_managed(id_str);
        if (!created_entity) return false; // Error logged in creator
        is_widget = false; // Mark as non-widget (doesn't take parent, no children)
    }
    else if (strcmp(type_str, "layer") == 0) {
        if (!id_str) {
            LOG_ERR("Render Error: Type 'layer' requires an 'id' property starting with '@'.");
            return false;
        }
        // Call custom creator which handles allocation, init, and registration
        created_entity = (void*)lv_layer_create_managed(id_str);
        if (!created_entity) return false; // Error logged in creator
        is_widget = false; // Mark as non-widget (doesn't take parent, no children)
    }
    else {
        // Default: Assume it's a widget type (lv_obj_t based)
        char create_func_name[64];
        snprintf(create_func_name, sizeof(create_func_name), "lv_%s_create", type_str);
        const invoke_table_entry_t* create_entry = find_invoke_entry(create_func_name);
        if (!create_entry) {
            LOG_ERR_JSON(node, "Render Error: Create function '%s' not found.", create_func_name);
            return false;
        }

        // Prepare arguments for creator (just the parent obj)
        cJSON *create_args_json = cJSON_CreateArray();
        if (!create_args_json) { LOG_ERR("Memory error creating args for %s", create_func_name); return false; }

        // How to pass the parent lv_obj_t*? 
        // Option A: Register parent temporarily? Seems complex.
        // Option B: Modify invoke_fn_t signature? Chosen earlier to avoid.
        // Option C: Assume create functions take exactly one lv_obj_t* arg 
        //           and handle it specially here or in the invoke func?
        // Let's assume the invoker for lv_..._create(lv_obj_t*) handles this.
        // The generated invoker needs to know its first arg is lv_obj_t* and get it from `parent`.
        // -- RETHINK invoke_fn_t signature --
        // Let's assume invoke_fn_t takes target_obj_ptr which is the parent here.
        // We pass NULL for json args as lv_..._create only takes parent.
        lv_obj_t* new_widget = NULL;
        // We need the invoker to place the result into &new_widget.
        if (!create_entry->invoke((void*)parent, &new_widget, NULL, create_entry->func_ptr)) { 
            LOG_ERR_JSON(node, "Render Error: Failed to invoke %s.", create_func_name);
            // cJSON_Delete(create_args_json); // Not needed if passing NULL
            return false;
        }
        // cJSON_Delete(create_args_json); // Not needed if passing NULL
        if (!new_widget) { LOG_ERR("Render Error: %s returned NULL.", create_func_name); return false; }
        created_entity = (void*)new_widget;
        is_widget = true;

        // Register if ID is provided and it wasn't a custom-created type
        if (id_str) {
            lvgl_json_register_ptr(id_str, created_entity);
        }
    }

    // 3. Set Properties
    cJSON *prop = NULL;
    for (prop = node->child; prop != NULL; prop = prop->next) {
        const char *prop_name = prop->string;
        if (!prop_name) continue;
        // Skip known control properties
        if (strcmp(prop_name, "type") == 0 || strcmp(prop_name, "id") == 0 || strcmp(prop_name, "children") == 0) {
            continue;
        }
        // Property value must be an array of arguments
        if (!cJSON_IsArray(prop)) {
            LOG_ERR_JSON(prop, "Render Warning: Property '%s' value is not a JSON array. Skipping.", prop_name);
            continue;
        }

        // Find the setter function: lv_<type>_set_<prop> or lv_obj_set_<prop>
        char setter_name[128];
        const invoke_table_entry_t* setter_entry = NULL;

        // Try specific type setter first (only if it's a widget)
        if (is_widget) {
           snprintf(setter_name, sizeof(setter_name), "lv_%s_set_%s", type_str, prop_name);
           setter_entry = find_invoke_entry(setter_name);
        }

        // Try generic lv_obj setter if specific not found or not applicable
        if (!setter_entry && is_widget) { // Only widgets have generic obj setters
            snprintf(setter_name, sizeof(setter_name), "lv_obj_set_%s", prop_name);
            setter_entry = find_invoke_entry(setter_name);
        }

        // Try non-widget setter (e.g., lv_style_set_...)
        if (!setter_entry && !is_widget) {
            snprintf(setter_name, sizeof(setter_name), "lv_%s_set_%s", type_str, prop_name);
            setter_entry = find_invoke_entry(setter_name);
        }

        if (!setter_entry) {
            LOG_ERR_JSON(node, "Render Warning: No setter function found for property '%s' on type '%s'. Searched lv_%s_set_..., lv_obj_set_....", prop_name, type_str, type_str);
            continue;
        }

        // Invoke the setter
        // Pass the created_entity as target_obj_ptr, NULL for dest (setters usually return void)
        // Pass the property's JSON array value as args_array
        if (!setter_entry->invoke(created_entity, NULL, prop, setter_entry->func_ptr)) {
             LOG_ERR_JSON(prop, "Render Error: Failed to set property '%s' using %s.", prop_name, setter_name);
             // Continue or return false? Let's continue for now.
        }
    }

    // 4. Process Children (only for widgets)
    if (is_widget) {
        cJSON *children_item = cJSON_GetObjectItemCaseSensitive(node, "children");
        if (children_item) {
            if (!cJSON_IsArray(children_item)) {
                LOG_ERR("Render Error: 'children' property must be an array.");
                // Cleanup created widget?
                return false;
            }
            cJSON *child_node = NULL;
            cJSON_ArrayForEach(child_node, children_item) {
                 if (!render_json_node(child_node, (lv_obj_t*)created_entity)) {
                     // Error rendering child, stop processing siblings? Or continue?
                     LOG_ERR("Render Error: Failed to render child node. Aborting sibling processing for this parent.");
                     // Cleanup? Difficult to manage partial tree creation.
                     return false; // Propagate failure
                 }
            }
        }
    }
    return true;
}

// --- Public API --- 

/**
 * @brief Renders a UI described by a cJSON object.
 *
 * @param root_json The root cJSON object (should be an array of UI nodes or a single node object).
 * @param implicit_root_parent The LVGL parent object for top-level UI elements.
 * @return true on success, false on failure.
 */
bool lvgl_json_render_ui(cJSON *root_json, lv_obj_t *implicit_root_parent) {
    if (!root_json) {
        LOG_ERR("Render Error: root_json is NULL.");
        return false;
    }
    if (!implicit_root_parent) {
        LOG_WARN("Render Warning: implicit_root_parent is NULL. Using lv_screen_active().");
        implicit_root_parent = lv_screen_active();
        if (!implicit_root_parent) {
             LOG_ERR("Render Error: Cannot get active screen.");
             return false;
        }
    }

    // Clear registry before starting? Optional, depends on desired behavior.
    // lvgl_json_registry_clear();

    bool success = true;
    if (cJSON_IsArray(root_json)) {
        cJSON *node = NULL;
        cJSON_ArrayForEach(node, root_json) {
            if (!render_json_node(node, implicit_root_parent)) {
                success = false;
                LOG_ERR("Render Error: Failed to render top-level node. Aborting.");
                break; // Stop processing further nodes on failure
            }
        }
    } else if (cJSON_IsObject(root_json)) {
        success = render_json_node(root_json, implicit_root_parent);
    } else {
        LOG_ERR("Render Error: root_json must be a JSON object or array.");
        success = false;
    }

    if (!success) {
         LOG_ERR("UI Rendering failed.");
         // Potential cleanup logic here?
    } else {
         LOG_INFO("UI Rendering completed successfully.");
    }
    return success;
}



