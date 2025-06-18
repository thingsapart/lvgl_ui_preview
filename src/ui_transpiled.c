#include "lvgl.h"


#define LV_MALLOC lv_malloc
#define LV_FREE lv_free
#define LV_GRID_FR_1 LV_GRID_FR(1)
#define LV_GRID_FR_2 LV_GRID_FR(2)
#define LV_GRID_FR_3 LV_GRID_FR(3)
#define LV_GRID_FR_4 LV_GRID_FR(4)
#define LV_GRID_FR_5 LV_GRID_FR(5)
#define LV_GRID_FR_10 LV_GRID_FR(10)
#define LV_BORDER_SIDE_TOP_BOTTOM (LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM)
#define LV_BORDER_SIDE_LEFT_RIGHT (LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT)
#define LV_BORDER_SIDE_LEFT_TOP (LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP)
#define LV_BORDER_SIDE_LEFT_BOTTOM (LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM)
#define LV_BORDER_SIDE_RIGHT_TOP (LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_TOP)
#define LV_BORDER_SIDE_RIGHT_BOTTOM (LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM)

// For LV_LOG_INFO, LV_LOG_ERROR, etc. Ensure lv_conf.h enables logging.
// And that these macros are available or mapped.
#ifndef LV_LOG_INFO
#define LV_LOG_INFO(...) // Log stub
#endif
#ifndef LV_LOG_ERROR
#define LV_LOG_ERROR(...) // Log stub
#endif
#include "data_binding.h" // For action/observes attributes
//extern data_binding_registry_t* REGISTRY; // Global registry for actions and data bindings
extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);
extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);

#ifndef LV_GRID_FR
#define LV_GRID_FR(x) (lv_coord_t)(LV_COORD_FRACT_MAX / (x))
#endif
#ifndef LV_SIZE_CONTENT
#define LV_SIZE_CONTENT LV_COORD_MAX
#endif
#ifndef LV_PART_MAIN
#define LV_PART_MAIN LV_STATE_DEFAULT
#endif

