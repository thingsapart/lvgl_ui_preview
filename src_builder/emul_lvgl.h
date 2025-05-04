#ifndef EMUL_LVGL_H
#define EMUL_LVGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t
#include "cJSON.h" // Include cJSON

// --- Core Emulation Type Definitions ---
typedef cJSON* lv_obj_t; // *** Map lv_obj_t to cJSON node ***

// Define lv_style_t as an opaque struct for type safety in C
// Its actual JSON representation is managed internally.
typedef struct _emul_lv_style_t { uint8_t _dummy; /* Needs at least one member */ } lv_style_t;

// --- Forward Declarations (for other structs if needed) ---
typedef struct lv_color_t lv_color_t;
typedef struct lv_img_dsc_t lv_img_dsc_t;

// --- Emulation Control ---
/** @brief Initializes the LVGL emulator. Call once at the beginning. */
void emul_lvgl_init(const char* output_json_path);

/** @brief Deinitializes the LVGL emulator and writes the final JSON state. */
void emul_lvgl_deinit(void);

/** @brief Registers a pointer (like a font or image) with a symbolic name for JSON output. */
void emul_lvgl_register_pointer(const void *ptr, const char *name);

// --- LVGL Type Definitions (Excluding lv_obj_t, lv_style_t) ---
typedef bool lv_anim_enable_t;
typedef int32_t lv_coord_t;
typedef char lv_freetype_font_src_t;
typedef void * lv_freetype_outline_t;
typedef intptr_t lv_intptr_t;
typedef uint8_t lv_ll_node_t;
typedef int8_t lv_log_level_t;
typedef void * lv_mem_pool_t;
typedef uint8_t lv_opa_t;
typedef uint32_t lv_part_t;
typedef uint32_t lv_prop_id_t;
typedef int8_t lv_rb_compare_res_t;
typedef uint16_t lv_state_t;
typedef uint8_t lv_style_prop_t;
typedef uint32_t lv_style_selector_t;
typedef uint8_t lv_tree_walk_mode_t;
typedef uintptr_t lv_uintptr_t;
typedef float lv_value_precise_t;

// --- LVGL Enum Definitions ---
typedef enum {
  LV_ALIGN_DEFAULT = 0x00,
  LV_ALIGN_TOP_LEFT = 0x01,
  LV_ALIGN_TOP_MID = 0x02,
  LV_ALIGN_TOP_RIGHT = 0x03,
  LV_ALIGN_BOTTOM_LEFT = 0x04,
  LV_ALIGN_BOTTOM_MID = 0x05,
  LV_ALIGN_BOTTOM_RIGHT = 0x06,
  LV_ALIGN_LEFT_MID = 0x07,
  LV_ALIGN_RIGHT_MID = 0x08,
  LV_ALIGN_CENTER = 0x09,
  LV_ALIGN_OUT_TOP_LEFT = 0x0A,
  LV_ALIGN_OUT_TOP_MID = 0x0B,
  LV_ALIGN_OUT_TOP_RIGHT = 0x0C,
  LV_ALIGN_OUT_BOTTOM_LEFT = 0x0D,
  LV_ALIGN_OUT_BOTTOM_MID = 0x0E,
  LV_ALIGN_OUT_BOTTOM_RIGHT = 0x0F,
  LV_ALIGN_OUT_LEFT_TOP = 0x10,
  LV_ALIGN_OUT_LEFT_MID = 0x11,
  LV_ALIGN_OUT_LEFT_BOTTOM = 0x12,
  LV_ALIGN_OUT_RIGHT_TOP = 0x13,
  LV_ALIGN_OUT_RIGHT_MID = 0x14,
  LV_ALIGN_OUT_RIGHT_BOTTOM = 0x15,
} lv_align_t;

typedef enum {
  LV_ANIM_IMAGE_PART_MAIN = 0x0,
} lv_animimg_part_t;

typedef enum {
  LV_ARC_MODE_NORMAL = 0x0,
  LV_ARC_MODE_SYMMETRICAL = 0x1,
  LV_ARC_MODE_REVERSE = 0x2,
} lv_arc_mode_t;

typedef enum {
  LV_BAR_MODE_NORMAL = 0x0,
  LV_BAR_MODE_SYMMETRICAL = 0x1,
  LV_BAR_MODE_RANGE = 0x2,
} lv_bar_mode_t;

typedef enum {
  LV_BAR_ORIENTATION_AUTO = 0x0,
  LV_BAR_ORIENTATION_HORIZONTAL = 0x1,
  LV_BAR_ORIENTATION_VERTICAL = 0x2,
} lv_bar_orientation_t;

typedef enum {
  LV_BASE_DIR_LTR = 0x00,
  LV_BASE_DIR_RTL = 0x01,
  LV_BASE_DIR_AUTO = 0x02,
  LV_BASE_DIR_NEUTRAL = 0x20,
  LV_BASE_DIR_WEAK = 0x21,
} lv_base_dir_t;

typedef enum {
  LV_BLEND_MODE_NORMAL = 0x0,
  LV_BLEND_MODE_ADDITIVE = 0x1,
  LV_BLEND_MODE_SUBTRACTIVE = 0x2,
  LV_BLEND_MODE_MULTIPLY = 0x3,
  LV_BLEND_MODE_DIFFERENCE = 0x4,
} lv_blend_mode_t;

typedef enum {
  LV_BORDER_SIDE_NONE = 0x00,
  LV_BORDER_SIDE_BOTTOM = 0x01,
  LV_BORDER_SIDE_TOP = 0x02,
  LV_BORDER_SIDE_LEFT = 0x04,
  LV_BORDER_SIDE_RIGHT = 0x08,
  LV_BORDER_SIDE_FULL = 0x0F,
  LV_BORDER_SIDE_INTERNAL = 0x10,
} lv_border_side_t;

typedef enum {
  LV_BUTTONMATRIX_CTRL_NONE = 0x0000,
  LV_BUTTONMATRIX_CTRL_WIDTH_1 = 0x0001,
  LV_BUTTONMATRIX_CTRL_WIDTH_2 = 0x0002,
  LV_BUTTONMATRIX_CTRL_WIDTH_3 = 0x0003,
  LV_BUTTONMATRIX_CTRL_WIDTH_4 = 0x0004,
  LV_BUTTONMATRIX_CTRL_WIDTH_5 = 0x0005,
  LV_BUTTONMATRIX_CTRL_WIDTH_6 = 0x0006,
  LV_BUTTONMATRIX_CTRL_WIDTH_7 = 0x0007,
  LV_BUTTONMATRIX_CTRL_WIDTH_8 = 0x0008,
  LV_BUTTONMATRIX_CTRL_WIDTH_9 = 0x0009,
  LV_BUTTONMATRIX_CTRL_WIDTH_10 = 0x000A,
  LV_BUTTONMATRIX_CTRL_WIDTH_11 = 0x000B,
  LV_BUTTONMATRIX_CTRL_WIDTH_12 = 0x000C,
  LV_BUTTONMATRIX_CTRL_WIDTH_13 = 0x000D,
  LV_BUTTONMATRIX_CTRL_WIDTH_14 = 0x000E,
  LV_BUTTONMATRIX_CTRL_WIDTH_15 = 0x000F,
  LV_BUTTONMATRIX_CTRL_HIDDEN = 0x0010,
  LV_BUTTONMATRIX_CTRL_NO_REPEAT = 0x0020,
  LV_BUTTONMATRIX_CTRL_DISABLED = 0x0040,
  LV_BUTTONMATRIX_CTRL_CHECKABLE = 0x0080,
  LV_BUTTONMATRIX_CTRL_CHECKED = 0x0100,
  LV_BUTTONMATRIX_CTRL_CLICK_TRIG = 0x0200,
  LV_BUTTONMATRIX_CTRL_POPOVER = 0x0400,
  LV_BUTTONMATRIX_CTRL_RECOLOR = 0x0800,
  LV_BUTTONMATRIX_CTRL_RESERVED_1 = 0x1000,
  LV_BUTTONMATRIX_CTRL_RESERVED_2 = 0x2000,
  LV_BUTTONMATRIX_CTRL_CUSTOM_1 = 0x4000,
  LV_BUTTONMATRIX_CTRL_CUSTOM_2 = 0x8000,
} lv_buttonmatrix_ctrl_t;

typedef enum {
  LV_CHART_AXIS_PRIMARY_Y = 0x0,
  LV_CHART_AXIS_SECONDARY_Y = 0x1,
  LV_CHART_AXIS_PRIMARY_X = 0x2,
  LV_CHART_AXIS_SECONDARY_X = 0x4,
  LV_CHART_AXIS_LAST = 0x5,
} lv_chart_axis_t;

typedef enum {
  LV_CHART_TYPE_NONE = 0x0,
  LV_CHART_TYPE_LINE = 0x1,
  LV_CHART_TYPE_BAR = 0x2,
  LV_CHART_TYPE_SCATTER = 0x3,
} lv_chart_type_t;

typedef enum {
  LV_CHART_UPDATE_MODE_SHIFT = 0x0,
  LV_CHART_UPDATE_MODE_CIRCULAR = 0x1,
} lv_chart_update_mode_t;

typedef enum {
  LV_COLOR_FORMAT_UNKNOWN = 0x00,
  LV_COLOR_FORMAT_RAW = 0x01,
  LV_COLOR_FORMAT_RAW_ALPHA = 0x02,
  LV_COLOR_FORMAT_L8 = 0x06,
  LV_COLOR_FORMAT_I1 = 0x07,
  LV_COLOR_FORMAT_I2 = 0x08,
  LV_COLOR_FORMAT_I4 = 0x09,
  LV_COLOR_FORMAT_I8 = 0x0A,
  LV_COLOR_FORMAT_A8 = 0x0E,
  LV_COLOR_FORMAT_RGB565 = 0x12,
  LV_COLOR_FORMAT_ARGB8565 = 0x13,
  LV_COLOR_FORMAT_RGB565A8 = 0x14,
  LV_COLOR_FORMAT_AL88 = 0x15,
  LV_COLOR_FORMAT_RGB888 = 0x0F,
  LV_COLOR_FORMAT_ARGB8888 = 0x10,
  LV_COLOR_FORMAT_XRGB8888 = 0x11,
  LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED = 0x1A,
  LV_COLOR_FORMAT_A1 = 0x0B,
  LV_COLOR_FORMAT_A2 = 0x0C,
  LV_COLOR_FORMAT_A4 = 0x0D,
  LV_COLOR_FORMAT_ARGB1555 = 0x16,
  LV_COLOR_FORMAT_ARGB4444 = 0x17,
  LV_COLOR_FORMAT_ARGB2222 = 0x18,
  LV_COLOR_FORMAT_YUV_START = 0x20,
  LV_COLOR_FORMAT_I420 = 0x20,
  LV_COLOR_FORMAT_I422 = 0x21,
  LV_COLOR_FORMAT_I444 = 0x22,
  LV_COLOR_FORMAT_I400 = 0x23,
  LV_COLOR_FORMAT_NV21 = 0x24,
  LV_COLOR_FORMAT_NV12 = 0x25,
  LV_COLOR_FORMAT_YUY2 = 0x26,
  LV_COLOR_FORMAT_UYVY = 0x27,
  LV_COLOR_FORMAT_YUV_END = 0x27,
  LV_COLOR_FORMAT_PROPRIETARY_START = 0x30,
  LV_COLOR_FORMAT_NEMA_TSC_START = 0x30,
  LV_COLOR_FORMAT_NEMA_TSC4 = 0x30,
  LV_COLOR_FORMAT_NEMA_TSC6 = 0x31,
  LV_COLOR_FORMAT_NEMA_TSC6A = 0x32,
  LV_COLOR_FORMAT_NEMA_TSC6AP = 0x33,
  LV_COLOR_FORMAT_NEMA_TSC12 = 0x34,
  LV_COLOR_FORMAT_NEMA_TSC12A = 0x35,
  LV_COLOR_FORMAT_NEMA_TSC_END = 0x35,
  LV_COLOR_FORMAT_NATIVE = 0x12,
  LV_COLOR_FORMAT_NATIVE_WITH_ALPHA = 0x14,
} lv_color_format_t;

typedef enum {
  LV_COVER_RES_COVER = 0x0,
  LV_COVER_RES_NOT_COVER = 0x1,
  LV_COVER_RES_MASKED = 0x2,
} lv_cover_res_t;

typedef enum {
  LV_DIR_NONE = 0x00,
  LV_DIR_LEFT = 0x01,
  LV_DIR_RIGHT = 0x02,
  LV_DIR_TOP = 0x04,
  LV_DIR_BOTTOM = 0x08,
  LV_DIR_HOR = 0x03,
  LV_DIR_VER = 0x0C,
  LV_DIR_ALL = 0x0F,
} lv_dir_t;

typedef enum {
  LV_DISPLAY_RENDER_MODE_PARTIAL = 0x0,
  LV_DISPLAY_RENDER_MODE_DIRECT = 0x1,
  LV_DISPLAY_RENDER_MODE_FULL = 0x2,
} lv_display_render_mode_t;

typedef enum {
  LV_DISPLAY_ROTATION_0 = 0x0,
  LV_DISPLAY_ROTATION_90 = 0x1,
  LV_DISPLAY_ROTATION_180 = 0x2,
  LV_DISPLAY_ROTATION_270 = 0x3,
} lv_display_rotation_t;

typedef enum {
  LV_DRAW_TASK_STATE_WAITING = 0x0,
  LV_DRAW_TASK_STATE_QUEUED = 0x1,
  LV_DRAW_TASK_STATE_IN_PROGRESS = 0x2,
  LV_DRAW_TASK_STATE_READY = 0x3,
} lv_draw_task_state_t;

typedef enum {
  LV_DRAW_TASK_TYPE_NONE = 0x0,
  LV_DRAW_TASK_TYPE_FILL = 0x1,
  LV_DRAW_TASK_TYPE_BORDER = 0x2,
  LV_DRAW_TASK_TYPE_BOX_SHADOW = 0x3,
  LV_DRAW_TASK_TYPE_LETTER = 0x4,
  LV_DRAW_TASK_TYPE_LABEL = 0x5,
  LV_DRAW_TASK_TYPE_IMAGE = 0x6,
  LV_DRAW_TASK_TYPE_LAYER = 0x7,
  LV_DRAW_TASK_TYPE_LINE = 0x8,
  LV_DRAW_TASK_TYPE_ARC = 0x9,
  LV_DRAW_TASK_TYPE_TRIANGLE = 0xA,
  LV_DRAW_TASK_TYPE_MASK_RECTANGLE = 0xB,
  LV_DRAW_TASK_TYPE_MASK_BITMAP = 0xC,
} lv_draw_task_type_t;

