
#ifndef EMUL_LVGL_H
#define EMUL_LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

// Define LV_EXPORT_CONST_INT if not defined elsewhere (e.g., by real LVGL headers)
// This is just a placeholder to avoid compilation errors if the macro is used.
#ifndef LV_EXPORT_CONST_INT
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning_ ## int_value {int unused;}
#endif


// --- Basic Type Definitions (Mimicking LVGL) ---

struct _lv_obj_t; // Forward declaration
typedef struct _lv_obj_t lv_obj_t;

// Coordinates (copied from example)
typedef int32_t lv_coord_t;
#define LV_COORD_TYPE_SHIFT    (29U)
#define LV_COORD_TYPE_MASK     (3 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE(x)       ((x) & LV_COORD_TYPE_MASK)
#define LV_COORD_PLAIN(x)      ((x) & ~LV_COORD_TYPE_MASK)
#define LV_COORD_TYPE_PX       (0 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_SPEC     (1 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_PX_NEG   (3 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_IS_PX(x)       (LV_COORD_TYPE(x) == LV_COORD_TYPE_PX || LV_COORD_TYPE(x) == LV_COORD_TYPE_PX_NEG)
#define LV_COORD_IS_SPEC(x)     (LV_COORD_TYPE(x) == LV_COORD_TYPE_SPEC)
#define LV_COORD_SET_SPEC(x)    ((x) | LV_COORD_TYPE_SPEC)
#define LV_COORD_MAX            ((1 << LV_COORD_TYPE_SHIFT) - 1)
#define LV_COORD_MIN            (-LV_COORD_MAX)
#define LV_SIZE_CONTENT         LV_COORD_SET_SPEC(LV_COORD_MAX)
#define LV_PCT_STORED_MAX       (LV_COORD_MAX - 1)
#if LV_PCT_STORED_MAX % 2 != 0
#error LV_PCT_STORED_MAX should be an even number
#endif
#define LV_PCT_POS_MAX          (LV_PCT_STORED_MAX / 2)
#define LV_PCT(x)               (LV_COORD_SET_SPEC(((x) < 0 ? (LV_PCT_POS_MAX - LV_MAX((x), -LV_PCT_POS_MAX)) : LV_MIN((x), LV_PCT_POS_MAX))))
#define LV_COORD_IS_PCT(x)      ((LV_COORD_IS_SPEC(x) && LV_COORD_PLAIN(x) <= LV_PCT_STORED_MAX))
#define LV_COORD_GET_PCT(x)     (LV_COORD_PLAIN(x) > LV_PCT_POS_MAX ? LV_PCT_POS_MAX - LV_COORD_PLAIN(x) : LV_COORD_PLAIN(x))

// Basic Math helpers (ensure LV_MAX/MIN are defined)
#ifndef LV_MAX
#define LV_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef LV_MIN
#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

LV_EXPORT_CONST_INT(LV_COORD_MAX);
LV_EXPORT_CONST_INT(LV_COORD_MIN);
LV_EXPORT_CONST_INT(LV_SIZE_CONTENT);

// Color (definition provided below if needed, or rely on lv_conf.h via user)
// Forward Declarations for other known structs/typedefs
typedef struct C_Pointer C_Pointer;
typedef struct _lv_mp_int_wrapper _lv_mp_int_wrapper;
typedef struct anim_bezier3_para_t anim_bezier3_para_t;
typedef struct anim_parameter_t anim_parameter_t;
typedef struct anim_state_t anim_state_t;
typedef struct anim_t anim_t;
typedef struct anim_timeline_t anim_timeline_t;
typedef struct area_t area_t;
typedef struct array_t array_t;
typedef struct cache_class_t cache_class_t;
typedef struct cache_entry_t cache_entry_t;
typedef struct cache_ops_t cache_ops_t;
typedef struct cache_slot_size_t cache_slot_size_t;
typedef struct cache_t cache_t;
typedef struct calendar_date_t calendar_date_t;
typedef struct chart_cursor_t chart_cursor_t;
typedef struct chart_series_t chart_series_t;
typedef struct circle_buf_t circle_buf_t;
typedef struct color16_t color16_t;
typedef struct color32_t color32_t;
typedef struct color_filter_dsc_t color_filter_dsc_t;
typedef struct color_hsv_t color_hsv_t;
typedef struct color_t color_t;
typedef struct display_t display_t;
typedef struct draw_arc_dsc_t draw_arc_dsc_t;
typedef struct draw_border_dsc_t draw_border_dsc_t;
typedef struct draw_box_shadow_dsc_t draw_box_shadow_dsc_t;
typedef struct draw_buf_handlers_t draw_buf_handlers_t;
typedef struct draw_buf_t draw_buf_t;
typedef struct draw_dsc_base_t draw_dsc_base_t;
typedef struct draw_fill_dsc_t draw_fill_dsc_t;
typedef struct draw_global_info_t draw_global_info_t;
typedef struct draw_glyph_dsc_t draw_glyph_dsc_t;
typedef struct draw_image_dsc_t draw_image_dsc_t;
typedef struct draw_image_sup_t draw_image_sup_t;
typedef struct draw_label_dsc_t draw_label_dsc_t;
typedef struct draw_label_hint_t draw_label_hint_t;
typedef struct draw_letter_dsc_t draw_letter_dsc_t;
typedef struct draw_line_dsc_t draw_line_dsc_t;
typedef struct draw_mask_rect_dsc_t draw_mask_rect_dsc_t;
typedef struct draw_rect_dsc_t draw_rect_dsc_t;
typedef struct draw_sw_blend_dsc_t draw_sw_blend_dsc_t;
typedef struct draw_sw_custom_blend_handler_t draw_sw_custom_blend_handler_t;
typedef struct draw_sw_mask_angle_param_cfg_t draw_sw_mask_angle_param_cfg_t;
typedef struct draw_sw_mask_angle_param_t draw_sw_mask_angle_param_t;
typedef struct draw_sw_mask_common_dsc_t draw_sw_mask_common_dsc_t;
typedef struct draw_sw_mask_fade_param_cfg_t draw_sw_mask_fade_param_cfg_t;
typedef struct draw_sw_mask_fade_param_t draw_sw_mask_fade_param_t;
typedef struct draw_sw_mask_line_param_cfg_t draw_sw_mask_line_param_cfg_t;
typedef struct draw_sw_mask_line_param_t draw_sw_mask_line_param_t;
typedef struct draw_sw_mask_map_param_cfg_t draw_sw_mask_map_param_cfg_t;
typedef struct draw_sw_mask_map_param_t draw_sw_mask_map_param_t;
typedef struct draw_sw_mask_radius_circle_dsc_t draw_sw_mask_radius_circle_dsc_t;
typedef struct draw_sw_mask_radius_param_cfg_t draw_sw_mask_radius_param_cfg_t;
typedef struct draw_sw_mask_radius_param_t draw_sw_mask_radius_param_t;
typedef struct draw_task_t draw_task_t;
typedef struct draw_triangle_dsc_t draw_triangle_dsc_t;
typedef struct draw_unit_t draw_unit_t;
typedef struct event_dsc_t event_dsc_t;
typedef struct event_list_t event_list_t;
typedef struct event_t event_t;
typedef struct font_class_t font_class_t;
typedef struct font_glyph_dsc_gid_t font_glyph_dsc_gid_t;
typedef struct font_glyph_dsc_t font_glyph_dsc_t;
typedef struct font_info_t font_info_t;
typedef struct font_t font_t;
typedef struct fs_dir_t fs_dir_t;
typedef struct fs_drv_t fs_drv_t;
typedef struct fs_file_cache_t fs_file_cache_t;
typedef struct fs_file_t fs_file_t;
typedef struct fs_path_ex_t fs_path_ex_t;
typedef struct gd_GCE gd_GCE;
typedef struct gd_GIF gd_GIF;
typedef struct gd_Palette gd_Palette;
typedef struct global_t global_t;
typedef struct grad_dsc_t grad_dsc_t;
typedef struct grad_stop_t grad_stop_t;
typedef struct group_t group_t;
typedef struct hit_test_info_t hit_test_info_t;
typedef struct image_cache_data_t image_cache_data_t;
typedef struct image_decoder_args_t image_decoder_args_t;
typedef struct image_decoder_dsc_t image_decoder_dsc_t;
typedef struct image_decoder_t image_decoder_t;
typedef struct image_dsc_t image_dsc_t;
typedef struct image_header_t image_header_t;
typedef struct indev_data_t indev_data_t;
typedef struct indev_keypad_t indev_keypad_t;
typedef struct indev_pointer_t indev_pointer_t;
typedef struct indev_t indev_t;
typedef struct iter_t iter_t;
typedef struct layer_t layer_t;
typedef struct layout_dsc_t layout_dsc_t;
typedef struct ll_t ll_t;
typedef struct matrix_t matrix_t;
typedef struct mem_monitor_t mem_monitor_t;
typedef struct obj_class_t obj_class_t;
typedef struct obj_style_transition_dsc_t obj_style_transition_dsc_t;
typedef struct observer_t observer_t;
typedef struct point_precise_t point_precise_t;
typedef struct point_t point_t;
typedef struct rb_node_t rb_node_t;
typedef struct rb_t rb_t;
typedef struct scale_section_t scale_section_t;
typedef struct span_coords_t span_coords_t;
typedef struct span_t span_t;
typedef struct sqrt_res_t sqrt_res_t;
typedef struct style_t style_t;
typedef struct style_transition_dsc_t style_transition_dsc_t;
typedef struct style_value_t style_value_t;
typedef struct subject_t subject_t;
typedef struct subject_value_t subject_value_t;
typedef struct theme_t theme_t;
typedef struct tick_state_t tick_state_t;
typedef struct timer_state_t timer_state_t;
typedef struct timer_t timer_t;
typedef struct tree_class_t tree_class_t;
typedef struct tree_node_t tree_node_t;

