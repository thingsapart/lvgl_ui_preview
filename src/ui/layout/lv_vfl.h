#ifndef LV_VFL_H
#define LV_VFL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h> // For variadic grid item function

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR < 2
#warning "lv_vfl requires LVGL version 9.2 or later."
#endif

//------------------------------------------------------------------------------
// Internal Enums, Structs, Constants
//------------------------------------------------------------------------------

// --- Alignment Sentinel ---
// Sentinel value for alignment to inherit from container (for H/V layout)
#define _VFL_ALIGN_INHERIT ((lv_align_t)0xFF) // Internal constant

// --- For Linear Layout ---
typedef enum {
    _VFL_ITEM_OBJ,
    _VFL_ITEM_SPACE
} _vfl_linear_item_type_t;

typedef enum {
    _VFL_SIZE_FIXED,    // Specific pixel value
    _VFL_SIZE_CONTENT,  // Size based on object's content
    _VFL_SIZE_FLEX      // Proportional share of remaining space
} _vfl_size_type_t;

typedef struct {
    _vfl_linear_item_type_t item_type;
    lv_coord_t value;           // Space value, Fixed size value, or Flex factor
    _vfl_size_type_t size_type; // Used only for ITEM_OBJ
    lv_obj_t *obj;              // Used only for ITEM_OBJ
    lv_align_t cross_align;     // Used only for ITEM_OBJ (alignment override)
} _vfl_linear_item_t;


// --- For Grid Layout ---
typedef struct {
    lv_obj_t* obj;
    uint16_t row;
    uint16_t col;
    uint8_t row_span;
    uint8_t col_span;
    lv_grid_align_t h_align;
    lv_grid_align_t v_align;
} _vfl_grid_item_placement_t; // Internal struct for variadic function

// Sentinel for terminating variadic grid item list
#define _VFL_GRID_ITEM_SENTINEL ((_vfl_grid_item_placement_t){.obj=NULL}) // Internal

#include "meta/macro_helpers.h"

//------------------------------------------------------------------------------
// Public Macros for Layout Definition (User-Facing API)
//------------------------------------------------------------------------------

// --- Linear Layout (H/V) ---

/**
 * @brief Defines a horizontal layout.
 * @param PARENT The parent lv_obj_t* container.
 * @param ALIGN Default cross-axis alignment (e.g., LV_ALIGN_TOP_MID, LV_ALIGN_CENTER).
 * @param ... Sequence of layout items (_obj, _space).
 * Example:
 *  _layout_h(container, LV_ALIGN_CENTER,
 *      _space(10),
 *      _obj(btn1, _fixed(80)),
 *      _space(), // Default space
 *      _obj(label, _content(), LV_ALIGN_BOTTOM_MID), // Override alignment
 *      _space(),
 *      _obj(slider, _flex(1)),
 *      _space(10)
 *  );
 */
#define __layout_size(obj, w, w_across) lv_obj_set_size(obj, __layout_vert__ ? w : w_across, __layout_vert__ ? w_across : w);
#define _spacer(w) do { lv_obj_t *__spcr = lv_obj_create(_vfl_parent_v); __layout_size(__spcr, w, 1); } while (0);
#define _flex(obj, fr) do { lv_obj_set_parent(obj, _vfl_parent_v); lv_obj_set_flex_grow(obj, fr); } while (0);
#define _fixed(obj, sz) do { lv_obj_set_parent(obj, _vfl_parent_v); __layout_size(obj, sz, LV_SIZE_CONTENT); } while (0);
#define _content(obj) do { lv_obj_set_parent(obj, _vfl_parent_v); __size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT); } while (0);
#define _sized(obj, w, h) do { lv_obj_set_parent(obj, _vfl_parent_v); __size(obj, w, h); } while (0);

#define _layout_v(PARENT, ALIGN, ...) \
    do { \
        bool __layout_vert__ = true; \
        lv_obj_t* _vfl_parent_v = (PARENT); \
        lv_obj_set_layout(PARENT, LV_LAYOUT_FLEX); \
        lv_obj_set_flex_flow(PARENT, LV_FLEX_FLOW_COLUMN); \
        lv_obj_set_flex_align(PARENT, ALIGN, ALIGN, ALIGN); \
        __scrollable(PARENT, false); \
        __max_client_area(PARENT); \
        lv_obj_update_layout(PARENT); \
        _process_args(__VA_ARGS__); \
        lv_obj_update_layout(PARENT); \
    } while (0);

