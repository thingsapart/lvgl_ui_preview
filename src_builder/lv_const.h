#if 0
#include "lv_types.h"

#define LV_COORD_TYPE_SHIFT    (29U)

#define LV_COORD_TYPE_MASK     (3 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE(x)       ((x) & LV_COORD_TYPE_MASK)  /*Extract type specifiers*/
#define LV_COORD_PLAIN(x)      ((x) & ~LV_COORD_TYPE_MASK) /*Remove type specifiers*/

#define LV_COORD_TYPE_PX       (0 << LV_COORD_TYPE_SHIFT)                                                                                                
#define LV_COORD_TYPE_SPEC     (1 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_PX_NEG   (3 << LV_COORD_TYPE_SHIFT)

#define LV_COORD_IS_PX(x)       (LV_COORD_TYPE(x) == LV_COORD_TYPE_PX || LV_COORD_TYPE(x) == LV_COORD_TYPE_PX_NEG)
#define LV_COORD_IS_SPEC(x)     (LV_COORD_TYPE(x) == LV_COORD_TYPE_SPEC)

#define LV_COORD_SET_SPEC(x)    ((x) | LV_COORD_TYPE_SPEC)

/** Max coordinate value */
#define LV_COORD_MAX            ((1 << LV_COORD_TYPE_SHIFT) - 1)
#define LV_COORD_MIN            (-LV_COORD_MAX)

/*Special coordinates*/
#define LV_SIZE_CONTENT         LV_COORD_SET_SPEC(LV_COORD_MAX)
#define LV_PCT_STORED_MAX       (LV_COORD_MAX - 1)
#if LV_PCT_STORED_MAX % 2 != 0
#error LV_PCT_STORED_MAX should be an even number
#endif
#define LV_PCT_POS_MAX          (LV_PCT_STORED_MAX / 2)
#define LV_PCT(x)               (LV_COORD_SET_SPEC(((x) < 0 ? (LV_PCT_POS_MAX - LV_MAX((x), -LV_PCT_POS_MAX)) : LV_MIN((x), LV_PCT_POS_MAX))))
#define LV_COORD_IS_PCT(x)      ((LV_COORD_IS_SPEC(x) && LV_COORD_PLAIN(x) <= LV_PCT_STORED_MAX))
#define LV_COORD_GET_PCT(x)     (LV_COORD_PLAIN(x) > LV_PCT_POS_MAX ? LV_PCT_POS_MAX - LV_COORD_PLAIN(x) : LV_COORD_PLAIN(x))

typedef int32_t lv_color_t;
typedef int32_t lv_grad_dsc_t;
//typedef int32_t lv_opa_t;
typedef int32_t lv_color_t;

#endif

typedef uint32_t lv_style_selector_t;
typedef void *lv_font_t;
//typedef struct _lv_obj_t lv_obj_t;

//typedef uint16_t lv_state_t;
//typedef uint32_t lv_part_t;

//typedef uint8_t lv_opa_t;

typedef uint8_t lv_style_prop_t;

typedef struct _lv_anim_t lv_anim_t;

typedef int32_t lv_coord_t;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} lv_color_t;

typedef struct {
    uint16_t blue : 5;
    uint16_t green : 6;
    uint16_t red : 5;
} lv_color16_t;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} lv_color32_t;

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
} lv_color_hsv_t;

typedef struct {
    uint8_t lumi;
    uint8_t alpha;
} lv_color16a_t;

/**
 * A common type to handle all the property types in the same way.
 */
typedef union {
    int32_t num;         /**< Number integer number (opacity, enums, booleans or "normal" numbers)*/
    const void * ptr;    /**< Constant pointers  (font, cone text, etc)*/
    lv_color_t color;    /**< Colors*/
} lv_style_value_t;

typedef int32_t (*lv_anim_path_cb_t)(const lv_anim_t *);

#define LV_GRADIENT_MAX_STOPS 1

lv_color_t lv_color_hex(uint32_t c);
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color32_t lv_color32_make(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
lv_color_t lv_color_hex3(uint32_t c);

#define color_to_str(color_str, value) (snprintf(color_str, sizeof(color_str), "#%02X%02X%02X", (unsigned int) value.red, (unsigned int) value.green, (unsigned int) value.blue));
#define lv_color_white() (lv_color_hex(0xffffff))
#define lv_color_black() (lv_color_hex(0x000000))

#define lv_font_default() (&lv_font_montserrat_14)
#define lv_pct(x) LV_PCT(x)