
#ifndef LVGL_JSON_RENDERER_H
#define LVGL_JSON_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>
#include <cjson/cJSON.h> // Needed for cJSON types and free()
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // For logging fprintf
#include <stdlib.h> // For logging malloc/free (if used in json_node_to_string)

// --- Logging Macros (Provide basic implementation or allow override) ---
// Helper to stringify JSON node (defined in .c file)
// NOTE: Caller must free the returned string using cJSON_free()
char* json_node_to_string(cJSON *node); // Exposed for potential external use? Maybe keep static.

#ifndef LOG_ERR
#define LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef LOG_ERR_JSON
#define LOG_ERR_JSON(node, fmt, ...) do { \
        char* _json_str = json_node_to_string(node); \
        fprintf(stderr, "ERROR: [%s:%d] " fmt " [Near JSON: %s]\n", __FILE__, __LINE__, ##__VA_ARGS__, _json_str ? _json_str : " N/A"); \
        if (_json_str) cJSON_free(_json_str); \
    } while(0)
#endif
#ifndef LOG_WARN
#define LOG_WARN(fmt, ...) fprintf(stderr, "WARN: [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef LOG_WARN_JSON
#define LOG_WARN_JSON(node, fmt, ...) do { \
        char* _json_str = json_node_to_string(node); \
        fprintf(stderr, "WARN: [%s:%d] " fmt " [Near JSON: %s]\n", __FILE__, __LINE__, ##__VA_ARGS__, _json_str ? _json_str : " N/A"); \
        if (_json_str) cJSON_free(_json_str); \
    } while(0)
#endif
#ifndef LOG_INFO
#define LOG_INFO(fmt, ...) printf("INFO: [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef LOG_DEBUG
#ifdef LVGL_JSON_RENDERER_DEBUG
#define LOG_DEBUG(fmt, ...) printf("DEBUG: [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) (void)0
#endif
#endif

#define LV_MALLOC lv_malloc
#define LV_FREE lv_free


// --- Public API ---

/**
 * @brief Adds a custom string-to-integer mapping for enum unmarshaling.
 * Allows overriding or extending generated enum values at runtime.
 * The 'name' string must persist for the lifetime of its use or be a literal.
 *
 * @param name The string representation of the enum member.
 * @param value The integer value of the enum member.
 * @return true if added successfully, false if the user enum table is full.
 */
bool lvgl_json_add_user_enum_mapping(const char *name, int value);

/**
 * @brief Clears all runtime user-added enum mappings.
 */
void lvgl_json_clear_user_enum_mappings(void);

/**
 * @brief Renders a UI described by a cJSON object tree.
 *
 * Parses the JSON definition and creates corresponding LVGL objects and applies properties.
 * Assumes cJSON library is linked. Requires LVGL to be initialized beforehand.
 *
 * @param root_json The root cJSON object (must be an array of objects or a single object).
 * @param implicit_root_parent The LVGL parent object for all top-level elements defined in the JSON.
 *                             If NULL, lv_screen_active() will be used.
 * @return true if rendering was successful, false otherwise. Errors are logged.
 */
bool lvgl_json_render_ui(cJSON *root_json, lv_obj_t *implicit_root_parent);

/**
 * @brief Registers a pointer with a given name. Used for referencing objects/styles by ID ('@name').
 *
 * @param name The name to register the pointer under (should not include '@').
 * @param ptr The pointer to register.
 */
void lvgl_json_register_ptr(const char *name, void *ptr);

/**
 * @brief Retrieves a previously registered pointer by name.
 *
 * @param name The name of the pointer to retrieve (should not include '@').
 * @return The registered pointer, or NULL if not found.
 */
void* lvgl_json_get_registered_ptr(const char *name);

/**
 * @brief Clears all entries from the pointer registry.
 * Behavior depends on registry implementation (e.g., frees memory in hash map).
 */
void lvgl_json_registry_clear();

/**
 * @brief Generates a JSON string of predefined macro values.
 *
 * This function checks a list of known macro names (configured at generation time).
 * For each name, if it's defined as a C macro AND it's not found in the
 * generated enum table (i.e., it's a true macro, not an enum member),
 * its name and value are added to the JSON string.
 *
 * The primary use case is to obtain values for constants like LV_SIZE_CONTENT,
 * LV_COORD_MAX, etc., which are macros, not enums.
 * The output JSON string can be copied and used to create a "string_values.json"
 * file for tools that might need these mappings.
 *
 * @return A dynamically allocated JSON string. The caller MUST free this string
 *         using `cJSON_free()`. Returns NULL or an error JSON string on failure.
 */
char* lvgl_json_generate_values_json(void);

// --- Custom Managed Object Creator Prototypes ---
/** @brief Creates a managed lv_fs_drv_t identified by name. Allocates memory. */
extern lv_fs_drv_t* lv_fs_drv_create_managed(const char *name);
/** @brief Creates a managed lv_layer_t identified by name. Allocates memory. */
extern lv_layer_t* lv_layer_create_managed(const char *name);
/** @brief Creates a managed lv_style_t identified by name. Allocates memory. */
extern lv_style_t* lv_style_create_managed(const char *name);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LVGL_JSON_RENDERER_H */