typedef enum {
  LV_EVENT_ALL = 0x00000,
  LV_EVENT_PRESSED = 0x00001,
  LV_EVENT_PRESSING = 0x00002,
  LV_EVENT_PRESS_LOST = 0x00003,
  LV_EVENT_SHORT_CLICKED = 0x00004,
  LV_EVENT_SINGLE_CLICKED = 0x00005,
  LV_EVENT_DOUBLE_CLICKED = 0x00006,
  LV_EVENT_TRIPLE_CLICKED = 0x00007,
  LV_EVENT_LONG_PRESSED = 0x00008,
  LV_EVENT_LONG_PRESSED_REPEAT = 0x00009,
  LV_EVENT_CLICKED = 0x0000A,
  LV_EVENT_RELEASED = 0x0000B,
  LV_EVENT_SCROLL_BEGIN = 0x0000C,
  LV_EVENT_SCROLL_THROW_BEGIN = 0x0000D,
  LV_EVENT_SCROLL_END = 0x0000E,
  LV_EVENT_SCROLL = 0x0000F,
  LV_EVENT_GESTURE = 0x00010,
  LV_EVENT_KEY = 0x00011,
  LV_EVENT_ROTARY = 0x00012,
  LV_EVENT_FOCUSED = 0x00013,
  LV_EVENT_DEFOCUSED = 0x00014,
  LV_EVENT_LEAVE = 0x00015,
  LV_EVENT_HIT_TEST = 0x00016,
  LV_EVENT_INDEV_RESET = 0x00017,
  LV_EVENT_HOVER_OVER = 0x00018,
  LV_EVENT_HOVER_LEAVE = 0x00019,
  LV_EVENT_COVER_CHECK = 0x0001A,
  LV_EVENT_REFR_EXT_DRAW_SIZE = 0x0001B,
  LV_EVENT_DRAW_MAIN_BEGIN = 0x0001C,
  LV_EVENT_DRAW_MAIN = 0x0001D,
  LV_EVENT_DRAW_MAIN_END = 0x0001E,
  LV_EVENT_DRAW_POST_BEGIN = 0x0001F,
  LV_EVENT_DRAW_POST = 0x00020,
  LV_EVENT_DRAW_POST_END = 0x00021,
  LV_EVENT_DRAW_TASK_ADDED = 0x00022,
  LV_EVENT_VALUE_CHANGED = 0x00023,
  LV_EVENT_INSERT = 0x00024,
  LV_EVENT_REFRESH = 0x00025,
  LV_EVENT_READY = 0x00026,
  LV_EVENT_CANCEL = 0x00027,
  LV_EVENT_CREATE = 0x00028,
  LV_EVENT_DELETE = 0x00029,
  LV_EVENT_CHILD_CHANGED = 0x0002A,
  LV_EVENT_CHILD_CREATED = 0x0002B,
  LV_EVENT_CHILD_DELETED = 0x0002C,
  LV_EVENT_SCREEN_UNLOAD_START = 0x0002D,
  LV_EVENT_SCREEN_LOAD_START = 0x0002E,
  LV_EVENT_SCREEN_LOADED = 0x0002F,
  LV_EVENT_SCREEN_UNLOADED = 0x00030,
  LV_EVENT_SIZE_CHANGED = 0x00031,
  LV_EVENT_STYLE_CHANGED = 0x00032,
  LV_EVENT_LAYOUT_CHANGED = 0x00033,
  LV_EVENT_GET_SELF_SIZE = 0x00034,
  LV_EVENT_INVALIDATE_AREA = 0x00035,
  LV_EVENT_RESOLUTION_CHANGED = 0x00036,
  LV_EVENT_COLOR_FORMAT_CHANGED = 0x00037,
  LV_EVENT_REFR_REQUEST = 0x00038,
  LV_EVENT_REFR_START = 0x00039,
  LV_EVENT_REFR_READY = 0x0003A,
  LV_EVENT_RENDER_START = 0x0003B,
  LV_EVENT_RENDER_READY = 0x0003C,
  LV_EVENT_FLUSH_START = 0x0003D,
  LV_EVENT_FLUSH_FINISH = 0x0003E,
  LV_EVENT_FLUSH_WAIT_START = 0x0003F,
  LV_EVENT_FLUSH_WAIT_FINISH = 0x00040,
  LV_EVENT_VSYNC = 0x00041,
  LV_EVENT_VSYNC_REQUEST = 0x00042,
  LV_EVENT_LAST = 0x00043,
  LV_EVENT_PREPROCESS = 0x08000,
  LV_EVENT_MARKED_DELETING = 0x10000,
} lv_event_code_t;

typedef enum {
  LV_FLEX_ALIGN_START = 0x0,
  LV_FLEX_ALIGN_END = 0x1,
  LV_FLEX_ALIGN_CENTER = 0x2,
  LV_FLEX_ALIGN_SPACE_EVENLY = 0x3,
  LV_FLEX_ALIGN_SPACE_AROUND = 0x4,
  LV_FLEX_ALIGN_SPACE_BETWEEN = 0x5,
} lv_flex_align_t;

typedef enum {
  LV_FLEX_FLOW_ROW = 0x0,
  LV_FLEX_FLOW_COLUMN = 0x1,
  LV_FLEX_FLOW_ROW_WRAP = 0x4,
  LV_FLEX_FLOW_ROW_REVERSE = 0x8,
  LV_FLEX_FLOW_ROW_WRAP_REVERSE = 0xC,
  LV_FLEX_FLOW_COLUMN_WRAP = 0x5,
  LV_FLEX_FLOW_COLUMN_REVERSE = 0x9,
  LV_FLEX_FLOW_COLUMN_WRAP_REVERSE = 0xD,
} lv_flex_flow_t;

typedef enum {
  LV_FONT_FMT_TXT_PLAIN = 0x0,
  LV_FONT_FMT_TXT_COMPRESSED = 0x1,
  LV_FONT_FMT_TXT_COMPRESSED_NO_PREFILTER = 0x2,
  LV_FONT_FMT_PLAIN_ALIGNED = 0x3,
} lv_font_fmt_txt_bitmap_format_t;

typedef enum {
  LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL = 0x0,
  LV_FONT_FMT_TXT_CMAP_SPARSE_FULL = 0x1,
  LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY = 0x2,
  LV_FONT_FMT_TXT_CMAP_SPARSE_TINY = 0x3,
} lv_font_fmt_txt_cmap_type_t;

typedef enum {
  LV_FONT_GLYPH_FORMAT_NONE = 0x000,
  LV_FONT_GLYPH_FORMAT_A1 = 0x001,
  LV_FONT_GLYPH_FORMAT_A2 = 0x002,
  LV_FONT_GLYPH_FORMAT_A3 = 0x003,
  LV_FONT_GLYPH_FORMAT_A4 = 0x004,
  LV_FONT_GLYPH_FORMAT_A8 = 0x008,
  LV_FONT_GLYPH_FORMAT_A1_ALIGNED = 0x011,
  LV_FONT_GLYPH_FORMAT_A2_ALIGNED = 0x012,
  LV_FONT_GLYPH_FORMAT_A4_ALIGNED = 0x014,
  LV_FONT_GLYPH_FORMAT_A8_ALIGNED = 0x018,
  LV_FONT_GLYPH_FORMAT_IMAGE = 0x019,
  LV_FONT_GLYPH_FORMAT_VECTOR = 0x01A,
  LV_FONT_GLYPH_FORMAT_SVG = 0x01B,
  LV_FONT_GLYPH_FORMAT_CUSTOM = 0x0FF,
} lv_font_glyph_format_t;

typedef enum {
  LV_FONT_KERNING_NORMAL = 0x0,
  LV_FONT_KERNING_NONE = 0x1,
} lv_font_kerning_t;

typedef enum {
  LV_FONT_SUBPX_NONE = 0x0,
  LV_FONT_SUBPX_HOR = 0x1,
  LV_FONT_SUBPX_VER = 0x2,
  LV_FONT_SUBPX_BOTH = 0x3,
} lv_font_subpx_t;

typedef enum {
  LV_FREETYPE_FONT_RENDER_MODE_BITMAP = 0x0,
  LV_FREETYPE_FONT_RENDER_MODE_OUTLINE = 0x1,
} lv_freetype_font_render_mode_t;

typedef enum {
  LV_FREETYPE_FONT_STYLE_NORMAL = 0x0,
  LV_FREETYPE_FONT_STYLE_ITALIC = 0x1,
  LV_FREETYPE_FONT_STYLE_BOLD = 0x2,
} lv_freetype_font_style_t;

typedef enum {
  LV_FREETYPE_OUTLINE_END = 0x0,
  LV_FREETYPE_OUTLINE_MOVE_TO = 0x1,
  LV_FREETYPE_OUTLINE_LINE_TO = 0x2,
  LV_FREETYPE_OUTLINE_CUBIC_TO = 0x3,
  LV_FREETYPE_OUTLINE_CONIC_TO = 0x4,
  LV_FREETYPE_OUTLINE_BORDER_START = 0x5,
} lv_freetype_outline_type_t;

typedef enum {
  LV_FS_MODE_WR = 0x1,
  LV_FS_MODE_RD = 0x2,
} lv_fs_mode_t;

typedef enum {
  LV_FS_RES_OK = 0x0,
  LV_FS_RES_HW_ERR = 0x1,
  LV_FS_RES_FS_ERR = 0x2,
  LV_FS_RES_NOT_EX = 0x3,
  LV_FS_RES_FULL = 0x4,
  LV_FS_RES_LOCKED = 0x5,
  LV_FS_RES_DENIED = 0x6,
  LV_FS_RES_BUSY = 0x7,
  LV_FS_RES_TOUT = 0x8,
  LV_FS_RES_NOT_IMP = 0x9,
  LV_FS_RES_OUT_OF_MEM = 0xA,
  LV_FS_RES_INV_PARAM = 0xB,
  LV_FS_RES_UNKNOWN = 0xC,
} lv_fs_res_t;

typedef enum {
  LV_FS_SEEK_SET = 0x0,
  LV_FS_SEEK_CUR = 0x1,
  LV_FS_SEEK_END = 0x2,
} lv_fs_whence_t;

typedef enum {
  LV_GRAD_DIR_NONE = 0x0,
  LV_GRAD_DIR_VER = 0x1,
  LV_GRAD_DIR_HOR = 0x2,
  LV_GRAD_DIR_LINEAR = 0x3,
  LV_GRAD_DIR_RADIAL = 0x4,
  LV_GRAD_DIR_CONICAL = 0x5,
} lv_grad_dir_t;

typedef enum {
  LV_GRAD_EXTEND_PAD = 0x0,
  LV_GRAD_EXTEND_REPEAT = 0x1,
  LV_GRAD_EXTEND_REFLECT = 0x2,
} lv_grad_extend_t;

typedef enum {
  LV_GRID_ALIGN_START = 0x0,
  LV_GRID_ALIGN_CENTER = 0x1,
  LV_GRID_ALIGN_END = 0x2,
  LV_GRID_ALIGN_STRETCH = 0x3,
  LV_GRID_ALIGN_SPACE_EVENLY = 0x4,
  LV_GRID_ALIGN_SPACE_AROUND = 0x5,
  LV_GRID_ALIGN_SPACE_BETWEEN = 0x6,
} lv_grid_align_t;

typedef enum {
  LV_GRIDNAV_CTRL_NONE = 0x0,
  LV_GRIDNAV_CTRL_ROLLOVER = 0x1,
  LV_GRIDNAV_CTRL_SCROLL_FIRST = 0x2,
  LV_GRIDNAV_CTRL_HORIZONTAL_MOVE_ONLY = 0x4,
  LV_GRIDNAV_CTRL_VERTICAL_MOVE_ONLY = 0x8,
} lv_gridnav_ctrl_t;

typedef enum {
  LV_GROUP_REFOCUS_POLICY_NEXT = 0x0,
  LV_GROUP_REFOCUS_POLICY_PREV = 0x1,
} lv_group_refocus_policy_t;

typedef enum {
  LV_IMAGE_ALIGN_DEFAULT = 0x0,
  LV_IMAGE_ALIGN_TOP_LEFT = 0x1,
  LV_IMAGE_ALIGN_TOP_MID = 0x2,
  LV_IMAGE_ALIGN_TOP_RIGHT = 0x3,
  LV_IMAGE_ALIGN_BOTTOM_LEFT = 0x4,
  LV_IMAGE_ALIGN_BOTTOM_MID = 0x5,
  LV_IMAGE_ALIGN_BOTTOM_RIGHT = 0x6,
  LV_IMAGE_ALIGN_LEFT_MID = 0x7,
  LV_IMAGE_ALIGN_RIGHT_MID = 0x8,
  LV_IMAGE_ALIGN_CENTER = 0x9,
  LV_IMAGE_ALIGN_AUTO_TRANSFORM = 0xA,
  LV_IMAGE_ALIGN_STRETCH = 0xB,
  LV_IMAGE_ALIGN_TILE = 0xC,
  LV_IMAGE_ALIGN_CONTAIN = 0xD,
  LV_IMAGE_ALIGN_COVER = 0xE,
} lv_image_align_t;

typedef enum {
  LV_IMAGE_COMPRESS_NONE = 0x0,
  LV_IMAGE_COMPRESS_RLE = 0x1,
  LV_IMAGE_COMPRESS_LZ4 = 0x2,
} lv_image_compress_t;

typedef enum {
  LV_IMAGE_SRC_VARIABLE = 0x0,
  LV_IMAGE_SRC_FILE = 0x1,
  LV_IMAGE_SRC_SYMBOL = 0x2,
  LV_IMAGE_SRC_UNKNOWN = 0x3,
} lv_image_src_t;

typedef enum {
  LV_IMAGEBUTTON_STATE_RELEASED = 0x0,
  LV_IMAGEBUTTON_STATE_PRESSED = 0x1,
  LV_IMAGEBUTTON_STATE_DISABLED = 0x2,
  LV_IMAGEBUTTON_STATE_CHECKED_RELEASED = 0x3,
  LV_IMAGEBUTTON_STATE_CHECKED_PRESSED = 0x4,
  LV_IMAGEBUTTON_STATE_CHECKED_DISABLED = 0x5,
  LV_IMAGEBUTTON_STATE_NUM = 0x6,
} lv_imagebutton_state_t;