// Enums
typedef enum {
    LV_ALIGN_DEFAULT,
    LV_ALIGN_TOP_LEFT,
    LV_ALIGN_TOP_MID,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_BOTTOM_LEFT,
    LV_ALIGN_BOTTOM_MID,
    LV_ALIGN_BOTTOM_RIGHT,
    LV_ALIGN_LEFT_MID,
    LV_ALIGN_RIGHT_MID,
    LV_ALIGN_CENTER,
    LV_ALIGN_OUT_TOP_LEFT,
    LV_ALIGN_OUT_TOP_MID,
    LV_ALIGN_OUT_TOP_RIGHT,
    LV_ALIGN_OUT_BOTTOM_LEFT,
    LV_ALIGN_OUT_BOTTOM_MID,
    LV_ALIGN_OUT_BOTTOM_RIGHT,
    LV_ALIGN_OUT_LEFT_TOP,
    LV_ALIGN_OUT_LEFT_MID,
    LV_ALIGN_OUT_LEFT_BOTTOM,
    LV_ALIGN_OUT_RIGHT_TOP,
    LV_ALIGN_OUT_RIGHT_MID,
    LV_ALIGN_OUT_RIGHT_BOTTOM,
    _LV_ALIGN_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_ALIGN_DEF_LAST = 0xFFFF
    #endif
} lv_align_t;

typedef enum {
    LV_ANIM_IMAGE_PART_MAIN,
    _LV_ANIM_IMAGE_PART_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_ANIM_IMAGE_PART_DEF_LAST = 0xFFFF
    #endif
} lv_anim_image_part_t;

typedef enum {
    LV_BASE_DIR_LTR,
    LV_BASE_DIR_RTL,
    LV_BASE_DIR_AUTO,
    LV_BASE_DIR_NEUTRAL,
    LV_BASE_DIR_WEAK,
    _LV_BASE_DIR_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_BASE_DIR_DEF_LAST = 0xFFFF
    #endif
} lv_base_dir_t;

typedef enum {
    LV_BLEND_MODE_NORMAL,
    LV_BLEND_MODE_ADDITIVE,
    LV_BLEND_MODE_SUBTRACTIVE,
    LV_BLEND_MODE_MULTIPLY,
    LV_BLEND_MODE_DIFFERENCE,
    _LV_BLEND_MODE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_BLEND_MODE_DEF_LAST = 0xFFFF
    #endif
} lv_blend_mode_t;

typedef enum {
    LV_BORDER_SIDE_NONE,
    LV_BORDER_SIDE_BOTTOM,
    LV_BORDER_SIDE_TOP,
    LV_BORDER_SIDE_LEFT,
    LV_BORDER_SIDE_RIGHT,
    LV_BORDER_SIDE_FULL,
    LV_BORDER_SIDE_INTERNAL,
    _LV_BORDER_SIDE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_BORDER_SIDE_DEF_LAST = 0xFFFF
    #endif
} lv_border_side_t;

typedef enum {
    LV_CACHE_RESERVE_COND_OK,
    LV_CACHE_RESERVE_COND_TOO_LARGE,
    LV_CACHE_RESERVE_COND_NEED_VICTIM,
    LV_CACHE_RESERVE_COND_ERROR,
    _LV_CACHE_RESERVE_COND_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_CACHE_RESERVE_COND_DEF_LAST = 0xFFFF
    #endif
} lv_cache_reserve_cond_t;

typedef enum {
    LV_COLOR_FORMAT_UNKNOWN,
    LV_COLOR_FORMAT_RAW,
    LV_COLOR_FORMAT_RAW_ALPHA,
    LV_COLOR_FORMAT_L8,
    LV_COLOR_FORMAT_I1,
    LV_COLOR_FORMAT_I2,
    LV_COLOR_FORMAT_I4,
    LV_COLOR_FORMAT_I8,
    LV_COLOR_FORMAT_A8,
    LV_COLOR_FORMAT_RGB565,
    LV_COLOR_FORMAT_ARGB8565,
    LV_COLOR_FORMAT_RGB565A8,
    LV_COLOR_FORMAT_AL88,
    LV_COLOR_FORMAT_RGB888,
    LV_COLOR_FORMAT_ARGB8888,
    LV_COLOR_FORMAT_XRGB8888,
    LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED,
    LV_COLOR_FORMAT_A1,
    LV_COLOR_FORMAT_A2,
    LV_COLOR_FORMAT_A4,
    LV_COLOR_FORMAT_ARGB1555,
    LV_COLOR_FORMAT_ARGB4444,
    LV_COLOR_FORMAT_ARGB2222,
    LV_COLOR_FORMAT_YUV_START,
    LV_COLOR_FORMAT_I420,
    LV_COLOR_FORMAT_I422,
    LV_COLOR_FORMAT_I444,
    LV_COLOR_FORMAT_I400,
    LV_COLOR_FORMAT_NV21,
    LV_COLOR_FORMAT_NV12,
    LV_COLOR_FORMAT_YUY2,
    LV_COLOR_FORMAT_UYVY,
    LV_COLOR_FORMAT_YUV_END,
    LV_COLOR_FORMAT_PROPRIETARY_START,
    LV_COLOR_FORMAT_NEMA_TSC_START,
    LV_COLOR_FORMAT_NEMA_TSC4,
    LV_COLOR_FORMAT_NEMA_TSC6,
    LV_COLOR_FORMAT_NEMA_TSC6A,
    LV_COLOR_FORMAT_NEMA_TSC6AP,
    LV_COLOR_FORMAT_NEMA_TSC12,
    LV_COLOR_FORMAT_NEMA_TSC12A,
    LV_COLOR_FORMAT_NEMA_TSC_END,
    LV_COLOR_FORMAT_NATIVE,
    LV_COLOR_FORMAT_NATIVE_WITH_ALPHA,
    _LV_COLOR_FORMAT_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_COLOR_FORMAT_DEF_LAST = 0xFFFF
    #endif
} lv_color_format_t;

