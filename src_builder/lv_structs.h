
/**
 * Descriptor for style transitions
 */
typedef struct {
    const lv_style_prop_t * props; /**< An array with the properties to animate.*/
    void * user_data;              /**< A custom user data that will be passed to the animation's user_data */
    lv_anim_path_cb_t path_xcb;    /**< A path for the animation.*/
    uint32_t time;                 /**< Duration of the transition in [ms]*/
    uint32_t delay;                /**< Delay before the transition in [ms]*/
} lv_style_transition_dsc_t;

/** A gradient stop definition.
 *  This matches a color and a position in a virtual 0-255 scale.
 */
typedef struct {
    lv_color_t color;   /**< The stop color */
    lv_opa_t   opa;     /**< The opacity of the color*/
    uint8_t    frac;    /**< The stop position in 1/255 unit */
} lv_grad_stop_t;

/** A descriptor of a gradient. */
typedef struct {
    lv_grad_stop_t   stops[LV_GRADIENT_MAX_STOPS];  /**< A gradient stop array */
    uint8_t          stops_count;                   /**< The number of used stops in the array */
    lv_grad_dir_t    dir : 4;                       /**< The gradient direction.
                                                         * Any of LV_GRAD_DIR_NONE, LV_GRAD_DIR_VER, LV_GRAD_DIR_HOR,
                                                         * LV_GRAD_TYPE_LINEAR, LV_GRAD_TYPE_RADIAL, LV_GRAD_TYPE_CONICAL */
    lv_grad_extend_t     extend : 3;                    /**< Behaviour outside the defined range.
                                                         * LV_GRAD_EXTEND_NONE, LV_GRAD_EXTEND_PAD, LV_GRAD_EXTEND_REPEAT, LV_GRAD_EXTEND_REFLECT */
#if LV_USE_DRAW_SW_COMPLEX_GRADIENTS
    union {
        /*Linear gradient parameters*/
        struct {
            lv_point_t  start;                          /**< Linear gradient vector start point */
            lv_point_t  end;                            /**< Linear gradient vector end point */
        } linear;
        /*Radial gradient parameters*/
        struct {
            lv_point_t  focal;                          /**< Center of the focal (starting) circle in local coordinates */
            /* (can be the same as the ending circle to create concentric circles) */
            lv_point_t  focal_extent;                   /**< Point on the circle (can be the same as the center) */
            lv_point_t  end;                            /**< Center of the ending circle in local coordinates */
            lv_point_t  end_extent;                     /**< Point on the circle determining the radius of the gradient */
        } radial;
        /*Conical gradient parameters*/
        struct {
            lv_point_t  center;                         /**< Conical gradient center point */
            int16_t     start_angle;                    /**< Start angle 0..3600 */
            int16_t     end_angle;                      /**< End angle 0..3600 */
        } conical;
    } params;
    void * state;
#endif
} lv_grad_dsc_t;

struct _lv_matrix_t {
    float m[3][3];
};

struct _lv_color_filter_dsc_t;

typedef lv_color_t (*lv_color_filter_cb_t)(const struct _lv_color_filter_dsc_t *, lv_color_t, lv_opa_t);

struct _lv_color_filter_dsc_t {
    lv_color_filter_cb_t filter_cb;
    void * user_data;
};

typedef struct _lv_color_filter_dsc_t lv_color_filter_dsc_t;
typedef struct _lv_matrix_t lv_matrix_t;