typedef enum {
  LV_INDEV_GESTURE_NONE = 0x0,
  LV_INDEV_GESTURE_PINCH = 0x1,
  LV_INDEV_GESTURE_SWIPE = 0x2,
  LV_INDEV_GESTURE_ROTATE = 0x3,
  LV_INDEV_GESTURE_TWO_FINGERS_SWIPE = 0x4,
  LV_INDEV_GESTURE_SCROLL = 0x5,
  LV_INDEV_GESTURE_CNT = 0x6,
} lv_indev_gesture_type_t;

typedef enum {
  LV_INDEV_MODE_NONE = 0x0,
  LV_INDEV_MODE_TIMER = 0x1,
  LV_INDEV_MODE_EVENT = 0x2,
} lv_indev_mode_t;

typedef enum {
  LV_INDEV_STATE_RELEASED = 0x0,
  LV_INDEV_STATE_PRESSED = 0x1,
} lv_indev_state_t;

typedef enum {
  LV_INDEV_TYPE_NONE = 0x0,
  LV_INDEV_TYPE_POINTER = 0x1,
  LV_INDEV_TYPE_KEYPAD = 0x2,
  LV_INDEV_TYPE_BUTTON = 0x3,
  LV_INDEV_TYPE_ENCODER = 0x4,
} lv_indev_type_t;

typedef enum {
  LV_KEY_UP = 0x11,
  LV_KEY_DOWN = 0x12,
  LV_KEY_RIGHT = 0x13,
  LV_KEY_LEFT = 0x14,
  LV_KEY_ESC = 0x1B,
  LV_KEY_DEL = 0x7F,
  LV_KEY_BACKSPACE = 0x8,
  LV_KEY_ENTER = 0xA,
  LV_KEY_NEXT = 0x9,
  LV_KEY_PREV = 0xB,
  LV_KEY_HOME = 0x2,
  LV_KEY_END = 0x3,
} lv_key_t;

typedef enum {
  LV_KEYBOARD_MODE_TEXT_LOWER = 0x0,
  LV_KEYBOARD_MODE_TEXT_UPPER = 0x1,
  LV_KEYBOARD_MODE_SPECIAL = 0x2,
  LV_KEYBOARD_MODE_NUMBER = 0x3,
  LV_KEYBOARD_MODE_USER_1 = 0x4,
  LV_KEYBOARD_MODE_USER_2 = 0x5,
  LV_KEYBOARD_MODE_USER_3 = 0x6,
  LV_KEYBOARD_MODE_USER_4 = 0x7,
} lv_keyboard_mode_t;

typedef enum {
  LV_LABEL_LONG_MODE_WRAP = 0x0,
  LV_LABEL_LONG_MODE_DOTS = 0x1,
  LV_LABEL_LONG_MODE_SCROLL = 0x2,
  LV_LABEL_LONG_MODE_SCROLL_CIRCULAR = 0x3,
  LV_LABEL_LONG_MODE_CLIP = 0x4,
} lv_label_long_mode_t;

typedef enum {
  LV_LAYER_TYPE_NONE = 0x0,
  LV_LAYER_TYPE_SIMPLE = 0x1,
  LV_LAYER_TYPE_TRANSFORM = 0x2,
} lv_layer_type_t;

typedef enum {
  LV_LAYOUT_NONE = 0x0,
  LV_LAYOUT_FLEX = 0x1,
  LV_LAYOUT_GRID = 0x2,
  LV_LAYOUT_LAST = 0x3,
} lv_layout_t;

typedef enum {
  LV_MENU_HEADER_TOP_FIXED = 0x0,
  LV_MENU_HEADER_TOP_UNFIXED = 0x1,
  LV_MENU_HEADER_BOTTOM_FIXED = 0x2,
} lv_menu_mode_header_t;

typedef enum {
  LV_MENU_ROOT_BACK_BUTTON_DISABLED = 0x0,
  LV_MENU_ROOT_BACK_BUTTON_ENABLED = 0x1,
} lv_menu_mode_root_back_button_t;

typedef enum {
  LV_OBJ_CLASS_EDITABLE_INHERIT = 0x0,
  LV_OBJ_CLASS_EDITABLE_TRUE = 0x1,
  LV_OBJ_CLASS_EDITABLE_FALSE = 0x2,
} lv_obj_class_editable_t;

typedef enum {
  LV_OBJ_CLASS_GROUP_DEF_INHERIT = 0x0,
  LV_OBJ_CLASS_GROUP_DEF_TRUE = 0x1,
  LV_OBJ_CLASS_GROUP_DEF_FALSE = 0x2,
} lv_obj_class_group_def_t;

typedef enum {
  LV_OBJ_CLASS_THEME_INHERITABLE_FALSE = 0x0,
  LV_OBJ_CLASS_THEME_INHERITABLE_TRUE = 0x1,
} lv_obj_class_theme_inheritable_t;

typedef enum {
  LV_OBJ_FLAG_HIDDEN = 0x00000001,
  LV_OBJ_FLAG_CLICKABLE = 0x00000002,
  LV_OBJ_FLAG_CLICK_FOCUSABLE = 0x00000004,
  LV_OBJ_FLAG_CHECKABLE = 0x00000008,
  LV_OBJ_FLAG_SCROLLABLE = 0x00000010,
  LV_OBJ_FLAG_SCROLL_ELASTIC = 0x00000020,
  LV_OBJ_FLAG_SCROLL_MOMENTUM = 0x00000040,
  LV_OBJ_FLAG_SCROLL_ONE = 0x00000080,
  LV_OBJ_FLAG_SCROLL_CHAIN_HOR = 0x00000100,
  LV_OBJ_FLAG_SCROLL_CHAIN_VER = 0x00000200,
  LV_OBJ_FLAG_SCROLL_CHAIN = 0x00000300,
  LV_OBJ_FLAG_SCROLL_ON_FOCUS = 0x00000400,
  LV_OBJ_FLAG_SCROLL_WITH_ARROW = 0x00000800,
  LV_OBJ_FLAG_SNAPPABLE = 0x00001000,
  LV_OBJ_FLAG_PRESS_LOCK = 0x00002000,
  LV_OBJ_FLAG_EVENT_BUBBLE = 0x00004000,
  LV_OBJ_FLAG_GESTURE_BUBBLE = 0x00008000,
  LV_OBJ_FLAG_ADV_HITTEST = 0x00010000,
  LV_OBJ_FLAG_IGNORE_LAYOUT = 0x00020000,
  LV_OBJ_FLAG_FLOATING = 0x00040000,
  LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS = 0x00080000,
  LV_OBJ_FLAG_OVERFLOW_VISIBLE = 0x00100000,
  LV_OBJ_FLAG_FLEX_IN_NEW_TRACK = 0x00200000,
  LV_OBJ_FLAG_LAYOUT_1 = 0x00800000,
  LV_OBJ_FLAG_LAYOUT_2 = 0x01000000,
  LV_OBJ_FLAG_WIDGET_1 = 0x02000000,
  LV_OBJ_FLAG_WIDGET_2 = 0x04000000,
  LV_OBJ_FLAG_USER_1 = 0x08000000,
  LV_OBJ_FLAG_USER_2 = 0x10000000,
  LV_OBJ_FLAG_USER_3 = 0x20000000,
  LV_OBJ_FLAG_USER_4 = 0x40000000,
} lv_obj_flag_t;

typedef enum {
  LV_OBJ_POINT_TRANSFORM_FLAG_NONE = 0x0,
  LV_OBJ_POINT_TRANSFORM_FLAG_RECURSIVE = 0x1,
  LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE = 0x2,
  LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE_RECURSIVE = 0x3,
} lv_obj_point_transform_flag_t;

typedef enum {
  LV_OBJ_TREE_WALK_NEXT = 0x0,
  LV_OBJ_TREE_WALK_SKIP_CHILDREN = 0x1,
  LV_OBJ_TREE_WALK_END = 0x2,
} lv_obj_tree_walk_res_t;

typedef enum {
  LV_OPA_TRANSP = 0x000,
  LV_OPA_0 = 0x000,
  LV_OPA_10 = 0x019,
  LV_OPA_20 = 0x033,
  LV_OPA_30 = 0x04C,
  LV_OPA_40 = 0x066,
  LV_OPA_50 = 0x07F,
  LV_OPA_60 = 0x099,
  LV_OPA_70 = 0x0B2,
  LV_OPA_80 = 0x0CC,
  LV_OPA_90 = 0x0E5,
  LV_OPA_100 = 0x0FF,
  LV_OPA_COVER = 0x0FF,
} lv_opa_enum_t;

typedef enum {
  LV_PALETTE_RED = 0x000,
  LV_PALETTE_PINK = 0x001,
  LV_PALETTE_PURPLE = 0x002,
  LV_PALETTE_DEEP_PURPLE = 0x003,
  LV_PALETTE_INDIGO = 0x004,
  LV_PALETTE_BLUE = 0x005,
  LV_PALETTE_LIGHT_BLUE = 0x006,
  LV_PALETTE_CYAN = 0x007,
  LV_PALETTE_TEAL = 0x008,
  LV_PALETTE_GREEN = 0x009,
  LV_PALETTE_LIGHT_GREEN = 0x00A,
  LV_PALETTE_LIME = 0x00B,
  LV_PALETTE_YELLOW = 0x00C,
  LV_PALETTE_AMBER = 0x00D,
  LV_PALETTE_ORANGE = 0x00E,
  LV_PALETTE_DEEP_ORANGE = 0x00F,
  LV_PALETTE_BROWN = 0x010,
  LV_PALETTE_BLUE_GREY = 0x011,
  LV_PALETTE_GREY = 0x012,
  LV_PALETTE_LAST = 0x013,
  LV_PALETTE_NONE = 0x0FF,
} lv_palette_t;

typedef enum {
  LV_RB_COLOR_RED = 0x0,
  LV_RB_COLOR_BLACK = 0x1,
} lv_rb_color_t;

typedef enum {
  LV_RESULT_INVALID = 0x0,
  LV_RESULT_OK = 0x1,
} lv_result_t;

typedef enum {
  LV_ROLLER_MODE_NORMAL = 0x0,
  LV_ROLLER_MODE_INFINITE = 0x1,
} lv_roller_mode_t;

typedef enum {
  LV_SCALE_MODE_HORIZONTAL_TOP = 0x0,
  LV_SCALE_MODE_HORIZONTAL_BOTTOM = 0x1,
  LV_SCALE_MODE_VERTICAL_LEFT = 0x2,
  LV_SCALE_MODE_VERTICAL_RIGHT = 0x3,
  LV_SCALE_MODE_ROUND_INNER = 0x4,
  LV_SCALE_MODE_ROUND_OUTER = 0x5,
  LV_SCALE_MODE_LAST = 0x6,
} lv_scale_mode_t;

typedef enum {
  LV_SCR_LOAD_ANIM_NONE = 0x0,
  LV_SCR_LOAD_ANIM_OVER_LEFT = 0x1,
  LV_SCR_LOAD_ANIM_OVER_RIGHT = 0x2,
  LV_SCR_LOAD_ANIM_OVER_TOP = 0x3,
  LV_SCR_LOAD_ANIM_OVER_BOTTOM = 0x4,
  LV_SCR_LOAD_ANIM_MOVE_LEFT = 0x5,
  LV_SCR_LOAD_ANIM_MOVE_RIGHT = 0x6,
  LV_SCR_LOAD_ANIM_MOVE_TOP = 0x7,
  LV_SCR_LOAD_ANIM_MOVE_BOTTOM = 0x8,
  LV_SCR_LOAD_ANIM_FADE_IN = 0x9,
  LV_SCR_LOAD_ANIM_FADE_ON = 0x9,
  LV_SCR_LOAD_ANIM_FADE_OUT = 0xA,
  LV_SCR_LOAD_ANIM_OUT_LEFT = 0xB,
  LV_SCR_LOAD_ANIM_OUT_RIGHT = 0xC,
  LV_SCR_LOAD_ANIM_OUT_TOP = 0xD,
  LV_SCR_LOAD_ANIM_OUT_BOTTOM = 0xE,
} lv_screen_load_anim_t;

typedef enum {
  LV_SCROLL_SNAP_NONE = 0x0,
  LV_SCROLL_SNAP_START = 0x1,
  LV_SCROLL_SNAP_END = 0x2,
  LV_SCROLL_SNAP_CENTER = 0x3,
} lv_scroll_snap_t;

typedef enum {
  LV_SCROLLBAR_MODE_OFF = 0x0,
  LV_SCROLLBAR_MODE_ON = 0x1,
  LV_SCROLLBAR_MODE_ACTIVE = 0x2,
  LV_SCROLLBAR_MODE_AUTO = 0x3,
} lv_scrollbar_mode_t;

typedef enum {
  LV_SLIDER_MODE_NORMAL = 0x0,
  LV_SLIDER_MODE_SYMMETRICAL = 0x1,
  LV_SLIDER_MODE_RANGE = 0x2,
} lv_slider_mode_t;

typedef enum {
  LV_SLIDER_ORIENTATION_AUTO = 0x0,
  LV_SLIDER_ORIENTATION_HORIZONTAL = 0x1,
  LV_SLIDER_ORIENTATION_VERTICAL = 0x2,
} lv_slider_orientation_t;

typedef enum {
  LV_SPAN_MODE_FIXED = 0x0,
  LV_SPAN_MODE_EXPAND = 0x1,
  LV_SPAN_MODE_BREAK = 0x2,
  LV_SPAN_MODE_LAST = 0x3,
} lv_span_mode_t;

typedef enum {
  LV_SPAN_OVERFLOW_CLIP = 0x0,
  LV_SPAN_OVERFLOW_ELLIPSIS = 0x1,
  LV_SPAN_OVERFLOW_LAST = 0x2,
} lv_span_overflow_t;

typedef enum {
  LV_STATE_DEFAULT = 0x00000,
  LV_STATE_CHECKED = 0x00001,
  LV_STATE_FOCUSED = 0x00002,
  LV_STATE_FOCUS_KEY = 0x00004,
  LV_STATE_EDITED = 0x00008,
  LV_STATE_HOVERED = 0x00010,
  LV_STATE_PRESSED = 0x00020,
  LV_STATE_SCROLLED = 0x00040,
  LV_STATE_DISABLED = 0x00080,
  LV_STATE_USER_1 = 0x01000,
  LV_STATE_USER_2 = 0x02000,
  LV_STATE_USER_3 = 0x04000,
  LV_STATE_USER_4 = 0x08000,
  LV_STATE_ANY = 0x0FFFF,
} lv_state_enum_t;