typedef enum {
    LV_COORD_MAX,
    LV_COORD_MIN,
    _LV_COORD_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_COORD_DEF_LAST = 0xFFFF
    #endif
} lv_coord_t;

typedef enum {
    LV_COVER_RES_COVER,
    LV_COVER_RES_NOT_COVER,
    LV_COVER_RES_MASKED,
    _LV_COVER_RES_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_COVER_RES_DEF_LAST = 0xFFFF
    #endif
} lv_cover_res_t;

typedef enum {
    LV_DIR_NONE,
    LV_DIR_LEFT,
    LV_DIR_RIGHT,
    LV_DIR_TOP,
    LV_DIR_BOTTOM,
    LV_DIR_HOR,
    LV_DIR_VER,
    LV_DIR_ALL,
    _LV_DIR_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DIR_DEF_LAST = 0xFFFF
    #endif
} lv_dir_t;

typedef enum {
    LV_DISPLAY_RENDER_MODE_PARTIAL,
    LV_DISPLAY_RENDER_MODE_DIRECT,
    LV_DISPLAY_RENDER_MODE_FULL,
    _LV_DISPLAY_RENDER_MODE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DISPLAY_RENDER_MODE_DEF_LAST = 0xFFFF
    #endif
} lv_display_render_mode_t;

typedef enum {
    LV_DISPLAY_ROTATION__0,
    LV_DISPLAY_ROTATION__90,
    LV_DISPLAY_ROTATION__180,
    LV_DISPLAY_ROTATION__270,
    _LV_DISPLAY_ROTATION_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DISPLAY_ROTATION_DEF_LAST = 0xFFFF
    #endif
} lv_display_rotation_t;

typedef enum {
    LV_DRAW_SW_MASK_LINE_SIDE_LEFT,
    LV_DRAW_SW_MASK_LINE_SIDE_RIGHT,
    LV_DRAW_SW_MASK_LINE_SIDE_TOP,
    LV_DRAW_SW_MASK_LINE_SIDE_BOTTOM,
    _LV_DRAW_SW_MASK_LINE_SIDE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DRAW_SW_MASK_LINE_SIDE_DEF_LAST = 0xFFFF
    #endif
} lv_draw_sw_mask_line_side_t;

typedef enum {
    LV_DRAW_SW_MASK_RES_TRANSP,
    LV_DRAW_SW_MASK_RES_FULL_COVER,
    LV_DRAW_SW_MASK_RES_CHANGED,
    LV_DRAW_SW_MASK_RES_UNKNOWN,
    _LV_DRAW_SW_MASK_RES_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DRAW_SW_MASK_RES_DEF_LAST = 0xFFFF
    #endif
} lv_draw_sw_mask_res_t;

typedef enum {
    LV_DRAW_SW_MASK_TYPE_LINE,
    LV_DRAW_SW_MASK_TYPE_ANGLE,
    LV_DRAW_SW_MASK_TYPE_RADIUS,
    LV_DRAW_SW_MASK_TYPE_FADE,
    LV_DRAW_SW_MASK_TYPE_MAP,
    _LV_DRAW_SW_MASK_TYPE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DRAW_SW_MASK_TYPE_DEF_LAST = 0xFFFF
    #endif
} lv_draw_sw_mask_type_t;

typedef enum {
    LV_DRAW_TASK_STATE_WAITING,
    LV_DRAW_TASK_STATE_QUEUED,
    LV_DRAW_TASK_STATE_IN_PROGRESS,
    LV_DRAW_TASK_STATE_READY,
    _LV_DRAW_TASK_STATE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DRAW_TASK_STATE_DEF_LAST = 0xFFFF
    #endif
} lv_draw_task_state_t;

typedef enum {
    LV_DRAW_TASK_TYPE_NONE,
    LV_DRAW_TASK_TYPE_FILL,
    LV_DRAW_TASK_TYPE_BORDER,
    LV_DRAW_TASK_TYPE_BOX_SHADOW,
    LV_DRAW_TASK_TYPE_LETTER,
    LV_DRAW_TASK_TYPE_LABEL,
    LV_DRAW_TASK_TYPE_IMAGE,
    LV_DRAW_TASK_TYPE_LAYER,
    LV_DRAW_TASK_TYPE_LINE,
    LV_DRAW_TASK_TYPE_ARC,
    LV_DRAW_TASK_TYPE_TRIANGLE,
    LV_DRAW_TASK_TYPE_MASK_RECTANGLE,
    LV_DRAW_TASK_TYPE_MASK_BITMAP,
    _LV_DRAW_TASK_TYPE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_DRAW_TASK_TYPE_DEF_LAST = 0xFFFF
    #endif
} lv_draw_task_type_t;

typedef enum {
    LV_EVENT_ALL,
    LV_EVENT_PRESSED,
    LV_EVENT_PRESSING,
    LV_EVENT_PRESS_LOST,
    LV_EVENT_SHORT_CLICKED,
    LV_EVENT_SINGLE_CLICKED,
    LV_EVENT_DOUBLE_CLICKED,
    LV_EVENT_TRIPLE_CLICKED,
    LV_EVENT_LONG_PRESSED,
    LV_EVENT_LONG_PRESSED_REPEAT,
    LV_EVENT_CLICKED,
    LV_EVENT_RELEASED,
    LV_EVENT_SCROLL_BEGIN,
    LV_EVENT_SCROLL_THROW_BEGIN,
    LV_EVENT_SCROLL_END,
    LV_EVENT_SCROLL,
    LV_EVENT_GESTURE,
    LV_EVENT_KEY,
    LV_EVENT_ROTARY,
    LV_EVENT_FOCUSED,
    LV_EVENT_DEFOCUSED,
    LV_EVENT_LEAVE,
    LV_EVENT_HIT_TEST,
    LV_EVENT_INDEV_RESET,
    LV_EVENT_HOVER_OVER,
    LV_EVENT_HOVER_LEAVE,
    LV_EVENT_COVER_CHECK,
    LV_EVENT_REFR_EXT_DRAW_SIZE,
    LV_EVENT_DRAW_MAIN_BEGIN,
    LV_EVENT_DRAW_MAIN,
    LV_EVENT_DRAW_MAIN_END,
    LV_EVENT_DRAW_POST_BEGIN,
    LV_EVENT_DRAW_POST,
    LV_EVENT_DRAW_POST_END,
    LV_EVENT_DRAW_TASK_ADDED,
    LV_EVENT_VALUE_CHANGED,
    LV_EVENT_INSERT,
    LV_EVENT_REFRESH,
    LV_EVENT_READY,
    LV_EVENT_CANCEL,
    LV_EVENT_CREATE,
    LV_EVENT_DELETE,
    LV_EVENT_CHILD_CHANGED,
    LV_EVENT_CHILD_CREATED,
    LV_EVENT_CHILD_DELETED,
    LV_EVENT_SCREEN_UNLOAD_START,
    LV_EVENT_SCREEN_LOAD_START,
    LV_EVENT_SCREEN_LOADED,
    LV_EVENT_SCREEN_UNLOADED,
    LV_EVENT_SIZE_CHANGED,
    LV_EVENT_STYLE_CHANGED,
    LV_EVENT_LAYOUT_CHANGED,
    LV_EVENT_GET_SELF_SIZE,
    LV_EVENT_INVALIDATE_AREA,
    LV_EVENT_RESOLUTION_CHANGED,
    LV_EVENT_COLOR_FORMAT_CHANGED,
    LV_EVENT_REFR_REQUEST,
    LV_EVENT_REFR_START,
    LV_EVENT_REFR_READY,
    LV_EVENT_RENDER_START,
    LV_EVENT_RENDER_READY,
    LV_EVENT_FLUSH_START,
    LV_EVENT_FLUSH_FINISH,
    LV_EVENT_FLUSH_WAIT_START,
    LV_EVENT_FLUSH_WAIT_FINISH,
    LV_EVENT_VSYNC,
    LV_EVENT_VSYNC_REQUEST,
    LV_EVENT_LAST,
    LV_EVENT_PREPROCESS,
    LV_EVENT_MARKED_DELETING,
    _LV_EVENT_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_EVENT_DEF_LAST = 0xFFFF
    #endif
} lv_event_t;

