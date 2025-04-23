#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include "lvgl.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clears the active screen and builds a new UI based on JSON data.
 *
 * @param json_string The JSON string describing the UI layout.
 * @return true if parsing and building was successful, false otherwise.
 */
bool build_ui_from_json(const char *json_string);

#ifdef __cplusplus
}
#endif

#endif // UI_BUILDER_H
