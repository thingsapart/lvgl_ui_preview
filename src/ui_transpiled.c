#include "lvgl.h"

extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);
extern void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name);

#ifndef LV_GRID_FR
#define LV_GRID_FR(x) (lv_coord_t)(LV_COORD_FRACT_MAX / (x))
#endif
#ifndef LV_SIZE_CONTENT
#define LV_SIZE_CONTENT LV_COORD_MAX
#endif

void create_ui_ui_transpiled(lv_obj_t *screen_parent) {
    // --- VARIABLE DECLARATIONS ---
    lv_style_t debug;
    lv_style_t container;
    lv_style_t bar_indicator;
    lv_style_t bg_gradient;
    lv_style_t flex_x;
    lv_style_t flex_y;
    lv_style_t indicator_green;
    lv_style_t indicator_yellow;
    lv_style_t jog_btn;
    lv_style_t border_top_btm;
    lv_style_t border_right;
    lv_style_t indicator_light;
    lv_style_t action_button;
    lv_obj_t * c_obj_1;
    lv_obj_t * c_obj_2;
    lv_obj_t * c_obj_3;
    lv_obj_t * c_obj_4;
    lv_obj_t * c_label_5;
    lv_obj_t * c_label_6;
    lv_obj_t * c_obj_7;
    lv_obj_t * c_label_8;
    lv_obj_t * c_label_9;
    lv_obj_t * c_label_10;
    lv_obj_t * c_label_11;
    lv_obj_t * c_obj_12;
    lv_obj_t * c_obj_13;
    lv_obj_t * c_label_14;
    lv_obj_t * c_label_15;
    lv_obj_t * c_obj_16;
    lv_obj_t * c_label_17;
    lv_obj_t * c_label_18;
    lv_obj_t * c_label_19;
    lv_obj_t * c_label_20;
    lv_obj_t * c_obj_21;
    lv_obj_t * c_label_22;
    lv_obj_t * c_button_23;
    lv_obj_t * c_label_24;
    lv_obj_t * c_button_25;
    lv_obj_t * c_label_26;
    lv_obj_t * c_button_27;
    lv_obj_t * c_label_28;
    lv_obj_t * c_button_29;
    lv_obj_t * c_label_30;
    lv_obj_t * c_obj_31;
    lv_obj_t * c_obj_32;
    lv_obj_t * c_obj_33;
    lv_obj_t * c_label_34;
    lv_obj_t * c_grid_obj_35;
    static const lv_coord_t c_coord_array_36[] = { LV_GRID_CONTENT, LV_GRID_FR_1 };
    static const lv_coord_t c_coord_array_37[] = { LV_GRID_CONTENT };
    lv_obj_t * c_label_38;
    lv_obj_t * c_label_39;
    lv_obj_t * c_obj_40;
    lv_obj_t * c_bar_41;
    lv_obj_t * c_scale_42;
    lv_obj_t * c_obj_43;
    lv_obj_t * c_label_44;
    lv_obj_t * c_label_45;
    lv_obj_t * c_label_46;
    lv_obj_t * c_label_47;
    lv_obj_t * c_obj_48;
    lv_obj_t * c_obj_49;
    lv_obj_t * c_label_50;
    lv_obj_t * c_grid_obj_51;
    static const lv_coord_t c_coord_array_52[] = { LV_GRID_CONTENT, LV_GRID_FR_1 };
    static const lv_coord_t c_coord_array_53[] = { LV_GRID_CONTENT };
    lv_obj_t * c_label_54;
    lv_obj_t * c_label_55;
    lv_obj_t * c_obj_56;
    lv_obj_t * c_bar_57;
    lv_obj_t * c_scale_58;
    lv_obj_t * c_obj_59;
    lv_obj_t * c_label_60;
    lv_obj_t * c_label_61;
    lv_obj_t * c_label_62;
    lv_obj_t * c_label_63;
    lv_obj_t * c_obj_64;
    lv_obj_t * c_obj_65;
    lv_obj_t * c_label_66;
    lv_obj_t * c_grid_obj_67;
    static const lv_coord_t c_coord_array_68[] = { 35, 45, 20, 40 };
    static const lv_coord_t c_coord_array_69[] = { LV_GRID_CONTENT };
    lv_obj_t * c_label_70;
    lv_obj_t * c_label_71;
    lv_obj_t * c_label_72;
    lv_obj_t * c_label_73;
    lv_obj_t * c_obj_74;
    lv_obj_t * c_label_75;
    lv_obj_t * c_label_76;
    lv_obj_t * c_label_77;
    lv_obj_t * c_dropdown_78;
    lv_obj_t * c_dropdown_79;

    lv_obj_t *parent_obj = screen_parent ? screen_parent : lv_screen_active();
    if (!parent_obj) {
        LV_LOG_ERROR("Cannot render UI: No parent screen available.");
        return;
    }
    
    lv_style_init(&debug);
    lv_style_set_outline_width(&debug, 1);
    lv_style_set_outline_color(&debug, lv_color_hex(0xFFEEFF));
    lv_style_set_outline_opa(&debug, 150);
    lv_style_set_border_width(&debug, 1);
    lv_style_set_border_color(&debug, lv_color_hex(0xFFEEFF));
    lv_style_set_border_opa(&debug, 150);
    lv_style_set_radius(&debug, 0);
    lv_style_init(&container);
    lv_style_set_pad_all(&container, 0);
    lv_style_set_margin_all(&container, 0);
    lv_style_set_border_width(&container, 0);
    lv_style_set_pad_row(&container, 3);
    lv_style_set_pad_column(&container, 5);
    lv_style_init(&bar_indicator);
    lv_style_set_radius(&bar_indicator, 4);
    lv_style_init(&bg_gradient);
    lv_style_set_bg_opa(&bg_gradient, 255);
    lv_style_set_bg_color(&bg_gradient, lv_color_hex(0x222222));
    lv_style_set_bg_grad_color(&bg_gradient, lv_color_hex(0x444444));
    lv_style_set_bg_grad_dir(&bg_gradient, LV_GRAD_DIR_HOR);
    lv_style_init(&flex_x);
    lv_style_set_layout(&flex_x, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&flex_x, LV_FLEX_FLOW_ROW);
    lv_style_init(&flex_y);
    lv_style_set_layout(&flex_y, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&flex_y, LV_FLEX_FLOW_COLUMN);
    lv_style_init(&indicator_green);
    lv_style_set_text_color(&indicator_green, lv_color_hex(0x44EE44));
    lv_style_init(&indicator_yellow);
    lv_style_set_text_color(&indicator_yellow, lv_color_hex(0xFFFF55));
    lv_style_init(&jog_btn);
    lv_style_set_pad_all(&jog_btn, 5);
    lv_style_set_pad_bottom(&jog_btn, 10);
    lv_style_set_pad_top(&jog_btn, 10);
    lv_style_set_margin_all(&jog_btn, 0);
    lv_style_set_radius(&jog_btn, 2);
    lv_style_init(&border_top_btm);
    lv_style_set_border_width(&border_top_btm, 1);
    lv_style_set_border_color(&border_top_btm, lv_color_hex(0xFFFFFF));
    lv_style_set_border_opa(&border_top_btm, 50);
    lv_style_set_border_side(&border_top_btm, LV_BORDER_SIDE_TOP_BOTTOM);
    lv_style_init(&border_right);
    lv_style_set_border_width(&border_right, 1);
    lv_style_set_border_color(&border_right, lv_color_hex(0xFFFFFF));
    lv_style_set_border_opa(&border_right, 50);
    lv_style_set_border_side(&border_right, LV_BORDER_SIDE_RIGHT);
    lv_style_init(&indicator_light);
    lv_style_set_border_width(&indicator_light, 6);
    lv_style_set_pad_left(&indicator_light, 10);
    lv_style_set_margin_left(&indicator_light, 10);
    lv_style_set_border_opa(&indicator_light, 200);
    lv_style_set_border_side(&indicator_light, LV_BORDER_SIDE_LEFT);
    // Component definition 'axis_jog_speed_adjust' processed and stored.
    // Component definition 'axis_pos_display' processed and stored.
    // Component definition 'feed_rate_scale' processed and stored.
    // Component definition 'jog_feed' processed and stored.
    lv_style_init(&action_button);
    lv_style_set_size(&action_button, 45, 45);
    lv_style_set_bg_color(&action_button, lv_color_hex(0x1F95F6));
    lv_style_set_radius(&action_button, LV_RADIUS_CIRCLE);
    // Component definition 'nav_action_button' processed and stored.
    // Component definition 'jog_action_button' processed and stored.
    c_obj_1 = lv_obj_create(parent_obj);
    lv_obj_set_layout(c_obj_1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(c_obj_1, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(c_obj_1, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
    lv_obj_set_size(c_obj_1, lv_pct(100), 320);
    lvgl_json_register_ptr("main", "lv_obj_t", (void*)c_obj_1);
    // Children of c_obj_1 (obj)
        c_obj_2 = lv_obj_create(c_obj_1);
        lv_obj_set_layout(c_obj_2, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(c_obj_2, LV_FLEX_FLOW_COLUMN);
        lv_obj_add_style(c_obj_2, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
        lv_obj_set_height(c_obj_2, lv_pct(100));
        lv_obj_set_flex_grow(c_obj_2, 60);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_side
        lv_obj_set_style_border_side(c_obj_2, LV_BORDER_SIDE_RIGHT, 0);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_width
        lv_obj_set_style_border_width(c_obj_2, 2, 0);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_radius
        lv_obj_set_style_radius(c_obj_2, 0, 0);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
        lv_obj_set_style_border_color(c_obj_2, lv_color_hex(0xFFFFFF), 0);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa
        lv_obj_set_style_border_opa(c_obj_2, 90, 0);
        // Children of c_obj_2 (obj)
            // Using view component 'axis_pos_display' with context {'axis': 'X', 'wcs_pos': '11.000', 'abs_pos': '51.000', 'delta_pos': '2.125', 'name': 'axis_pos_x'}
                c_obj_3 = lv_obj_create(c_obj_2);
                lv_obj_add_style(c_obj_3, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                lv_obj_set_size(c_obj_3, lv_pct(100), LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all
                lv_obj_set_style_pad_all(c_obj_3, 10, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_3, 18, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                lv_obj_set_style_border_color(c_obj_3, lv_color_hex(0xFFFFFF), 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa
                lv_obj_set_style_border_opa(c_obj_3, 40, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all
                lv_obj_set_style_margin_all(c_obj_3, 2, 0);
                lvgl_json_register_ptr("main:$name", "lv_obj_t", (void*)c_obj_3);
                // Children of c_obj_3 (obj)
                    c_obj_4 = lv_obj_create(c_obj_3);
                    lv_obj_add_style(c_obj_4, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_size(c_obj_4, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_4 (obj)
                        c_label_5 = lv_label_create(c_obj_4);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_5, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_5, "X");
                        lv_obj_set_width(c_label_5, LV_SIZE_CONTENT);
                        lv_obj_add_style(c_label_5, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_light", "lv_style_t *")), 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                        lv_obj_set_style_border_color(c_label_5, lv_color_hex(0x55FF55), 0);
                        c_label_6 = lv_label_create(c_obj_4);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_6, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_6, "11.000");
                        lv_obj_set_flex_grow(c_label_6, 1);
                        lv_obj_add_style(c_label_6, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_green", "lv_style_t *")), 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_6, LV_TEXT_ALIGN_RIGHT, 0);
                    c_obj_7 = lv_obj_create(c_obj_3);
                    lv_obj_add_style(c_obj_7, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_size(c_obj_7, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_7 (obj)
                        c_label_8 = lv_label_create(c_obj_7);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_8, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_8, "51.000");
                        lv_obj_set_flex_grow(c_label_8, 1);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_8, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_8, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_yellow", "lv_style_t *")), 0);
                        c_label_9 = lv_label_create(c_obj_7);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_9, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_9, "");
                        lv_obj_set_width(c_label_9, 14);
                        c_label_10 = lv_label_create(c_obj_7);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_10, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_10, "2.125");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_10, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_10, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_yellow", "lv_style_t *")), 0);
                        lv_obj_set_flex_grow(c_label_10, 1);
                        c_label_11 = lv_label_create(c_obj_7);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_11, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_11, "");
                        lv_obj_set_width(c_label_11, 14);
            // Using view component 'axis_pos_display' with context {'axis': 'Y', 'wcs_pos': '22.000', 'abs_pos': '72.000', 'delta_pos': '-12.125', 'name': 'axis_pos_y'}
                c_obj_12 = lv_obj_create(c_obj_2);
                lv_obj_add_style(c_obj_12, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                lv_obj_set_size(c_obj_12, lv_pct(100), LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all
                lv_obj_set_style_pad_all(c_obj_12, 10, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_12, 18, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                lv_obj_set_style_border_color(c_obj_12, lv_color_hex(0xFFFFFF), 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa
                lv_obj_set_style_border_opa(c_obj_12, 40, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all
                lv_obj_set_style_margin_all(c_obj_12, 2, 0);
                lvgl_json_register_ptr("main:$name", "lv_obj_t", (void*)c_obj_12);
                // Children of c_obj_12 (obj)
                    c_obj_13 = lv_obj_create(c_obj_12);
                    lv_obj_add_style(c_obj_13, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_size(c_obj_13, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_13 (obj)
                        c_label_14 = lv_label_create(c_obj_13);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_14, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_14, "Y");
                        lv_obj_set_width(c_label_14, LV_SIZE_CONTENT);
                        lv_obj_add_style(c_label_14, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_light", "lv_style_t *")), 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                        lv_obj_set_style_border_color(c_label_14, lv_color_hex(0x55FF55), 0);
                        c_label_15 = lv_label_create(c_obj_13);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_15, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_15, "22.000");
                        lv_obj_set_flex_grow(c_label_15, 1);
                        lv_obj_add_style(c_label_15, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_green", "lv_style_t *")), 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_15, LV_TEXT_ALIGN_RIGHT, 0);
                    c_obj_16 = lv_obj_create(c_obj_12);
                    lv_obj_add_style(c_obj_16, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_size(c_obj_16, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_16 (obj)
                        c_label_17 = lv_label_create(c_obj_16);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_17, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_17, "72.000");
                        lv_obj_set_flex_grow(c_label_17, 1);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_17, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_17, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_yellow", "lv_style_t *")), 0);
                        c_label_18 = lv_label_create(c_obj_16);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_18, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_18, "");
                        lv_obj_set_width(c_label_18, 14);
                        c_label_19 = lv_label_create(c_obj_16);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_19, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_19, "-12.125");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_19, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_19, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_yellow", "lv_style_t *")), 0);
                        lv_obj_set_flex_grow(c_label_19, 1);
                        c_label_20 = lv_label_create(c_obj_16);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_20, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_20, "");
                        lv_obj_set_width(c_label_20, 14);
            // Using view component 'axis_jog_speed_adjust' with context {'label': 'Z', 'dist_0': '0.05mm', 'dist_1': '0.1mm', 'dist_2': '1.0mm', 'dist_3': '5.0mm'}
                c_obj_21 = lv_obj_create(c_obj_2);
                lv_obj_add_style(c_obj_21, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                lv_obj_set_size(c_obj_21, lv_pct(100), LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all
                lv_obj_set_style_pad_all(c_obj_21, 10, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_21, 20, 0);
                // Children of c_obj_21 (obj)
                    c_label_22 = lv_label_create(c_obj_21);
                    lv_label_set_text(c_label_22, "Z");
                    lv_obj_set_width(c_label_22, 20);
                    c_button_23 = lv_button_create(c_obj_21);
                    lv_obj_add_style(c_button_23, ((lv_style_t *)lvgl_json_get_registered_ptr("jog_btn", "lv_style_t *")), 0);
                    // Children of c_button_23 (button)
                        c_label_24 = lv_label_create(c_button_23);
                        lv_label_set_text(c_label_24, "0.05mm");
                    c_button_25 = lv_button_create(c_obj_21);
                    lv_obj_add_style(c_button_25, ((lv_style_t *)lvgl_json_get_registered_ptr("jog_btn", "lv_style_t *")), 0);
                    // Children of c_button_25 (button)
                        c_label_26 = lv_label_create(c_button_25);
                        lv_label_set_text(c_label_26, "0.1mm");
                    c_button_27 = lv_button_create(c_obj_21);
                    lv_obj_add_style(c_button_27, ((lv_style_t *)lvgl_json_get_registered_ptr("jog_btn", "lv_style_t *")), 0);
                    // Children of c_button_27 (button)
                        c_label_28 = lv_label_create(c_button_27);
                        lv_label_set_text(c_label_28, "1.0mm");
                    c_button_29 = lv_button_create(c_obj_21);
                    lv_obj_add_style(c_button_29, ((lv_style_t *)lvgl_json_get_registered_ptr("jog_btn", "lv_style_t *")), 0);
                    // Children of c_button_29 (button)
                        c_label_30 = lv_label_create(c_button_29);
                        lv_label_set_text(c_label_30, "5.0mm");
        c_obj_31 = lv_obj_create(c_obj_1);
        lv_obj_set_layout(c_obj_31, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(c_obj_31, LV_FLEX_FLOW_COLUMN);
        lv_obj_add_style(c_obj_31, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
        lv_obj_set_height(c_obj_31, lv_pct(100));
        lv_obj_set_flex_grow(c_obj_31, 45);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_top
        lv_obj_set_style_pad_top(c_obj_31, 5, 0);
        // Children of c_obj_31 (obj)
            // Using view component 'feed_rate_scale' with context {'unit': 'MM/MIN', 'label': 'FEED', 'letter': 'F', 'ovr': 'Feed Ovr', 'pad_bottom': 12}
                c_obj_32 = lv_obj_create(c_obj_31);
                lv_obj_set_size(c_obj_32, lv_pct(100), lv_pct(100));
                lv_obj_add_style(c_obj_32, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                lv_obj_set_layout(c_obj_32, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_32, LV_FLEX_FLOW_ROW);
                lv_obj_set_height(c_obj_32, LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column
                lv_obj_set_style_pad_column(c_obj_32, 0, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_32, 12, 0);
                // Children of c_obj_32 (obj)
                    c_obj_33 = lv_obj_create(c_obj_32);
                    lv_obj_set_layout(c_obj_33, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_33, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_flex_flow(c_obj_33, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_33, lv_pct(100));
                    lv_obj_set_height(c_obj_33, LV_SIZE_CONTENT);
                    lv_obj_set_flex_grow(c_obj_33, 1);
                    // Children of c_obj_33 (obj)
                        c_label_34 = lv_label_create(c_obj_33);
                        lv_label_set_text(c_label_34, "FEED");
                        lv_obj_set_height(c_label_34, LV_SIZE_CONTENT);
                        lv_obj_set_width(c_label_34, lv_pct(100));
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_34, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_34, ((lv_style_t *)lvgl_json_get_registered_ptr("border_top_btm", "lv_style_t *")), 0);
                        c_grid_obj_35 = lv_obj_create(c_obj_33);
                        lv_obj_add_style(c_grid_obj_35, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                        lv_obj_set_width(c_grid_obj_35, lv_pct(100));
                        lv_obj_set_height(c_grid_obj_35, LV_SIZE_CONTENT);
                        lv_obj_set_grid_dsc_array(c_grid_obj_35, c_coord_array_36, c_coord_array_37);
                        // Children of c_grid_obj_35 (grid)
                            c_label_38 = lv_label_create(c_grid_obj_35);
                            lv_label_set_text(c_label_38, "F");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_38, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_38, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                            lv_obj_set_height(c_label_38, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left
                            lv_obj_set_style_pad_left(c_label_38, 10, 0);
                            c_label_39 = lv_label_create(c_grid_obj_35);
                            lv_obj_set_grid_cell(c_label_39, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_39, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_39, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_39, "1000");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                            lv_obj_set_style_pad_right(c_label_39, 10, 0);
                        c_obj_40 = lv_obj_create(c_obj_33);
                        lv_obj_set_layout(c_obj_40, LV_LAYOUT_FLEX);
                        lv_obj_set_flex_flow(c_obj_40, LV_FLEX_FLOW_COLUMN);
                        lv_obj_add_style(c_obj_40, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                        lv_obj_set_width(c_obj_40, lv_pct(100));
                        lv_obj_set_height(c_obj_40, LV_SIZE_CONTENT);
                        // Children of c_obj_40 (obj)
                            c_bar_41 = lv_bar_create(c_obj_40);
                            lv_obj_set_width(c_bar_41, lv_pct(100));
                            lv_obj_set_height(c_bar_41, 15);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_bar_41, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_bar_41, 15, 0);
                            lv_obj_add_style(c_bar_41, ((lv_style_t *)lvgl_json_get_registered_ptr("bar_indicator", "lv_style_t *")), LV_PART_INDICATOR);
                            lv_bar_set_value(c_bar_41, 65, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color
                            lv_obj_set_style_bg_color(c_bar_41, lv_color_hex(0x5DD555), 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa
                            lv_obj_set_style_bg_opa(c_bar_41, 255, 0);
                            c_scale_42 = lv_scale_create(c_obj_40);
                            lv_obj_set_width(c_scale_42, lv_pct(100));
                            lv_obj_set_height(c_scale_42, 18);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_scale_42, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_scale_42, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_scale_42, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t *")), 0);
                    c_obj_43 = lv_obj_create(c_obj_32);
                    lv_obj_set_layout(c_obj_43, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_43, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_flex_flow(c_obj_43, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_43, LV_SIZE_CONTENT);
                    lv_obj_set_height(c_obj_43, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                    lv_obj_set_style_pad_right(c_obj_43, 0, 0);
                    lv_obj_set_flex_align(c_obj_43, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                    // Children of c_obj_43 (obj)
                        c_label_44 = lv_label_create(c_obj_43);
                        lv_label_set_text(c_label_44, "MM/MIN");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_44, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_44, ((lv_style_t *)lvgl_json_get_registered_ptr("border_top_btm", "lv_style_t *")), 0);
                        c_label_45 = lv_label_create(c_obj_43);
                        lv_label_set_text(c_label_45, "Feed Ovr");
                        c_label_46 = lv_label_create(c_obj_43);
                        lv_label_set_text(c_label_46, "100%%");
                        c_label_47 = lv_label_create(c_obj_43);
                        lv_label_set_text(c_label_47, "65%%");
            // Using view component 'feed_rate_scale' with context {'unit': '/MIN', 'label': 'SPEED', 'letter': 'S', 'ovr': 'Speed Ovr', 'pad_bottom': 12}
                c_obj_48 = lv_obj_create(c_obj_31);
                lv_obj_set_size(c_obj_48, lv_pct(100), lv_pct(100));
                lv_obj_add_style(c_obj_48, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                lv_obj_set_layout(c_obj_48, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_48, LV_FLEX_FLOW_ROW);
                lv_obj_set_height(c_obj_48, LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column
                lv_obj_set_style_pad_column(c_obj_48, 0, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_48, 12, 0);
                // Children of c_obj_48 (obj)
                    c_obj_49 = lv_obj_create(c_obj_48);
                    lv_obj_set_layout(c_obj_49, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_49, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_flex_flow(c_obj_49, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_49, lv_pct(100));
                    lv_obj_set_height(c_obj_49, LV_SIZE_CONTENT);
                    lv_obj_set_flex_grow(c_obj_49, 1);
                    // Children of c_obj_49 (obj)
                        c_label_50 = lv_label_create(c_obj_49);
                        lv_label_set_text(c_label_50, "SPEED");
                        lv_obj_set_height(c_label_50, LV_SIZE_CONTENT);
                        lv_obj_set_width(c_label_50, lv_pct(100));
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_50, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_50, ((lv_style_t *)lvgl_json_get_registered_ptr("border_top_btm", "lv_style_t *")), 0);
                        c_grid_obj_51 = lv_obj_create(c_obj_49);
                        lv_obj_add_style(c_grid_obj_51, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                        lv_obj_set_width(c_grid_obj_51, lv_pct(100));
                        lv_obj_set_height(c_grid_obj_51, LV_SIZE_CONTENT);
                        lv_obj_set_grid_dsc_array(c_grid_obj_51, c_coord_array_52, c_coord_array_53);
                        // Children of c_grid_obj_51 (grid)
                            c_label_54 = lv_label_create(c_grid_obj_51);
                            lv_label_set_text(c_label_54, "S");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_54, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_54, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                            lv_obj_set_height(c_label_54, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left
                            lv_obj_set_style_pad_left(c_label_54, 10, 0);
                            c_label_55 = lv_label_create(c_grid_obj_51);
                            lv_obj_set_grid_cell(c_label_55, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_55, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_55, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_55, "1000");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                            lv_obj_set_style_pad_right(c_label_55, 10, 0);
                        c_obj_56 = lv_obj_create(c_obj_49);
                        lv_obj_set_layout(c_obj_56, LV_LAYOUT_FLEX);
                        lv_obj_set_flex_flow(c_obj_56, LV_FLEX_FLOW_COLUMN);
                        lv_obj_add_style(c_obj_56, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                        lv_obj_set_width(c_obj_56, lv_pct(100));
                        lv_obj_set_height(c_obj_56, LV_SIZE_CONTENT);
                        // Children of c_obj_56 (obj)
                            c_bar_57 = lv_bar_create(c_obj_56);
                            lv_obj_set_width(c_bar_57, lv_pct(100));
                            lv_obj_set_height(c_bar_57, 15);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_bar_57, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_bar_57, 15, 0);
                            lv_obj_add_style(c_bar_57, ((lv_style_t *)lvgl_json_get_registered_ptr("bar_indicator", "lv_style_t *")), LV_PART_INDICATOR);
                            lv_bar_set_value(c_bar_57, 65, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color
                            lv_obj_set_style_bg_color(c_bar_57, lv_color_hex(0x5DD555), 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa
                            lv_obj_set_style_bg_opa(c_bar_57, 255, 0);
                            c_scale_58 = lv_scale_create(c_obj_56);
                            lv_obj_set_width(c_scale_58, lv_pct(100));
                            lv_obj_set_height(c_scale_58, 18);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_scale_58, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_scale_58, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_scale_58, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t *")), 0);
                    c_obj_59 = lv_obj_create(c_obj_48);
                    lv_obj_set_layout(c_obj_59, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_59, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_flex_flow(c_obj_59, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_59, LV_SIZE_CONTENT);
                    lv_obj_set_height(c_obj_59, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                    lv_obj_set_style_pad_right(c_obj_59, 0, 0);
                    lv_obj_set_flex_align(c_obj_59, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                    // Children of c_obj_59 (obj)
                        c_label_60 = lv_label_create(c_obj_59);
                        lv_label_set_text(c_label_60, "/MIN");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_60, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_60, ((lv_style_t *)lvgl_json_get_registered_ptr("border_top_btm", "lv_style_t *")), 0);
                        c_label_61 = lv_label_create(c_obj_59);
                        lv_label_set_text(c_label_61, "Speed Ovr");
                        c_label_62 = lv_label_create(c_obj_59);
                        lv_label_set_text(c_label_62, "100%%");
                        c_label_63 = lv_label_create(c_obj_59);
                        lv_label_set_text(c_label_63, "65%%");
            // Using view component 'jog_feed' with context {'unit': 'MM', 'label': 'JOG', 'ovr': 'Jog Ovr', 'pad_bottom': 0}
                c_obj_64 = lv_obj_create(c_obj_31);
                lv_obj_set_size(c_obj_64, lv_pct(100), lv_pct(100));
                lv_obj_add_style(c_obj_64, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                lv_obj_set_layout(c_obj_64, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_64, LV_FLEX_FLOW_ROW);
                lv_obj_set_height(c_obj_64, LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column
                lv_obj_set_style_pad_column(c_obj_64, 0, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_64, 0, 0);
                // Children of c_obj_64 (obj)
                    c_obj_65 = lv_obj_create(c_obj_64);
                    lv_obj_set_layout(c_obj_65, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_65, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_flex_flow(c_obj_65, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_65, lv_pct(100));
                    lv_obj_set_height(c_obj_65, LV_SIZE_CONTENT);
                    lv_obj_set_flex_grow(c_obj_65, 1);
                    // Children of c_obj_65 (obj)
                        c_label_66 = lv_label_create(c_obj_65);
                        lv_label_set_text(c_label_66, "JOG");
                        lv_obj_set_height(c_label_66, LV_SIZE_CONTENT);
                        lv_obj_set_width(c_label_66, lv_pct(100));
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_66, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_66, ((lv_style_t *)lvgl_json_get_registered_ptr("border_top_btm", "lv_style_t *")), 0);
                        c_grid_obj_67 = lv_obj_create(c_obj_65);
                        lv_obj_add_style(c_grid_obj_67, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                        lv_obj_set_width(c_grid_obj_67, lv_pct(100));
                        lv_obj_set_height(c_grid_obj_67, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left
                        lv_obj_set_style_pad_left(c_grid_obj_67, 10, 0);
                        lv_obj_set_grid_dsc_array(c_grid_obj_67, c_coord_array_68, c_coord_array_69);
                        // Children of c_grid_obj_67 (grid)
                            c_label_70 = lv_label_create(c_grid_obj_67);
                            lv_label_set_text(c_label_70, "XY");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_70, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_70, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
                            lv_obj_set_height(c_label_70, LV_SIZE_CONTENT);
                            c_label_71 = lv_label_create(c_grid_obj_67);
                            lv_obj_set_grid_cell(c_label_71, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_71, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_71, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_71, "10");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                            lv_obj_set_style_pad_right(c_label_71, 10, 0);
                            lv_obj_add_style(c_label_71, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_yellow", "lv_style_t *")), 0);
                            c_label_72 = lv_label_create(c_grid_obj_67);
                            lv_label_set_text(c_label_72, "Z");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_72, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_72, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 0, 1);
                            lv_obj_set_height(c_label_72, LV_SIZE_CONTENT);
                            c_label_73 = lv_label_create(c_grid_obj_67);
                            lv_obj_set_grid_cell(c_label_73, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_73, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_73, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_73, " 5");
                            lv_obj_add_style(c_label_73, ((lv_style_t *)lvgl_json_get_registered_ptr("indicator_yellow", "lv_style_t *")), 0);
                    c_obj_74 = lv_obj_create(c_obj_64);
                    lv_obj_set_layout(c_obj_74, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_74, ((lv_style_t *)lvgl_json_get_registered_ptr("container", "lv_style_t *")), 0);
                    lv_obj_set_flex_flow(c_obj_74, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_74, LV_SIZE_CONTENT);
                    lv_obj_set_height(c_obj_74, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                    lv_obj_set_style_pad_right(c_obj_74, 0, 0);
                    // Children of c_obj_74 (obj)
                        c_label_75 = lv_label_create(c_obj_74);
                        lv_label_set_text(c_label_75, "MM");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_75, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_75, ((lv_style_t *)lvgl_json_get_registered_ptr("border_top_btm", "lv_style_t *")), 0);
                        c_label_76 = lv_label_create(c_obj_74);
                        lv_label_set_text(c_label_76, "Jog Ovr");
                        c_label_77 = lv_label_create(c_obj_74);
                        lv_label_set_text(c_label_77, "100%%");
    // Using view component 'nav_action_button' with context {}
        c_dropdown_78 = lv_dropdown_create(parent_obj);
        lv_obj_add_style(c_dropdown_78, ((lv_style_t *)lvgl_json_get_registered_ptr("action_button", "lv_style_t *")), 0);
        lv_dropdown_set_options(c_dropdown_78, " Jog\n Probe\n Status\n X\n y\n Z\n Off");
        lv_obj_align(c_dropdown_78, LV_ALIGN_BOTTOM_LEFT, 20, -10);
        lv_obj_add_flag(c_dropdown_78, LV_OBJ_FLAG_FLOATING);
        lv_dropdown_set_text(c_dropdown_78, "");
        lv_dropdown_set_symbol(c_dropdown_78, NULL);
        lv_obj_move_foreground(c_dropdown_78);
        // WARNING: No setter function found for property 'with' on type 'dropdown'. Skipping.
    // Using view component 'jog_action_button' with context {}
        c_dropdown_79 = lv_dropdown_create(parent_obj);
        lv_dropdown_set_options(c_dropdown_79, " Home\n Zero\n G54\n G55\n G56\n G57\n G58");
        lv_obj_add_style(c_dropdown_79, ((lv_style_t *)lvgl_json_get_registered_ptr("action_button", "lv_style_t *")), 0);
        lv_obj_align(c_dropdown_79, LV_ALIGN_BOTTOM_LEFT, 90, -10);
        lv_obj_add_flag(c_dropdown_79, LV_OBJ_FLAG_FLOATING);
        lv_dropdown_set_text(c_dropdown_79, "");
        lv_dropdown_set_symbol(c_dropdown_79, NULL);
        lv_obj_move_foreground(c_dropdown_79);
        lvgl_json_register_ptr("jog_action_button", "lv_dropdown_t", (void*)c_dropdown_79);
        // WARNING: No setter function found for property 'with' on type 'dropdown'. Skipping.
}