typedef enum {
    LV_FLEX_ALIGN_START,
    LV_FLEX_ALIGN_END,
    LV_FLEX_ALIGN_CENTER,
    LV_FLEX_ALIGN_SPACE_EVENLY,
    LV_FLEX_ALIGN_SPACE_AROUND,
    LV_FLEX_ALIGN_SPACE_BETWEEN,
    _LV_FLEX_ALIGN_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FLEX_ALIGN_DEF_LAST = 0xFFFF
    #endif
} lv_flex_align_t;

typedef enum {
    LV_FLEX_FLOW_ROW,
    LV_FLEX_FLOW_COLUMN,
    LV_FLEX_FLOW_ROW_WRAP,
    LV_FLEX_FLOW_ROW_REVERSE,
    LV_FLEX_FLOW_ROW_WRAP_REVERSE,
    LV_FLEX_FLOW_COLUMN_WRAP,
    LV_FLEX_FLOW_COLUMN_REVERSE,
    LV_FLEX_FLOW_COLUMN_WRAP_REVERSE,
    _LV_FLEX_FLOW_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FLEX_FLOW_DEF_LAST = 0xFFFF
    #endif
} lv_flex_flow_t;

typedef enum {
    LV_FONT_FMT_TXT_PLAIN,
    LV_FONT_FMT_TXT_COMPRESSED,
    LV_FONT_FMT_TXT_COMPRESSED_NO_PREFILTER,
    LV_FONT_FMT_PLAIN_ALIGNED,
    _LV_FONT_FMT_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FONT_FMT_DEF_LAST = 0xFFFF
    #endif
} lv_font_fmt_t;

typedef enum {
    LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL,
    LV_FONT_FMT_TXT_CMAP_SPARSE_FULL,
    LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY,
    LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
    _LV_FONT_FMT_TXT_CMAP_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FONT_FMT_TXT_CMAP_DEF_LAST = 0xFFFF
    #endif
} lv_font_fmt_txt_cmap_t;

typedef enum {
    LV_FONT_GLYPH_FORMAT_NONE,
    LV_FONT_GLYPH_FORMAT_A1,
    LV_FONT_GLYPH_FORMAT_A2,
    LV_FONT_GLYPH_FORMAT_A3,
    LV_FONT_GLYPH_FORMAT_A4,
    LV_FONT_GLYPH_FORMAT_A8,
    LV_FONT_GLYPH_FORMAT_A1_ALIGNED,
    LV_FONT_GLYPH_FORMAT_A2_ALIGNED,
    LV_FONT_GLYPH_FORMAT_A4_ALIGNED,
    LV_FONT_GLYPH_FORMAT_A8_ALIGNED,
    LV_FONT_GLYPH_FORMAT_IMAGE,
    LV_FONT_GLYPH_FORMAT_VECTOR,
    LV_FONT_GLYPH_FORMAT_SVG,
    LV_FONT_GLYPH_FORMAT_CUSTOM,
    _LV_FONT_GLYPH_FORMAT_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FONT_GLYPH_FORMAT_DEF_LAST = 0xFFFF
    #endif
} lv_font_glyph_format_t;

typedef enum {
    LV_FONT_KERNING_NORMAL,
    LV_FONT_KERNING_NONE,
    _LV_FONT_KERNING_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FONT_KERNING_DEF_LAST = 0xFFFF
    #endif
} lv_font_kerning_t;

typedef enum {
    LV_FONT_SUBPX_NONE,
    LV_FONT_SUBPX_HOR,
    LV_FONT_SUBPX_VER,
    LV_FONT_SUBPX_BOTH,
    _LV_FONT_SUBPX_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FONT_SUBPX_DEF_LAST = 0xFFFF
    #endif
} lv_font_subpx_t;

typedef enum {
    LV_FS_MODE_WR,
    LV_FS_MODE_RD,
    _LV_FS_MODE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FS_MODE_DEF_LAST = 0xFFFF
    #endif
} lv_fs_mode_t;

typedef enum {
    LV_FS_RES_OK,
    LV_FS_RES_HW_ERR,
    LV_FS_RES_FS_ERR,
    LV_FS_RES_NOT_EX,
    LV_FS_RES_FULL,
    LV_FS_RES_LOCKED,
    LV_FS_RES_DENIED,
    LV_FS_RES_BUSY,
    LV_FS_RES_TOUT,
    LV_FS_RES_NOT_IMP,
    LV_FS_RES_OUT_OF_MEM,
    LV_FS_RES_INV_PARAM,
    LV_FS_RES_UNKNOWN,
    _LV_FS_RES_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FS_RES_DEF_LAST = 0xFFFF
    #endif
} lv_fs_res_t;

typedef enum {
    LV_FS_SEEK_SET,
    LV_FS_SEEK_CUR,
    LV_FS_SEEK_END,
    _LV_FS_SEEK_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_FS_SEEK_DEF_LAST = 0xFFFF
    #endif
} lv_fs_seek_t;

typedef enum {
    LV_GRAD_DIR_NONE,
    LV_GRAD_DIR_VER,
    LV_GRAD_DIR_HOR,
    LV_GRAD_DIR_LINEAR,
    LV_GRAD_DIR_RADIAL,
    LV_GRAD_DIR_CONICAL,
    _LV_GRAD_DIR_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_GRAD_DIR_DEF_LAST = 0xFFFF
    #endif
} lv_grad_dir_t;

typedef enum {
    LV_GRAD_EXTEND_PAD,
    LV_GRAD_EXTEND_REPEAT,
    LV_GRAD_EXTEND_REFLECT,
    _LV_GRAD_EXTEND_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_GRAD_EXTEND_DEF_LAST = 0xFFFF
    #endif
} lv_grad_extend_t;

typedef enum {
    LV_GRID_ALIGN_START,
    LV_GRID_ALIGN_CENTER,
    LV_GRID_ALIGN_END,
    LV_GRID_ALIGN_STRETCH,
    LV_GRID_ALIGN_SPACE_EVENLY,
    LV_GRID_ALIGN_SPACE_AROUND,
    LV_GRID_ALIGN_SPACE_BETWEEN,
    _LV_GRID_ALIGN_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_GRID_ALIGN_DEF_LAST = 0xFFFF
    #endif
} lv_grid_align_t;

typedef enum {
    LV_GROUP_REFOCUS_POLICY_NEXT,
    LV_GROUP_REFOCUS_POLICY_PREV,
    _LV_GROUP_REFOCUS_POLICY_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_GROUP_REFOCUS_POLICY_DEF_LAST = 0xFFFF
    #endif
} lv_group_refocus_policy_t;

typedef enum {
    LV_INDEV_GESTURE_NONE,
    LV_INDEV_GESTURE_PINCH,
    LV_INDEV_GESTURE_SWIPE,
    LV_INDEV_GESTURE_ROTATE,
    LV_INDEV_GESTURE_TWO_FINGERS_SWIPE,
    LV_INDEV_GESTURE_SCROLL,
    LV_INDEV_GESTURE_CNT,
    _LV_INDEV_GESTURE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_INDEV_GESTURE_DEF_LAST = 0xFFFF
    #endif
} lv_indev_gesture_t;

typedef enum {
    LV_INDEV_MODE_NONE,
    LV_INDEV_MODE_TIMER,
    LV_INDEV_MODE_EVENT,
    _LV_INDEV_MODE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_INDEV_MODE_DEF_LAST = 0xFFFF
    #endif
} lv_indev_mode_t;