typedef enum {
  LV_PART_MAIN = 0x00000,
  LV_PART_SCROLLBAR = 0x10000,
  LV_PART_INDICATOR = 0x20000,
  LV_PART_KNOB = 0x30000,
  LV_PART_SELECTED = 0x40000,
  LV_PART_ITEMS = 0x50000,
  LV_PART_CURSOR = 0x60000,
  LV_PART_CUSTOM_FIRST = 0x80000,
  LV_PART_ANY = 0xF0000,
} lv_style_parts_t;

typedef enum {
  LV_STYLE_RES_NOT_FOUND = 0x0,
  LV_STYLE_RES_FOUND = 0x1,
} lv_style_res_t;

typedef enum {
  LV_STYLE_STATE_CMP_SAME = 0x0,
  LV_STYLE_STATE_CMP_DIFF_REDRAW = 0x1,
  LV_STYLE_STATE_CMP_DIFF_DRAW_PAD = 0x2,
  LV_STYLE_STATE_CMP_DIFF_LAYOUT = 0x3,
} lv_style_state_cmp_t;

typedef enum {
  LV_SUBJECT_TYPE_INVALID = 0x0,
  LV_SUBJECT_TYPE_NONE = 0x1,
  LV_SUBJECT_TYPE_INT = 0x2,
  LV_SUBJECT_TYPE_POINTER = 0x3,
  LV_SUBJECT_TYPE_COLOR = 0x4,
  LV_SUBJECT_TYPE_GROUP = 0x5,
  LV_SUBJECT_TYPE_STRING = 0x6,
} lv_subject_type_t;

typedef enum {
  LV_SWITCH_ORIENTATION_AUTO = 0x0,
  LV_SWITCH_ORIENTATION_HORIZONTAL = 0x1,
  LV_SWITCH_ORIENTATION_VERTICAL = 0x2,
} lv_switch_orientation_t;

typedef enum {
  LV_TABLE_CELL_CTRL_NONE = 0x00,
  LV_TABLE_CELL_CTRL_MERGE_RIGHT = 0x01,
  LV_TABLE_CELL_CTRL_TEXT_CROP = 0x02,
  LV_TABLE_CELL_CTRL_CUSTOM_1 = 0x10,
  LV_TABLE_CELL_CTRL_CUSTOM_2 = 0x20,
  LV_TABLE_CELL_CTRL_CUSTOM_3 = 0x40,
  LV_TABLE_CELL_CTRL_CUSTOM_4 = 0x80,
} lv_table_cell_ctrl_t;

typedef enum {
  LV_TEXT_ALIGN_AUTO = 0x0,
  LV_TEXT_ALIGN_LEFT = 0x1,
  LV_TEXT_ALIGN_CENTER = 0x2,
  LV_TEXT_ALIGN_RIGHT = 0x3,
} lv_text_align_t;

typedef enum {
  LV_TEXT_CMD_STATE_WAIT = 0x0,
  LV_TEXT_CMD_STATE_PAR = 0x1,
  LV_TEXT_CMD_STATE_IN = 0x2,
} lv_text_cmd_state_t;

typedef enum {
  LV_TEXT_DECOR_NONE = 0x0,
  LV_TEXT_DECOR_UNDERLINE = 0x1,
  LV_TEXT_DECOR_STRIKETHROUGH = 0x2,
} lv_text_decor_t;

typedef enum {
  LV_TEXT_FLAG_NONE = 0x0,
  LV_TEXT_FLAG_EXPAND = 0x1,
  LV_TEXT_FLAG_FIT = 0x2,
  LV_TEXT_FLAG_BREAK_ALL = 0x4,
  LV_TEXT_FLAG_RECOLOR = 0x8,
} lv_text_flag_t;

// --- LVGL Struct Definitions (Excluding lv_obj_t, lv_style_t) ---
struct lv_color_t {
  uint8_t blue;
  uint8_t green;
  uint8_t red;
};

struct lv_img_dsc_t {
  lv_image_header_t header;
  uint32_t data_size;
  const uint8_t * data;
  const void * reserved;
  const void * reserved_2;
};

// --- LVGL Union Definitions ---


// --- LVGL Macro Definitions ---
#define LV_ABS(x) ((x) > 0 ? (x) : (-(x))) 
#define LV_ALIGN_UP(x,align) (((x) + ((align) - 1)) & ~((align) - 1)) 
#define LV_ANIM_OFF false 
#define LV_ANIM_ON true 
#define LV_ANIM_PAUSE_FOREVER 0xFFFFFFFF 
#define LV_ANIM_PLAYTIME_INFINITE 0xFFFFFFFF 
#define LV_ANIM_REPEAT_INFINITE 0xFFFFFFFF 
#define LV_ANIM_SET_EASE_IN_BACK(a) _PARA(a, 0.36, 0, 0.66, -0.56) 
#define LV_ANIM_SET_EASE_IN_CIRC(a) _PARA(a, 0.55, 0, 1, 0.45) 
#define LV_ANIM_SET_EASE_IN_CUBIC(a) _PARA(a, 0.32, 0, 0.67, 0) 
#define LV_ANIM_SET_EASE_IN_EXPO(a) _PARA(a, 0.7, 0, 0.84, 0) 
#define LV_ANIM_SET_EASE_IN_OUT_BACK(a) _PARA(a, 0.68, -0.6, 0.32, 1.6) 
#define LV_ANIM_SET_EASE_IN_OUT_CIRC(a) _PARA(a, 0.85, 0, 0.15, 1) 
#define LV_ANIM_SET_EASE_IN_OUT_CUBIC(a) _PARA(a, 0.65, 0, 0.35, 1) 
#define LV_ANIM_SET_EASE_IN_OUT_EXPO(a) _PARA(a, 0.87, 0, 0.13, 1) 
#define LV_ANIM_SET_EASE_IN_OUT_QUAD(a) _PARA(a, 0.45, 0, 0.55, 1) 
#define LV_ANIM_SET_EASE_IN_OUT_QUART(a) _PARA(a, 0.76, 0, 0.24, 1) 
#define LV_ANIM_SET_EASE_IN_OUT_QUINT(a) _PARA(a, 0.83, 0, 0.17, 1) 
#define LV_ANIM_SET_EASE_IN_OUT_SINE(a) _PARA(a, 0.37, 0, 0.63, 1) 
#define LV_ANIM_SET_EASE_IN_QUAD(a) _PARA(a, 0.11, 0, 0.5, 0) 
#define LV_ANIM_SET_EASE_IN_QUART(a) _PARA(a, 0.5, 0, 0.75, 0) 
#define LV_ANIM_SET_EASE_IN_QUINT(a) _PARA(a, 0.64, 0, 0.78, 0) 
#define LV_ANIM_SET_EASE_IN_SINE(a) _PARA(a, 0.12, 0, 0.39, 0) 
#define LV_ANIM_SET_EASE_OUT_BACK(a) _PARA(a, 0.34, 1.56, 0.64, 1) 
#define LV_ANIM_SET_EASE_OUT_CIRC(a) _PARA(a, 0, 0.55, 0.45, 1) 
#define LV_ANIM_SET_EASE_OUT_CUBIC(a) _PARA(a, 0.33, 1, 0.68, 1) 
#define LV_ANIM_SET_EASE_OUT_EXPO(a) _PARA(a, 0.16, 1, 0.3, 1) 
#define LV_ANIM_SET_EASE_OUT_QUAD(a) _PARA(a, 0.5, 1, 0.89, 1) 
#define LV_ANIM_SET_EASE_OUT_QUART(a) _PARA(a, 0.25, 1, 0.5, 1) 
#define LV_ANIM_SET_EASE_OUT_QUINT(a) _PARA(a, 0.22, 1, 0.36, 1) 
#define LV_ANIM_SET_EASE_OUT_SINE(a) _PARA(a, 0.61, 1, 0.88, 1) 
#define LV_ANIM_TIMELINE_PROGRESS_MAX 0xFFFF 
#define LV_ARRAY_DEFAULT_CAPACITY 4 
#define LV_ARRAY_DEFAULT_SHRINK_RATIO 2 
#define LV_ASSERT(expr) do {                                                       \
        if(!(expr)) {                                          \
            LV_LOG_ERROR("Asserted at expression: %s", #expr); \
            LV_ASSERT_HANDLER                                  \
        }                                                      \
    } while(0) 
#define LV_ASSERT_FORMAT_MSG(expr,format,...) do {                                                                                \
        if(!(expr)) {                                                                   \
            LV_LOG_ERROR("Asserted at expression: %s " format , #expr, __VA_ARGS__);    \
            LV_ASSERT_HANDLER                                                           \
        }                                                                               \
    } while(0) 
#define LV_ASSERT_FREETYPE_FONT_DSC(dsc) do {                                                                                   \
        LV_ASSERT_NULL(dsc);                                                               \
        LV_ASSERT_FORMAT_MSG(LV_FREETYPE_FONT_DSC_HAS_MAGIC_NUM(dsc),                      \
                             "Invalid font descriptor: 0x%" LV_PRIx32, (dsc)->magic_num);  \
    } while (0) 
#define LV_ASSERT_MALLOC(p) LV_ASSERT_MSG(p != NULL, "Out of memory"); 
#define LV_ASSERT_MEM_INTEGRITY
#define LV_ASSERT_MSG(expr,msg) do {                                                                 \
        if(!(expr)) {                                                    \
            LV_LOG_ERROR("Asserted at expression: %s (%s)", #expr, msg); \
            LV_ASSERT_HANDLER                                            \
        }                                                                \
    } while(0) 
#define LV_ASSERT_NULL(p) LV_ASSERT_MSG(p != NULL, "NULL pointer"); 
#define LV_ASSERT_OBJ(obj_p,obj_class) LV_ASSERT_NULL(obj_p) 
#define LV_ASSERT_STYLE(p) do{}while(0) 
#define LV_BEZIER_VAL_FLOAT(f) ((int32_t)((f) * LV_BEZIER_VAL_MAX )) 
#define LV_BEZIER_VAL_MAX (1L << LV_BEZIER_VAL_SHIFT ) 
#define LV_BEZIER_VAL_SHIFT 10 
#define LV_BIDI_LRO "\xE2\x80\xAD" /*U+202D*/ 
#define LV_BIDI_RLO "\xE2\x80\xAE" /*U+202E*/ 
#define LV_BTNMATRIX_BTN_NONE LV_BUTTONMATRIX_BUTTON_NONE 
#define LV_BTNMATRIX_CTRL_CHECKABLE LV_BUTTONMATRIX_CTRL_CHECKABLE 
#define LV_BTNMATRIX_CTRL_CHECKED LV_BUTTONMATRIX_CTRL_CHECKED 
#define LV_BTNMATRIX_CTRL_CLICK_TRIG LV_BUTTONMATRIX_CTRL_CLICK_TRIG 
#define LV_BTNMATRIX_CTRL_CUSTOM_1 LV_BUTTONMATRIX_CTRL_CUSTOM_1 
#define LV_BTNMATRIX_CTRL_CUSTOM_2 LV_BUTTONMATRIX_CTRL_CUSTOM_2 
#define LV_BTNMATRIX_CTRL_DISABLED LV_BUTTONMATRIX_CTRL_DISABLED 
#define LV_BTNMATRIX_CTRL_HIDDEN LV_BUTTONMATRIX_CTRL_HIDDEN 
#define LV_BTNMATRIX_CTRL_NO_REPEAT LV_BUTTONMATRIX_CTRL_NO_REPEAT 
#define LV_BTNMATRIX_CTRL_POPOVER LV_BUTTONMATRIX_CTRL_POPOVER 
#define LV_BUTTONMATRIX_BUTTON_NONE 0xFFFF 
#define LV_CANVAS_BUF_SIZE(w,h,bpp,stride) (((((w * bpp + 7) >> 3) + stride - 1) & ~(stride - 1)) * h + LV_DRAW_BUF_ALIGN) 
#define LV_CHART_POINT_NONE (INT32_MAX) 
#define LV_CLAMP(min,val,max) (LV_MAX(min, (LV_MIN(val, max)))) 
#define LV_COLOR_FORMAT_GET_BPP(cf) (       \
                                            (cf) == LV_COLOR_FORMAT_I1 ? 1 :        \
                                            (cf) == LV_COLOR_FORMAT_A1 ? 1 :        \
                                            (cf) == LV_COLOR_FORMAT_I2 ? 2 :        \
                                            (cf) == LV_COLOR_FORMAT_A2 ? 2 :        \
                                            (cf) == LV_COLOR_FORMAT_I4 ? 4 :        \
                                            (cf) == LV_COLOR_FORMAT_A4 ? 4 :        \
                                            (cf) == LV_COLOR_FORMAT_NEMA_TSC4 ? 4 : \
                                            (cf) == LV_COLOR_FORMAT_NEMA_TSC6 ? 6 : \
                                            (cf) == LV_COLOR_FORMAT_NEMA_TSC6A ? 6 : \
                                            (cf) == LV_COLOR_FORMAT_NEMA_TSC6AP ? 6 : \
                                            (cf) == LV_COLOR_FORMAT_L8 ? 8 :        \
                                            (cf) == LV_COLOR_FORMAT_A8 ? 8 :        \
                                            (cf) == LV_COLOR_FORMAT_I8 ? 8 :        \
                                            (cf) == LV_COLOR_FORMAT_ARGB2222 ? 8 :  \
                                            (cf) == LV_COLOR_FORMAT_NEMA_TSC12 ? 12 : \
                                            (cf) == LV_COLOR_FORMAT_NEMA_TSC12A ? 12 : \
                                            (cf) == LV_COLOR_FORMAT_AL88 ? 16 :     \
                                            (cf) == LV_COLOR_FORMAT_RGB565 ? 16 :   \
                                            (cf) == LV_COLOR_FORMAT_RGB565A8 ? 16 : \
                                            (cf) == LV_COLOR_FORMAT_YUY2 ? 16 :     \
                                            (cf) == LV_COLOR_FORMAT_ARGB1555 ? 16 : \
                                            (cf) == LV_COLOR_FORMAT_ARGB4444 ? 16 : \
                                            (cf) == LV_COLOR_FORMAT_ARGB8565 ? 24 : \
                                            (cf) == LV_COLOR_FORMAT_RGB888 ? 24 :   \
                                            (cf) == LV_COLOR_FORMAT_ARGB8888 ? 32 : \
                                            (cf) == LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED ? 32 : \
                                            (cf) == LV_COLOR_FORMAT_XRGB8888 ? 32 : \
                                            0                                       \
                                    ) 
