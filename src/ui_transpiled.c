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
    lv_obj_t * c_obj_16;
    lv_obj_t * c_obj_17;
    lv_obj_t * c_obj_18;
    lv_obj_t * c_obj_19;
    lv_obj_t * c_obj_20;
    lv_obj_t * c_label_21;
    lv_obj_t * c_label_22;
    lv_obj_t * c_obj_23;
    lv_obj_t * c_label_24;
    lv_obj_t * c_label_25;
    lv_obj_t * c_label_26;
    lv_obj_t * c_label_27;
    lv_obj_t * c_obj_28;
    lv_obj_t * c_obj_29;
    lv_obj_t * c_label_30;
    lv_obj_t * c_label_31;
    lv_obj_t * c_obj_32;
    lv_obj_t * c_label_33;
    lv_obj_t * c_label_34;
    lv_obj_t * c_label_35;
    lv_obj_t * c_label_36;
    lv_obj_t * c_obj_37;
    lv_obj_t * c_obj_38;
    lv_obj_t * c_label_39;
    lv_obj_t * c_label_40;
    lv_obj_t * c_obj_41;
    lv_obj_t * c_label_42;
    lv_obj_t * c_label_43;
    lv_obj_t * c_label_44;
    lv_obj_t * c_label_45;
    lv_obj_t * c_obj_46;
    lv_obj_t * c_obj_47;
    lv_obj_t * c_obj_48;
    lv_obj_t * c_label_49;
    lv_obj_t * c_grid_obj_50;
    static const lv_coord_t c_coord_array_51[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_52[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_53;
    lv_obj_t * c_label_54;
    lv_obj_t * c_obj_55;
    lv_obj_t * c_bar_56;
    lv_obj_t * c_scale_57;
    lv_obj_t * c_obj_58;
    lv_obj_t * c_label_59;
    lv_obj_t * c_label_60;
    lv_obj_t * c_label_61;
    lv_obj_t * c_label_62;
    lv_obj_t * c_obj_63;
    lv_obj_t * c_obj_64;
    lv_obj_t * c_label_65;
    lv_obj_t * c_grid_obj_66;
    static const lv_coord_t c_coord_array_67[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_68[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_69;
    lv_obj_t * c_label_70;
    lv_obj_t * c_obj_71;
    lv_obj_t * c_bar_72;
    lv_obj_t * c_scale_73;
    lv_obj_t * c_obj_74;
    lv_obj_t * c_label_75;
    lv_obj_t * c_label_76;
    lv_obj_t * c_label_77;
    lv_obj_t * c_label_78;
    lv_obj_t * c_obj_79;
    lv_obj_t * c_obj_80;
    lv_obj_t * c_label_81;
    lv_obj_t * c_grid_obj_82;
    static const lv_coord_t c_coord_array_83[] = { 35, 45, 20, 40, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_84[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_85;
    lv_obj_t * c_label_86;
    lv_obj_t * c_label_87;
    lv_obj_t * c_label_88;
    lv_obj_t * c_obj_89;
    lv_obj_t * c_label_90;
    lv_obj_t * c_label_91;
    lv_obj_t * c_label_92;
    lv_obj_t * c_with_target_93;
    lv_obj_t * c_obj_94;
    lv_obj_t * c_obj_95;
    lv_obj_t * c_obj_96;
    lv_obj_t * c_obj_97;
    lv_obj_t * c_obj_98;
    lv_obj_t * c_label_99;
    lv_obj_t * c_label_100;
    lv_obj_t * c_obj_101;
    lv_obj_t * c_label_102;
    lv_obj_t * c_label_103;
    lv_obj_t * c_label_104;
    lv_obj_t * c_label_105;
    lv_obj_t * c_obj_106;
    lv_obj_t * c_obj_107;
    lv_obj_t * c_label_108;
    lv_obj_t * c_label_109;
    lv_obj_t * c_obj_110;
    lv_obj_t * c_label_111;
    lv_obj_t * c_label_112;
    lv_obj_t * c_label_113;
    lv_obj_t * c_label_114;
    lv_obj_t * c_obj_115;
    lv_obj_t * c_obj_116;
    lv_obj_t * c_label_117;
    lv_obj_t * c_label_118;
    lv_obj_t * c_obj_119;
    lv_obj_t * c_label_120;
    lv_obj_t * c_label_121;
    lv_obj_t * c_label_122;
    lv_obj_t * c_label_123;
    lv_obj_t * c_obj_124;
    lv_obj_t * c_obj_125;
    lv_obj_t * c_obj_126;
    lv_obj_t * c_label_127;
    lv_obj_t * c_grid_obj_128;
    static const lv_coord_t c_coord_array_129[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_130[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_131;
    lv_obj_t * c_label_132;
    lv_obj_t * c_obj_133;
    lv_obj_t * c_bar_134;
    lv_obj_t * c_scale_135;
    lv_obj_t * c_obj_136;
    lv_obj_t * c_label_137;
    lv_obj_t * c_label_138;
    lv_obj_t * c_label_139;
    lv_obj_t * c_label_140;
    lv_obj_t * c_obj_141;
    lv_obj_t * c_obj_142;
    lv_obj_t * c_label_143;
    lv_obj_t * c_grid_obj_144;
    static const lv_coord_t c_coord_array_145[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_146[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_147;
    lv_obj_t * c_label_148;
    lv_obj_t * c_obj_149;
    lv_obj_t * c_bar_150;
    lv_obj_t * c_scale_151;
    lv_obj_t * c_obj_152;
    lv_obj_t * c_label_153;
    lv_obj_t * c_label_154;
    lv_obj_t * c_label_155;
    lv_obj_t * c_label_156;
    lv_obj_t * c_obj_157;
    lv_obj_t * c_obj_158;
    lv_obj_t * c_label_159;
    lv_obj_t * c_grid_obj_160;
    static const lv_coord_t c_coord_array_161[] = { 35, 45, 20, 40, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_162[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_163;
    lv_obj_t * c_label_164;
    lv_obj_t * c_label_165;
    lv_obj_t * c_label_166;
    lv_obj_t * c_obj_167;
    lv_obj_t * c_label_168;
    lv_obj_t * c_label_169;
    lv_obj_t * c_label_170;
    lv_obj_t * c_dropdown_171;
    lv_obj_t * c_with_target_172;
    lv_obj_t * c_dropdown_173;
    lv_obj_t * c_with_target_174;
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
    c_with_target_15 = lv_tileview_add_tile(c_tileview_14, 0, 0, LV_DIR_RIGHT);
    if (c_with_target_15) { // Check if 'with.obj' for c_tileview_14 resolved
        lv_obj_add_style(c_with_target_15, &c_container_2, 0);
        // Children of c_with_target_15 (obj) with path prefix 'None'
            // Using view component 'jog_view'
            c_obj_16 = lv_obj_create(c_with_target_15);
            lv_obj_set_layout(c_obj_16, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(c_obj_16, LV_FLEX_FLOW_ROW);
            lv_obj_add_style(c_obj_16, &c_container_2, 0);
            lv_obj_set_size(c_obj_16, LV_PCT(100), 320);
            lvgl_json_register_ptr("main", "lv_obj_t", (void*)c_obj_16);
            // Children of c_obj_16 (obj) with path prefix 'main'
                c_obj_17 = lv_obj_create(c_obj_16);
                lv_obj_set_layout(c_obj_17, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_17, LV_FLEX_FLOW_COLUMN);
                lv_obj_add_style(c_obj_17, &c_container_2, 0);
                lv_obj_set_height(c_obj_17, LV_PCT(100));
                lv_obj_set_flex_grow(c_obj_17, 60);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_side on c_obj_17
                lv_obj_set_style_border_side(c_obj_17, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_width on c_obj_17
                lv_obj_set_style_border_width(c_obj_17, 2, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius on c_obj_17
                lv_obj_set_style_radius(c_obj_17, 0, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_17
                lv_obj_set_style_border_color(c_obj_17, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_17
                lv_obj_set_style_border_opa(c_obj_17, 90, LV_PART_MAIN);
                // Children of c_obj_17 (obj) with path prefix 'main'
                    // Using view component 'xyz_axis_pos_display'
                    c_obj_18 = lv_obj_create(c_obj_17);
                    lv_obj_set_layout(c_obj_18, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_18, LV_FLEX_FLOW_COLUMN);
                    lv_obj_add_style(c_obj_18, &c_container_2, 0);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius on c_obj_18
                    lv_obj_set_style_radius(c_obj_18, 0, LV_PART_MAIN);
                    lv_obj_set_size(c_obj_18, LV_PCT(100), LV_PCT(100));
                    // Children of c_obj_18 (obj) with path prefix 'main'
                        // Using view component 'axis_pos_display'
                        c_obj_19 = lv_obj_create(c_obj_18);
                        lv_obj_add_style(c_obj_19, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_19, &c_container_2, 0);
                        lv_obj_set_size(c_obj_19, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_19
                        lv_obj_set_style_pad_all(c_obj_19, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_19
                        lv_obj_set_style_pad_bottom(c_obj_19, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_19
                        lv_obj_set_style_border_color(c_obj_19, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_19
                        lv_obj_set_style_border_opa(c_obj_19, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_19
                        lv_obj_set_style_margin_all(c_obj_19, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_x", "lv_obj_t", (void*)c_obj_19);
                        // Children of c_obj_19 (obj) with path prefix 'main:axis_pos_x'
                            c_obj_20 = lv_obj_create(c_obj_19);
                            lv_obj_add_style(c_obj_20, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_20, &c_container_2, 0);
                            lv_obj_set_size(c_obj_20, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_20 (obj) with path prefix 'main:axis_pos_x'
                                c_label_21 = lv_label_create(c_obj_20);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_21
                                lv_obj_set_style_text_font(c_label_21, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_21, "X");
                                lv_obj_set_width(c_label_21, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_21, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_21
                                lv_obj_set_style_border_color(c_label_21, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_22 = lv_label_create(c_obj_20);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_22
                                lv_obj_set_style_text_font(c_label_22, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_22, "11.000");
                                lv_obj_set_flex_grow(c_label_22, 1);
                                lv_obj_add_style(c_label_22, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_22
                                lv_obj_set_style_text_align(c_label_22, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_23 = lv_obj_create(c_obj_19);
                            lv_obj_add_style(c_obj_23, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_23, &c_container_2, 0);
                            lv_obj_set_size(c_obj_23, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_23 (obj) with path prefix 'main:axis_pos_x'
                                c_label_24 = lv_label_create(c_obj_23);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_24
                                lv_obj_set_style_text_font(c_label_24, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_24, "51.000");
                                lv_obj_set_flex_grow(c_label_24, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_24
                                lv_obj_set_style_text_align(c_label_24, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_24, &c_indicator_yellow_8, 0);
                                c_label_25 = lv_label_create(c_obj_23);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_25
                                lv_obj_set_style_text_font(c_label_25, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_25, "");
                                lv_obj_set_width(c_label_25, 14);
                                c_label_26 = lv_label_create(c_obj_23);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_26
                                lv_obj_set_style_text_font(c_label_26, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_26, "2.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_26
                                lv_obj_set_style_text_align(c_label_26, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_26, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_26, 1);
                                c_label_27 = lv_label_create(c_obj_23);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_27
                                lv_obj_set_style_text_font(c_label_27, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_27, "");
                                lv_obj_set_width(c_label_27, 14);
                        // Using view component 'axis_pos_display'
                        c_obj_28 = lv_obj_create(c_obj_18);
                        lv_obj_add_style(c_obj_28, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_28, &c_container_2, 0);
                        lv_obj_set_size(c_obj_28, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_28
                        lv_obj_set_style_pad_all(c_obj_28, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_28
                        lv_obj_set_style_pad_bottom(c_obj_28, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_28
                        lv_obj_set_style_border_color(c_obj_28, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_28
                        lv_obj_set_style_border_opa(c_obj_28, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_28
                        lv_obj_set_style_margin_all(c_obj_28, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_y", "lv_obj_t", (void*)c_obj_28);
                        // Children of c_obj_28 (obj) with path prefix 'main:axis_pos_y'
                            c_obj_29 = lv_obj_create(c_obj_28);
                            lv_obj_add_style(c_obj_29, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_29, &c_container_2, 0);
                            lv_obj_set_size(c_obj_29, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_29 (obj) with path prefix 'main:axis_pos_y'
                                c_label_30 = lv_label_create(c_obj_29);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_30
                                lv_obj_set_style_text_font(c_label_30, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_30, "Y");
                                lv_obj_set_width(c_label_30, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_30, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_30
                                lv_obj_set_style_border_color(c_label_30, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_31 = lv_label_create(c_obj_29);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_31
                                lv_obj_set_style_text_font(c_label_31, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_31, "22.000");
                                lv_obj_set_flex_grow(c_label_31, 1);
                                lv_obj_add_style(c_label_31, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_31
                                lv_obj_set_style_text_align(c_label_31, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_32 = lv_obj_create(c_obj_28);
                            lv_obj_add_style(c_obj_32, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_32, &c_container_2, 0);
                            lv_obj_set_size(c_obj_32, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_32 (obj) with path prefix 'main:axis_pos_y'
                                c_label_33 = lv_label_create(c_obj_32);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_33
                                lv_obj_set_style_text_font(c_label_33, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_33, "72.000");
                                lv_obj_set_flex_grow(c_label_33, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_33
                                lv_obj_set_style_text_align(c_label_33, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_33, &c_indicator_yellow_8, 0);
                                c_label_34 = lv_label_create(c_obj_32);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_34
                                lv_obj_set_style_text_font(c_label_34, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_34, "");
                                lv_obj_set_width(c_label_34, 14);
                                c_label_35 = lv_label_create(c_obj_32);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_35
                                lv_obj_set_style_text_font(c_label_35, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_35, "-12.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_35
                                lv_obj_set_style_text_align(c_label_35, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_35, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_35, 1);
                                c_label_36 = lv_label_create(c_obj_32);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_36
                                lv_obj_set_style_text_font(c_label_36, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_36, "");
                                lv_obj_set_width(c_label_36, 14);
                        // Using view component 'axis_pos_display'
                        c_obj_37 = lv_obj_create(c_obj_18);
                        lv_obj_add_style(c_obj_37, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_37, &c_container_2, 0);
                        lv_obj_set_size(c_obj_37, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_37
                        lv_obj_set_style_pad_all(c_obj_37, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_37
                        lv_obj_set_style_pad_bottom(c_obj_37, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_37
                        lv_obj_set_style_border_color(c_obj_37, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_37
                        lv_obj_set_style_border_opa(c_obj_37, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_37
                        lv_obj_set_style_margin_all(c_obj_37, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_z", "lv_obj_t", (void*)c_obj_37);
                        // Children of c_obj_37 (obj) with path prefix 'main:axis_pos_z'
                            c_obj_38 = lv_obj_create(c_obj_37);
                            lv_obj_add_style(c_obj_38, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_38, &c_container_2, 0);
                            lv_obj_set_size(c_obj_38, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_38 (obj) with path prefix 'main:axis_pos_z'
                                c_label_39 = lv_label_create(c_obj_38);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_39
                                lv_obj_set_style_text_font(c_label_39, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_39, "Z");
                                lv_obj_set_width(c_label_39, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_39, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_39
                                lv_obj_set_style_border_color(c_label_39, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_40 = lv_label_create(c_obj_38);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_40
                                lv_obj_set_style_text_font(c_label_40, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_40, "1.000");
                                lv_obj_set_flex_grow(c_label_40, 1);
                                lv_obj_add_style(c_label_40, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_40
                                lv_obj_set_style_text_align(c_label_40, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_41 = lv_obj_create(c_obj_37);
                            lv_obj_add_style(c_obj_41, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_41, &c_container_2, 0);
                            lv_obj_set_size(c_obj_41, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_41 (obj) with path prefix 'main:axis_pos_z'
                                c_label_42 = lv_label_create(c_obj_41);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_42
                                lv_obj_set_style_text_font(c_label_42, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_42, "1.000");
                                lv_obj_set_flex_grow(c_label_42, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_42
                                lv_obj_set_style_text_align(c_label_42, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_42, &c_indicator_yellow_8, 0);
                                c_label_43 = lv_label_create(c_obj_41);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_43
                                lv_obj_set_style_text_font(c_label_43, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_43, "");
                                lv_obj_set_width(c_label_43, 14);
                                c_label_44 = lv_label_create(c_obj_41);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_44
                                lv_obj_set_style_text_font(c_label_44, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_44, "0.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_44
                                lv_obj_set_style_text_align(c_label_44, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_44, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_44, 1);
                                c_label_45 = lv_label_create(c_obj_41);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_45
                                lv_obj_set_style_text_font(c_label_45, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_45, "");
                                lv_obj_set_width(c_label_45, 14);
                c_obj_46 = lv_obj_create(c_obj_16);
                lv_obj_set_layout(c_obj_46, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_46, LV_FLEX_FLOW_COLUMN);
                lv_obj_add_style(c_obj_46, &c_container_2, 0);
                lv_obj_set_height(c_obj_46, LV_PCT(100));
                lv_obj_set_flex_grow(c_obj_46, 45);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_top on c_obj_46
                lv_obj_set_style_pad_top(c_obj_46, 5, LV_PART_MAIN);
                // Children of c_obj_46 (obj) with path prefix 'main'
                    // Using view component 'feed_rate_scale'
                    c_obj_47 = lv_obj_create(c_obj_46);
                    lv_obj_set_size(c_obj_47, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_47, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_47, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_47, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_47, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_47
                    lv_obj_set_style_pad_column(c_obj_47, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_47
                    lv_obj_set_style_pad_bottom(c_obj_47, 12, LV_PART_MAIN);
                    // Children of c_obj_47 (obj) with path prefix 'main'
                        c_obj_48 = lv_obj_create(c_obj_47);
                        lv_obj_set_layout(c_obj_48, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_48, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_48, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_48, LV_PCT(100));
                        lv_obj_set_height(c_obj_48, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_48, 1);
                        // Children of c_obj_48 (obj) with path prefix 'main'
                            c_label_49 = lv_label_create(c_obj_48);
                            lv_label_set_text(c_label_49, "FEED");
                            lv_obj_set_height(c_label_49, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_49, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_49
                            lv_obj_set_style_text_font(c_label_49, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_49, &c_border_top_btm_10, 0);
                            c_grid_obj_50 = lv_obj_create(c_obj_48);
                            lv_obj_add_style(c_grid_obj_50, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_50, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_50, LV_SIZE_CONTENT);
                            // Children of c_grid_obj_50 (obj) with path prefix 'main'
                                c_label_53 = lv_label_create(c_grid_obj_50);
                                lv_label_set_text(c_label_53, "F");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_53
                                lv_obj_set_style_text_font(c_label_53, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_53, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                                lv_obj_set_height(c_label_53, LV_SIZE_CONTENT);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_label_53
                                lv_obj_set_style_pad_left(c_label_53, 10, LV_PART_MAIN);
                                c_label_54 = lv_label_create(c_grid_obj_50);
                                lv_obj_set_grid_cell(c_label_54, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_54
                                lv_obj_set_style_text_font(c_label_54, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_54, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_54, "1000");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_54
                                lv_obj_set_style_pad_right(c_label_54, 10, LV_PART_MAIN);
                            lv_obj_set_grid_dsc_array(c_grid_obj_50, c_coord_array_51, c_coord_array_52);
                            c_obj_55 = lv_obj_create(c_obj_48);
                            lv_obj_set_layout(c_obj_55, LV_LAYOUT_FLEX);
                            lv_obj_set_flex_flow(c_obj_55, LV_FLEX_FLOW_COLUMN);
                            lv_obj_add_style(c_obj_55, &c_container_2, 0);
                            lv_obj_set_width(c_obj_55, LV_PCT(100));
                            lv_obj_set_height(c_obj_55, LV_SIZE_CONTENT);
                            // Children of c_obj_55 (obj) with path prefix 'main'
                                c_bar_56 = lv_bar_create(c_obj_55);
                                lv_obj_set_width(c_bar_56, LV_PCT(100));
                                lv_obj_set_height(c_bar_56, 15);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_bar_56
                                lv_obj_set_style_margin_left(c_bar_56, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_bar_56
                                lv_obj_set_style_margin_right(c_bar_56, 15, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_56, &c_bar_indicator_3, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_56, &c_bar_indicator_3, LV_PART_INDICATOR);
                                lv_bar_set_value(c_bar_56, 65, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_bar_56
                                lv_obj_set_style_bg_color(c_bar_56, lv_color_hex(0x5DD555), LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_bar_56
                                lv_obj_set_style_bg_opa(c_bar_56, 255, LV_PART_MAIN);
                                c_scale_57 = lv_scale_create(c_obj_55);
                                lv_obj_set_width(c_scale_57, LV_PCT(100));
                                lv_obj_set_height(c_scale_57, 18);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_scale_57
                                lv_obj_set_style_margin_left(c_scale_57, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_scale_57
                                lv_obj_set_style_margin_right(c_scale_57, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_scale_57
                                lv_obj_set_style_text_font(c_scale_57, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t")), LV_PART_MAIN);
                        c_obj_58 = lv_obj_create(c_obj_47);
                        lv_obj_set_layout(c_obj_58, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_58, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_58, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_58, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_58, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_58
                        lv_obj_set_style_pad_right(c_obj_58, 0, LV_PART_MAIN);
                        lv_obj_set_flex_align(c_obj_58, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                        // Children of c_obj_58 (obj) with path prefix 'main'
                            c_label_59 = lv_label_create(c_obj_58);
                            lv_label_set_text(c_label_59, "MM/MIN");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_59
                            lv_obj_set_style_text_font(c_label_59, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_59, &c_border_top_btm_10, 0);
                            c_label_60 = lv_label_create(c_obj_58);
                            lv_label_set_text(c_label_60, "Feed Ovr");
                            c_label_61 = lv_label_create(c_obj_58);
                            lv_label_set_text(c_label_61, "100%%");
                            c_label_62 = lv_label_create(c_obj_58);
                            lv_label_set_text(c_label_62, "65%%");
                    // Using view component 'feed_rate_scale'
                    c_obj_63 = lv_obj_create(c_obj_46);
                    lv_obj_set_size(c_obj_63, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_63, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_63, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_63, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_63, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_63
                    lv_obj_set_style_pad_column(c_obj_63, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_63
                    lv_obj_set_style_pad_bottom(c_obj_63, 12, LV_PART_MAIN);
                    // Children of c_obj_63 (obj) with path prefix 'main'
                        c_obj_64 = lv_obj_create(c_obj_63);
                        lv_obj_set_layout(c_obj_64, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_64, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_64, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_64, LV_PCT(100));
                        lv_obj_set_height(c_obj_64, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_64, 1);
                        // Children of c_obj_64 (obj) with path prefix 'main'
                            c_label_65 = lv_label_create(c_obj_64);
                            lv_label_set_text(c_label_65, "SPEED");
                            lv_obj_set_height(c_label_65, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_65, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_65
                            lv_obj_set_style_text_font(c_label_65, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_65, &c_border_top_btm_10, 0);
                            c_grid_obj_66 = lv_obj_create(c_obj_64);
                            lv_obj_add_style(c_grid_obj_66, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_66, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_66, LV_SIZE_CONTENT);
                            // Children of c_grid_obj_66 (obj) with path prefix 'main'
                                c_label_69 = lv_label_create(c_grid_obj_66);
                                lv_label_set_text(c_label_69, "S");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_69
                                lv_obj_set_style_text_font(c_label_69, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_69, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                                lv_obj_set_height(c_label_69, LV_SIZE_CONTENT);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_label_69
                                lv_obj_set_style_pad_left(c_label_69, 10, LV_PART_MAIN);
                                c_label_70 = lv_label_create(c_grid_obj_66);
                                lv_obj_set_grid_cell(c_label_70, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_70
                                lv_obj_set_style_text_font(c_label_70, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_70, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_70, "1000");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_70
                                lv_obj_set_style_pad_right(c_label_70, 10, LV_PART_MAIN);
                            lv_obj_set_grid_dsc_array(c_grid_obj_66, c_coord_array_67, c_coord_array_68);
                            c_obj_71 = lv_obj_create(c_obj_64);
                            lv_obj_set_layout(c_obj_71, LV_LAYOUT_FLEX);
                            lv_obj_set_flex_flow(c_obj_71, LV_FLEX_FLOW_COLUMN);
                            lv_obj_add_style(c_obj_71, &c_container_2, 0);
                            lv_obj_set_width(c_obj_71, LV_PCT(100));
                            lv_obj_set_height(c_obj_71, LV_SIZE_CONTENT);
                            // Children of c_obj_71 (obj) with path prefix 'main'
                                c_bar_72 = lv_bar_create(c_obj_71);
                                lv_obj_set_width(c_bar_72, LV_PCT(100));
                                lv_obj_set_height(c_bar_72, 15);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_bar_72
                                lv_obj_set_style_margin_left(c_bar_72, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_bar_72
                                lv_obj_set_style_margin_right(c_bar_72, 15, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_72, &c_bar_indicator_3, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_72, &c_bar_indicator_3, LV_PART_INDICATOR);
                                lv_bar_set_value(c_bar_72, 65, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_bar_72
                                lv_obj_set_style_bg_color(c_bar_72, lv_color_hex(0x5DD555), LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_bar_72
                                lv_obj_set_style_bg_opa(c_bar_72, 255, LV_PART_MAIN);
                                c_scale_73 = lv_scale_create(c_obj_71);
                                lv_obj_set_width(c_scale_73, LV_PCT(100));
                                lv_obj_set_height(c_scale_73, 18);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_scale_73
                                lv_obj_set_style_margin_left(c_scale_73, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_scale_73
                                lv_obj_set_style_margin_right(c_scale_73, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_scale_73
                                lv_obj_set_style_text_font(c_scale_73, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t")), LV_PART_MAIN);
                        c_obj_74 = lv_obj_create(c_obj_63);
                        lv_obj_set_layout(c_obj_74, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_74, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_74, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_74, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_74, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_74
                        lv_obj_set_style_pad_right(c_obj_74, 0, LV_PART_MAIN);
                        lv_obj_set_flex_align(c_obj_74, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                        // Children of c_obj_74 (obj) with path prefix 'main'
                            c_label_75 = lv_label_create(c_obj_74);
                            lv_label_set_text(c_label_75, "/MIN");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_75
                            lv_obj_set_style_text_font(c_label_75, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_75, &c_border_top_btm_10, 0);
                            c_label_76 = lv_label_create(c_obj_74);
                            lv_label_set_text(c_label_76, "Speed Ovr");
                            c_label_77 = lv_label_create(c_obj_74);
                            lv_label_set_text(c_label_77, "100%%");
                            c_label_78 = lv_label_create(c_obj_74);
                            lv_label_set_text(c_label_78, "65%%");
                    // Using view component 'jog_feed'
                    c_obj_79 = lv_obj_create(c_obj_46);
                    lv_obj_set_size(c_obj_79, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_79, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_79, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_79, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_79, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_79
                    lv_obj_set_style_pad_column(c_obj_79, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_79
                    lv_obj_set_style_pad_bottom(c_obj_79, 0, LV_PART_MAIN);
                    // Children of c_obj_79 (obj) with path prefix 'main'
                        c_obj_80 = lv_obj_create(c_obj_79);
                        lv_obj_set_layout(c_obj_80, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_80, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_80, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_80, LV_PCT(100));
                        lv_obj_set_height(c_obj_80, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_80, 1);
                        // Children of c_obj_80 (obj) with path prefix 'main'
                            c_label_81 = lv_label_create(c_obj_80);
                            lv_label_set_text(c_label_81, "JOG");
                            lv_obj_set_height(c_label_81, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_81, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_81
                            lv_obj_set_style_text_font(c_label_81, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_81, &c_border_top_btm_10, 0);
                            c_grid_obj_82 = lv_obj_create(c_obj_80);
                            lv_obj_add_style(c_grid_obj_82, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_82, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_82, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_grid_obj_82
                            lv_obj_set_style_pad_left(c_grid_obj_82, 10, LV_PART_MAIN);
                            // Children of c_grid_obj_82 (obj) with path prefix 'main'
                                c_label_85 = lv_label_create(c_grid_obj_82);
                                lv_label_set_text(c_label_85, "XY");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_85
                                lv_obj_set_style_text_font(c_label_85, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_85, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
                                lv_obj_set_height(c_label_85, LV_SIZE_CONTENT);
                                c_label_86 = lv_label_create(c_grid_obj_82);
                                lv_obj_set_grid_cell(c_label_86, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_86
                                lv_obj_set_style_text_font(c_label_86, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_86, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_86, "10");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_86
                                lv_obj_set_style_pad_right(c_label_86, 10, LV_PART_MAIN);
                                lv_obj_add_style(c_label_86, &c_border_right_11, 0);
                                lv_obj_add_style(c_label_86, &c_indicator_yellow_8, 0);
                                c_label_87 = lv_label_create(c_grid_obj_82);
                                lv_label_set_text(c_label_87, "Z");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_87
                                lv_obj_set_style_text_font(c_label_87, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_87, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 0, 1);
                                lv_obj_set_height(c_label_87, LV_SIZE_CONTENT);
                                c_label_88 = lv_label_create(c_grid_obj_82);
                                lv_obj_set_grid_cell(c_label_88, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_88
                                lv_obj_set_style_text_font(c_label_88, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_88, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_88, " 5");
                                lv_obj_add_style(c_label_88, &c_indicator_yellow_8, 0);
                                lv_obj_add_event_cb(c_label_88, ((lv_event_cb_t)lvgl_json_get_registered_ptr("btn_clicked", "lv_event_cb_t")), LV_EVENT_CLICKED, NULL);
                            lv_obj_set_grid_dsc_array(c_grid_obj_82, c_coord_array_83, c_coord_array_84);
                        c_obj_89 = lv_obj_create(c_obj_79);
                        lv_obj_set_layout(c_obj_89, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_89, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_89, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_89, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_89, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_89
                        lv_obj_set_style_pad_right(c_obj_89, 0, LV_PART_MAIN);
                        // Children of c_obj_89 (obj) with path prefix 'main'
                            c_label_90 = lv_label_create(c_obj_89);
                            lv_label_set_text(c_label_90, "MM");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_90
                            lv_obj_set_style_text_font(c_label_90, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_90, &c_border_top_btm_10, 0);
                            c_label_91 = lv_label_create(c_obj_89);
                            lv_label_set_text(c_label_91, "Jog Ovr");
                            c_label_92 = lv_label_create(c_obj_89);
                            lv_label_set_text(c_label_92, "100%%");
    } else {
        // WARNING: 'with.obj' for c_tileview_14 resolved to NULL. 'do' block not applied.
    }
    c_with_target_93 = lv_tileview_add_tile(c_tileview_14, 1, 0, LV_DIR_LEFT);
    if (c_with_target_93) { // Check if 'with.obj' for c_tileview_14 resolved
        lv_obj_add_style(c_with_target_93, &c_container_2, 0);
        // Children of c_with_target_93 (obj) with path prefix 'None'
            // Using view component 'jog_view'
            c_obj_94 = lv_obj_create(c_with_target_93);
            lv_obj_set_layout(c_obj_94, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(c_obj_94, LV_FLEX_FLOW_ROW);
            lv_obj_add_style(c_obj_94, &c_container_2, 0);
            lv_obj_set_size(c_obj_94, LV_PCT(100), 320);
            lvgl_json_register_ptr("main", "lv_obj_t", (void*)c_obj_94);
            // Children of c_obj_94 (obj) with path prefix 'main'
                c_obj_95 = lv_obj_create(c_obj_94);
                lv_obj_set_layout(c_obj_95, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_95, LV_FLEX_FLOW_COLUMN);
                lv_obj_add_style(c_obj_95, &c_container_2, 0);
                lv_obj_set_height(c_obj_95, LV_PCT(100));
                lv_obj_set_flex_grow(c_obj_95, 60);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_side on c_obj_95
                lv_obj_set_style_border_side(c_obj_95, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_width on c_obj_95
                lv_obj_set_style_border_width(c_obj_95, 2, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius on c_obj_95
                lv_obj_set_style_radius(c_obj_95, 0, LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_95
                lv_obj_set_style_border_color(c_obj_95, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_95
                lv_obj_set_style_border_opa(c_obj_95, 90, LV_PART_MAIN);
                // Children of c_obj_95 (obj) with path prefix 'main'
                    // Using view component 'xyz_axis_pos_display'
                    c_obj_96 = lv_obj_create(c_obj_95);
                    lv_obj_set_layout(c_obj_96, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_96, LV_FLEX_FLOW_COLUMN);
                    lv_obj_add_style(c_obj_96, &c_container_2, 0);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius on c_obj_96
                    lv_obj_set_style_radius(c_obj_96, 0, LV_PART_MAIN);
                    lv_obj_set_size(c_obj_96, LV_PCT(100), LV_PCT(100));
                    // Children of c_obj_96 (obj) with path prefix 'main'
                        // Using view component 'axis_pos_display'
                        c_obj_97 = lv_obj_create(c_obj_96);
                        lv_obj_add_style(c_obj_97, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_97, &c_container_2, 0);
                        lv_obj_set_size(c_obj_97, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_97
                        lv_obj_set_style_pad_all(c_obj_97, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_97
                        lv_obj_set_style_pad_bottom(c_obj_97, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_97
                        lv_obj_set_style_border_color(c_obj_97, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_97
                        lv_obj_set_style_border_opa(c_obj_97, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_97
                        lv_obj_set_style_margin_all(c_obj_97, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_x", "lv_obj_t", (void*)c_obj_97);
                        // Children of c_obj_97 (obj) with path prefix 'main:axis_pos_x'
                            c_obj_98 = lv_obj_create(c_obj_97);
                            lv_obj_add_style(c_obj_98, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_98, &c_container_2, 0);
                            lv_obj_set_size(c_obj_98, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_98 (obj) with path prefix 'main:axis_pos_x'
                                c_label_99 = lv_label_create(c_obj_98);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_99
                                lv_obj_set_style_text_font(c_label_99, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_99, "X");
                                lv_obj_set_width(c_label_99, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_99, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_99
                                lv_obj_set_style_border_color(c_label_99, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_100 = lv_label_create(c_obj_98);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_100
                                lv_obj_set_style_text_font(c_label_100, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_100, "11.000");
                                lv_obj_set_flex_grow(c_label_100, 1);
                                lv_obj_add_style(c_label_100, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_100
                                lv_obj_set_style_text_align(c_label_100, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_101 = lv_obj_create(c_obj_97);
                            lv_obj_add_style(c_obj_101, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_101, &c_container_2, 0);
                            lv_obj_set_size(c_obj_101, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_101 (obj) with path prefix 'main:axis_pos_x'
                                c_label_102 = lv_label_create(c_obj_101);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_102
                                lv_obj_set_style_text_font(c_label_102, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_102, "51.000");
                                lv_obj_set_flex_grow(c_label_102, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_102
                                lv_obj_set_style_text_align(c_label_102, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_102, &c_indicator_yellow_8, 0);
                                c_label_103 = lv_label_create(c_obj_101);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_103
                                lv_obj_set_style_text_font(c_label_103, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_103, "");
                                lv_obj_set_width(c_label_103, 14);
                                c_label_104 = lv_label_create(c_obj_101);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_104
                                lv_obj_set_style_text_font(c_label_104, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_104, "2.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_104
                                lv_obj_set_style_text_align(c_label_104, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_104, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_104, 1);
                                c_label_105 = lv_label_create(c_obj_101);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_105
                                lv_obj_set_style_text_font(c_label_105, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_105, "");
                                lv_obj_set_width(c_label_105, 14);
                        // Using view component 'axis_pos_display'
                        c_obj_106 = lv_obj_create(c_obj_96);
                        lv_obj_add_style(c_obj_106, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_106, &c_container_2, 0);
                        lv_obj_set_size(c_obj_106, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_106
                        lv_obj_set_style_pad_all(c_obj_106, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_106
                        lv_obj_set_style_pad_bottom(c_obj_106, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_106
                        lv_obj_set_style_border_color(c_obj_106, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_106
                        lv_obj_set_style_border_opa(c_obj_106, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_106
                        lv_obj_set_style_margin_all(c_obj_106, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_y", "lv_obj_t", (void*)c_obj_106);
                        // Children of c_obj_106 (obj) with path prefix 'main:axis_pos_y'
                            c_obj_107 = lv_obj_create(c_obj_106);
                            lv_obj_add_style(c_obj_107, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_107, &c_container_2, 0);
                            lv_obj_set_size(c_obj_107, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_107 (obj) with path prefix 'main:axis_pos_y'
                                c_label_108 = lv_label_create(c_obj_107);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_108
                                lv_obj_set_style_text_font(c_label_108, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_108, "Y");
                                lv_obj_set_width(c_label_108, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_108, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_108
                                lv_obj_set_style_border_color(c_label_108, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_109 = lv_label_create(c_obj_107);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_109
                                lv_obj_set_style_text_font(c_label_109, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_109, "22.000");
                                lv_obj_set_flex_grow(c_label_109, 1);
                                lv_obj_add_style(c_label_109, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_109
                                lv_obj_set_style_text_align(c_label_109, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_110 = lv_obj_create(c_obj_106);
                            lv_obj_add_style(c_obj_110, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_110, &c_container_2, 0);
                            lv_obj_set_size(c_obj_110, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_110 (obj) with path prefix 'main:axis_pos_y'
                                c_label_111 = lv_label_create(c_obj_110);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_111
                                lv_obj_set_style_text_font(c_label_111, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_111, "72.000");
                                lv_obj_set_flex_grow(c_label_111, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_111
                                lv_obj_set_style_text_align(c_label_111, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_111, &c_indicator_yellow_8, 0);
                                c_label_112 = lv_label_create(c_obj_110);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_112
                                lv_obj_set_style_text_font(c_label_112, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_112, "");
                                lv_obj_set_width(c_label_112, 14);
                                c_label_113 = lv_label_create(c_obj_110);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_113
                                lv_obj_set_style_text_font(c_label_113, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_113, "-12.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_113
                                lv_obj_set_style_text_align(c_label_113, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_113, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_113, 1);
                                c_label_114 = lv_label_create(c_obj_110);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_114
                                lv_obj_set_style_text_font(c_label_114, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_114, "");
                                lv_obj_set_width(c_label_114, 14);
                        // Using view component 'axis_pos_display'
                        c_obj_115 = lv_obj_create(c_obj_96);
                        lv_obj_add_style(c_obj_115, &c_flex_y_6, 0);
                        lv_obj_add_style(c_obj_115, &c_container_2, 0);
                        lv_obj_set_size(c_obj_115, LV_PCT(100), LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all on c_obj_115
                        lv_obj_set_style_pad_all(c_obj_115, 10, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_115
                        lv_obj_set_style_pad_bottom(c_obj_115, 18, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_obj_115
                        lv_obj_set_style_border_color(c_obj_115, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa on c_obj_115
                        lv_obj_set_style_border_opa(c_obj_115, 40, LV_PART_MAIN);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all on c_obj_115
                        lv_obj_set_style_margin_all(c_obj_115, 2, LV_PART_MAIN);
                        lvgl_json_register_ptr("main:axis_pos_z", "lv_obj_t", (void*)c_obj_115);
                        // Children of c_obj_115 (obj) with path prefix 'main:axis_pos_z'
                            c_obj_116 = lv_obj_create(c_obj_115);
                            lv_obj_add_style(c_obj_116, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_116, &c_container_2, 0);
                            lv_obj_set_size(c_obj_116, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_116 (obj) with path prefix 'main:axis_pos_z'
                                c_label_117 = lv_label_create(c_obj_116);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_117
                                lv_obj_set_style_text_font(c_label_117, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_117, "Z");
                                lv_obj_set_width(c_label_117, LV_SIZE_CONTENT);
                                lv_obj_add_style(c_label_117, &c_indicator_light_12, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color on c_label_117
                                lv_obj_set_style_border_color(c_label_117, lv_color_hex(0x55FF55), LV_PART_MAIN);
                                c_label_118 = lv_label_create(c_obj_116);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_118
                                lv_obj_set_style_text_font(c_label_118, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_118, "1.000");
                                lv_obj_set_flex_grow(c_label_118, 1);
                                lv_obj_add_style(c_label_118, &c_indicator_green_7, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_118
                                lv_obj_set_style_text_align(c_label_118, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                            c_obj_119 = lv_obj_create(c_obj_115);
                            lv_obj_add_style(c_obj_119, &c_flex_x_5, 0);
                            lv_obj_add_style(c_obj_119, &c_container_2, 0);
                            lv_obj_set_size(c_obj_119, LV_PCT(100), LV_SIZE_CONTENT);
                            // Children of c_obj_119 (obj) with path prefix 'main:axis_pos_z'
                                c_label_120 = lv_label_create(c_obj_119);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_120
                                lv_obj_set_style_text_font(c_label_120, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_120, "1.000");
                                lv_obj_set_flex_grow(c_label_120, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_120
                                lv_obj_set_style_text_align(c_label_120, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_120, &c_indicator_yellow_8, 0);
                                c_label_121 = lv_label_create(c_obj_119);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_121
                                lv_obj_set_style_text_font(c_label_121, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_121, "");
                                lv_obj_set_width(c_label_121, 14);
                                c_label_122 = lv_label_create(c_obj_119);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_122
                                lv_obj_set_style_text_font(c_label_122, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_122, "0.125");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align on c_label_122
                                lv_obj_set_style_text_align(c_label_122, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
                                lv_obj_add_style(c_label_122, &c_indicator_yellow_8, 0);
                                lv_obj_set_flex_grow(c_label_122, 1);
                                c_label_123 = lv_label_create(c_obj_119);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_123
                                lv_obj_set_style_text_font(c_label_123, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t")), LV_PART_MAIN);
                                lv_label_set_text(c_label_123, "");
                                lv_obj_set_width(c_label_123, 14);
                c_obj_124 = lv_obj_create(c_obj_94);
                lv_obj_set_layout(c_obj_124, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_124, LV_FLEX_FLOW_COLUMN);
                lv_obj_add_style(c_obj_124, &c_container_2, 0);
                lv_obj_set_height(c_obj_124, LV_PCT(100));
                lv_obj_set_flex_grow(c_obj_124, 45);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_top on c_obj_124
                lv_obj_set_style_pad_top(c_obj_124, 5, LV_PART_MAIN);
                // Children of c_obj_124 (obj) with path prefix 'main'
                    // Using view component 'feed_rate_scale'
                    c_obj_125 = lv_obj_create(c_obj_124);
                    lv_obj_set_size(c_obj_125, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_125, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_125, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_125, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_125, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_125
                    lv_obj_set_style_pad_column(c_obj_125, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_125
                    lv_obj_set_style_pad_bottom(c_obj_125, 12, LV_PART_MAIN);
                    // Children of c_obj_125 (obj) with path prefix 'main'
                        c_obj_126 = lv_obj_create(c_obj_125);
                        lv_obj_set_layout(c_obj_126, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_126, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_126, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_126, LV_PCT(100));
                        lv_obj_set_height(c_obj_126, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_126, 1);
                        // Children of c_obj_126 (obj) with path prefix 'main'
                            c_label_127 = lv_label_create(c_obj_126);
                            lv_label_set_text(c_label_127, "FEED");
                            lv_obj_set_height(c_label_127, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_127, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_127
                            lv_obj_set_style_text_font(c_label_127, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_127, &c_border_top_btm_10, 0);
                            c_grid_obj_128 = lv_obj_create(c_obj_126);
                            lv_obj_add_style(c_grid_obj_128, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_128, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_128, LV_SIZE_CONTENT);
                            // Children of c_grid_obj_128 (obj) with path prefix 'main'
                                c_label_131 = lv_label_create(c_grid_obj_128);
                                lv_label_set_text(c_label_131, "F");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_131
                                lv_obj_set_style_text_font(c_label_131, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_131, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                                lv_obj_set_height(c_label_131, LV_SIZE_CONTENT);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_label_131
                                lv_obj_set_style_pad_left(c_label_131, 10, LV_PART_MAIN);
                                c_label_132 = lv_label_create(c_grid_obj_128);
                                lv_obj_set_grid_cell(c_label_132, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_132
                                lv_obj_set_style_text_font(c_label_132, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_132, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_132, "1000");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_132
                                lv_obj_set_style_pad_right(c_label_132, 10, LV_PART_MAIN);
                            lv_obj_set_grid_dsc_array(c_grid_obj_128, c_coord_array_129, c_coord_array_130);
                            c_obj_133 = lv_obj_create(c_obj_126);
                            lv_obj_set_layout(c_obj_133, LV_LAYOUT_FLEX);
                            lv_obj_set_flex_flow(c_obj_133, LV_FLEX_FLOW_COLUMN);
                            lv_obj_add_style(c_obj_133, &c_container_2, 0);
                            lv_obj_set_width(c_obj_133, LV_PCT(100));
                            lv_obj_set_height(c_obj_133, LV_SIZE_CONTENT);
                            // Children of c_obj_133 (obj) with path prefix 'main'
                                c_bar_134 = lv_bar_create(c_obj_133);
                                lv_obj_set_width(c_bar_134, LV_PCT(100));
                                lv_obj_set_height(c_bar_134, 15);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_bar_134
                                lv_obj_set_style_margin_left(c_bar_134, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_bar_134
                                lv_obj_set_style_margin_right(c_bar_134, 15, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_134, &c_bar_indicator_3, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_134, &c_bar_indicator_3, LV_PART_INDICATOR);
                                lv_bar_set_value(c_bar_134, 65, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_bar_134
                                lv_obj_set_style_bg_color(c_bar_134, lv_color_hex(0x5DD555), LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_bar_134
                                lv_obj_set_style_bg_opa(c_bar_134, 255, LV_PART_MAIN);
                                c_scale_135 = lv_scale_create(c_obj_133);
                                lv_obj_set_width(c_scale_135, LV_PCT(100));
                                lv_obj_set_height(c_scale_135, 18);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_scale_135
                                lv_obj_set_style_margin_left(c_scale_135, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_scale_135
                                lv_obj_set_style_margin_right(c_scale_135, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_scale_135
                                lv_obj_set_style_text_font(c_scale_135, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t")), LV_PART_MAIN);
                        c_obj_136 = lv_obj_create(c_obj_125);
                        lv_obj_set_layout(c_obj_136, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_136, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_136, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_136, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_136, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_136
                        lv_obj_set_style_pad_right(c_obj_136, 0, LV_PART_MAIN);
                        lv_obj_set_flex_align(c_obj_136, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                        // Children of c_obj_136 (obj) with path prefix 'main'
                            c_label_137 = lv_label_create(c_obj_136);
                            lv_label_set_text(c_label_137, "MM/MIN");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_137
                            lv_obj_set_style_text_font(c_label_137, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_137, &c_border_top_btm_10, 0);
                            c_label_138 = lv_label_create(c_obj_136);
                            lv_label_set_text(c_label_138, "Feed Ovr");
                            c_label_139 = lv_label_create(c_obj_136);
                            lv_label_set_text(c_label_139, "100%%");
                            c_label_140 = lv_label_create(c_obj_136);
                            lv_label_set_text(c_label_140, "65%%");
                    // Using view component 'feed_rate_scale'
                    c_obj_141 = lv_obj_create(c_obj_124);
                    lv_obj_set_size(c_obj_141, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_141, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_141, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_141, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_141, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_141
                    lv_obj_set_style_pad_column(c_obj_141, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_141
                    lv_obj_set_style_pad_bottom(c_obj_141, 12, LV_PART_MAIN);
                    // Children of c_obj_141 (obj) with path prefix 'main'
                        c_obj_142 = lv_obj_create(c_obj_141);
                        lv_obj_set_layout(c_obj_142, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_142, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_142, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_142, LV_PCT(100));
                        lv_obj_set_height(c_obj_142, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_142, 1);
                        // Children of c_obj_142 (obj) with path prefix 'main'
                            c_label_143 = lv_label_create(c_obj_142);
                            lv_label_set_text(c_label_143, "SPEED");
                            lv_obj_set_height(c_label_143, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_143, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_143
                            lv_obj_set_style_text_font(c_label_143, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_143, &c_border_top_btm_10, 0);
                            c_grid_obj_144 = lv_obj_create(c_obj_142);
                            lv_obj_add_style(c_grid_obj_144, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_144, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_144, LV_SIZE_CONTENT);
                            // Children of c_grid_obj_144 (obj) with path prefix 'main'
                                c_label_147 = lv_label_create(c_grid_obj_144);
                                lv_label_set_text(c_label_147, "S");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_147
                                lv_obj_set_style_text_font(c_label_147, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_147, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                                lv_obj_set_height(c_label_147, LV_SIZE_CONTENT);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_label_147
                                lv_obj_set_style_pad_left(c_label_147, 10, LV_PART_MAIN);
                                c_label_148 = lv_label_create(c_grid_obj_144);
                                lv_obj_set_grid_cell(c_label_148, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_148
                                lv_obj_set_style_text_font(c_label_148, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_148, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_148, "1000");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_148
                                lv_obj_set_style_pad_right(c_label_148, 10, LV_PART_MAIN);
                            lv_obj_set_grid_dsc_array(c_grid_obj_144, c_coord_array_145, c_coord_array_146);
                            c_obj_149 = lv_obj_create(c_obj_142);
                            lv_obj_set_layout(c_obj_149, LV_LAYOUT_FLEX);
                            lv_obj_set_flex_flow(c_obj_149, LV_FLEX_FLOW_COLUMN);
                            lv_obj_add_style(c_obj_149, &c_container_2, 0);
                            lv_obj_set_width(c_obj_149, LV_PCT(100));
                            lv_obj_set_height(c_obj_149, LV_SIZE_CONTENT);
                            // Children of c_obj_149 (obj) with path prefix 'main'
                                c_bar_150 = lv_bar_create(c_obj_149);
                                lv_obj_set_width(c_bar_150, LV_PCT(100));
                                lv_obj_set_height(c_bar_150, 15);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_bar_150
                                lv_obj_set_style_margin_left(c_bar_150, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_bar_150
                                lv_obj_set_style_margin_right(c_bar_150, 15, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_150, &c_bar_indicator_3, LV_PART_MAIN);
                                lv_obj_add_style(c_bar_150, &c_bar_indicator_3, LV_PART_INDICATOR);
                                lv_bar_set_value(c_bar_150, 65, 0);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color on c_bar_150
                                lv_obj_set_style_bg_color(c_bar_150, lv_color_hex(0x5DD555), LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa on c_bar_150
                                lv_obj_set_style_bg_opa(c_bar_150, 255, LV_PART_MAIN);
                                c_scale_151 = lv_scale_create(c_obj_149);
                                lv_obj_set_width(c_scale_151, LV_PCT(100));
                                lv_obj_set_height(c_scale_151, 18);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left on c_scale_151
                                lv_obj_set_style_margin_left(c_scale_151, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right on c_scale_151
                                lv_obj_set_style_margin_right(c_scale_151, 15, LV_PART_MAIN);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_scale_151
                                lv_obj_set_style_text_font(c_scale_151, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t")), LV_PART_MAIN);
                        c_obj_152 = lv_obj_create(c_obj_141);
                        lv_obj_set_layout(c_obj_152, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_152, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_152, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_152, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_152, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_152
                        lv_obj_set_style_pad_right(c_obj_152, 0, LV_PART_MAIN);
                        lv_obj_set_flex_align(c_obj_152, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                        // Children of c_obj_152 (obj) with path prefix 'main'
                            c_label_153 = lv_label_create(c_obj_152);
                            lv_label_set_text(c_label_153, "/MIN");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_153
                            lv_obj_set_style_text_font(c_label_153, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_153, &c_border_top_btm_10, 0);
                            c_label_154 = lv_label_create(c_obj_152);
                            lv_label_set_text(c_label_154, "Speed Ovr");
                            c_label_155 = lv_label_create(c_obj_152);
                            lv_label_set_text(c_label_155, "100%%");
                            c_label_156 = lv_label_create(c_obj_152);
                            lv_label_set_text(c_label_156, "65%%");
                    // Using view component 'jog_feed'
                    c_obj_157 = lv_obj_create(c_obj_124);
                    lv_obj_set_size(c_obj_157, LV_PCT(100), LV_PCT(100));
                    lv_obj_add_style(c_obj_157, &c_container_2, 0);
                    lv_obj_set_layout(c_obj_157, LV_LAYOUT_FLEX);
                    lv_obj_set_flex_flow(c_obj_157, LV_FLEX_FLOW_ROW);
                    lv_obj_set_height(c_obj_157, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column on c_obj_157
                    lv_obj_set_style_pad_column(c_obj_157, 0, LV_PART_MAIN);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom on c_obj_157
                    lv_obj_set_style_pad_bottom(c_obj_157, 0, LV_PART_MAIN);
                    // Children of c_obj_157 (obj) with path prefix 'main'
                        c_obj_158 = lv_obj_create(c_obj_157);
                        lv_obj_set_layout(c_obj_158, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_158, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_158, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_158, LV_PCT(100));
                        lv_obj_set_height(c_obj_158, LV_SIZE_CONTENT);
                        lv_obj_set_flex_grow(c_obj_158, 1);
                        // Children of c_obj_158 (obj) with path prefix 'main'
                            c_label_159 = lv_label_create(c_obj_158);
                            lv_label_set_text(c_label_159, "JOG");
                            lv_obj_set_height(c_label_159, LV_SIZE_CONTENT);
                            lv_obj_set_width(c_label_159, LV_PCT(100));
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_159
                            lv_obj_set_style_text_font(c_label_159, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_159, &c_border_top_btm_10, 0);
                            c_grid_obj_160 = lv_obj_create(c_obj_158);
                            lv_obj_add_style(c_grid_obj_160, &c_container_2, 0);
                            lv_obj_set_width(c_grid_obj_160, LV_PCT(100));
                            lv_obj_set_height(c_grid_obj_160, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left on c_grid_obj_160
                            lv_obj_set_style_pad_left(c_grid_obj_160, 10, LV_PART_MAIN);
                            // Children of c_grid_obj_160 (obj) with path prefix 'main'
                                c_label_163 = lv_label_create(c_grid_obj_160);
                                lv_label_set_text(c_label_163, "XY");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_163
                                lv_obj_set_style_text_font(c_label_163, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_163, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
                                lv_obj_set_height(c_label_163, LV_SIZE_CONTENT);
                                c_label_164 = lv_label_create(c_grid_obj_160);
                                lv_obj_set_grid_cell(c_label_164, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_164
                                lv_obj_set_style_text_font(c_label_164, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_164, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_164, "10");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_label_164
                                lv_obj_set_style_pad_right(c_label_164, 10, LV_PART_MAIN);
                                lv_obj_add_style(c_label_164, &c_border_right_11, 0);
                                lv_obj_add_style(c_label_164, &c_indicator_yellow_8, 0);
                                c_label_165 = lv_label_create(c_grid_obj_160);
                                lv_label_set_text(c_label_165, "Z");
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_165
                                lv_obj_set_style_text_font(c_label_165, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_grid_cell(c_label_165, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 0, 1);
                                lv_obj_set_height(c_label_165, LV_SIZE_CONTENT);
                                c_label_166 = lv_label_create(c_grid_obj_160);
                                lv_obj_set_grid_cell(c_label_166, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 0, 1);
                                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_166
                                lv_obj_set_style_text_font(c_label_166, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t")), LV_PART_MAIN);
                                lv_obj_set_height(c_label_166, LV_SIZE_CONTENT);
                                lv_label_set_text(c_label_166, " 5");
                                lv_obj_add_style(c_label_166, &c_indicator_yellow_8, 0);
                                lv_obj_add_event_cb(c_label_166, ((lv_event_cb_t)lvgl_json_get_registered_ptr("btn_clicked", "lv_event_cb_t")), LV_EVENT_CLICKED, NULL);
                            lv_obj_set_grid_dsc_array(c_grid_obj_160, c_coord_array_161, c_coord_array_162);
                        c_obj_167 = lv_obj_create(c_obj_157);
                        lv_obj_set_layout(c_obj_167, LV_LAYOUT_FLEX);
                        lv_obj_add_style(c_obj_167, &c_container_2, 0);
                        lv_obj_set_flex_flow(c_obj_167, LV_FLEX_FLOW_COLUMN);
                        lv_obj_set_width(c_obj_167, LV_SIZE_CONTENT);
                        lv_obj_set_height(c_obj_167, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right on c_obj_167
                        lv_obj_set_style_pad_right(c_obj_167, 0, LV_PART_MAIN);
                        // Children of c_obj_167 (obj) with path prefix 'main'
                            c_label_168 = lv_label_create(c_obj_167);
                            lv_label_set_text(c_label_168, "MM");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font on c_label_168
                            lv_obj_set_style_text_font(c_label_168, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t")), LV_PART_MAIN);
                            lv_obj_add_style(c_label_168, &c_border_top_btm_10, 0);
                            c_label_169 = lv_label_create(c_obj_167);
                            lv_label_set_text(c_label_169, "Jog Ovr");
                            c_label_170 = lv_label_create(c_obj_167);
                            lv_label_set_text(c_label_170, "100%%");
    } else {
        // WARNING: 'with.obj' for c_tileview_14 resolved to NULL. 'do' block not applied.
    }
    // Using view component 'nav_action_button'
    c_dropdown_171 = lv_dropdown_create(parent_obj);
    lv_obj_add_style(c_dropdown_171, &c_action_button_13, 0);
    lv_dropdown_set_options(c_dropdown_171, " Jog\n Probe\n Status\n X\n y\n Z\n Off");
    lv_obj_align(c_dropdown_171, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_add_flag(c_dropdown_171, LV_OBJ_FLAG_FLOATING);
    lv_dropdown_set_text(c_dropdown_171, "");
    lv_dropdown_set_symbol(c_dropdown_171, NULL);
    lv_obj_move_foreground(c_dropdown_171);
    c_with_target_172 = lv_dropdown_get_list(c_dropdown_171);
    if (c_with_target_172) { // Check if 'with.obj' for c_dropdown_171 resolved
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_min_width on c_with_target_172
        lv_obj_set_style_min_width(c_with_target_172, 200, LV_PART_MAIN);
    } else {
        // WARNING: 'with.obj' for c_dropdown_171 resolved to NULL. 'do' block not applied.
    }
    // Using view component 'jog_action_button'
    c_dropdown_173 = lv_dropdown_create(parent_obj);
    lv_dropdown_set_options(c_dropdown_173, " Home\n Zero\n G54\n G55\n G56\n G57\n G58");
    lv_obj_add_style(c_dropdown_173, &c_action_button_13, 0);
    lv_obj_align(c_dropdown_173, LV_ALIGN_BOTTOM_LEFT, 90, -10);
    lv_obj_add_flag(c_dropdown_173, LV_OBJ_FLAG_FLOATING);
    lv_dropdown_set_text(c_dropdown_173, "");
    lv_dropdown_set_symbol(c_dropdown_173, NULL);
    lv_obj_move_foreground(c_dropdown_173);
    lvgl_json_register_ptr("jog_action_button", "lv_dropdown_t", (void*)c_dropdown_173);
    c_with_target_174 = lv_dropdown_get_list(c_dropdown_173);
    if (c_with_target_174) { // Check if 'with.obj' for c_dropdown_173 resolved
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_min_width on c_with_target_174
        lv_obj_set_style_min_width(c_with_target_174, 200, LV_PART_MAIN);
    } else {
        // WARNING: 'with.obj' for c_dropdown_173 resolved to NULL. 'do' block not applied.
    }
}