typedef enum {
    LV_INDEV_STATE_RELEASED,
    LV_INDEV_STATE_PRESSED,
    _LV_INDEV_STATE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_INDEV_STATE_DEF_LAST = 0xFFFF
    #endif
} lv_indev_state_t;

typedef enum {
    LV_INDEV_TYPE_NONE,
    LV_INDEV_TYPE_POINTER,
    LV_INDEV_TYPE_KEYPAD,
    LV_INDEV_TYPE_BUTTON,
    LV_INDEV_TYPE_ENCODER,
    _LV_INDEV_TYPE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_INDEV_TYPE_DEF_LAST = 0xFFFF
    #endif
} lv_indev_type_t;

typedef enum {
    LV_KEY_UP,
    LV_KEY_DOWN,
    LV_KEY_RIGHT,
    LV_KEY_LEFT,
    LV_KEY_ESC,
    LV_KEY_DEL,
    LV_KEY_BACKSPACE,
    LV_KEY_ENTER,
    LV_KEY_NEXT,
    LV_KEY_PREV,
    LV_KEY_HOME,
    LV_KEY_END,
    _LV_KEY_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_KEY_DEF_LAST = 0xFFFF
    #endif
} lv_key_t;

typedef enum {
    LV_LAYER_TYPE_NONE,
    LV_LAYER_TYPE_SIMPLE,
    LV_LAYER_TYPE_TRANSFORM,
    _LV_LAYER_TYPE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_LAYER_TYPE_DEF_LAST = 0xFFFF
    #endif
} lv_layer_type_t;

typedef enum {
    LV_LAYOUT_NONE,
    LV_LAYOUT_FLEX,
    LV_LAYOUT_GRID,
    LV_LAYOUT_LAST,
    _LV_LAYOUT_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_LAYOUT_DEF_LAST = 0xFFFF
    #endif
} lv_layout_t;

typedef enum {
    LV_LOG_LEVEL_TRACE,
    LV_LOG_LEVEL_INFO,
    LV_LOG_LEVEL_WARN,
    LV_LOG_LEVEL_ERROR,
    LV_LOG_LEVEL_USER,
    LV_LOG_LEVEL_NONE,
    _LV_LOG_LEVEL_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_LOG_LEVEL_DEF_LAST = 0xFFFF
    #endif
} lv_log_level_t;

typedef enum {
    LV_OPA_TRANSP,
    LV_OPA__0,
    LV_OPA__10,
    LV_OPA__20,
    LV_OPA__30,
    LV_OPA__40,
    LV_OPA__50,
    LV_OPA__60,
    LV_OPA__70,
    LV_OPA__80,
    LV_OPA__90,
    LV_OPA__100,
    LV_OPA_COVER,
    _LV_OPA_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_OPA_DEF_LAST = 0xFFFF
    #endif
} lv_opa_t;

typedef enum {
    LV_PALETTE_RED,
    LV_PALETTE_PINK,
    LV_PALETTE_PURPLE,
    LV_PALETTE_DEEP_PURPLE,
    LV_PALETTE_INDIGO,
    LV_PALETTE_BLUE,
    LV_PALETTE_LIGHT_BLUE,
    LV_PALETTE_CYAN,
    LV_PALETTE_TEAL,
    LV_PALETTE_GREEN,
    LV_PALETTE_LIGHT_GREEN,
    LV_PALETTE_LIME,
    LV_PALETTE_YELLOW,
    LV_PALETTE_AMBER,
    LV_PALETTE_ORANGE,
    LV_PALETTE_DEEP_ORANGE,
    LV_PALETTE_BROWN,
    LV_PALETTE_BLUE_GREY,
    LV_PALETTE_GREY,
    LV_PALETTE_LAST,
    LV_PALETTE_NONE,
    _LV_PALETTE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_PALETTE_DEF_LAST = 0xFFFF
    #endif
} lv_palette_t;

typedef enum {
    LV_PART_MAIN,
    LV_PART_SCROLLBAR,
    LV_PART_INDICATOR,
    LV_PART_KNOB,
    LV_PART_SELECTED,
    LV_PART_ITEMS,
    LV_PART_CURSOR,
    LV_PART_CUSTOM_FIRST,
    LV_PART_ANY,
    _LV_PART_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_PART_DEF_LAST = 0xFFFF
    #endif
} lv_part_t;

typedef enum {
    LV_PART_TEXTAREA_PLACEHOLDER,
    _LV_PART_TEXTAREA_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_PART_TEXTAREA_DEF_LAST = 0xFFFF
    #endif
} lv_part_textarea_t;

typedef enum {
    LV_RB_COLOR_RED,
    LV_RB_COLOR_BLACK,
    _LV_RB_COLOR_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_RB_COLOR_DEF_LAST = 0xFFFF
    #endif
} lv_rb_color_t;

typedef enum {
    LV_RESULT_INVALID,
    LV_RESULT_OK,
    _LV_RESULT_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_RESULT_DEF_LAST = 0xFFFF
    #endif
} lv_result_t;

typedef enum {
    LV_SCROLLBAR_MODE_OFF,
    LV_SCROLLBAR_MODE_ON,
    LV_SCROLLBAR_MODE_ACTIVE,
    LV_SCROLLBAR_MODE_AUTO,
    _LV_SCROLLBAR_MODE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SCROLLBAR_MODE_DEF_LAST = 0xFFFF
    #endif
} lv_scrollbar_mode_t;

typedef enum {
    LV_SCROLL_SNAP_NONE,
    LV_SCROLL_SNAP_START,
    LV_SCROLL_SNAP_END,
    LV_SCROLL_SNAP_CENTER,
    _LV_SCROLL_SNAP_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SCROLL_SNAP_DEF_LAST = 0xFFFF
    #endif
} lv_scroll_snap_t;

typedef enum {
    LV_SCR_LOAD_ANIM_NONE,
    LV_SCR_LOAD_ANIM_OVER_LEFT,
    LV_SCR_LOAD_ANIM_OVER_RIGHT,
    LV_SCR_LOAD_ANIM_OVER_TOP,
    LV_SCR_LOAD_ANIM_OVER_BOTTOM,
    LV_SCR_LOAD_ANIM_MOVE_LEFT,
    LV_SCR_LOAD_ANIM_MOVE_RIGHT,
    LV_SCR_LOAD_ANIM_MOVE_TOP,
    LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
    LV_SCR_LOAD_ANIM_FADE_IN,
    LV_SCR_LOAD_ANIM_FADE_ON,
    LV_SCR_LOAD_ANIM_FADE_OUT,
    LV_SCR_LOAD_ANIM_OUT_LEFT,
    LV_SCR_LOAD_ANIM_OUT_RIGHT,
    LV_SCR_LOAD_ANIM_OUT_TOP,
    LV_SCR_LOAD_ANIM_OUT_BOTTOM,
    _LV_SCR_LOAD_ANIM_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SCR_LOAD_ANIM_DEF_LAST = 0xFFFF
    #endif
} lv_scr_load_anim_t;

typedef enum {
    LV_SPAN_MODE_FIXED,
    LV_SPAN_MODE_EXPAND,
    LV_SPAN_MODE_BREAK,
    LV_SPAN_MODE_LAST,
    _LV_SPAN_MODE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SPAN_MODE_DEF_LAST = 0xFFFF
    #endif
} lv_span_mode_t;

typedef enum {
    LV_SPAN_OVERFLOW_CLIP,
    LV_SPAN_OVERFLOW_ELLIPSIS,
    LV_SPAN_OVERFLOW_LAST,
    _LV_SPAN_OVERFLOW_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SPAN_OVERFLOW_DEF_LAST = 0xFFFF
    #endif
} lv_span_overflow_t;