#define _layout_h(PARENT, ALIGN, ...) \
    do { \
        bool __layout_vert__ = false; \
        lv_obj_t* _vfl_parent_v = (PARENT); \
        lv_obj_set_layout(PARENT, LV_LAYOUT_FLEX); \
        lv_obj_set_flex_flow(PARENT, LV_FLEX_FLOW_ROW); \
        lv_obj_set_flex_align(PARENT, ALIGN, ALIGN, ALIGN); \
        __scrollable(PARENT, false); \
        __max_client_area(PARENT); \
        lv_obj_update_layout(PARENT); \
        _process_args(__VA_ARGS__); \
        lv_obj_update_layout(PARENT); \
    } while (0);

// --- Grid Layout ---

/**
 * @brief Defines a grid layout using LVGL's grid engine.
 * @param PARENT The parent lv_obj_t* container.
 * @param COLS_DEF Column definitions using _cols(...) macro.
 * @param ROWS_DEF Row definitions using _rows(...) macro.
 * @param ... Sequence of grid items defined using _grid_item(...).
 * Example:
 *  _layout_grid(container,
 *      _cols(_fr(1), _px(100), _content()),
 *      _rows(_px(50), _fr(1)),
 *      _grid_item(.obj=btn1, .row=0, .col=0),
 *      _grid_item(.obj=label, .row=0, .col=1, .col_span=2, .h_align=LV_GRID_ALIGN_CENTER),
 *      _grid_item(.obj=slider, .row=1, .col=0, .col_span=3, .v_align=LV_GRID_ALIGN_STRETCH)
 *  );
 */
#define _layout_grid(PARENT, COLS_DEF, ROWS_DEF, ...) \
    do { \
        lv_obj_t* _vfl_grid_parent = (PARENT); \
        if(!_vfl_grid_parent) { LV_LOG_WARN("_layout_grid: NULL parent"); break; } \
        lv_obj_set_layout(_vfl_grid_parent, LV_LAYOUT_GRID); \
        static const lv_coord_t _vfl_grid_cols[] = COLS_DEF; \
        static const lv_coord_t _vfl_grid_rows[] = ROWS_DEF; \
        lv_obj_set_grid_dsc_array(_vfl_grid_parent, _vfl_grid_cols, _vfl_grid_rows); \
        __expand_client_area(PARENT); \
        __scrollable(PARENT, false); \
        lv_obj_update_layout(_vfl_grid_parent); \
        _process_args(__VA_ARGS__); \
        lv_obj_update_layout(_vfl_grid_parent); \
    } while(0);

typedef struct {
    int32_t col_span;
    int32_t row_span;
    int32_t col_align;
    int32_t row_align;
} __cell_args_t;

void lv_vfl_set_grid_cell(lv_obj_t *obj, int32_t col, int32_t row, __cell_args_t opt);

#define _cell_opts(...) ((__cell_args_t) { .col_align = LV_GRID_ALIGN_START, .row_align = LV_GRID_ALIGN_START, __VA_ARGS__ })
#define _cell_4_(obj, col, row, opt) lv_vfl_set_grid_cell(obj, col, row, opt);
#define _cell_3_(obj, col, row) lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_CENTER, row, 1);
#define _cell(...) _PASTE(_cell_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// --- Grid Definition Helpers ---

/** Defines column track sizes for _layout_grid. Use _px, _fr, _content inside. */
#define _cols(...) { __VA_ARGS__, LV_GRID_TEMPLATE_LAST }
/* Defines row track sizes for _layout_grid. Use _px, _fr, _content inside. */
#define _rows(...) { __VA_ARGS__, LV_GRID_TEMPLATE_LAST }

// --- Grid Track Size Constants ---
/** Fixed pixel size for grid track. */
#define _px(x)     (x)
/** Fractional unit size for grid track. */
#define _fr(x)     LV_GRID_FR(x)
/* Note: _content() macro already defined for linear layout is reused for grid tracks */

//------------------------------------------------------------------------------
// Internal Core Layout Function Prototypes (Implementation in lv_vfl.c)
//------------------------------------------------------------------------------

/** @brief Internal: Performs linear (H/V) layout. */
void _vfl_do_linear_layout(lv_obj_t *parent,
                           _vfl_linear_item_t items[],
                           size_t item_count,
                           bool is_horizontal,
                           lv_align_t default_cross_align);

/** @brief Internal: Places items onto the LVGL grid. */
void _vfl_place_grid_items(lv_obj_t *parent, ...);


//------------------------------------------------------------------------------
// View def
//------------------------------------------------------------------------------

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_VFL_H */