
#ifndef UI_BUILDER_H
#define UI_BUILDER_H

// Include base LVGL types and cJSON
#include "lvgl.h" // Assumes lvgl.h is available in the include path
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clears the active screen and builds a new UI based on JSON data.
 *
 * Parses the provided JSON string which describes an LVGL UI hierarchy,
 * properties, and styles, then creates the corresponding LVGL objects
 * on the currently active screen (after cleaning it).
 *
 * @param json_string A null-terminated C string containing the UI definition in JSON format.
 *                    The format should match the one produced by the lvgl_emulation library.
 * @return true if parsing and building the UI was successful, false on error
 *         (e.g., invalid JSON, memory allocation failure, unknown object type).
 *         Check logs for specific error details.
 */
bool build_ui_from_json(const char *json_string);

/**
 * @brief Registers a custom font symbol to be used by the builder.
 *
 * Allows the UI builder to find fonts referenced by name in the JSON.
 * Call this before build_ui_from_json for any custom or non-default fonts used.
 *
 * @param name The name of the font as it appears in the JSON (e.g., "montserrat_16").
 * @param font Pointer to the actual lv_font_t variable.
 * @return true if registration was successful (or updated), false on error (e.g., OOM).
 */
bool ui_builder_register_font(const char *name, const lv_font_t *font);

#ifdef __cplusplus
}
#endif

#endif // UI_BUILDER_H