typedef enum {
    LV_STATE_DEFAULT,
    LV_STATE_CHECKED,
    LV_STATE_FOCUSED,
    LV_STATE_FOCUS_KEY,
    LV_STATE_EDITED,
    LV_STATE_HOVERED,
    LV_STATE_PRESSED,
    LV_STATE_SCROLLED,
    LV_STATE_DISABLED,
    LV_STATE_USER_1,
    LV_STATE_USER_2,
    LV_STATE_USER_3,
    LV_STATE_USER_4,
    LV_STATE_ANY,
    _LV_STATE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_STATE_DEF_LAST = 0xFFFF
    #endif
} lv_state_t;

typedef enum {
    LV_STR_SYMBOL_BULLET,
    LV_STR_SYMBOL_AUDIO,
    LV_STR_SYMBOL_VIDEO,
    LV_STR_SYMBOL_LIST,
    LV_STR_SYMBOL_OK,
    LV_STR_SYMBOL_CLOSE,
    LV_STR_SYMBOL_POWER,
    LV_STR_SYMBOL_SETTINGS,
    LV_STR_SYMBOL_HOME,
    LV_STR_SYMBOL_DOWNLOAD,
    LV_STR_SYMBOL_DRIVE,
    LV_STR_SYMBOL_REFRESH,
    LV_STR_SYMBOL_MUTE,
    LV_STR_SYMBOL_VOLUME_MID,
    LV_STR_SYMBOL_VOLUME_MAX,
    LV_STR_SYMBOL_IMAGE,
    LV_STR_SYMBOL_TINT,
    LV_STR_SYMBOL_PREV,
    LV_STR_SYMBOL_PLAY,
    LV_STR_SYMBOL_PAUSE,
    LV_STR_SYMBOL_STOP,
    LV_STR_SYMBOL_NEXT,
    LV_STR_SYMBOL_EJECT,
    LV_STR_SYMBOL_LEFT,
    LV_STR_SYMBOL_RIGHT,
    LV_STR_SYMBOL_PLUS,
    LV_STR_SYMBOL_MINUS,
    LV_STR_SYMBOL_EYE_OPEN,
    LV_STR_SYMBOL_EYE_CLOSE,
    LV_STR_SYMBOL_WARNING,
    LV_STR_SYMBOL_SHUFFLE,
    LV_STR_SYMBOL_UP,
    LV_STR_SYMBOL_DOWN,
    LV_STR_SYMBOL_LOOP,
    LV_STR_SYMBOL_DIRECTORY,
    LV_STR_SYMBOL_UPLOAD,
    LV_STR_SYMBOL_CALL,
    LV_STR_SYMBOL_CUT,
    LV_STR_SYMBOL_COPY,
    LV_STR_SYMBOL_SAVE,
    LV_STR_SYMBOL_BARS,
    LV_STR_SYMBOL_ENVELOPE,
    LV_STR_SYMBOL_CHARGE,
    LV_STR_SYMBOL_PASTE,
    LV_STR_SYMBOL_BELL,
    LV_STR_SYMBOL_KEYBOARD,
    LV_STR_SYMBOL_GPS,
    LV_STR_SYMBOL_FILE,
    LV_STR_SYMBOL_WIFI,
    LV_STR_SYMBOL_BATTERY_FULL,
    LV_STR_SYMBOL_BATTERY_3,
    LV_STR_SYMBOL_BATTERY_2,
    LV_STR_SYMBOL_BATTERY_1,
    LV_STR_SYMBOL_BATTERY_EMPTY,
    LV_STR_SYMBOL_USB,
    LV_STR_SYMBOL_BLUETOOTH,
    LV_STR_SYMBOL_TRASH,
    LV_STR_SYMBOL_EDIT,
    LV_STR_SYMBOL_BACKSPACE,
    LV_STR_SYMBOL_SD_CARD,
    LV_STR_SYMBOL_NEW_LINE,
    LV_STR_SYMBOL_DUMMY,
    _LV_STR_SYMBOL_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_STR_SYMBOL_DEF_LAST = 0xFFFF
    #endif
} lv_str_symbol_t;

typedef enum {
    LV_STYLE_PROP_INV,
    LV_STYLE_WIDTH,
    LV_STYLE_HEIGHT,
    LV_STYLE_LENGTH,
    LV_STYLE_MIN_WIDTH,
    LV_STYLE_MAX_WIDTH,
    LV_STYLE_MIN_HEIGHT,
    LV_STYLE_MAX_HEIGHT,
    LV_STYLE_X,
    LV_STYLE_Y,
    LV_STYLE_ALIGN,
    LV_STYLE_RADIUS,
    LV_STYLE_RADIAL_OFFSET,
    LV_STYLE_PAD_RADIAL,
    LV_STYLE_PAD_TOP,
    LV_STYLE_PAD_BOTTOM,
    LV_STYLE_PAD_LEFT,
    LV_STYLE_PAD_RIGHT,
    LV_STYLE_PAD_ROW,
    LV_STYLE_PAD_COLUMN,
    LV_STYLE_LAYOUT,
    LV_STYLE_MARGIN_TOP,
    LV_STYLE_MARGIN_BOTTOM,
    LV_STYLE_MARGIN_LEFT,
    LV_STYLE_MARGIN_RIGHT,
    LV_STYLE_BG_COLOR,
    LV_STYLE_BG_OPA,
    LV_STYLE_BG_GRAD_DIR,
    LV_STYLE_BG_MAIN_STOP,
    LV_STYLE_BG_GRAD_STOP,
    LV_STYLE_BG_GRAD_COLOR,
    LV_STYLE_BG_MAIN_OPA,
    LV_STYLE_BG_GRAD_OPA,
    LV_STYLE_BG_GRAD,
    LV_STYLE_BASE_DIR,
    LV_STYLE_BG_IMAGE_SRC,
    LV_STYLE_BG_IMAGE_OPA,
    LV_STYLE_BG_IMAGE_RECOLOR,
    LV_STYLE_BG_IMAGE_RECOLOR_OPA,
    LV_STYLE_BG_IMAGE_TILED,
    LV_STYLE_CLIP_CORNER,
    LV_STYLE_BORDER_WIDTH,
    LV_STYLE_BORDER_COLOR,
    LV_STYLE_BORDER_OPA,
    LV_STYLE_BORDER_SIDE,
    LV_STYLE_BORDER_POST,
    LV_STYLE_OUTLINE_WIDTH,
    LV_STYLE_OUTLINE_COLOR,
    LV_STYLE_OUTLINE_OPA,
    LV_STYLE_OUTLINE_PAD,
    LV_STYLE_SHADOW_WIDTH,
    LV_STYLE_SHADOW_COLOR,
    LV_STYLE_SHADOW_OPA,
    LV_STYLE_SHADOW_OFFSET_X,
    LV_STYLE_SHADOW_OFFSET_Y,
    LV_STYLE_SHADOW_SPREAD,
    LV_STYLE_IMAGE_OPA,
    LV_STYLE_IMAGE_RECOLOR,
    LV_STYLE_IMAGE_RECOLOR_OPA,
    LV_STYLE_LINE_WIDTH,
    LV_STYLE_LINE_DASH_WIDTH,
    LV_STYLE_LINE_DASH_GAP,
    LV_STYLE_LINE_ROUNDED,
    LV_STYLE_LINE_COLOR,
    LV_STYLE_LINE_OPA,
    LV_STYLE_ARC_WIDTH,
    LV_STYLE_ARC_ROUNDED,
    LV_STYLE_ARC_COLOR,
    LV_STYLE_ARC_OPA,
    LV_STYLE_ARC_IMAGE_SRC,
    LV_STYLE_TEXT_COLOR,
    LV_STYLE_TEXT_OPA,
    LV_STYLE_TEXT_FONT,
    LV_STYLE_TEXT_LETTER_SPACE,
    LV_STYLE_TEXT_LINE_SPACE,
    LV_STYLE_TEXT_DECOR,
    LV_STYLE_TEXT_ALIGN,
    LV_STYLE_TEXT_OUTLINE_STROKE_WIDTH,
    LV_STYLE_TEXT_OUTLINE_STROKE_OPA,
    LV_STYLE_TEXT_OUTLINE_STROKE_COLOR,
    LV_STYLE_OPA,
    LV_STYLE_OPA_LAYERED,
    LV_STYLE_COLOR_FILTER_DSC,
    LV_STYLE_COLOR_FILTER_OPA,
    LV_STYLE_ANIM,
    LV_STYLE_ANIM_DURATION,
    LV_STYLE_TRANSITION,
    LV_STYLE_BLEND_MODE,
    LV_STYLE_TRANSFORM_WIDTH,
    LV_STYLE_TRANSFORM_HEIGHT,
    LV_STYLE_TRANSLATE_X,
    LV_STYLE_TRANSLATE_Y,
    LV_STYLE_TRANSFORM_SCALE_X,
    LV_STYLE_TRANSFORM_SCALE_Y,
    LV_STYLE_TRANSFORM_ROTATION,
    LV_STYLE_TRANSFORM_PIVOT_X,
    LV_STYLE_TRANSFORM_PIVOT_Y,
    LV_STYLE_TRANSFORM_SKEW_X,
    LV_STYLE_TRANSFORM_SKEW_Y,
    LV_STYLE_BITMAP_MASK_SRC,
    LV_STYLE_ROTARY_SENSITIVITY,
    LV_STYLE_TRANSLATE_RADIAL,
    LV_STYLE_RECOLOR,
    LV_STYLE_RECOLOR_OPA,
    LV_STYLE_FLEX_FLOW,
    LV_STYLE_FLEX_MAIN_PLACE,
    LV_STYLE_FLEX_CROSS_PLACE,
    LV_STYLE_FLEX_TRACK_PLACE,
    LV_STYLE_FLEX_GROW,
    LV_STYLE_GRID_COLUMN_ALIGN,
    LV_STYLE_GRID_ROW_ALIGN,
    LV_STYLE_GRID_ROW_DSC_ARRAY,
    LV_STYLE_GRID_COLUMN_DSC_ARRAY,
    LV_STYLE_GRID_CELL_COLUMN_POS,
    LV_STYLE_GRID_CELL_COLUMN_SPAN,
    LV_STYLE_GRID_CELL_X_ALIGN,
    LV_STYLE_GRID_CELL_ROW_POS,
    LV_STYLE_GRID_CELL_ROW_SPAN,
    LV_STYLE_GRID_CELL_Y_ALIGN,
    LV_STYLE_LAST_BUILT_IN_PROP,
    LV_STYLE_NUM_BUILT_IN_PROPS,
    LV_STYLE_PROP_ANY,
    LV_STYLE_PROP_CONST,
    _LV_STYLE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_STYLE_DEF_LAST = 0xFFFF
    #endif
} lv_style_t;