#define LV_COLOR_FORMAT_GET_SIZE(cf) (( LV_COLOR_FORMAT_GET_BPP (cf) + 7) >> 3) 
#define LV_COLOR_FORMAT_IS_ALPHA_ONLY(cf) ((cf) >= LV_COLOR_FORMAT_A1 && (cf) <= LV_COLOR_FORMAT_A8) 
#define LV_COLOR_FORMAT_IS_INDEXED(cf) ((cf) >= LV_COLOR_FORMAT_I1 && (cf) <= LV_COLOR_FORMAT_I8) 
#define LV_COLOR_FORMAT_IS_YUV(cf) ((cf) >= LV_COLOR_FORMAT_YUV_START && (cf) <= LV_COLOR_FORMAT_YUV_END) 
#define LV_COLOR_INDEXED_PALETTE_SIZE(cf) ((cf) == LV_COLOR_FORMAT_I1 ? 2 :\
                                           (cf) == LV_COLOR_FORMAT_I2 ? 4 :\
                                           (cf) == LV_COLOR_FORMAT_I4 ? 16 :\
                                           (cf) == LV_COLOR_FORMAT_I8 ? 256 : 0) 
#define LV_COLOR_MAKE(r8,g8,b8) {b8, g8, r8} 
#define LV_COLOR_NATIVE_WITH_ALPHA_SIZE 3 
#define LV_COORD_GET_PCT(x) (LV_COORD_PLAIN(x) > LV_PCT_POS_MAX ? LV_PCT_POS_MAX - LV_COORD_PLAIN(x) : LV_COORD_PLAIN(x)) 
#define LV_COORD_IS_PCT(x) ((LV_COORD_IS_SPEC(x) && LV_COORD_PLAIN(x) <= LV_PCT_STORED_MAX)) 
#define LV_COORD_IS_PX(x) (LV_COORD_TYPE(x) == LV_COORD_TYPE_PX || LV_COORD_TYPE(x) == LV_COORD_TYPE_PX_NEG) 
#define LV_COORD_IS_SPEC(x) (LV_COORD_TYPE(x) == LV_COORD_TYPE_SPEC) 
#define LV_COORD_MAX ((1 << LV_COORD_TYPE_SHIFT) - 1) 
#define LV_COORD_MIN (- LV_COORD_MAX ) 
#define LV_COORD_PLAIN(x) ((x) & ~LV_COORD_TYPE_MASK) /*Remove type specifiers*/ 
#define LV_COORD_SET_SPEC(x) ((x) | LV_COORD_TYPE_SPEC) 
#define LV_COORD_TYPE(x) ((x) & LV_COORD_TYPE_MASK)  /*Extract type specifiers*/ 
#define LV_COORD_TYPE_MASK (3 << LV_COORD_TYPE_SHIFT) 
#define LV_COORD_TYPE_PX (0 << LV_COORD_TYPE_SHIFT) 
#define LV_COORD_TYPE_PX_NEG (3 << LV_COORD_TYPE_SHIFT) 
#define LV_COORD_TYPE_SHIFT (29U) 
#define LV_COORD_TYPE_SPEC (1 << LV_COORD_TYPE_SHIFT) 
#define LV_DISP_RENDER_MODE_DIRECT LV_DISPLAY_RENDER_MODE_DIRECT 
#define LV_DISP_RENDER_MODE_FULL LV_DISPLAY_RENDER_MODE_FULL 
#define LV_DISP_RENDER_MODE_PARTIAL LV_DISPLAY_RENDER_MODE_PARTIAL 
#define LV_DISP_ROTATION_0 LV_DISPLAY_ROTATION_0 
#define LV_DISP_ROTATION_180 LV_DISPLAY_ROTATION_180 
#define LV_DISP_ROTATION_270 LV_DISPLAY_ROTATION_270 
#define LV_DISP_ROTATION_90 LV_DISPLAY_ROTATION_90 
#define LV_DPX(n) LV_DPX_CALC ( lv_display_get_dpi (NULL), n) 
#define LV_DPX_CALC(dpi,n) ((n) == 0 ? 0 :LV_MAX((( (dpi) * (n) + 80) / 160), 1)) /*+80 for rounding*/ 
#define LV_DRAW_BUF_DEFINE LV_DRAW_BUF_DEFINE_STATIC 
#define LV_DRAW_BUF_DEFINE_STATIC(name,_w,_h,_cf) static LV_ATTRIBUTE_MEM_ALIGN uint8_t buf_##name[ LV_DRAW_BUF_SIZE (_w, _h, _cf)]; \
    static lv_draw_buf_t name = { \
                                  .header = { \
                                              .magic = LV_IMAGE_HEADER_MAGIC , \
                                              .cf = (_cf), \
                                              .flags = LV_IMAGE_FLAGS_MODIFIABLE , \
                                              .w = (_w), \
                                              .h = (_h), \
                                              .stride = LV_DRAW_BUF_STRIDE (_w, _cf), \
                                              .reserved_2 = 0, \
                                            }, \
                                  .data_size = sizeof(buf_##name), \
                                  .data = buf_##name, \
                                  .unaligned_data = buf_##name, \
                                } 
#define LV_DRAW_BUF_INIT_STATIC(name) do { \ lv_image_header_t * header = &name.header; \ lv_draw_buf_init (&name, header->w, header->h, ( lv_color_format_t )header->cf, header->stride, buf_##name, sizeof(buf_##name)); \
        lv_draw_buf_set_flag(&name, LV_IMAGE_FLAGS_MODIFIABLE ); \
    } while(0) 
#define LV_DRAW_BUF_SIZE(w,h,cf) ( LV_DRAW_BUF_STRIDE (w, cf) * (h) + LV_DRAW_BUF_ALIGN + \
     LV_COLOR_INDEXED_PALETTE_SIZE(cf) * sizeof( lv_color32_t )) 