void create_ui_ui_transpiled(lv_obj_t *screen_parent) {
    // --- VARIABLE DECLARATIONS ---
    static lv_style_t c_debug_1;
    static lv_style_t c_container_2;
    static lv_style_t c_bar_indicator_3;
    static lv_style_t c_bg_gradient_4;
    static lv_style_t c_flex_x_5;
    static lv_style_t c_flex_y_6;
    static lv_style_t c_indicator_green_7;
    static lv_style_t c_indicator_yellow_8;
    static lv_style_t c_jog_btn_9;
    static lv_style_t c_border_top_btm_10;
    static lv_style_t c_border_right_11;
    static lv_style_t c_indicator_light_12;
    static lv_style_t c_action_button_13;
    lv_obj_t * c_tileview_14;
    lv_obj_t * c_with_target_15;
    lv_obj_t * c_label_16;
    lv_obj_t * c_with_target_17;
    lv_obj_t * c_obj_18;
    lv_obj_t * c_obj_19;
    lv_obj_t * c_obj_20;
    lv_obj_t * c_obj_21;
    lv_obj_t * c_obj_22;
    lv_obj_t * c_label_23;
    lv_obj_t * c_label_24;
    lv_obj_t * c_obj_25;
    lv_obj_t * c_label_26;
    lv_obj_t * c_label_27;
    lv_obj_t * c_label_28;
    lv_obj_t * c_label_29;
    lv_obj_t * c_obj_30;
    lv_obj_t * c_obj_31;
    lv_obj_t * c_label_32;
    lv_obj_t * c_label_33;
    lv_obj_t * c_obj_34;
    lv_obj_t * c_label_35;
    lv_obj_t * c_label_36;
    lv_obj_t * c_label_37;
    lv_obj_t * c_label_38;
    lv_obj_t * c_obj_39;
    lv_obj_t * c_obj_40;
    lv_obj_t * c_label_41;
    lv_obj_t * c_label_42;
    lv_obj_t * c_obj_43;
    lv_obj_t * c_label_44;
    lv_obj_t * c_label_45;
    lv_obj_t * c_label_46;
    lv_obj_t * c_label_47;
    lv_obj_t * c_obj_48;
    lv_obj_t * c_obj_49;
    lv_obj_t * c_obj_50;
    lv_obj_t * c_label_51;
    lv_obj_t * c_grid_obj_52;
    static const lv_coord_t c_coord_array_53[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_54[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_55;
    lv_obj_t * c_label_56;
    lv_obj_t * c_obj_57;
    lv_obj_t * c_bar_58;
    lv_obj_t * c_scale_59;
    lv_obj_t * c_obj_60;
    lv_obj_t * c_label_61;
    lv_obj_t * c_label_62;
    lv_obj_t * c_label_63;
    lv_obj_t * c_label_64;
    lv_obj_t * c_obj_65;
    lv_obj_t * c_obj_66;
    lv_obj_t * c_label_67;
    lv_obj_t * c_grid_obj_68;
    static const lv_coord_t c_coord_array_69[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_70[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_71;
    lv_obj_t * c_label_72;
    lv_obj_t * c_obj_73;
    lv_obj_t * c_bar_74;
    lv_obj_t * c_scale_75;
    lv_obj_t * c_obj_76;
    lv_obj_t * c_label_77;
    lv_obj_t * c_label_78;
    lv_obj_t * c_label_79;
    lv_obj_t * c_label_80;
    lv_obj_t * c_obj_81;
    lv_obj_t * c_obj_82;
    lv_obj_t * c_label_83;
    lv_obj_t * c_grid_obj_84;
    static const lv_coord_t c_coord_array_85[] = { 35, 45, 20, 40, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_86[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_87;
    lv_obj_t * c_label_88;
    lv_obj_t * c_label_89;
    lv_obj_t * c_label_90;
    lv_obj_t * c_obj_91;
    lv_obj_t * c_label_92;
    lv_obj_t * c_label_93;
    lv_obj_t * c_label_94;
    lv_obj_t * c_dropdown_95;
    lv_obj_t * c_with_target_96;
    lv_obj_t * c_dropdown_97;
    lv_obj_t * c_with_target_98;
    // --- END VARIABLE DECLARATIONS ---
    lv_obj_t *parent_obj = screen_parent ? screen_parent : lv_screen_active();
    if (!parent_obj) {
        LV_LOG_ERROR("Cannot render UI: No parent screen available.");
        return;
    }
    
    lv_style_init(&c_debug_1);
    lvgl_json_register_ptr("debug", "lv_style_t", (void*)&c_debug_1);
    lv_style_set_outline_width(&c_debug_1, 1);
    lv_style_set_outline_color(&c_debug_1, lv_color_hex(0xFFEEFF));
    lv_style_set_outline_opa(&c_debug_1, 150);
    lv_style_set_border_width(&c_debug_1, 1);
    lv_style_set_border_color(&c_debug_1, lv_color_hex(0xFFEEFF));
    lv_style_set_border_opa(&c_debug_1, 150);
    lv_style_set_radius(&c_debug_1, 0);
    lv_style_init(&c_container_2);
    lvgl_json_register_ptr("container", "lv_style_t", (void*)&c_container_2);
    lv_style_set_pad_all(&c_container_2, 0);
    lv_style_set_margin_all(&c_container_2, 0);
    lv_style_set_border_width(&c_container_2, 0);
    lv_style_set_pad_row(&c_container_2, 3);
    lv_style_set_pad_column(&c_container_2, 5);
    lv_style_init(&c_bar_indicator_3);
    lvgl_json_register_ptr("bar_indicator", "lv_style_t", (void*)&c_bar_indicator_3);
    lv_style_set_radius(&c_bar_indicator_3, 4);
    lv_style_init(&c_bg_gradient_4);
    lvgl_json_register_ptr("bg_gradient", "lv_style_t", (void*)&c_bg_gradient_4);
    lv_style_set_bg_opa(&c_bg_gradient_4, 255);
    lv_style_set_bg_color(&c_bg_gradient_4, lv_color_hex(0x222222));
    lv_style_set_bg_grad_color(&c_bg_gradient_4, lv_color_hex(0x444444));
    lv_style_set_bg_grad_dir(&c_bg_gradient_4, LV_GRAD_DIR_HOR);
    lv_style_init(&c_flex_x_5);
    lvgl_json_register_ptr("flex_x", "lv_style_t", (void*)&c_flex_x_5);
    lv_style_set_layout(&c_flex_x_5, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&c_flex_x_5, LV_FLEX_FLOW_ROW);
    lv_style_init(&c_flex_y_6);
    lvgl_json_register_ptr("flex_y", "lv_style_t", (void*)&c_flex_y_6);
    lv_style_set_layout(&c_flex_y_6, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&c_flex_y_6, LV_FLEX_FLOW_COLUMN);
    lv_style_init(&c_indicator_green_7);
    lvgl_json_register_ptr("indicator_green", "lv_style_t", (void*)&c_indicator_green_7);
    lv_style_set_text_color(&c_indicator_green_7, lv_color_hex(0x44EE44));
    lv_style_init(&c_indicator_yellow_8);
    lvgl_json_register_ptr("indicator_yellow", "lv_style_t", (void*)&c_indicator_yellow_8);
    lv_style_set_text_color(&c_indicator_yellow_8, lv_color_hex(0xFFFF55));
    lv_style_init(&c_jog_btn_9);
    lvgl_json_register_ptr("jog_btn", "lv_style_t", (void*)&c_jog_btn_9);
    lv_style_set_pad_all(&c_jog_btn_9, 5);
    lv_style_set_pad_bottom(&c_jog_btn_9, 10);
    lv_style_set_pad_top(&c_jog_btn_9, 10);
    lv_style_set_margin_all(&c_jog_btn_9, 0);
    lv_style_set_radius(&c_jog_btn_9, 2);
    lv_style_init(&c_border_top_btm_10);
    lvgl_json_register_ptr("border_top_btm", "lv_style_t", (void*)&c_border_top_btm_10);
    lv_style_set_border_width(&c_border_top_btm_10, 1);
    lv_style_set_border_color(&c_border_top_btm_10, lv_color_hex(0xFFFFFF));
    lv_style_set_border_opa(&c_border_top_btm_10, 50);
    lv_style_set_border_side(&c_border_top_btm_10, LV_BORDER_SIDE_TOP_BOTTOM);
    lv_style_init(&c_border_right_11);
    lvgl_json_register_ptr("border_right", "lv_style_t", (void*)&c_border_right_11);
    lv_style_set_border_width(&c_border_right_11, 1);
    lv_style_set_border_color(&c_border_right_11, lv_color_hex(0xFFFFFF));
    lv_style_set_border_opa(&c_border_right_11, 50);
    lv_style_set_border_side(&c_border_right_11, LV_BORDER_SIDE_RIGHT);
    lv_style_init(&c_indicator_light_12);
    lvgl_json_register_ptr("indicator_light", "lv_style_t", (void*)&c_indicator_light_12);
    lv_style_set_border_width(&c_indicator_light_12, 6);
    lv_style_set_pad_left(&c_indicator_light_12, 10);
    lv_style_set_margin_left(&c_indicator_light_12, 10);
    lv_style_set_border_opa(&c_indicator_light_12, 200);
    lv_style_set_border_side(&c_indicator_light_12, LV_BORDER_SIDE_LEFT);
    // Component definition 'axis_jog_speed_adjust' processed and stored.
    // Component definition 'axis_pos_display' processed and stored.
    // Component definition 'feed_rate_scale' processed and stored.
    // Component definition 'jog_feed' processed and stored.
    lv_style_init(&c_action_button_13);
    lvgl_json_register_ptr("action_button", "lv_style_t", (void*)&c_action_button_13);
    lv_style_set_size(&c_action_button_13, 45, 45);
    lv_style_set_bg_color(&c_action_button_13, lv_color_hex(0x1F95F6));
    lv_style_set_radius(&c_action_button_13, LV_RADIUS_CIRCLE);
    // Component definition 'nav_action_button' processed and stored.
    // Component definition 'jog_action_button' processed and stored.
    // Component definition 'xyz_axis_pos_display' processed and stored.
    // Component definition 'jog_view' processed and stored.
    // Component definition 'btn_label' processed and stored.
    c_tileview_14 = lv_tileview_create(parent_obj);
    lv_obj_add_style(c_tileview_14, &c_container_2, 0);
    lv_obj_set_size(c_tileview_14, LV_PCT(100), LV_PCT(100));
    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_row on c_tileview_14
    lv_obj_set_style_pad_row(c_tileview_14, 0, LV_PART_MAIN);
    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_tileview_14
    lv_obj_set_style_pad_column(c_tileview_14, 0, LV_PART_MAIN);
    // ERROR: 'with' block for c_tileview_14 is missing 'do' object or it's not an object. Skipping.
    c_with_target_15 = lv_tileview_add_tile(c_tileview_14, 0, 0, LV_DIR_RIGHT);
    if (c_with_target_15) { // Check if 'with.obj' for c_tileview_14 resolved
        lv_obj_add_style(c_with_target_15, &c_container_2, 0);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_with_target_15
        lv_obj_set_style_bg_color(c_with_target_15, lv_color_hex(0xFF0000), LV_PART_MAIN);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_with_target_15
        lv_obj_set_style_bg_opa(c_with_target_15, LV_OPA_COVER, LV_PART_MAIN);
        // Children of c_with_target_15 (obj) with path prefix 'None'
            c_label_16 = lv_label_create(c_with_target_15);
            lv_obj_center(c_label_16);
            lv_label_set_text(c_label_16, "Scroll right");
    } else {
        // WARNING: 'with.obj' for c_tileview_14 resolved to NULL. 'do' block not applied.
    }
    c_with_target_17 = lv_tileview_add_tile(c_tileview_14, 1, 0, LV_DIR_LEFT);
    if (c_with_target_17) { // Check if 'with.obj' for c_tileview_14 resolved
        lv_obj_add_style(c_with_target_17, &c_container_2, 0);
        // Children of c_with_target_17 (obj) with path prefix 'None'
            // Using view component 'jog_view'
            c_obj_18 = lv_obj_create(c_with_target_17);
            lv_obj_set_layout(c_obj_18, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(c_obj_18, LV_FLEX_FLOW_ROW);
            lv_obj_add_style(c_obj_18, &c_container_2, 0);
            lv_obj_set_size(c_obj_18, LV_PCT(100), 320);
            lvgl_json_register_ptr("main", "lv_obj_t", (void*)c_obj_18);
            // Children of c_obj_18 (obj) with path prefix 'main'
                c_obj_19 = lv_obj_create(c_obj_18);
                lv_obj_set_layout(c_obj_19, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_19, LV_FLEX_FLOW_COLUMN);
                lv_obj_add_style(c_obj_19, &c_container_2, 0);
                lv_obj_set_height(c_obj_19, LV_PCT(100));
                lv_obj_set_flex_grow(c_obj_19, 60);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_side on c_obj_19
                lv_obj_set_style_border_side(c_obj_19, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_width on c_obj_19
                lv_obj_set_style_border_width(c_obj_19, 2, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius on c_obj_19
                lv_obj_set_style_radius(c_obj_19, 0, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_19
                lv_obj_set_style_border_color(c_obj_19, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_19
                lv_obj_set_style_border_opa(c_obj_19, 90, LV_PART_MAIN);
                // Children of c_obj_19 (obj) with path prefix 'main'
                    // Using view component 'xyz_axis_pos_display'
                    c_obj_20 = lv_obj_create(c_obj_19);
                    lv_obj_set_layout(c_obj_20, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_20, LV_FLEX_FLOW_COLUMN);
                    lv_obj_add_style(c_obj_20, &c_container_2, 0);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius on c_obj_20
                    lv_obj_set_style_radius(c_obj_20, 0, LV_PART_MAIN);
                    lv_obj_set_size(c_obj_20, LV_PCT(100), LV_PCT(100));
                    // Children of c_obj_20 (obj) with path prefix 'main'
                        // Using view component 'axis_pos_display'
                        c_obj_21 = lv_obj_create(c_obj_20);
                        lv_obj_add_style(c_obj_21, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_21, &c_container_2, 0);
                        lv_obj_set_size(c_obj_21, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_21
                        lv_obj_set_style_pad_all(c_obj_21, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_21
                        lv_obj_set_style_pad_bottom(c_obj_21, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_21
                        lv_obj_set_style_border_color(c_obj_21, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_21
                        lv_obj_set_style_border_opa(c_obj_21, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_21
                        lv_obj_set_style_margin_all(c_obj_21, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_x", "lv_obj_t", (void*)c_obj_21);
                        // Children of c_obj_21 (obj) with path prefix 'main:axis_pos_x'
                            c_obj_22 = lv_obj_create(c_obj_21);
                            lv_obj_add_style(c_obj_22, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_22, &c_container_2, 0);
                            lv_obj_set_size(c_obj_22, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_22 (obj) with path prefix 'main:axis_pos_x'
                                c_label_23 = lv_label_create(c_obj_22);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_23
                                lv_obj_set_style_text_font(c_label_23, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_23, "X");
                                lv_obj_set_width(c_label_23, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_23, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_23
                                lv_obj_set_style_border_color(c_label_23, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_24 = lv_label_create(c_obj_22);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_24
                                lv_obj_set_style_text_font(c_label_24, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_24, "11.000");
                                lv_obj_set_flex_grow(c_label_24, 1);
                                lv_obj_add_style(c_label_24, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_24
                                lv_obj_set_style_text_align(c_label_24, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_25 = lv_obj_create(c_obj_21);
                            lv_obj_add_style(c_obj_25, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_25, &c_container_2, 0);
                            lv_obj_set_size(c_obj_25, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_25 (obj) with path prefix 'main:axis_pos_x'
                                c_label_26 = lv_label_create(c_obj_25);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_26
                                lv_obj_set_style_text_font(c_label_26, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_26, "51.000");
                                lv_obj_set_flex_grow(c_label_26, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_26
                                lv_obj_set_style_text_align(c_label_26, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_26, &c_indicator_yellow_8, 0);
                                c_label_27 = lv_label_create(c_obj_25);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_27
                                lv_obj_set_style_text_font(c_label_27, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_27, "");
                                lv_obj_set_width(c_label_27, 14);
                                c_label_28 = lv_label_create(c_obj_25);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_28
                                lv_obj_set_style_text_font(c_label_28, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_28, "2.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_28
                                lv_obj_set_style_text_align(c_label_28, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_28, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_28, 1);
                                c_label_29 = lv_label_create(c_obj_25);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_29
                                lv_obj_set_style_text_font(c_label_29, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_29, "");
                                lv_obj_set_width(c_label_29, 14);
                        // Using view component 'axis_pos_display'
                        c_obj_30 = lv_obj_create(c_obj_20);
                        lv_obj_add_style(c_obj_30, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_30, &c_container_2, 0);
                        lv_obj_set_size(c_obj_30, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_30
                        lv_obj_set_style_pad_all(c_obj_30, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_30
                        lv_obj_set_style_pad_bottom(c_obj_30, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_30
                        lv_obj_set_style_border_color(c_obj_30, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_30
                        lv_obj_set_style_border_opa(c_obj_30, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_30
                        lv_obj_set_style_margin_all(c_obj_30, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_y", "lv_obj_t", (void*)c_obj_30);
                        // Children of c_obj_30 (obj) with path prefix 'main:axis_pos_y'
                            c_obj_31 = lv_obj_create(c_obj_30);
                            lv_obj_add_style(c_obj_31, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_31, &c_container_2, 0);
                            lv_obj_set_size(c_obj_31, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_31 (obj) with path prefix 'main:axis_pos_y'
                                c_label_32 = lv_label_create(c_obj_31);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_32
                                lv_obj_set_style_text_font(c_label_32, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_32, "Y");
                                lv_obj_set_width(c_label_32, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_32, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_32
                                lv_obj_set_style_border_color(c_label_32, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_33 = lv_label_create(c_obj_31);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_33
                                lv_obj_set_style_text_font(c_label_33, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_33, "22.000");
                                lv_obj_set_flex_grow(c_label_33, 1);
                                lv_obj_add_style(c_label_33, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_33
                                lv_obj_set_style_text_align(c_label_33, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_34 = lv_obj_create(c_obj_30);
                            lv_obj_add_style(c_obj_34, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_34, &c_container_2, 0);
                            lv_obj_set_size(c_obj_34, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_34 (obj) with path prefix 'main:axis_pos_y'
                                c_label_35 = lv_label_create(c_obj_34);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_35
                                lv_obj_set_style_text_font(c_label_35, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_35, "72.000");
                                lv_obj_set_flex_grow(c_label_35, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_35
                                lv_obj_set_style_text_align(c_label_35, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_35, &c_indicator_yellow_8, 0);
                                c_label_36 = lv_label_create(c_obj_34);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_36
                                lv_obj_set_style_text_font(c_label_36, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_36, "");
                                lv_obj_set_width(c_label_36, 14);
                                c_label_37 = lv_label_create(c_obj_34);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_37
                                lv_obj_set_style_text_font(c_label_37, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_37, "-12.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_37
                                lv_obj_set_style_text_align(c_label_37, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_37, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_37, 1);
                                c_label_38 = lv_label_create(c_obj_34);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_38
                                lv_obj_set_style_text_font(c_label_38, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_38, "");
                                lv_obj_set_width(c_label_38, 14);
                        // Using view component 'axis_pos_display'
                        c_obj_39 = lv_obj_create(c_obj_20);
                        lv_obj_add_style(c_obj_39, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_39, &c_container_2, 0);
                        lv_obj_set_size(c_obj_39, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_39
                        lv_obj_set_style_pad_all(c_obj_39, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_39
                        lv_obj_set_style_pad_bottom(c_obj_39, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_39
                        lv_obj_set_style_border_color(c_obj_39, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_39
                        lv_obj_set_style_border_opa(c_obj_39, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_39
                        lv_obj_set_style_margin_all(c_obj_39, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_z", "lv_obj_t", (void*)c_obj_39);
                        // Children of c_obj_39 (obj) with path prefix 'main:axis_pos_z'
                            c_obj_40 = lv_obj_create(c_obj_39);
                            lv_obj_add_style(c_obj_40, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_40, &c_container_2, 0);
                            lv_obj_set_size(c_obj_40, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_40 (obj) with path prefix 'main:axis_pos_z'
                                c_label_41 = lv_label_create(c_obj_40);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_41
                                lv_obj_set_style_text_font(c_label_41, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_41, "Z");
                                lv_obj_set_width(c_label_41, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_41, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_41
                                lv_obj_set_style_border_color(c_label_41, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_42 = lv_label_create(c_obj_40);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_42
                                lv_obj_set_style_text_font(c_label_42, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_42, "1.000");
                                lv_obj_set_flex_grow(c_label_42, 1);
                                lv_obj_add_style(c_label_42, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_42
                                lv_obj_set_style_text_align(c_label_42, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_43 = lv_obj_create(c_obj_39);
                            lv_obj_add_style(c_obj_43, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_43, &c_container_2, 0);
                            lv_obj_set_size(c_obj_43, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_43 (obj) with path prefix 'main:axis_pos_z'
                                c_label_44 = lv_label_create(c_obj_43);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_44
                                lv_obj_set_style_text_font(c_label_44, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_44, "1.000");
                                lv_obj_set_flex_grow(c_label_44, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_44
                                lv_obj_set_style_text_align(c_label_44, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_44, &c_indicator_yellow_8, 0);
                                c_label_45 = lv_label_create(c_obj_43);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_45
                                lv_obj_set_style_text_font(c_label_45, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_45, "");
                                lv_obj_set_width(c_label_45, 14);
                                c_label_46 = lv_label_create(c_obj_43);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_46
                                lv_obj_set_style_text_font(c_label_46, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_46, "0.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_46
                                lv_obj_set_style_text_align(c_label_46, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_46, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_46, 1);
                                c_label_47 = lv_label_create(c_obj_43);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_47
                                lv_obj_set_style_text_font(c_label_47, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_47, "");
                                lv_obj_set_width(c_label_47, 14);
                c_obj_48 = lv_obj_create(c_obj_18);
                lv_obj_set_layout(c_obj_48, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_48, LV_FLEX_FLOW_COLUMN);
                lv_obj_add_style(c_obj_48, &c_container_2, 0);
                lv_obj_set_height(c_obj_48, LV_PCT(100));
                lv_obj_set_flex_grow(c_obj_48, 45);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_top on c_obj_48
                lv_obj_set_style_pad_top(c_obj_48, 5, LV_PART_MAIN);
                // Children of c_obj_48 (obj) with path prefix 'main'
                    // Using view component 'feed_rate_scale'
                    c_obj_49 = lv_obj_create(c_obj_48);
                    lv_obj_set_size(c_obj_49, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_49, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_49, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_49, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_49, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_49
                    lv_obj_set_style_pad_column(c_obj_49, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_49
                    lv_obj_set_style_pad_bottom(c_obj_49, 12, LV_PART_MAIN);
                    // Children of c_obj_49 (obj) with path prefix 'main'
                        c_obj_50 = lv_obj_create(c_obj_49);
                        lv_obj_set_layout(c_obj_50, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_50, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_50, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_50, LV_PCT(100));
                        lv_obj_set_height(c_obj_50, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_50, 1);
                        // Children of c_obj_50 (obj) with path prefix 'main'
                            c_label_51 = lv_label_create(c_obj_50);
                            lv_label_set_text(c_label_51, "FEED");
                            lv_obj_set_height(c_label_51, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_51, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_51
                            lv_obj_set_style_text_font(c_label_51, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_51, &c_border_top_btm_10, 0);
                            c_grid_obj_52 = lv_obj_create(c_obj_50);
                            lv_obj_add_style(c_grid_obj_52, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_52, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_52, LV_SIZE_CONTENT);
                            // Children of c_grid_obj_52 (obj) with path prefix 'main'
                                c_label_55 = lv_label_create(c_grid_obj_52);
                                lv_label_set_text(c_label_55, "F");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_55
                                lv_obj_set_style_text_font(c_label_55, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_55, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                                lv_obj_set_height(c_label_55, LV_SIZE_CONTENT);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_label_55
                                lv_obj_set_style_pad_left(c_label_55, 10, LV_PART_MAIN);
                                c_label_56 = lv_label_create(c_grid_obj_52);
                                lv_obj_set_grid_cell(c_label_56, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_56
                                lv_obj_set_style_text_font(c_label_56, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_56, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_56, "1000");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_56
                                lv_obj_set_style_pad_right(c_label_56, 10, LV_PART_MAIN);
                            lv_obj_set_grid_dsc_array(c_grid_obj_52, c_coord_array_53, c_coord_array_54);
                            c_obj_57 = lv_obj_create(c_obj_50);
                            lv_obj_set_layout(c_obj_57, LV_LAYOUT_FLEX);
                            lv_obj_set_flex_flow(c_obj_57, LV_FLEX_FLOW_COLUMN);
                            lv_obj_add_style(c_obj_57, &c_container_2, 0);
                            lv_obj_set_width(c_obj_57, LV_PCT(100));
                            lv_obj_set_height(c_obj_57, LV_SIZE_CONTENT);
                            // Children of c_obj_57 (obj) with path prefix 'main'
                                c_bar_58 = lv_bar_create(c_obj_57);
                                lv_obj_set_width(c_bar_58, LV_PCT(100));
                                lv_obj_set_height(c_bar_58, 15);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_bar_58
                                lv_obj_set_style_margin_left(c_bar_58, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_bar_58
                                lv_obj_set_style_margin_right(c_bar_58, 15, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_58, &c_bar_indicator_3, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_58, &c_bar_indicator_3, LV_PART_INDICATOR);
                                lv_bar_set_value(c_bar_58, 65, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_bar_58
                                lv_obj_set_style_bg_color(c_bar_58, lv_color_hex(0x5DD555), LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_bar_58
                                lv_obj_set_style_bg_opa(c_bar_58, 255, LV_PART_MAIN);
                                c_scale_59 = lv_scale_create(c_obj_57);
                                lv_obj_set_width(c_scale_59, LV_PCT(100));
                                lv_obj_set_height(c_scale_59, 18);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_scale_59
                                lv_obj_set_style_margin_left(c_scale_59, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_scale_59
                                lv_obj_set_style_margin_right(c_scale_59, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_scale_59
                                lv_obj_set_style_text_font(c_scale_59, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t")), LV_PART_MAIN);
                        c_obj_60 = lv_obj_create(c_obj_49);
                        lv_obj_set_layout(c_obj_60, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_60, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_60, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_60, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_60, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_60
                        lv_obj_set_style_pad_right(c_obj_60, 0, LV_PART_MAIN);
                        lv_obj_set_flex_align(c_obj_60, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                        // Children of c_obj_60 (obj) with path prefix 'main'
                            c_label_61 = lv_label_create(c_obj_60);
                            lv_label_set_text(c_label_61, "MM/MIN");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_61
                            lv_obj_set_style_text_font(c_label_61, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_61, &c_border_top_btm_10, 0);
                            c_label_62 = lv_label_create(c_obj_60);
                            lv_label_set_text(c_label_62, "Feed Ovr");
                            c_label_63 = lv_label_create(c_obj_60);
                            lv_label_set_text(c_label_63, "100%%");
                            c_label_64 = lv_label_create(c_obj_60);
                            lv_label_set_text(c_label_64, "65%%");
                    // Using view component 'feed_rate_scale'
                    c_obj_65 = lv_obj_create(c_obj_48);
                    lv_obj_set_size(c_obj_65, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_65, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_65, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_65, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_65, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_65
                    lv_obj_set_style_pad_column(c_obj_65, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_65
                    lv_obj_set_style_pad_bottom(c_obj_65, 12, LV_PART_MAIN);
                    // Children of c_obj_65 (obj) with path prefix 'main'
                        c_obj_66 = lv_obj_create(c_obj_65);
                        lv_obj_set_layout(c_obj_66, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_66, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_66, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_66, LV_PCT(100));
                        lv_obj_set_height(c_obj_66, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_66, 1);
                        // Children of c_obj_66 (obj) with path prefix 'main'
                            c_label_67 = lv_label_create(c_obj_66);
                            lv_label_set_text(c_label_67, "SPEED");
                            lv_obj_set_height(c_label_67, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_67, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_67
                            lv_obj_set_style_text_font(c_label_67, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_67, &c_border_top_btm_10, 0);
                            c_grid_obj_68 = lv_obj_create(c_obj_66);
                            lv_obj_add_style(c_grid_obj_68, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_68, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_68, LV_SIZE_CONTENT);
                            // Children of c_grid_obj_68 (obj) with path prefix 'main'
                                c_label_71 = lv_label_create(c_grid_obj_68);
                                lv_label_set_text(c_label_71, "S");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_71
                                lv_obj_set_style_text_font(c_label_71, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_71, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                                lv_obj_set_height(c_label_71, LV_SIZE_CONTENT);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_label_71
                                lv_obj_set_style_pad_left(c_label_71, 10, LV_PART_MAIN);
                                c_label_72 = lv_label_create(c_grid_obj_68);
                                lv_obj_set_grid_cell(c_label_72, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_72
                                lv_obj_set_style_text_font(c_label_72, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_72, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_72, "1000");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_72
                                lv_obj_set_style_pad_right(c_label_72, 10, LV_PART_MAIN);
                            lv_obj_set_grid_dsc_array(c_grid_obj_68, c_coord_array_69, c_coord_array_70);
                            c_obj_73 = lv_obj_create(c_obj_66);
                            lv_obj_set_layout(c_obj_73, LV_LAYOUT_FLEX);
                            lv_obj_set_flex_flow(c_obj_73, LV_FLEX_FLOW_COLUMN);
                            lv_obj_add_style(c_obj_73, &c_container_2, 0);
                            lv_obj_set_width(c_obj_73, LV_PCT(100));
                            lv_obj_set_height(c_obj_73, LV_SIZE_CONTENT);
                            // Children of c_obj_73 (obj) with path prefix 'main'
                                c_bar_74 = lv_bar_create(c_obj_73);
                                lv_obj_set_width(c_bar_74, LV_PCT(100));
                                lv_obj_set_height(c_bar_74, 15);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_bar_74
                                lv_obj_set_style_margin_left(c_bar_74, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_bar_74
                                lv_obj_set_style_margin_right(c_bar_74, 15, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_74, &c_bar_indicator_3, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_74, &c_bar_indicator_3, LV_PART_INDICATOR);
                                lv_bar_set_value(c_bar_74, 65, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_bar_74
                                lv_obj_set_style_bg_color(c_bar_74, lv_color_hex(0x5DD555), LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_bar_74
                                lv_obj_set_style_bg_opa(c_bar_74, 255, LV_PART_MAIN);
                                c_scale_75 = lv_scale_create(c_obj_73);
                                lv_obj_set_width(c_scale_75, LV_PCT(100));
                                lv_obj_set_height(c_scale_75, 18);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_scale_75
                                lv_obj_set_style_margin_left(c_scale_75, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_scale_75
                                lv_obj_set_style_margin_right(c_scale_75, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_scale_75
                                lv_obj_set_style_text_font(c_scale_75, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t")), LV_PART_MAIN);
                        c_obj_76 = lv_obj_create(c_obj_65);
                        lv_obj_set_layout(c_obj_76, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_76, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_76, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_76, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_76, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_76
                        lv_obj_set_style_pad_right(c_obj_76, 0, LV_PART_MAIN);
                        lv_obj_set_flex_align(c_obj_76, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                        // Children of c_obj_76 (obj) with path prefix 'main'
                            c_label_77 = lv_label_create(c_obj_76);
                            lv_label_set_text(c_label_77, "/MIN");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_77
                            lv_obj_set_style_text_font(c_label_77, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_77, &c_border_top_btm_10, 0);
                            c_label_78 = lv_label_create(c_obj_76);
                            lv_label_set_text(c_label_78, "Speed Ovr");
                            c_label_79 = lv_label_create(c_obj_76);
                            lv_label_set_text(c_label_79, "100%%");
                            c_label_80 = lv_label_create(c_obj_76);
                            lv_label_set_text(c_label_80, "65%%");
                    // Using view component 'jog_feed'
                    c_obj_81 = lv_obj_create(c_obj_48);
                    lv_obj_set_size(c_obj_81, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_81, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_81, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_81, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_81, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_81
                    lv_obj_set_style_pad_column(c_obj_81, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_81
                    lv_obj_set_style_pad_bottom(c_obj_81, 0, LV_PART_MAIN);
                    // Children of c_obj_81 (obj) with path prefix 'main'
                        c_obj_82 = lv_obj_create(c_obj_81);
                        lv_obj_set_layout(c_obj_82, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_82, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_82, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_82, LV_PCT(100));
                        lv_obj_set_height(c_obj_82, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_82, 1);
                        // Children of c_obj_82 (obj) with path prefix 'main'
                            c_label_83 = lv_label_create(c_obj_82);
                            lv_label_set_text(c_label_83, "JOG");
                            lv_obj_set_height(c_label_83, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_83, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_83
                            lv_obj_set_style_text_font(c_label_83, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_83, &c_border_top_btm_10, 0);
                            c_grid_obj_84 = lv_obj_create(c_obj_82);
                            lv_obj_add_style(c_grid_obj_84, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_84, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_84, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_grid_obj_84
                            lv_obj_set_style_pad_left(c_grid_obj_84, 10, LV_PART_MAIN);
                            // Children of c_grid_obj_84 (obj) with path prefix 'main'
                                c_label_87 = lv_label_create(c_grid_obj_84);
                                lv_label_set_text(c_label_87, "XY");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_87
                                lv_obj_set_style_text_font(c_label_87, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_87, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
                                lv_obj_set_height(c_label_87, LV_SIZE_CONTENT);
                                c_label_88 = lv_label_create(c_grid_obj_84);
                                lv_obj_set_grid_cell(c_label_88, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_88
                                lv_obj_set_style_text_font(c_label_88, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_88, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_88, "10");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_88
                                lv_obj_set_style_pad_right(c_label_88, 10, LV_PART_MAIN);
                                lv_obj_add_style(c_label_88, &c_border_right_11, 0);
                                lv_obj_add_style(c_label_88, &c_indicator_yellow_8, 0);
                                c_label_89 = lv_label_create(c_grid_obj_84);
                                lv_label_set_text(c_label_89, "Z");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_89
                                lv_obj_set_style_text_font(c_label_89, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_89, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 0, 1);
                                lv_obj_set_height(c_label_89, LV_SIZE_CONTENT);
                                c_label_90 = lv_label_create(c_grid_obj_84);
                                lv_obj_set_grid_cell(c_label_90, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_90
                                lv_obj_set_style_text_font(c_label_90, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_90, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_90, " 5");
                                lv_obj_add_style(c_label_90, &c_indicator_yellow_8, 0);
                                lv_obj_add_event_cb(c_label_90, ((lv_event_cb_t)lvgl_json_get_registered_ptr("btn_clicked", "lv_event_cb_t")), LV_EVENT_CLICKED, NULL);
                            lv_obj_set_grid_dsc_array(c_grid_obj_84, c_coord_array_85, c_coord_array_86);
                        c_obj_91 = lv_obj_create(c_obj_81);
                        lv_obj_set_layout(c_obj_91, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_91, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_91, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_91, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_91, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_91
                        lv_obj_set_style_pad_right(c_obj_91, 0, LV_PART_MAIN);
                        // Children of c_obj_91 (obj) with path prefix 'main'
                            c_label_92 = lv_label_create(c_obj_91);
                            lv_label_set_text(c_label_92, "MM");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_92
                            lv_obj_set_style_text_font(c_label_92, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_92, &c_border_top_btm_10, 0);
                            c_label_93 = lv_label_create(c_obj_91);
                            lv_label_set_text(c_label_93, "Jog Ovr");
                            c_label_94 = lv_label_create(c_obj_91);
                            lv_label_set_text(c_label_94, "100%%");
    } else {
        // WARNING: 'with.obj' for c_tileview_14 resolved to NULL. 'do' block not applied.
    }
    // Using view component 'nav_action_button'
    c_dropdown_95 = lv_dropdown_create(parent_obj);
    lv_obj_add_style(c_dropdown_95, &c_action_button_13, 0);
    lv_dropdown_set_options(c_dropdown_95, " Jog\n Probe\n Status\n X\n y\n Z\n Off");
    lv_obj_align(c_dropdown_95, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_add_flag(c_dropdown_95, LV_OBJ_FLAG_FLOATING);
    lv_dropdown_set_text(c_dropdown_95, "");
    lv_dropdown_set_symbol(c_dropdown_95, NULL);
    lv_obj_move_foreground(c_dropdown_95);
    c_with_target_96 = lv_dropdown_get_list(c_dropdown_95);
    if (c_with_target_96) { // Check if 'with.obj' for c_dropdown_95 resolved
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_min_width on c_with_target_96
        lv_obj_set_style_min_width(c_with_target_96, 200, LV_PART_MAIN);
    } else {
        // WARNING: 'with.obj' for c_dropdown_95 resolved to NULL. 'do' block not applied.
    }
    // Using view component 'jog_action_button'
    c_dropdown_97 = lv_dropdown_create(parent_obj);
    lv_dropdown_set_options(c_dropdown_97, " Home\n Zero\n G54\n G55\n G56\n G57\n G58");
    lv_obj_add_style(c_dropdown_97, &c_action_button_13, 0);
    lv_obj_align(c_dropdown_97, LV_ALIGN_BOTTOM_LEFT, 90, -10);
    lv_obj_add_flag(c_dropdown_97, LV_OBJ_FLAG_FLOATING);
    lv_dropdown_set_text(c_dropdown_97, "");
    lv_dropdown_set_symbol(c_dropdown_97, NULL);
    lv_obj_move_foreground(c_dropdown_97);
    lvgl_json_register_ptr("jog_action_button", "lv_dropdown_t", (void*)c_dropdown_97);
    c_with_target_98 = lv_dropdown_get_list(c_dropdown_97);
    if (c_with_target_98) { // Check if 'with.obj' for c_dropdown_97 resolved
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_min_width on c_with_target_98
        lv_obj_set_style_min_width(c_with_target_98, 200, LV_PART_MAIN);
    } else {
        // WARNING: 'with.obj' for c_dropdown_97 resolved to NULL. 'do' block not applied.
    }
}