typedef enum {
    LV_STYLE_RES_NOT_FOUND,
    LV_STYLE_RES_FOUND,
    _LV_STYLE_RES_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_STYLE_RES_DEF_LAST = 0xFFFF
    #endif
} lv_style_res_t;

typedef enum {
    LV_STYLE_STATE_CMP_SAME,
    LV_STYLE_STATE_CMP_DIFF_REDRAW,
    LV_STYLE_STATE_CMP_DIFF_DRAW_PAD,
    LV_STYLE_STATE_CMP_DIFF_LAYOUT,
    _LV_STYLE_STATE_CMP_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_STYLE_STATE_CMP_DEF_LAST = 0xFFFF
    #endif
} lv_style_state_cmp_t;

typedef enum {
    LV_SUBJECT_TYPE_INVALID,
    LV_SUBJECT_TYPE_NONE,
    LV_SUBJECT_TYPE_INT,
    LV_SUBJECT_TYPE_POINTER,
    LV_SUBJECT_TYPE_COLOR,
    LV_SUBJECT_TYPE_GROUP,
    LV_SUBJECT_TYPE_STRING,
    _LV_SUBJECT_TYPE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SUBJECT_TYPE_DEF_LAST = 0xFFFF
    #endif
} lv_subject_type_t;

typedef enum {
    LV_SYMBOL_BULLET,
    LV_SYMBOL_AUDIO,
    LV_SYMBOL_VIDEO,
    LV_SYMBOL_LIST,
    LV_SYMBOL_OK,
    LV_SYMBOL_CLOSE,
    LV_SYMBOL_POWER,
    LV_SYMBOL_SETTINGS,
    LV_SYMBOL_HOME,
    LV_SYMBOL_DOWNLOAD,
    LV_SYMBOL_DRIVE,
    LV_SYMBOL_REFRESH,
    LV_SYMBOL_MUTE,
    LV_SYMBOL_VOLUME_MID,
    LV_SYMBOL_VOLUME_MAX,
    LV_SYMBOL_IMAGE,
    LV_SYMBOL_TINT,
    LV_SYMBOL_PREV,
    LV_SYMBOL_PLAY,
    LV_SYMBOL_PAUSE,
    LV_SYMBOL_STOP,
    LV_SYMBOL_NEXT,
    LV_SYMBOL_EJECT,
    LV_SYMBOL_LEFT,
    LV_SYMBOL_RIGHT,
    LV_SYMBOL_PLUS,
    LV_SYMBOL_MINUS,
    LV_SYMBOL_EYE_OPEN,
    LV_SYMBOL_EYE_CLOSE,
    LV_SYMBOL_WARNING,
    LV_SYMBOL_SHUFFLE,
    LV_SYMBOL_UP,
    LV_SYMBOL_DOWN,
    LV_SYMBOL_LOOP,
    LV_SYMBOL_DIRECTORY,
    LV_SYMBOL_UPLOAD,
    LV_SYMBOL_CALL,
    LV_SYMBOL_CUT,
    LV_SYMBOL_COPY,
    LV_SYMBOL_SAVE,
    LV_SYMBOL_BARS,
    LV_SYMBOL_ENVELOPE,
    LV_SYMBOL_CHARGE,
    LV_SYMBOL_PASTE,
    LV_SYMBOL_BELL,
    LV_SYMBOL_KEYBOARD,
    LV_SYMBOL_GPS,
    LV_SYMBOL_FILE,
    LV_SYMBOL_WIFI,
    LV_SYMBOL_BATTERY_FULL,
    LV_SYMBOL_BATTERY_3,
    LV_SYMBOL_BATTERY_2,
    LV_SYMBOL_BATTERY_1,
    LV_SYMBOL_BATTERY_EMPTY,
    LV_SYMBOL_USB,
    LV_SYMBOL_BLUETOOTH,
    LV_SYMBOL_TRASH,
    LV_SYMBOL_EDIT,
    LV_SYMBOL_BACKSPACE,
    LV_SYMBOL_SD_CARD,
    LV_SYMBOL_NEW_LINE,
    LV_SYMBOL_DUMMY,
    _LV_SYMBOL_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_SYMBOL_DEF_LAST = 0xFFFF
    #endif
} lv_symbol_t;

typedef enum {
    LV_TEXT_ALIGN_AUTO,
    LV_TEXT_ALIGN_LEFT,
    LV_TEXT_ALIGN_CENTER,
    LV_TEXT_ALIGN_RIGHT,
    _LV_TEXT_ALIGN_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_TEXT_ALIGN_DEF_LAST = 0xFFFF
    #endif
} lv_text_align_t;

typedef enum {
    LV_TEXT_CMD_STATE_WAIT,
    LV_TEXT_CMD_STATE_PAR,
    LV_TEXT_CMD_STATE_IN,
    _LV_TEXT_CMD_STATE_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_TEXT_CMD_STATE_DEF_LAST = 0xFFFF
    #endif
} lv_text_cmd_state_t;

typedef enum {
    LV_TEXT_DECOR_NONE,
    LV_TEXT_DECOR_UNDERLINE,
    LV_TEXT_DECOR_STRIKETHROUGH,
    _LV_TEXT_DECOR_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_TEXT_DECOR_DEF_LAST = 0xFFFF
    #endif
} lv_text_decor_t;

