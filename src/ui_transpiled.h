#ifndef UI_TRANSPILED_H
#define UI_TRANSPILED_H

#include "lvgl.h"

// If using the runtime registry with transpiled code, ensure these are linked:
// extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);
// extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the UI defined in '../../ui.json' onto the given parent.
 * @param screen_parent The parent LVGL object. If NULL, lv_screen_active() will be used.
 */
void create_ui_ui_transpiled(lv_obj_t *screen_parent);

#ifdef __cplusplus
}
#endif

#endif // UI_TRANSPILED_H