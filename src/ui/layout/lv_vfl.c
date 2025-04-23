#include "lv_vfl.h"

// Override LVGL debug functions.
#if 1

#include "debug.h"

static const char *TAG = "ui/lv_vfl";

#undef LV_LOG_INFO
#undef LV_LOG_WARN
#undef LV_LOG_ERROR

#define LV_LOG_INFO(fmt, ...) LOGI(TAG, fmt __VA_OPT__(, )##__VA_ARGS__)
#define LV_LOG_ERROR(fmt, ...) LOGE(TAG, fmt __VA_OPT__(, )##__VA_ARGS__)
#define LV_LOG_WARN(fmt, ...) LOGW(TAG, fmt __VA_OPT__(, )##__VA_ARGS__)

#endif

#include <stdlib.h> // For NULL
#include <stdarg.h> // For va_list, va_start, va_arg, va_end
#include <string.h> // For memset

// --- Internal helper to get default spacing ---
static lv_coord_t _vfl_get_default_space(lv_obj_t *parent, bool is_horizontal) {
    // ... (implementation unchanged) ...
    if (!parent) return 0;
    return is_horizontal ? lv_obj_get_style_pad_column(parent, LV_PART_MAIN)
                         : lv_obj_get_style_pad_row(parent, LV_PART_MAIN);
}

// --- Core linear layout logic ---
void _vfl_do_linear_layout(lv_obj_t *parent,
                           _vfl_linear_item_t items[],
                           size_t item_count,
                           bool is_horizontal,
                           lv_align_t default_cross_align)
{
    // ... (implementation unchanged, includes child obj NULL check) ...
        if (!parent || !items || item_count == 0) {
        LV_LOG_WARN("Invalid arguments for _vfl_do_linear_layout.");
        return;
    }

    // --- Pass 1 ---
    lv_coord_t total_fixed_size = 0;
    lv_coord_t total_space = 0;
    lv_coord_t total_flex_factor = 0;
    lv_coord_t default_space = _vfl_get_default_space(parent, is_horizontal);

    for (size_t i = 0; i < item_count; ++i) {
        _vfl_linear_item_t *item = &items[i];
        if (item->item_type == _VFL_ITEM_OBJ) {
            if (!item->obj) {
                 LV_LOG_WARN("VFL Linear Layout: NULL object encountered at index %zu", i);
                 item->size_type = _VFL_SIZE_FIXED; item->value = 0;
                 continue;
            }
#if LVGL_VERSION_MAJOR >= 9 // Check child validity early if possible
            if (!lv_obj_is_valid(item->obj)) {
                LV_LOG_WARN("VFL Linear Layout: Invalid object %p encountered at index %zu", (void*)item->obj, i);
                item->size_type = _VFL_SIZE_FIXED; item->value = 0; // Treat as zero size
                continue; // Skip this invalid object entirely
            }
#endif
            if (lv_obj_get_parent(item->obj) != parent) {
                 lv_obj_set_parent(item->obj, parent);
            }
            switch (item->size_type) { /* ... size calculation ... */
                case _VFL_SIZE_FIXED: total_fixed_size += item->value; break;
                case _VFL_SIZE_CONTENT:
                     if (is_horizontal) {
                         lv_obj_set_width(item->obj, LV_COORD_MAX);
                         item->value = lv_obj_get_content_width(item->obj);
                     } else {
                         lv_obj_set_height(item->obj, LV_COORD_MAX);
                         item->value = lv_obj_get_content_height(item->obj);
                     }
                    if(item->value < 0) item->value = 0;
                    total_fixed_size += item->value;
                    break;
                case _VFL_SIZE_FLEX: total_flex_factor += item->value; break;
            }
        } else if (item->item_type == _VFL_ITEM_SPACE) {
            total_space += (item->value == -1) ? default_space : item->value;
        }
    }

    // --- Calculate available space --- /* ... calculation ... */
    lv_coord_t parent_inner_width = lv_obj_get_content_width(parent);
    lv_coord_t parent_inner_height = lv_obj_get_content_height(parent);
    lv_coord_t available_size = is_horizontal ? parent_inner_width : parent_inner_height;
    lv_coord_t remaining_space = available_size - total_fixed_size - total_space;
    lv_coord_t space_per_flex_unit = 0;
    if (total_flex_factor > 0 && remaining_space > 0) {
        space_per_flex_unit = remaining_space / total_flex_factor;
    } else if (remaining_space < 0) {
        LV_LOG_INFO("VFL Linear Layout: Not enough space. Flex items will have zero size.");
        remaining_space = 0;
    }


    // --- Pass 2 --- /* ... positioning ... */
    lv_coord_t current_pos = is_horizontal ? lv_obj_get_style_pad_left(parent, LV_PART_MAIN)
                                          : lv_obj_get_style_pad_top(parent, LV_PART_MAIN);
    for (size_t i = 0; i < item_count; ++i) {
        _vfl_linear_item_t *item = &items[i];
        if (item->item_type == _VFL_ITEM_OBJ) {
            // NULL check and validity check already done in Pass 1
            if (!item->obj
#if LVGL_VERSION_MAJOR >= 9
                || !lv_obj_is_valid(item->obj) // Re-check validity in case it changed? Usually not needed.
#endif
            ) continue;

            lv_coord_t item_size = 0;
            switch (item->size_type) { /* ... size assignment ... */
                case _VFL_SIZE_FIXED: case _VFL_SIZE_CONTENT: item_size = item->value; break;
                case _VFL_SIZE_FLEX: item_size = space_per_flex_unit * item->value; break;
            }
            if (item_size < 0) item_size = 0;
            lv_align_t item_align = (item->cross_align == _VFL_ALIGN_INHERIT) ? default_cross_align : item->cross_align;
            if (is_horizontal) { /* ... position H ... */
                lv_obj_set_width(item->obj, item_size);
                lv_obj_set_pos(item->obj, current_pos, 0);
                lv_obj_align(item->obj, item_align, 0, 0);
                lv_obj_set_x(item->obj, current_pos);
            } else { /* ... position V ... */
                lv_obj_set_height(item->obj, item_size);
                lv_obj_set_pos(item->obj, 0, current_pos);
                lv_obj_align(item->obj, item_align, 0, 0);
                lv_obj_set_y(item->obj, current_pos);
            }
            current_pos += item_size;
        } else if (item->item_type == _VFL_ITEM_SPACE) { /* ... space ... */
            lv_coord_t space_value = (item->value == -1) ? default_space : item->value;
            current_pos += space_value;
        }
    }
    lv_obj_update_layout(parent);
}