typedef enum {
    LV_TEXT_FLAG_NONE,
    LV_TEXT_FLAG_EXPAND,
    LV_TEXT_FLAG_FIT,
    LV_TEXT_FLAG_BREAK_ALL,
    LV_TEXT_FLAG_RECOLOR,
    _LV_TEXT_FLAG_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_TEXT_FLAG_DEF_LAST = 0xFFFF
    #endif
} lv_text_flag_t;

typedef enum {
    LV_THREAD_PRIO_LOWEST,
    LV_THREAD_PRIO_LOW,
    LV_THREAD_PRIO_MID,
    LV_THREAD_PRIO_HIGH,
    LV_THREAD_PRIO_HIGHEST,
    _LV_THREAD_PRIO_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_THREAD_PRIO_DEF_LAST = 0xFFFF
    #endif
} lv_thread_prio_t;

typedef enum {
    LV_TREE_WALK_PRE_ORDER,
    LV_TREE_WALK_POST_ORDER,
    _LV_TREE_WALK_LAST #if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR < 9 
    ,
    _LV_TREE_WALK_DEF_LAST = 0xFFFF
    #endif
} lv_tree_walk_t;


// Constants
#define ANIM_PLAYTIME_INFINITE ANIM_PLAYTIME_INFINITE
#define ANIM_REPEAT_INFINITE ANIM_REPEAT_INFINITE
#define BUTTONMATRIX_BUTTON_NONE BUTTONMATRIX_BUTTON_NONE
#define CHART_POINT_NONE CHART_POINT_NONE
#define COLOR_DEPTH COLOR_DEPTH
#define DPI_DEF DPI_DEF
#define DRAW_BUF_ALIGN DRAW_BUF_ALIGN
#define DRAW_BUF_STRIDE_ALIGN DRAW_BUF_STRIDE_ALIGN
#define DROPDOWN_POS_LAST DROPDOWN_POS_LAST
#define GRID_CONTENT GRID_CONTENT
#define GRID_TEMPLATE_LAST GRID_TEMPLATE_LAST
#define IMAGE_HEADER_MAGIC IMAGE_HEADER_MAGIC
#define LABEL_DOT_NUM LABEL_DOT_NUM
#define LABEL_POS_LAST LABEL_POS_LAST
#define LABEL_TEXT_SELECTION_OFF LABEL_TEXT_SELECTION_OFF
#define RADIUS_CIRCLE RADIUS_CIRCLE
#define SCALE_LABEL_ENABLED_DEFAULT SCALE_LABEL_ENABLED_DEFAULT
#define SCALE_LABEL_ROTATE_KEEP_UPRIGHT SCALE_LABEL_ROTATE_KEEP_UPRIGHT
#define SCALE_LABEL_ROTATE_MATCH_TICKS SCALE_LABEL_ROTATE_MATCH_TICKS
#define SCALE_MAJOR_TICK_EVERY_DEFAULT SCALE_MAJOR_TICK_EVERY_DEFAULT
#define SCALE_NONE SCALE_NONE
#define SCALE_ROTATION_ANGLE_MASK SCALE_ROTATION_ANGLE_MASK
#define SCALE_TOTAL_TICK_COUNT_DEFAULT SCALE_TOTAL_TICK_COUNT_DEFAULT
#define SIZE_CONTENT SIZE_CONTENT
#define STRIDE_AUTO STRIDE_AUTO
#define TABLE_CELL_NONE TABLE_CELL_NONE
#define TEXTAREA_CURSOR_LAST TEXTAREA_CURSOR_LAST

typedef struct lv_obj_class_t lv_obj_class_t;
typedef struct lv_theme_t lv_theme_t;
// Blobs (Extern Declarations)
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * animimg_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * arc_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * bar_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * barcode_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * binfont_font_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * builtin_font_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * button_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * buttonmatrix_class;
extern const void * cache_class_lru_rb_count;
extern const void * cache_class_lru_rb_size;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * calendar_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * calendar_header_arrow_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * calendar_header_dropdown_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * canvas_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * chart_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * checkbox_class;
extern const void * color_filter_shade;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * dropdown_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * dropdownlist_class;
extern const lv_font_t * font_montserrat_14;
extern const lv_font_t * font_montserrat_16;
extern const lv_font_t * font_montserrat_24;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * gif_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * image_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * imagebutton_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * keyboard_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * label_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * led_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * line_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * list_button_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * list_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * list_text_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_cont_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_main_cont_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_main_header_cont_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_page_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_section_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_separator_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_sidebar_cont_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * menu_sidebar_header_cont_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_backdrop_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_content_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_footer_button_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_footer_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_header_button_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * msgbox_header_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * obj_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * qrcode_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * roller_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * scale_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * slider_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * spangroup_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * spinbox_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * spinner_class;
extern const void * style_const_prop_id_inv;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * switch_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * table_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * tabview_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * textarea_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * tileview_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * tileview_tile_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * tree_node_class;
typedef struct lv_obj_class_t lv_obj_class_t;
extern const lv_obj_class_t * win_class;


// Opacity (copied from example)
enum {
    LV_OPA_TRANSP = 0, OPA_TRANSP = 0,
    LV_OPA_0      = 0, OPA_0 = 0,
    LV_OPA_10     = 25, OPA_10 = 25,
    LV_OPA_20     = 51, OPA_20 = 51,
    LV_OPA_30     = 76, OPA_30 = 76,
    LV_OPA_40     = 102, OPA_40 = 102,
    LV_OPA_50     = 127, OPA_50 = 127,
    LV_OPA_60     = 153, OPA_60 = 153,
    LV_OPA_70     = 178, OPA_70 = 178,
    LV_OPA_80     = 204, OPA_80 = 204,
    LV_OPA_90     = 229, OPA_90 = 229,
    LV_OPA_100    = 255, OPA_100 = 255,
    LV_OPA_COVER  = 255, OPA_COVER = 255,
};
typedef uint8_t lv_opa_t;

// Grid Content/Last (copied)
#define LV_GRID_CONTENT        (LV_COORD_MAX - 101)
LV_EXPORT_CONST_INT(LV_GRID_CONTENT);
#define LV_GRID_TEMPLATE_LAST  (LV_COORD_MAX)
LV_EXPORT_CONST_INT(LV_GRID_TEMPLATE_LAST);
#define LV_GRID_FR(x)          (LV_COORD_MAX - 100 + (x)) // Basic definition

// --- Forward Declarations for Structs ---
// (Generated below based on lv_def.json 'structs')

// --- Enums ---
// (Generated below based on lv_def.json 'enums')

// --- Constants ---
// (Generated below based on lv_def.json 'int_constants')

// --- Blobs (Extern Declarations) ---
// (Generated below based on lv_def.json 'blobs')

// --- Emulation Library Control ---
void emul_lvgl_init(void);
void emul_lvgl_reset(void);
void emul_lvgl_deinit(void);
void emul_lvgl_register_font(lv_font_t font_ptr, const char* name);
char* emul_lvgl_get_json(lv_obj_t *root_obj);

// --- LVGL API Subset ---
// (Generated below based on lv_def.json 'objects')

// --- Helper Value Creators (Mimicking LVGL) ---
// Declare them here for user code convenience
lv_color_t lv_color_hex(uint32_t c);
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_white(void);
lv_color_t lv_color_black(void); // Added
int32_t lv_pct(int32_t v); // Creates the percentage coordinate

#endif // EMUL_LVGL_H
// --- LVGL API Subset Declarations ---

// --- Font Definitions (Placeholders - Define these in your application) ---
extern const lv_font_t lv_font_montserrat_14;
extern const lv_font_t lv_font_montserrat_18;
extern const lv_font_t lv_font_montserrat_24;

#endif // EMUL_LVGL_H