#define LV_DRAW_BUF_STRIDE(w,cf) LV_ROUND_UP (((w) * LV_COLOR_FORMAT_GET_BPP (cf) + 7) / 8, LV_DRAW_BUF_STRIDE_ALIGN) 
#define LV_DRAW_LABEL_NO_TXT_SEL (0xFFFF) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_ARGB8888(dsc) lv_argb8888_blend_normal_to_argb8888_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_ARGB8888_MIX_MASK_OPA(dsc) lv_argb8888_blend_normal_to_argb8888_mix_mask_opa_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_ARGB8888_WITH_MASK(dsc) lv_argb8888_blend_normal_to_argb8888_with_mask_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_ARGB8888_WITH_OPA(dsc) lv_argb8888_blend_normal_to_argb8888_with_opa_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565(dsc) lv_argb8888_blend_normal_to_rgb565_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA(dsc) lv_argb8888_blend_normal_to_rgb565_mix_mask_opa_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_WITH_MASK(dsc) lv_argb8888_blend_normal_to_rgb565_with_mask_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB565_WITH_OPA(dsc) lv_argb8888_blend_normal_to_rgb565_with_opa_neon(dsc) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB888(dsc,dst_px_size) lv_argb8888_blend_normal_to_rgb888_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB888_MIX_MASK_OPA(dsc,dst_px_size) lv_argb8888_blend_normal_to_rgb888_mix_mask_opa_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB888_WITH_MASK(dsc,dst_px_size) lv_argb8888_blend_normal_to_rgb888_with_mask_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_ARGB8888_BLEND_NORMAL_TO_RGB888_WITH_OPA(dsc,dst_px_size) lv_argb8888_blend_normal_to_rgb888_with_opa_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_ASM_CUSTOM 255 
#define LV_DRAW_SW_ASM_HELIUM 2 
#define LV_DRAW_SW_ASM_NEON 1 
#define LV_DRAW_SW_ASM_NONE 0 
#define LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888(dsc) lv_color_blend_to_argb8888_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888_MIX_MASK_OPA(dsc) lv_color_blend_to_argb8888_mix_mask_opa_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888_WITH_MASK(dsc) lv_color_blend_to_argb8888_with_mask_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888_WITH_OPA(dsc) lv_color_blend_to_argb8888_with_opa_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565(dsc) lv_color_blend_to_rgb565_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565_MIX_MASK_OPA(dsc) lv_color_blend_to_rgb565_mix_mask_opa_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565_WITH_MASK(dsc) lv_color_blend_to_rgb565_with_mask_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565_WITH_OPA(dsc) lv_color_blend_to_rgb565_with_opa_neon(dsc) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB888(dsc,dst_px_size) lv_color_blend_to_rgb888_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB888_MIX_MASK_OPA(dsc,dst_px_size) lv_color_blend_to_rgb888_mix_mask_opa_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB888_WITH_MASK(dsc,dst_px_size) lv_color_blend_to_rgb888_with_mask_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB888_WITH_OPA(dsc,dst_px_size) lv_color_blend_to_rgb888_with_opa_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_ARGB8888(dsc) lv_rgb565_blend_normal_to_argb8888_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_ARGB8888_MIX_MASK_OPA(dsc) lv_rgb565_blend_normal_to_argb8888_mix_mask_opa_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_ARGB8888_WITH_MASK(dsc) lv_rgb565_blend_normal_to_argb8888_with_mask_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_ARGB8888_WITH_OPA(dsc) lv_rgb565_blend_normal_to_argb8888_with_opa_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565(dsc) lv_rgb565_blend_normal_to_rgb565_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA(dsc) lv_rgb565_blend_normal_to_rgb565_mix_mask_opa_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_WITH_MASK(dsc) lv_rgb565_blend_normal_to_rgb565_with_mask_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565_WITH_OPA(dsc) lv_rgb565_blend_normal_to_rgb565_with_opa_neon(dsc) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB888(dsc,dst_px_size) lv_rgb565_blend_normal_to_rgb888_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB888_MIX_MASK_OPA(dsc,dst_px_size) lv_rgb565_blend_normal_to_rgb888_mix_mask_opa_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB888_WITH_MASK(dsc,dst_px_size) lv_rgb565_blend_normal_to_rgb888_with_mask_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB888_WITH_OPA(dsc,dst_px_size) lv_rgb565_blend_normal_to_rgb888_with_opa_neon(dsc, dst_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_ARGB8888(dsc,src_px_size) lv_rgb888_blend_normal_to_argb8888_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_ARGB8888_MIX_MASK_OPA(dsc,src_px_size) lv_rgb888_blend_normal_to_argb8888_mix_mask_opa_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_ARGB8888_WITH_MASK(dsc,src_px_size) lv_rgb888_blend_normal_to_argb8888_with_mask_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_ARGB8888_WITH_OPA(dsc,src_px_size) lv_rgb888_blend_normal_to_argb8888_with_opa_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565(dsc,src_px_size) lv_rgb888_blend_normal_to_rgb565_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_MIX_MASK_OPA(dsc,src_px_size) lv_rgb888_blend_normal_to_rgb565_mix_mask_opa_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_WITH_MASK(dsc,src_px_size) lv_rgb888_blend_normal_to_rgb565_with_mask_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB565_WITH_OPA(dsc,src_px_size) lv_rgb888_blend_normal_to_rgb565_with_opa_neon(dsc, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB888(dsc,dst_px_size,src_px_size) lv_rgb888_blend_normal_to_rgb888_neon(dsc, dst_px_size, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB888_MIX_MASK_OPA(dsc,dst_px_size,src_px_size) lv_rgb888_blend_normal_to_rgb888_mix_mask_opa_neon(dsc, dst_px_size, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB888_WITH_MASK(dsc,dst_px_size,src_px_size) lv_rgb888_blend_normal_to_rgb888_with_mask_neon(dsc, dst_px_size, src_px_size) 
#define LV_DRAW_SW_RGB888_BLEND_NORMAL_TO_RGB888_WITH_OPA(dsc,dst_px_size,src_px_size) lv_rgb888_blend_normal_to_rgb888_with_opa_neon(dsc, dst_px_size, src_px_size) 
#define LV_DRAW_UNIT_IDLE -1 
#define LV_DRAW_UNIT_NONE 0 
#define LV_DROPDOWN_POS_LAST 0xFFFF 
#define LV_FLEX_COLUMN (1 << 0) 
#define LV_FLEX_REVERSE (1 << 3) 
#define LV_FLEX_WRAP (1 << 2) 
#define LV_FONT_DECLARE(font_name) LV_ATTRIBUTE_EXTERN_DATA extern const lv_font_t font_name; 
#define LV_FREETYPE_F26DOT6_TO_FLOAT(x) ((float)(x) / 64) 
#define LV_FREETYPE_F26DOT6_TO_INT(x) ((x) >> 6) 
#define LV_FREETYPE_FONT_DSC_HAS_MAGIC_NUM(dsc) ((dsc)->magic_num == LV_FREETYPE_FONT_DSC_MAGIC_NUM) 
#define LV_FREETYPE_FONT_DSC_MAGIC_NUM 0x5F5F4654 /* '__FT' */ 
#define LV_FS_CACHE_FROM_BUFFER UINT32_MAX 
#define LV_FS_IS_VALID_LETTER(l) ((l) == '/' || ((l) >= 'A' && (l) <= 'Z')) 
#define LV_FS_MAX_FN_LENGTH 64 
#define LV_FS_MAX_PATH_LEN 256 
#define LV_FS_MAX_PATH_LENGTH 256 
#define LV_GLOBAL_DEFAULT (&lv_global) 
#define LV_GRAD_BOTTOM LV_PCT(100) 
#define LV_GRAD_CENTER LV_PCT(50) 
#define LV_GRAD_LEFT LV_PCT(0) 
#define LV_GRAD_RIGHT LV_PCT(100) 
#define LV_GRAD_TOP LV_PCT(0) 
#define LV_GRID_CONTENT ( LV_COORD_MAX - 101) 
#define LV_GRID_FR(x) ( LV_COORD_MAX - 100 + x) 
#define LV_GRID_TEMPLATE_LAST ( LV_COORD_MAX ) 
#define LV_HOR_RES lv_display_get_horizontal_resolution ( lv_display_get_default ()) 
#define LV_IMAGE_DECLARE(var_name) extern const lv_image_dsc_t var_name 
#define LV_IMAGE_HEADER_MAGIC (0x19) 
#define LV_IMGBTN_STATE_CHECKED_DISABLED LV_IMAGEBUTTON_STATE_CHECKED_DISABLED 
#define LV_IMGBTN_STATE_CHECKED_PRESSED LV_IMAGEBUTTON_STATE_CHECKED_PRESSED 
#define LV_IMGBTN_STATE_CHECKED_RELEASED LV_IMAGEBUTTON_STATE_CHECKED_RELEASED 
#define LV_IMGBTN_STATE_DISABLED LV_IMAGEBUTTON_STATE_DISABLED 
#define LV_IMGBTN_STATE_PRESSED LV_IMAGEBUTTON_STATE_PRESSED 
#define LV_IMGBTN_STATE_RELEASED LV_IMAGEBUTTON_STATE_RELEASED 
#define LV_IMG_DECLARE(var_name) extern const lv_image_dsc_t var_name; 
#define LV_INDEV_STATE_PR LV_INDEV_STATE_PRESSED 
#define LV_INDEV_STATE_REL LV_INDEV_STATE_RELEASED 
#define LV_INV_BUF_SIZE 32 
#define LV_IS_SIGNED(t) (((t)(-1)) < ((t)0)) 
#define LV_KEYBOARD_CTRL_BUTTON_FLAGS ( LV_BUTTONMATRIX_CTRL_NO_REPEAT | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | LV_BUTTONMATRIX_CTRL_CHECKED ) 
#define LV_LABEL_DEFAULT_TEXT "Text" 
#define LV_LABEL_DOT_NUM 3 
#define LV_LABEL_LONG_CLIP LV_LABEL_LONG_MODE_CLIP 
#define LV_LABEL_LONG_DOT LV_LABEL_LONG_MODE_DOTS 
#define LV_LABEL_LONG_SCROLL LV_LABEL_LONG_MODE_SCROLL 
#define LV_LABEL_LONG_SCROLL_CIRCULAR LV_LABEL_LONG_MODE_SCROLL_CIRCULAR 
#define LV_LABEL_LONG_WRAP LV_LABEL_LONG_MODE_WRAP 
#define LV_LABEL_POS_LAST 0xFFFF 
#define LV_LABEL_TEXT_SELECTION_OFF LV_DRAW_LABEL_NO_TXT_SEL 
#define LV_LED_BRIGHT_MAX 255 
#define LV_LED_BRIGHT_MIN 80 
#define LV_LL_READ(list,i) for(i = lv_ll_get_head (list); i != NULL; i = lv_ll_get_next (list, i)) 
#define LV_LL_READ_BACK(list,i) for(i = lv_ll_get_tail (list); i != NULL; i = lv_ll_get_prev (list, i)) 
#define LV_LOG(...) lv_log (__VA_ARGS__) 
#define LV_LOG_ERROR(...) lv_log_add ( LV_LOG_LEVEL_ERROR , LV_LOG_FILE, LV_LOG_LINE, __func__, __VA_ARGS__) 
#define LV_LOG_FILE __FILE__ 
#define LV_LOG_INFO(...) do {}while(0) 
#define LV_LOG_LEVEL_ERROR 3 
#define LV_LOG_LEVEL_INFO 1 
#define LV_LOG_LEVEL_NONE 5 
#define LV_LOG_LEVEL_NUM 6 
#define LV_LOG_LEVEL_TRACE 0 
#define LV_LOG_LEVEL_USER 4 
#define LV_LOG_LEVEL_WARN 2 
#define LV_LOG_LINE __LINE__ 
#define LV_LOG_TRACE(...) do {}while(0) 
#define LV_LOG_USER(...) lv_log_add ( LV_LOG_LEVEL_USER , LV_LOG_FILE, LV_LOG_LINE, __func__, __VA_ARGS__) 
#define LV_LOG_WARN(...) lv_log_add ( LV_LOG_LEVEL_WARN , LV_LOG_FILE, LV_LOG_LINE, __func__, __VA_ARGS__) 
#define LV_MASK_ID_INV (-1) 
#define LV_MASK_MAX_NUM 16 
#define LV_MAX(a,b) ((a) > (b) ? (a) : (b)) 
#define LV_MAX3(a,b,c) (LV_MAX(LV_MAX(a,b), c)) 
#define LV_MAX4(a,b,c,d) (LV_MAX(LV_MAX(a,b), LV_MAX(c,d))) 
#define LV_MAX_OF(t) ((unsigned long)(LV_IS_SIGNED(t) ? LV_SMAX_OF(t) : LV_UMAX_OF(t))) 
#define LV_MIN(a,b) ((a) < (b) ? (a) : (b)) 
#define LV_MIN3(a,b,c) (LV_MIN(LV_MIN(a,b), c)) 
#define LV_MIN4(a,b,c,d) (LV_MIN(LV_MIN(a,b), LV_MIN(c,d))) 
#define LV_NEMA_HAL_CUSTOM 0 
#define LV_NEMA_HAL_STM32 1 
#define LV_NO_TIMER_READY 0xFFFFFFFF 
#define LV_OPA_MAX 253 
#define LV_OPA_MIN 2 
#define LV_OPA_MIX2(a1,a2) ((lv_opa_t)(((int32_t)(a1) * (a2)) >> 8)) 
#define LV_OPA_MIX3(a1,a2,a3) ((lv_opa_t)(((int32_t)(a1) * (a2) * (a3)) >> 16)) 
#define LV_OS_CMSIS_RTOS2 3 
#define LV_OS_CUSTOM 255 
#define LV_OS_FREERTOS 2 
#define LV_OS_MQX 6 
#define LV_OS_NONE 0 
#define LV_OS_PTHREAD 1 
#define LV_OS_RTTHREAD 4 
#define LV_OS_SDL2 7 
#define LV_OS_WINDOWS 5 
#define LV_PCT(x) (LV_COORD_SET_SPEC(((x) < 0 ? (LV_PCT_POS_MAX - LV_MAX((x), -LV_PCT_POS_MAX)) : LV_MIN((x), LV_PCT_POS_MAX)))) 
#define LV_PCT_POS_MAX (LV_PCT_STORED_MAX / 2) 
#define LV_PCT_STORED_MAX ( LV_COORD_MAX - 1) 
#define LV_PRIX32 "X" 
#define LV_PRIX64 "llX" 
#define LV_PRId32 "d" 
#define LV_PRId64 "lld" 
#define LV_PRIu32 "u" 
#define LV_PRIu64 "llu" 
#define LV_PRIx32 "x" 
#define LV_PRIx64 "llx" 
#define LV_PROC_STAT_PARAMS_LEN 7 
#define LV_PROFILER_BEGIN
#define LV_PROFILER_BEGIN_TAG(tag) LV_UNUSED(tag) 
#define LV_PROFILER_CACHE_BEGIN
#define LV_PROFILER_CACHE_BEGIN_TAG(tag)
#define LV_PROFILER_CACHE_END
#define LV_PROFILER_CACHE_END_TAG(tag)
#define LV_PROFILER_DECODER_BEGIN
#define LV_PROFILER_DECODER_BEGIN_TAG(tag)
#define LV_PROFILER_DECODER_END
#define LV_PROFILER_DECODER_END_TAG(tag)
#define LV_PROFILER_DRAW_BEGIN
#define LV_PROFILER_DRAW_BEGIN_TAG(tag)
#define LV_PROFILER_DRAW_END
#define LV_PROFILER_DRAW_END_TAG(tag)
#define LV_PROFILER_END
#define LV_PROFILER_END_TAG(tag) LV_UNUSED(tag) 
#define LV_PROFILER_EVENT_BEGIN
#define LV_PROFILER_EVENT_BEGIN_TAG(tag)
#define LV_PROFILER_EVENT_END
#define LV_PROFILER_EVENT_END_TAG(tag)
#define LV_PROFILER_FONT_BEGIN
#define LV_PROFILER_FONT_BEGIN_TAG(tag)
#define LV_PROFILER_FONT_END
#define LV_PROFILER_FONT_END_TAG(tag)
#define LV_PROFILER_FS_BEGIN
#define LV_PROFILER_FS_BEGIN_TAG(tag)
#define LV_PROFILER_FS_END
#define LV_PROFILER_FS_END_TAG(tag)
#define LV_PROFILER_INDEV_BEGIN
#define LV_PROFILER_INDEV_BEGIN_TAG(tag)
#define LV_PROFILER_INDEV_END
#define LV_PROFILER_INDEV_END_TAG(tag)
#define LV_PROFILER_LAYOUT_BEGIN
#define LV_PROFILER_LAYOUT_BEGIN_TAG(tag)
#define LV_PROFILER_LAYOUT_END
#define LV_PROFILER_LAYOUT_END_TAG(tag)
#define LV_PROFILER_REFR_BEGIN
#define LV_PROFILER_REFR_BEGIN_TAG(tag)
#define LV_PROFILER_REFR_END
#define LV_PROFILER_REFR_END_TAG(tag)
#define LV_PROFILER_STYLE_BEGIN
#define LV_PROFILER_STYLE_BEGIN_TAG(tag)
#define LV_PROFILER_STYLE_END
#define LV_PROFILER_STYLE_END_TAG(tag)
#define LV_PROFILER_TIMER_BEGIN
#define LV_PROFILER_TIMER_BEGIN_TAG(tag)
#define LV_PROFILER_TIMER_END
#define LV_PROFILER_TIMER_END_TAG(tag)
#define LV_RADIUS_CIRCLE 0x7FFF 
#define LV_RES_INV LV_RESULT_INVALID 
#define LV_RES_OK LV_RESULT_OK 
#define LV_ROUND_UP(x,round) ((((x) + ((round) - 1)) / (round)) * (round)) 
#define LV_SCALE_LABEL_ENABLED_DEFAULT (1U) 
#define LV_SCALE_LABEL_ROTATE_KEEP_UPRIGHT 0x80000 
#define LV_SCALE_LABEL_ROTATE_MATCH_TICKS 0x100000 
#define LV_SCALE_MAJOR_TICK_EVERY_DEFAULT (5U) 
#define LV_SCALE_NONE 256 
#define LV_SCALE_ROTATION_ANGLE_MASK 0x7FFFF 
#define LV_SCALE_TOTAL_TICK_COUNT_DEFAULT (11U) 
#define LV_SIZE_CONTENT LV_COORD_SET_SPEC( LV_COORD_MAX ) 
#define LV_SMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0x7ULL << ((sizeof(t) * 8ULL) - 4ULL))) 
#define LV_SPINBOX_MAX_DIGIT_COUNT 10 
#define LV_STDLIB_BUILTIN 0 
#define LV_STDLIB_CLIB 1 
#define LV_STDLIB_CUSTOM 255 
#define LV_STDLIB_MICROPYTHON 2 
#define LV_STDLIB_RTTHREAD 3 
#define LV_STRIDE_AUTO 0 
#define LV_STYLE_ANIM_TIME LV_STYLE_ANIM_DURATION 
#define LV_STYLE_CONST_ALIGN(val) { \
        .prop = LV_STYLE_ALIGN, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_ANIM(val) { \
        .prop = LV_STYLE_ANIM, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_ANIM_DURATION(val) { \
        .prop = LV_STYLE_ANIM_DURATION, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_ARC_COLOR(val) { \
        .prop = LV_STYLE_ARC_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_ARC_IMAGE_SRC(val) { \
        .prop = LV_STYLE_ARC_IMAGE_SRC, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_ARC_OPA(val) { \
        .prop = LV_STYLE_ARC_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_ARC_ROUNDED(val) { \
        .prop = LV_STYLE_ARC_ROUNDED, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_ARC_WIDTH(val) { \
        .prop = LV_STYLE_ARC_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BASE_DIR(val) { \
        .prop = LV_STYLE_BASE_DIR, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_COLOR(val) { \
        .prop = LV_STYLE_BG_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_BG_GRAD(val) { \
        .prop = LV_STYLE_BG_GRAD, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_BG_GRAD_COLOR(val) { \
        .prop = LV_STYLE_BG_GRAD_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_BG_GRAD_DIR(val) { \
        .prop = LV_STYLE_BG_GRAD_DIR, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_GRAD_OPA(val) { \
        .prop = LV_STYLE_BG_GRAD_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_GRAD_STOP(val) { \
        .prop = LV_STYLE_BG_GRAD_STOP, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_IMAGE_OPA(val) { \
        .prop = LV_STYLE_BG_IMAGE_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_IMAGE_RECOLOR(val) { \
        .prop = LV_STYLE_BG_IMAGE_RECOLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_BG_IMAGE_RECOLOR_OPA(val) { \
        .prop = LV_STYLE_BG_IMAGE_RECOLOR_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_IMAGE_SRC(val) { \
        .prop = LV_STYLE_BG_IMAGE_SRC, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_BG_IMAGE_TILED(val) { \
        .prop = LV_STYLE_BG_IMAGE_TILED, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_MAIN_OPA(val) { \
        .prop = LV_STYLE_BG_MAIN_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_MAIN_STOP(val) { \
        .prop = LV_STYLE_BG_MAIN_STOP, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BG_OPA(val) { \
        .prop = LV_STYLE_BG_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BITMAP_MASK_SRC(val) { \
        .prop = LV_STYLE_BITMAP_MASK_SRC, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_BLEND_MODE(val) { \
        .prop = LV_STYLE_BLEND_MODE, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BORDER_COLOR(val) { \
        .prop = LV_STYLE_BORDER_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_BORDER_OPA(val) { \
        .prop = LV_STYLE_BORDER_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BORDER_POST(val) { \
        .prop = LV_STYLE_BORDER_POST, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BORDER_SIDE(val) { \
        .prop = LV_STYLE_BORDER_SIDE, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_BORDER_WIDTH(val) { \
        .prop = LV_STYLE_BORDER_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_CLIP_CORNER(val) { \
        .prop = LV_STYLE_CLIP_CORNER, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_COLOR_FILTER_DSC(val) { \
        .prop = LV_STYLE_COLOR_FILTER_DSC, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_COLOR_FILTER_OPA(val) { \
        .prop = LV_STYLE_COLOR_FILTER_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_HEIGHT(val) { \
        .prop = LV_STYLE_HEIGHT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_IMAGE_OPA(val) { \
        .prop = LV_STYLE_IMAGE_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_IMAGE_RECOLOR(val) { \
        .prop = LV_STYLE_IMAGE_RECOLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_IMAGE_RECOLOR_OPA(val) { \
        .prop = LV_STYLE_IMAGE_RECOLOR_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_INIT(var_name,prop_array) const lv_style_t var_name = {                                       \
        .values_and_props = (void*)prop_array,                          \
        .has_group = 0xFFFFFFFF,                                        \
        .prop_cnt = 255,                                                \
    } 
#define LV_STYLE_CONST_LAYOUT(val) { \
        .prop = LV_STYLE_LAYOUT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_LENGTH(val) { \
        .prop = LV_STYLE_LENGTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_LINE_COLOR(val) { \
        .prop = LV_STYLE_LINE_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_LINE_DASH_GAP(val) { \
        .prop = LV_STYLE_LINE_DASH_GAP, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_LINE_DASH_WIDTH(val) { \
        .prop = LV_STYLE_LINE_DASH_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_LINE_OPA(val) { \
        .prop = LV_STYLE_LINE_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_LINE_ROUNDED(val) { \
        .prop = LV_STYLE_LINE_ROUNDED, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_LINE_WIDTH(val) { \
        .prop = LV_STYLE_LINE_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MARGIN_BOTTOM(val) { \
        .prop = LV_STYLE_MARGIN_BOTTOM, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MARGIN_LEFT(val) { \
        .prop = LV_STYLE_MARGIN_LEFT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MARGIN_RIGHT(val) { \
        .prop = LV_STYLE_MARGIN_RIGHT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MARGIN_TOP(val) { \
        .prop = LV_STYLE_MARGIN_TOP, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MAX_HEIGHT(val) { \
        .prop = LV_STYLE_MAX_HEIGHT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MAX_WIDTH(val) { \
        .prop = LV_STYLE_MAX_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MIN_HEIGHT(val) { \
        .prop = LV_STYLE_MIN_HEIGHT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_MIN_WIDTH(val) { \
        .prop = LV_STYLE_MIN_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_OPA(val) { \
        .prop = LV_STYLE_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_OPA_LAYERED(val) { \
        .prop = LV_STYLE_OPA_LAYERED, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_OUTLINE_COLOR(val) { \
        .prop = LV_STYLE_OUTLINE_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_OUTLINE_OPA(val) { \
        .prop = LV_STYLE_OUTLINE_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_OUTLINE_PAD(val) { \
        .prop = LV_STYLE_OUTLINE_PAD, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_OUTLINE_WIDTH(val) { \
        .prop = LV_STYLE_OUTLINE_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_BOTTOM(val) { \
        .prop = LV_STYLE_PAD_BOTTOM, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_COLUMN(val) { \
        .prop = LV_STYLE_PAD_COLUMN, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_LEFT(val) { \
        .prop = LV_STYLE_PAD_LEFT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_RADIAL(val) { \
        .prop = LV_STYLE_PAD_RADIAL, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_RIGHT(val) { \
        .prop = LV_STYLE_PAD_RIGHT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_ROW(val) { \
        .prop = LV_STYLE_PAD_ROW, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PAD_TOP(val) { \
        .prop = LV_STYLE_PAD_TOP, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_PROPS_END { .prop = LV_STYLE_PROP_INV, .value = { .num = 0 } } 
#define LV_STYLE_CONST_RADIAL_OFFSET(val) { \
        .prop = LV_STYLE_RADIAL_OFFSET, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_RADIUS(val) { \
        .prop = LV_STYLE_RADIUS, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_RECOLOR(val) { \
        .prop = LV_STYLE_RECOLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_RECOLOR_OPA(val) { \
        .prop = LV_STYLE_RECOLOR_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_ROTARY_SENSITIVITY(val) { \
        .prop = LV_STYLE_ROTARY_SENSITIVITY, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_SHADOW_COLOR(val) { \
        .prop = LV_STYLE_SHADOW_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_SHADOW_OFFSET_X(val) { \
        .prop = LV_STYLE_SHADOW_OFFSET_X, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_SHADOW_OFFSET_Y(val) { \
        .prop = LV_STYLE_SHADOW_OFFSET_Y, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_SHADOW_OPA(val) { \
        .prop = LV_STYLE_SHADOW_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_SHADOW_SPREAD(val) { \
        .prop = LV_STYLE_SHADOW_SPREAD, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_SHADOW_WIDTH(val) { \
        .prop = LV_STYLE_SHADOW_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_ALIGN(val) { \
        .prop = LV_STYLE_TEXT_ALIGN, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_COLOR(val) { \
        .prop = LV_STYLE_TEXT_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_TEXT_DECOR(val) { \
        .prop = LV_STYLE_TEXT_DECOR, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_FONT(val) { \
        .prop = LV_STYLE_TEXT_FONT, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_TEXT_LETTER_SPACE(val) { \
        .prop = LV_STYLE_TEXT_LETTER_SPACE, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_LINE_SPACE(val) { \
        .prop = LV_STYLE_TEXT_LINE_SPACE, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_OPA(val) { \
        .prop = LV_STYLE_TEXT_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_OUTLINE_STROKE_COLOR(val) { \
        .prop = LV_STYLE_TEXT_OUTLINE_STROKE_COLOR, .value = { .color = val } \
    } 
#define LV_STYLE_CONST_TEXT_OUTLINE_STROKE_OPA(val) { \
        .prop = LV_STYLE_TEXT_OUTLINE_STROKE_OPA, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TEXT_OUTLINE_STROKE_WIDTH(val) { \
        .prop = LV_STYLE_TEXT_OUTLINE_STROKE_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_HEIGHT(val) { \
        .prop = LV_STYLE_TRANSFORM_HEIGHT, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_PIVOT_X(val) { \
        .prop = LV_STYLE_TRANSFORM_PIVOT_X, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_PIVOT_Y(val) { \
        .prop = LV_STYLE_TRANSFORM_PIVOT_Y, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_ROTATION(val) { \
        .prop = LV_STYLE_TRANSFORM_ROTATION, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_SCALE_X(val) { \
        .prop = LV_STYLE_TRANSFORM_SCALE_X, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_SCALE_Y(val) { \
        .prop = LV_STYLE_TRANSFORM_SCALE_Y, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_SKEW_X(val) { \
        .prop = LV_STYLE_TRANSFORM_SKEW_X, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_SKEW_Y(val) { \
        .prop = LV_STYLE_TRANSFORM_SKEW_Y, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSFORM_WIDTH(val) { \
        .prop = LV_STYLE_TRANSFORM_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSITION(val) { \
        .prop = LV_STYLE_TRANSITION, .value = { .ptr = val } \
    } 
#define LV_STYLE_CONST_TRANSLATE_RADIAL(val) { \
        .prop = LV_STYLE_TRANSLATE_RADIAL, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSLATE_X(val) { \
        .prop = LV_STYLE_TRANSLATE_X, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_TRANSLATE_Y(val) { \
        .prop = LV_STYLE_TRANSLATE_Y, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_WIDTH(val) { \
        .prop = LV_STYLE_WIDTH, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_X(val) { \
        .prop = LV_STYLE_X, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_CONST_Y(val) { \
        .prop = LV_STYLE_Y, .value = { .num = (int32_t)val } \
    } 
#define LV_STYLE_IMG_OPA LV_STYLE_IMAGE_OPA 
#define LV_STYLE_IMG_RECOLOR LV_STYLE_IMAGE_RECOLOR 
#define LV_STYLE_IMG_RECOLOR_OPA LV_STYLE_IMAGE_RECOLOR_OPA 
#define LV_STYLE_PROP_FLAG_ALL (0x3F) 
#define LV_STYLE_PROP_FLAG_EXT_DRAW_UPDATE (1 << 1) 
#define LV_STYLE_PROP_FLAG_INHERITABLE (1 << 0) 
#define LV_STYLE_PROP_FLAG_LAYER_UPDATE (1 << 4) 
#define LV_STYLE_PROP_FLAG_LAYOUT_UPDATE (1 << 2) 
#define LV_STYLE_PROP_FLAG_NONE (0) 
#define LV_STYLE_PROP_FLAG_PARENT_LAYOUT_UPDATE (1 << 3) 
#define LV_STYLE_PROP_FLAG_TRANSFORM (1 << 5) 
#define LV_STYLE_SENTINEL_VALUE 0xAABBCCDD 
#define LV_STYLE_SHADOW_OFS_X LV_STYLE_SHADOW_OFFSET_X 
#define LV_STYLE_SHADOW_OFS_Y LV_STYLE_SHADOW_OFFSET_Y 
#define LV_STYLE_TRANSFORM_ANGLE LV_STYLE_TRANSFORM_ROTATION 
#define LV_SWITCH_KNOB_EXT_AREA_CORRECTION 2 
#define LV_SYMBOL_AUDIO "\xEF\x80\x81" /*61441, 0xF001*/ 
#define LV_SYMBOL_BACKSPACE "\xEF\x95\x9A" /*62810, 0xF55A*/ 
#define LV_SYMBOL_BARS "\xEF\x83\x89" /*61641, 0xF0C9*/ 
#define LV_SYMBOL_BATTERY_1 "\xEF\x89\x83" /*62019, 0xF243*/ 
#define LV_SYMBOL_BATTERY_2 "\xEF\x89\x82" /*62018, 0xF242*/ 
#define LV_SYMBOL_BATTERY_3 "\xEF\x89\x81" /*62017, 0xF241*/ 
#define LV_SYMBOL_BATTERY_EMPTY "\xEF\x89\x84" /*62020, 0xF244*/ 
#define LV_SYMBOL_BATTERY_FULL "\xEF\x89\x80" /*62016, 0xF240*/ 
#define LV_SYMBOL_BELL "\xEF\x83\xB3" /*61683, 0xF0F3*/ 
#define LV_SYMBOL_BLUETOOTH "\xEF\x8a\x93" /*62099, 0xF293*/ 
#define LV_SYMBOL_BULLET "\xE2\x80\xA2" /*20042, 0x2022*/ 
#define LV_SYMBOL_CALL "\xEF\x82\x95" /*61589, 0xF095*/ 
#define LV_SYMBOL_CHARGE "\xEF\x83\xA7" /*61671, 0xF0E7*/ 
#define LV_SYMBOL_CLOSE "\xEF\x80\x8D" /*61453, 0xF00D*/ 
#define LV_SYMBOL_COPY "\xEF\x83\x85" /*61637, 0xF0C5*/ 
#define LV_SYMBOL_CUT "\xEF\x83\x84" /*61636, 0xF0C4*/ 
#define LV_SYMBOL_DIRECTORY "\xEF\x81\xBB" /*61563, 0xF07B*/ 
#define LV_SYMBOL_DOWN "\xEF\x81\xB8" /*61560, 0xF078*/ 
#define LV_SYMBOL_DOWNLOAD "\xEF\x80\x99" /*61465, 0xF019*/ 
#define LV_SYMBOL_DRIVE "\xEF\x80\x9C" /*61468, 0xF01C*/ 
#define LV_SYMBOL_DUMMY "\xEF\xA3\xBF" 
#define LV_SYMBOL_EDIT "\xEF\x8C\x84" /*62212, 0xF304*/ 
#define LV_SYMBOL_EJECT "\xEF\x81\x92" /*61522, 0xF052*/ 
#define LV_SYMBOL_ENVELOPE "\xEF\x83\xA0" /*61664, 0xF0E0*/ 
#define LV_SYMBOL_EYE_CLOSE "\xEF\x81\xB0" /*61552, 0xF070*/ 
#define LV_SYMBOL_EYE_OPEN "\xEF\x81\xAE" /*61550, 0xF06E*/ 
#define LV_SYMBOL_FILE "\xEF\x85\x9B" /*61787, 0xF158*/ 
#define LV_SYMBOL_GPS "\xEF\x84\xA4" /*61732, 0xF124*/ 
#define LV_SYMBOL_HOME "\xEF\x80\x95" /*61461, 0xF015*/ 
#define LV_SYMBOL_IMAGE "\xEF\x80\xBE" /*61502, 0xF03E*/ 
#define LV_SYMBOL_KEYBOARD "\xEF\x84\x9C" /*61724, 0xF11C*/ 
#define LV_SYMBOL_LEFT "\xEF\x81\x93" /*61523, 0xF053*/ 
#define LV_SYMBOL_LIST "\xEF\x80\x8B" /*61451, 0xF00B*/ 
#define LV_SYMBOL_LOOP "\xEF\x81\xB9" /*61561, 0xF079*/ 
#define LV_SYMBOL_MINUS "\xEF\x81\xA8" /*61544, 0xF068*/ 
#define LV_SYMBOL_MUTE "\xEF\x80\xA6" /*61478, 0xF026*/ 
#define LV_SYMBOL_NEW_LINE "\xEF\xA2\xA2" /*63650, 0xF8A2*/ 
#define LV_SYMBOL_NEXT "\xEF\x81\x91" /*61521, 0xF051*/ 
#define LV_SYMBOL_OK "\xEF\x80\x8C" /*61452, 0xF00C*/ 
#define LV_SYMBOL_PASTE "\xEF\x83\xAA" /*61674, 0xF0EA*/ 
#define LV_SYMBOL_PAUSE "\xEF\x81\x8C" /*61516, 0xF04C*/ 
#define LV_SYMBOL_PLAY "\xEF\x81\x8B" /*61515, 0xF04B*/ 
#define LV_SYMBOL_PLUS "\xEF\x81\xA7" /*61543, 0xF067*/ 
#define LV_SYMBOL_POWER "\xEF\x80\x91" /*61457, 0xF011*/ 
#define LV_SYMBOL_PREV "\xEF\x81\x88" /*61512, 0xF048*/ 
#define LV_SYMBOL_REFRESH "\xEF\x80\xA1" /*61473, 0xF021*/ 
#define LV_SYMBOL_RIGHT "\xEF\x81\x94" /*61524, 0xF054*/ 
#define LV_SYMBOL_SAVE "\xEF\x83\x87" /*61639, 0xF0C7*/ 
#define LV_SYMBOL_SD_CARD "\xEF\x9F\x82" /*63426, 0xF7C2*/ 
#define LV_SYMBOL_SETTINGS "\xEF\x80\x93" /*61459, 0xF013*/ 
#define LV_SYMBOL_SHUFFLE "\xEF\x81\xB4" /*61556, 0xF074*/ 
#define LV_SYMBOL_STOP "\xEF\x81\x8D" /*61517, 0xF04D*/ 
#define LV_SYMBOL_TINT "\xEF\x81\x83" /*61507, 0xF043*/ 
#define LV_SYMBOL_TRASH "\xEF\x8B\xAD" /*62189, 0xF2ED*/ 
#define LV_SYMBOL_UP "\xEF\x81\xB7" /*61559, 0xF077*/ 
#define LV_SYMBOL_UPLOAD "\xEF\x82\x93" /*61587, 0xF093*/ 
#define LV_SYMBOL_USB "\xEF\x8a\x87" /*62087, 0xF287*/ 
#define LV_SYMBOL_VIDEO "\xEF\x80\x88" /*61448, 0xF008*/ 
#define LV_SYMBOL_VOLUME_MAX "\xEF\x80\xA8" /*61480, 0xF028*/ 
#define LV_SYMBOL_VOLUME_MID "\xEF\x80\xA7" /*61479, 0xF027*/ 
#define LV_SYMBOL_WARNING "\xEF\x81\xB1" /*61553, 0xF071*/ 
#define LV_SYMBOL_WIFI "\xEF\x87\xAB" /*61931, 0xF1EB*/ 
#define LV_TABLE_CELL_NONE 0XFFFF 
#define LV_TEXTAREA_CURSOR_LAST (0x7FFF) /*Put the cursor after the last character*/ 
#define LV_TEXT_LEN_MAX UINT32_MAX 
#define LV_TRACE_OBJ_CREATE(...) LV_LOG_TRACE(__VA_ARGS__) 
#define LV_TREE_NODE(n) (( lv_tree_node_t *)(n)) 
#define LV_TRIGO_SHIFT 15 
#define LV_TRIGO_SIN_MAX 32768 
#define LV_TXT_ENC_ASCII 2 
#define LV_TXT_ENC_UTF8 1 
#define LV_UDIV255(x) (((x) * 0x8081U) >> 0x17) 
#define LV_UMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0xFULL << ((sizeof(t) * 8ULL) - 4ULL))) 
#define LV_USE_ANIMIMAGE LV_USE_ANIMIMG 
#define LV_USE_LZ4 0 
#define LV_USE_MEM_MONITOR 0 
#define LV_USE_PERF_MONITOR 0 
#define LV_USE_THORVG 0 
#define LV_VER_RES lv_display_get_vertical_resolution ( lv_display_get_default ()) 
#define LV_ZOOM_NONE LV_SCALE_NONE 

// --- LVGL Function Prototypes (Using emulated types) ---
uint16_t lv_color_16_16_mix(uint16_t c1, uint16_t c2, uint8_t mix);
lv_color_t lv_color_black(void);
uint8_t lv_color_brightness(lv_color_t c);
lv_color_t lv_color_darken(lv_color_t c, lv_opa_t lvl);
bool lv_color_eq(lv_color_t c1, lv_color_t c2);
void lv_color_filter_dsc_init(lv_color_filter_dsc_t * dsc, lv_color_filter_cb_t cb);
uint8_t lv_color_format_get_bpp(lv_color_format_t cf);
uint8_t lv_color_format_get_size(lv_color_format_t cf);
bool lv_color_format_has_alpha(lv_color_format_t src_cf);
lv_color_t lv_color_hex(uint32_t c);
lv_color_t lv_color_hex3(uint32_t c);
lv_color_t lv_color_hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v);
lv_color_t lv_color_lighten(lv_color_t c, lv_opa_t lvl);
uint8_t lv_color_luminance(lv_color_t c);
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix);
lv_color32_t lv_color_mix32(lv_color32_t fg, lv_color32_t bg);
lv_color32_t lv_color_mix32_premultiplied(lv_color32_t fg, lv_color32_t bg);
lv_color32_t lv_color_over32(lv_color32_t fg, lv_color32_t bg);
void lv_color_premultiply(lv_color32_t * c);
lv_color_hsv_t lv_color_rgb_to_hsv(uint8_t r8, uint8_t g8, uint8_t b8);
lv_color32_t lv_color_to_32(lv_color_t color, lv_opa_t opa);
lv_color_hsv_t lv_color_to_hsv(lv_color_t color);
uint32_t lv_color_to_int(lv_color_t c);
uint16_t lv_color_to_u16(lv_color_t color);
uint32_t lv_color_to_u32(lv_color_t color);
lv_color_t lv_color_white(void);
void lv_deinit(void);
const void * lv_font_get_bitmap_fmt_txt(lv_font_glyph_dsc_t * g_dsc, lv_draw_buf_t * draw_buf);
const lv_font_t * lv_font_get_default(void);
const void * lv_font_get_glyph_bitmap(lv_font_glyph_dsc_t * g_dsc, lv_draw_buf_t * draw_buf);
bool lv_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t letter, uint32_t letter_next);
bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next);
uint16_t lv_font_get_glyph_width(const lv_font_t * font, uint32_t letter, uint32_t letter_next);
int32_t lv_font_get_line_height(const lv_font_t * font);
void lv_init(void);
/* ERROR: Cannot pass lv_obj_t by value */ lv_label_create(lv_obj_t parent);
void lv_label_set_long_mode(lv_obj_t obj, lv_label_long_mode_t long_mode);
void lv_label_set_text(lv_obj_t obj, const char * text);
void lv_label_set_text_fmt(lv_obj_t obj, const char * fmt, ...);
void lv_label_set_text_selection_end(lv_obj_t obj, uint32_t index);
void lv_label_set_text_selection_start(lv_obj_t obj, uint32_t index);
void lv_label_set_text_static(lv_obj_t obj, const char * text);
void lv_obj_add_flag(lv_obj_t obj, lv_obj_flag_t f);
void lv_obj_add_style(lv_obj_t obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_align(lv_obj_t obj, lv_align_t align, int32_t x_ofs, int32_t y_ofs);
void lv_obj_align_to(lv_obj_t obj, lv_obj_t base, lv_align_t align, int32_t x_ofs, int32_t y_ofs);
/* ERROR: Cannot pass lv_obj_t by value */ lv_obj_create(lv_obj_t parent);
void lv_obj_delete(lv_obj_t obj);
void lv_obj_delete_anim_completed_cb(lv_anim_t * a);
void lv_obj_delete_async(lv_obj_t obj);
void lv_obj_delete_delayed(lv_obj_t obj, uint32_t delay_ms);
void lv_obj_remove_style(lv_obj_t obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style_all(lv_obj_t obj);
void lv_obj_set_align(lv_obj_t obj, lv_align_t align);
void lv_obj_set_height(lv_obj_t obj, int32_t h);
void lv_obj_set_parent(lv_obj_t obj, lv_obj_t parent);
void lv_obj_set_pos(lv_obj_t obj, int32_t x, int32_t y);
void lv_obj_set_size(lv_obj_t obj, int32_t w, int32_t h);
void lv_obj_set_state(lv_obj_t obj, lv_state_t state, bool v);
void lv_obj_set_width(lv_obj_t obj, int32_t w);
void lv_obj_set_x(lv_obj_t obj, int32_t x);
void lv_obj_set_y(lv_obj_t obj, int32_t y);
void lv_style_init(lv_style_t * style);
void lv_style_reset(lv_style_t * style);
void lv_style_set_align(lv_style_t * style, lv_align_t value);
void lv_style_set_anim(lv_style_t * style, const lv_anim_t * value);
void lv_style_set_anim_duration(lv_style_t * style, uint32_t value);
void lv_style_set_arc_color(lv_style_t * style, lv_color_t value);
void lv_style_set_arc_image_src(lv_style_t * style, const void * value);
void lv_style_set_arc_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_arc_rounded(lv_style_t * style, bool value);
void lv_style_set_arc_width(lv_style_t * style, int32_t value);
void lv_style_set_base_dir(lv_style_t * style, lv_base_dir_t value);
void lv_style_set_bg_color(lv_style_t * style, lv_color_t value);
void lv_style_set_bg_grad(lv_style_t * style, const lv_grad_dsc_t * value);
void lv_style_set_bg_grad_color(lv_style_t * style, lv_color_t value);
void lv_style_set_bg_grad_dir(lv_style_t * style, lv_grad_dir_t value);
void lv_style_set_bg_grad_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_bg_grad_stop(lv_style_t * style, int32_t value);
void lv_style_set_bg_image_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_bg_image_recolor(lv_style_t * style, lv_color_t value);
void lv_style_set_bg_image_recolor_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_bg_image_src(lv_style_t * style, const void * value);
void lv_style_set_bg_image_tiled(lv_style_t * style, bool value);
void lv_style_set_bg_main_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_bg_main_stop(lv_style_t * style, int32_t value);
void lv_style_set_bg_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_bitmap_mask_src(lv_style_t * style, const void * value);
void lv_style_set_blend_mode(lv_style_t * style, lv_blend_mode_t value);
void lv_style_set_border_color(lv_style_t * style, lv_color_t value);
void lv_style_set_border_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_border_post(lv_style_t * style, bool value);
void lv_style_set_border_side(lv_style_t * style, lv_border_side_t value);
void lv_style_set_border_width(lv_style_t * style, int32_t value);
void lv_style_set_clip_corner(lv_style_t * style, bool value);
void lv_style_set_color_filter_dsc(lv_style_t * style, const lv_color_filter_dsc_t * value);
void lv_style_set_color_filter_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_flex_cross_place(lv_style_t * style, lv_flex_align_t value);
void lv_style_set_flex_flow(lv_style_t * style, lv_flex_flow_t value);
void lv_style_set_flex_grow(lv_style_t * style, uint8_t value);
void lv_style_set_flex_main_place(lv_style_t * style, lv_flex_align_t value);
void lv_style_set_flex_track_place(lv_style_t * style, lv_flex_align_t value);
void lv_style_set_grid_cell_column_pos(lv_style_t * style, int32_t value);
void lv_style_set_grid_cell_column_span(lv_style_t * style, int32_t value);
void lv_style_set_grid_cell_row_pos(lv_style_t * style, int32_t value);
void lv_style_set_grid_cell_row_span(lv_style_t * style, int32_t value);
void lv_style_set_grid_cell_x_align(lv_style_t * style, lv_grid_align_t value);
void lv_style_set_grid_cell_y_align(lv_style_t * style, lv_grid_align_t value);
void lv_style_set_grid_column_align(lv_style_t * style, lv_grid_align_t value);
void lv_style_set_grid_column_dsc_array(lv_style_t * style, const int32_t * value);
void lv_style_set_grid_row_align(lv_style_t * style, lv_grid_align_t value);
void lv_style_set_grid_row_dsc_array(lv_style_t * style, const int32_t * value);
void lv_style_set_height(lv_style_t * style, int32_t value);
void lv_style_set_image_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_image_recolor(lv_style_t * style, lv_color_t value);
void lv_style_set_image_recolor_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_layout(lv_style_t * style, uint16_t value);
void lv_style_set_length(lv_style_t * style, int32_t value);
void lv_style_set_line_color(lv_style_t * style, lv_color_t value);
void lv_style_set_line_dash_gap(lv_style_t * style, int32_t value);
void lv_style_set_line_dash_width(lv_style_t * style, int32_t value);
void lv_style_set_line_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_line_rounded(lv_style_t * style, bool value);
void lv_style_set_line_width(lv_style_t * style, int32_t value);
void lv_style_set_margin_all(lv_style_t * style, int32_t value);
void lv_style_set_margin_bottom(lv_style_t * style, int32_t value);
void lv_style_set_margin_hor(lv_style_t * style, int32_t value);
void lv_style_set_margin_left(lv_style_t * style, int32_t value);
void lv_style_set_margin_right(lv_style_t * style, int32_t value);
void lv_style_set_margin_top(lv_style_t * style, int32_t value);
void lv_style_set_margin_ver(lv_style_t * style, int32_t value);
void lv_style_set_max_height(lv_style_t * style, int32_t value);
void lv_style_set_max_width(lv_style_t * style, int32_t value);
void lv_style_set_min_height(lv_style_t * style, int32_t value);
void lv_style_set_min_width(lv_style_t * style, int32_t value);
void lv_style_set_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_opa_layered(lv_style_t * style, lv_opa_t value);
void lv_style_set_outline_color(lv_style_t * style, lv_color_t value);
void lv_style_set_outline_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_outline_pad(lv_style_t * style, int32_t value);
void lv_style_set_outline_width(lv_style_t * style, int32_t value);
void lv_style_set_pad_all(lv_style_t * style, int32_t value);
void lv_style_set_pad_bottom(lv_style_t * style, int32_t value);
void lv_style_set_pad_column(lv_style_t * style, int32_t value);
void lv_style_set_pad_gap(lv_style_t * style, int32_t value);
void lv_style_set_pad_hor(lv_style_t * style, int32_t value);
void lv_style_set_pad_left(lv_style_t * style, int32_t value);
void lv_style_set_pad_radial(lv_style_t * style, int32_t value);
void lv_style_set_pad_right(lv_style_t * style, int32_t value);
void lv_style_set_pad_row(lv_style_t * style, int32_t value);
void lv_style_set_pad_top(lv_style_t * style, int32_t value);
void lv_style_set_pad_ver(lv_style_t * style, int32_t value);
void lv_style_set_prop(lv_style_t * style, lv_style_prop_t prop, lv_style_value_t value);
void lv_style_set_radial_offset(lv_style_t * style, int32_t value);
void lv_style_set_radius(lv_style_t * style, int32_t value);
void lv_style_set_recolor(lv_style_t * style, lv_color_t value);
void lv_style_set_recolor_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_rotary_sensitivity(lv_style_t * style, uint32_t value);
void lv_style_set_shadow_color(lv_style_t * style, lv_color_t value);
void lv_style_set_shadow_offset_x(lv_style_t * style, int32_t value);
void lv_style_set_shadow_offset_y(lv_style_t * style, int32_t value);
void lv_style_set_shadow_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_shadow_spread(lv_style_t * style, int32_t value);
void lv_style_set_shadow_width(lv_style_t * style, int32_t value);
void lv_style_set_size(lv_style_t * style, int32_t width, int32_t height);
void lv_style_set_text_align(lv_style_t * style, lv_text_align_t value);
void lv_style_set_text_color(lv_style_t * style, lv_color_t value);
void lv_style_set_text_decor(lv_style_t * style, lv_text_decor_t value);
void lv_style_set_text_font(lv_style_t * style, const lv_font_t * value);
void lv_style_set_text_letter_space(lv_style_t * style, int32_t value);
void lv_style_set_text_line_space(lv_style_t * style, int32_t value);
void lv_style_set_text_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_text_outline_stroke_color(lv_style_t * style, lv_color_t value);
void lv_style_set_text_outline_stroke_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_text_outline_stroke_width(lv_style_t * style, int32_t value);
void lv_style_set_transform_height(lv_style_t * style, int32_t value);
void lv_style_set_transform_pivot_x(lv_style_t * style, int32_t value);
void lv_style_set_transform_pivot_y(lv_style_t * style, int32_t value);
void lv_style_set_transform_rotation(lv_style_t * style, int32_t value);
void lv_style_set_transform_scale(lv_style_t * style, int32_t value);
void lv_style_set_transform_scale_x(lv_style_t * style, int32_t value);
void lv_style_set_transform_scale_y(lv_style_t * style, int32_t value);
void lv_style_set_transform_skew_x(lv_style_t * style, int32_t value);
void lv_style_set_transform_skew_y(lv_style_t * style, int32_t value);
void lv_style_set_transform_width(lv_style_t * style, int32_t value);
void lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value);
void lv_style_set_translate_radial(lv_style_t * style, int32_t value);
void lv_style_set_translate_x(lv_style_t * style, int32_t value);
void lv_style_set_translate_y(lv_style_t * style, int32_t value);
void lv_style_set_width(lv_style_t * style, int32_t value);
void lv_style_set_x(lv_style_t * style, int32_t value);
void lv_style_set_y(lv_style_t * style, int32_t value);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* EMUL_LVGL_H */