// --- Function to handle placing items on the grid (Further Corrected) ---
void _vfl_place_grid_items(lv_obj_t *parent, ...) {
    // Check parent validity FIRST and FOREMOST
    if (!parent) {
        LV_LOG_ERROR("_vfl_place_grid_items: FATAL: Called with NULL parent pointer.");
        // Consider assert(parent != NULL);
        return;
    }

    // Use lv_obj_is_valid to check the parent object itself
#if LVGL_VERSION_MAJOR >= 9
    if (!lv_obj_is_valid(parent)) {
        LV_LOG_ERROR("_vfl_place_grid_items: FATAL: Parent object %p provided to _layout_grid is invalid/deleted.", (void*)parent);
        // Consider assert(lv_obj_is_valid(parent));
        return; // Cannot proceed if parent is invalid
    }
#else
    // Fallback if lv_obj_is_valid isn't available/used: Log that we're proceeding without the check.
    // LV_LOG_WARN("_vfl_place_grid_items: lv_obj_is_valid check not available/enabled for parent.");
#endif

    va_list args;
    va_start(args, parent);

    int item_index = 0; // For logging

    while (1) {
        _vfl_grid_item_placement_t placement;
        placement = va_arg(args, _vfl_grid_item_placement_t);

        // Check for sentinel
        if (placement.obj == _VFL_GRID_ITEM_SENTINEL.obj) {
            break; // End of arguments list
        }

        // Check if the user-provided child object pointer is NULL
        if (placement.obj == NULL) {
            LV_LOG_WARN("_vfl_place_grid_items: Skipped grid item #%d because a NULL object pointer was provided in _grid_item().", item_index);
            item_index++;
            continue;
        }

        // Check if the child object pointer is valid
#if LVGL_VERSION_MAJOR >= 9
        if (!lv_obj_is_valid(placement.obj)) {
            LV_LOG_WARN("_vfl_place_grid_items: Skipped grid item #%d for object at %p because it's not a valid LVGL object.", item_index, (void*)placement.obj);
            item_index++;
            continue;
        }
#else
         // LV_LOG_WARN("_vfl_place_grid_items: lv_obj_is_valid check not available/enabled for child %p.", (void*)placement.obj);
#endif

        // --- Proceed with VALID parent and VALID, non-NULL child ---

        // Reparent if necessary
        if (lv_obj_get_parent(placement.obj) != parent) {
            // Parent validity checked at function start, should still be valid here unless
            // there's a severe concurrency issue or a bug deletes it mid-function.
            lv_obj_set_parent(placement.obj, parent);
        }

        // Apply grid cell properties - Line ~187 in lv_vfl.c
        // If crash still occurs here, it implies an issue within LVGL's grid handling
        // for a valid parent and valid child.
        lv_obj_set_grid_cell(placement.obj,
                             placement.h_align, placement.col, placement.col_span,
                             placement.v_align, placement.row, placement.row_span);

        item_index++;
    }

    va_end(args);
}

void lv_vfl_set_grid_cell(lv_obj_t *obj, int32_t col, int32_t row, __cell_args_t opt) {
    lv_obj_set_grid_cell(obj, opt.col_align, col, opt.col_span == 0 ? 1 : opt.col_span, opt.row_align, row, opt.row_span == 0 ? 1 : opt.row_span);
}