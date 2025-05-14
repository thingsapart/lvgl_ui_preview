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
    static lv_style_t debug;
    static lv_style_t container;
    static lv_style_t bar_indicator;
    static lv_style_t bg_gradient;
    static lv_style_t flex_x;
    static lv_style_t flex_y;
    static lv_style_t indicator_green;
    static lv_style_t indicator_yellow;
    static lv_style_t jog_btn;
    static lv_style_t border_top_btm;
    static lv_style_t border_right;
    static lv_style_t indicator_light;
    static lv_style_t action_button;
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
    lv_obj_t * c_obj_32;
    lv_obj_t * c_label_33;
    lv_obj_t * c_grid_obj_34;
    static const lv_coord_t c_coord_array_35[] = { LV_GRID_CONTENT, LV_GRID_FR_1, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_36[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_37;
    lv_obj_t * c_label_38;
    lv_obj_t * c_obj_39;
    lv_obj_t * c_bar_40;
    lv_obj_t * c_scale_41;
    lv_obj_t * c_obj_42;
    lv_obj_t * c_label_43;
    lv_obj_t * c_label_44;
    lv_obj_t * c_label_45;
    lv_obj_t * c_label_46;
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
    static const lv_coord_t c_coord_array_67[] = { 35, 45, 20, 40, LV_GRID_TEMPLATE_LAST };
    static const lv_coord_t c_coord_array_68[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_t * c_label_69;
    lv_obj_t * c_label_70;
    lv_obj_t * c_label_71;
    lv_obj_t * c_label_72;
    lv_obj_t * c_obj_73;
    lv_obj_t * c_label_74;
    lv_obj_t * c_label_75;
    lv_obj_t * c_label_76;
    lv_obj_t * c_dropdown_77;
    lv_obj_t * c_dropdown_78;

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
    lvgl_json_register_ptr("debug", "lv_style_t", (void*)&debug);
    lv_style_init(&container);
    lv_style_set_pad_all(&container, 0);
    lv_style_set_margin_all(&container, 0);
    lv_style_set_border_width(&container, 0);
    lv_style_set_pad_row(&container, 3);
    lv_style_set_pad_column(&container, 5);
    lvgl_json_register_ptr("container", "lv_style_t", (void*)&container);
    lv_style_init(&bar_indicator);
    lv_style_set_radius(&bar_indicator, 4);
    lvgl_json_register_ptr("bar_indicator", "lv_style_t", (void*)&bar_indicator);
    lv_style_init(&bg_gradient);
    lv_style_set_bg_opa(&bg_gradient, 255);
    lv_style_set_bg_color(&bg_gradient, lv_color_hex(0x222222));
    lv_style_set_bg_grad_color(&bg_gradient, lv_color_hex(0x444444));
    lv_style_set_bg_grad_dir(&bg_gradient, LV_GRAD_DIR_HOR);
    lvgl_json_register_ptr("bg_gradient", "lv_style_t", (void*)&bg_gradient);
    lv_style_init(&flex_x);
    lv_style_set_layout(&flex_x, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&flex_x, LV_FLEX_FLOW_ROW);
    lvgl_json_register_ptr("flex_x", "lv_style_t", (void*)&flex_x);
    lv_style_init(&flex_y);
    lv_style_set_layout(&flex_y, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&flex_y, LV_FLEX_FLOW_COLUMN);
    lvgl_json_register_ptr("flex_y", "lv_style_t", (void*)&flex_y);
    lv_style_init(&indicator_green);
    lv_style_set_text_color(&indicator_green, lv_color_hex(0x44EE44));
    lvgl_json_register_ptr("indicator_green", "lv_style_t", (void*)&indicator_green);
    lv_style_init(&indicator_yellow);
    lv_style_set_text_color(&indicator_yellow, lv_color_hex(0xFFFF55));
    lvgl_json_register_ptr("indicator_yellow", "lv_style_t", (void*)&indicator_yellow);
    lv_style_init(&jog_btn);
    lv_style_set_pad_all(&jog_btn, 5);
    lv_style_set_pad_bottom(&jog_btn, 10);
    lv_style_set_pad_top(&jog_btn, 10);
    lv_style_set_margin_all(&jog_btn, 0);
    lv_style_set_radius(&jog_btn, 2);
    lvgl_json_register_ptr("jog_btn", "lv_style_t", (void*)&jog_btn);
    lv_style_init(&border_top_btm);
    lv_style_set_border_width(&border_top_btm, 1);
    lv_style_set_border_color(&border_top_btm, lv_color_hex(0xFFFFFF));
    lv_style_set_border_opa(&border_top_btm, 50);
    lv_style_set_border_side(&border_top_btm, LV_BORDER_SIDE_TOP_BOTTOM);
    lvgl_json_register_ptr("border_top_btm", "lv_style_t", (void*)&border_top_btm);
    lv_style_init(&border_right);
    lv_style_set_border_width(&border_right, 1);
    lv_style_set_border_color(&border_right, lv_color_hex(0xFFFFFF));
    lv_style_set_border_opa(&border_right, 50);
    lv_style_set_border_side(&border_right, LV_BORDER_SIDE_RIGHT);
    lvgl_json_register_ptr("border_right", "lv_style_t", (void*)&border_right);
    lv_style_init(&indicator_light);
    lv_style_set_border_width(&indicator_light, 6);
    lv_style_set_pad_left(&indicator_light, 10);
    lv_style_set_margin_left(&indicator_light, 10);
    lv_style_set_border_opa(&indicator_light, 200);
    lv_style_set_border_side(&indicator_light, LV_BORDER_SIDE_LEFT);
    lvgl_json_register_ptr("indicator_light", "lv_style_t", (void*)&indicator_light);
    // Component definition 'axis_jog_speed_adjust' processed and stored.
    // Component definition 'axis_pos_display' processed and stored.
    // Component definition 'feed_rate_scale' processed and stored.
    // Component definition 'jog_feed' processed and stored.
    lv_style_init(&action_button);
    lv_style_set_size(&action_button, 45, 45);
    lv_style_set_bg_color(&action_button, lv_color_hex(0x1F95F6));
    lv_style_set_radius(&action_button, LV_RADIUS_CIRCLE);
    lvgl_json_register_ptr("action_button", "lv_style_t", (void*)&action_button);
    // Component definition 'nav_action_button' processed and stored.
    // Component definition 'jog_action_button' processed and stored.
    c_obj_1 = lv_obj_create(parent_obj);
    lv_obj_set_layout(c_obj_1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(c_obj_1, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(c_obj_1, &container, 0);
    lv_obj_set_size(c_obj_1, lv_pct(100), 320);
    lvgl_json_register_ptr("main", "lv_obj_t", (void*)c_obj_1);
    // Children of c_obj_1 (obj)
        c_obj_2 = lv_obj_create(c_obj_1);
        lv_obj_set_layout(c_obj_2, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(c_obj_2, LV_FLEX_FLOW_COLUMN);
        lv_obj_add_style(c_obj_2, &container, 0);
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
            // Using view component 'axis_pos_display' with context CJSONObject([('axis', 'X'), ('wcs_pos', '11.000'), ('abs_pos', '51.000'), ('delta_pos', '2.125'), ('name', 'axis_pos_x')])
                c_obj_3 = lv_obj_create(c_obj_2);
                lv_obj_add_style(c_obj_3, &flex_y, 0);
                lv_obj_add_style(c_obj_3, &container, 0);
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
                lvgl_json_register_ptr("main:axis_pos_x", "lv_obj_t", (void*)c_obj_3);
                // Children of c_obj_3 (obj)
                    c_obj_4 = lv_obj_create(c_obj_3);
                    lv_obj_add_style(c_obj_4, &flex_x, 0);
                    lv_obj_add_style(c_obj_4, &container, 0);
                    lv_obj_set_size(c_obj_4, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_4 (obj)
                        c_label_5 = lv_label_create(c_obj_4);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_5, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_5, "X");
                        lv_obj_set_width(c_label_5, LV_SIZE_CONTENT);
                        lv_obj_add_style(c_label_5, &indicator_light, 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                        lv_obj_set_style_border_color(c_label_5, lv_color_hex(0x55FF55), 0);
                        c_label_6 = lv_label_create(c_obj_4);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_6, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_6, "11.000");
                        lv_obj_set_flex_grow(c_label_6, 1);
                        lv_obj_add_style(c_label_6, &indicator_green, 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_6, LV_TEXT_ALIGN_RIGHT, 0);
                    c_obj_7 = lv_obj_create(c_obj_3);
                    lv_obj_add_style(c_obj_7, &flex_x, 0);
                    lv_obj_add_style(c_obj_7, &container, 0);
                    lv_obj_set_size(c_obj_7, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_7 (obj)
                        c_label_8 = lv_label_create(c_obj_7);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_8, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_8, "51.000");
                        lv_obj_set_flex_grow(c_label_8, 1);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_8, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_8, &indicator_yellow, 0);
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
                        lv_obj_add_style(c_label_10, &indicator_yellow, 0);
                        lv_obj_set_flex_grow(c_label_10, 1);
                        c_label_11 = lv_label_create(c_obj_7);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_11, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_11, "");
                        lv_obj_set_width(c_label_11, 14);
            // Using view component 'axis_pos_display' with context CJSONObject([('axis', 'Y'), ('wcs_pos', '22.000'), ('abs_pos', '72.000'), ('delta_pos', '-12.125'), ('name', 'axis_pos_y')])
                c_obj_12 = lv_obj_create(c_obj_2);
                lv_obj_add_style(c_obj_12, &flex_y, 0);
                lv_obj_add_style(c_obj_12, &container, 0);
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
                lvgl_json_register_ptr("main:axis_pos_y", "lv_obj_t", (void*)c_obj_12);
                // Children of c_obj_12 (obj)
                    c_obj_13 = lv_obj_create(c_obj_12);
                    lv_obj_add_style(c_obj_13, &flex_x, 0);
                    lv_obj_add_style(c_obj_13, &container, 0);
                    lv_obj_set_size(c_obj_13, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_13 (obj)
                        c_label_14 = lv_label_create(c_obj_13);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_14, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_14, "Y");
                        lv_obj_set_width(c_label_14, LV_SIZE_CONTENT);
                        lv_obj_add_style(c_label_14, &indicator_light, 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                        lv_obj_set_style_border_color(c_label_14, lv_color_hex(0x55FF55), 0);
                        c_label_15 = lv_label_create(c_obj_13);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_15, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_15, "22.000");
                        lv_obj_set_flex_grow(c_label_15, 1);
                        lv_obj_add_style(c_label_15, &indicator_green, 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_15, LV_TEXT_ALIGN_RIGHT, 0);
                    c_obj_16 = lv_obj_create(c_obj_12);
                    lv_obj_add_style(c_obj_16, &flex_x, 0);
                    lv_obj_add_style(c_obj_16, &container, 0);
                    lv_obj_set_size(c_obj_16, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_16 (obj)
                        c_label_17 = lv_label_create(c_obj_16);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_17, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_17, "72.000");
                        lv_obj_set_flex_grow(c_label_17, 1);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_17, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_17, &indicator_yellow, 0);
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
                        lv_obj_add_style(c_label_19, &indicator_yellow, 0);
                        lv_obj_set_flex_grow(c_label_19, 1);
                        c_label_20 = lv_label_create(c_obj_16);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_20, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_20, "");
                        lv_obj_set_width(c_label_20, 14);
            // Using view component 'axis_pos_display' with context CJSONObject([('axis', 'Z'), ('wcs_pos', '1.000'), ('abs_pos', '1.000'), ('delta_pos', '0.125'), ('name', 'axis_pos_z')])
                c_obj_21 = lv_obj_create(c_obj_2);
                lv_obj_add_style(c_obj_21, &flex_y, 0);
                lv_obj_add_style(c_obj_21, &container, 0);
                lv_obj_set_size(c_obj_21, lv_pct(100), LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_all
                lv_obj_set_style_pad_all(c_obj_21, 10, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_21, 18, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                lv_obj_set_style_border_color(c_obj_21, lv_color_hex(0xFFFFFF), 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_opa
                lv_obj_set_style_border_opa(c_obj_21, 40, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_all
                lv_obj_set_style_margin_all(c_obj_21, 2, 0);
                lvgl_json_register_ptr("main:axis_pos_z", "lv_obj_t", (void*)c_obj_21);
                // Children of c_obj_21 (obj)
                    c_obj_22 = lv_obj_create(c_obj_21);
                    lv_obj_add_style(c_obj_22, &flex_x, 0);
                    lv_obj_add_style(c_obj_22, &container, 0);
                    lv_obj_set_size(c_obj_22, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_22 (obj)
                        c_label_23 = lv_label_create(c_obj_22);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_23, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_23, "Z");
                        lv_obj_set_width(c_label_23, LV_SIZE_CONTENT);
                        lv_obj_add_style(c_label_23, &indicator_light, 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_border_color
                        lv_obj_set_style_border_color(c_label_23, lv_color_hex(0x55FF55), 0);
                        c_label_24 = lv_label_create(c_obj_22);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_24, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_24, "1.000");
                        lv_obj_set_flex_grow(c_label_24, 1);
                        lv_obj_add_style(c_label_24, &indicator_green, 0);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_24, LV_TEXT_ALIGN_RIGHT, 0);
                    c_obj_25 = lv_obj_create(c_obj_21);
                    lv_obj_add_style(c_obj_25, &flex_x, 0);
                    lv_obj_add_style(c_obj_25, &container, 0);
                    lv_obj_set_size(c_obj_25, lv_pct(100), LV_SIZE_CONTENT);
                    // Children of c_obj_25 (obj)
                        c_label_26 = lv_label_create(c_obj_25);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_26, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_26, "1.000");
                        lv_obj_set_flex_grow(c_label_26, 1);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_26, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_26, &indicator_yellow, 0);
                        c_label_27 = lv_label_create(c_obj_25);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_27, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_27, "");
                        lv_obj_set_width(c_label_27, 14);
                        c_label_28 = lv_label_create(c_obj_25);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_28, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_28, "0.125");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_align
                        lv_obj_set_style_text_align(c_label_28, LV_TEXT_ALIGN_RIGHT, 0);
                        lv_obj_add_style(c_label_28, &indicator_yellow, 0);
                        lv_obj_set_flex_grow(c_label_28, 1);
                        c_label_29 = lv_label_create(c_obj_25);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_29, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_14", "lv_font_t *")), 0);
                        lv_label_set_text(c_label_29, "");
                        lv_obj_set_width(c_label_29, 14);
        c_obj_30 = lv_obj_create(c_obj_1);
        lv_obj_set_layout(c_obj_30, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(c_obj_30, LV_FLEX_FLOW_COLUMN);
        lv_obj_add_style(c_obj_30, &container, 0);
        lv_obj_set_height(c_obj_30, lv_pct(100));
        lv_obj_set_flex_grow(c_obj_30, 45);
        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_top
        lv_obj_set_style_pad_top(c_obj_30, 5, 0);
        // Children of c_obj_30 (obj)
            // Using view component 'feed_rate_scale' with context CJSONObject([('unit', 'MM/MIN'), ('label', 'FEED'), ('letter', 'F'), ('ovr', 'Feed Ovr'), ('pad_bottom', 12)])
                c_obj_31 = lv_obj_create(c_obj_30);
                lv_obj_set_size(c_obj_31, lv_pct(100), lv_pct(100));
                lv_obj_add_style(c_obj_31, &container, 0);
                lv_obj_set_layout(c_obj_31, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_31, LV_FLEX_FLOW_ROW);
                lv_obj_set_height(c_obj_31, LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column
                lv_obj_set_style_pad_column(c_obj_31, 0, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_31, 12, 0);
                // Children of c_obj_31 (obj)
                    c_obj_32 = lv_obj_create(c_obj_31);
                    lv_obj_set_layout(c_obj_32, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_32, &container, 0);
                    lv_obj_set_flex_flow(c_obj_32, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_32, lv_pct(100));
                    lv_obj_set_height(c_obj_32, LV_SIZE_CONTENT);
                    lv_obj_set_flex_grow(c_obj_32, 1);
                    // Children of c_obj_32 (obj)
                        c_label_33 = lv_label_create(c_obj_32);
                        lv_label_set_text(c_label_33, "FEED");
                        lv_obj_set_height(c_label_33, LV_SIZE_CONTENT);
                        lv_obj_set_width(c_label_33, lv_pct(100));
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_33, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_33, &border_top_btm, 0);
                        c_grid_obj_34 = lv_obj_create(c_obj_32);
                        lv_obj_set_layout(c_grid_obj_34, LV_LAYOUT_GRID);
                        lv_obj_add_style(c_grid_obj_34, &container, 0);
                        lv_obj_set_width(c_grid_obj_34, lv_pct(100));
                        lv_obj_set_height(c_grid_obj_34, LV_SIZE_CONTENT);
                        lv_obj_set_grid_dsc_array(c_grid_obj_34, c_coord_array_35, c_coord_array_36);
                        // Children of c_grid_obj_34 (grid)
                            c_label_37 = lv_label_create(c_grid_obj_34);
                            lv_label_set_text(c_label_37, "F");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_37, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_37, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                            lv_obj_set_height(c_label_37, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left
                            lv_obj_set_style_pad_left(c_label_37, 10, 0);
                            c_label_38 = lv_label_create(c_grid_obj_34);
                            lv_obj_set_grid_cell(c_label_38, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_38, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_38, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_38, "1000");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                            lv_obj_set_style_pad_right(c_label_38, 10, 0);
                        c_obj_39 = lv_obj_create(c_obj_32);
                        lv_obj_set_layout(c_obj_39, LV_LAYOUT_FLEX);
                        lv_obj_set_flex_flow(c_obj_39, LV_FLEX_FLOW_COLUMN);
                        lv_obj_add_style(c_obj_39, &container, 0);
                        lv_obj_set_width(c_obj_39, lv_pct(100));
                        lv_obj_set_height(c_obj_39, LV_SIZE_CONTENT);
                        // Children of c_obj_39 (obj)
                            c_bar_40 = lv_bar_create(c_obj_39);
                            lv_obj_set_width(c_bar_40, lv_pct(100));
                            lv_obj_set_height(c_bar_40, 15);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_bar_40, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_bar_40, 15, 0);
                            lv_obj_add_style(c_bar_40, &bar_indicator, LV_PART_MAIN);
                            lv_obj_add_style(c_bar_40, &bar_indicator, LV_PART_INDICATOR);
                            lv_bar_set_value(c_bar_40, 65, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color
                            lv_obj_set_style_bg_color(c_bar_40, lv_color_hex(0x5DD555), 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa
                            lv_obj_set_style_bg_opa(c_bar_40, 255, 0);
                            c_scale_41 = lv_scale_create(c_obj_39);
                            lv_obj_set_width(c_scale_41, lv_pct(100));
                            lv_obj_set_height(c_scale_41, 18);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_scale_41, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_scale_41, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_scale_41, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t *")), 0);
                    c_obj_42 = lv_obj_create(c_obj_31);
                    lv_obj_set_layout(c_obj_42, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_42, &container, 0);
                    lv_obj_set_flex_flow(c_obj_42, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_42, LV_SIZE_CONTENT);
                    lv_obj_set_height(c_obj_42, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                    lv_obj_set_style_pad_right(c_obj_42, 0, 0);
                    lv_obj_set_flex_align(c_obj_42, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                    // Children of c_obj_42 (obj)
                        c_label_43 = lv_label_create(c_obj_42);
                        lv_label_set_text(c_label_43, "MM/MIN");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_43, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_43, &border_top_btm, 0);
                        c_label_44 = lv_label_create(c_obj_42);
                        lv_label_set_text(c_label_44, "Feed Ovr");
                        c_label_45 = lv_label_create(c_obj_42);
                        lv_label_set_text(c_label_45, "100%");
                        c_label_46 = lv_label_create(c_obj_42);
                        lv_label_set_text(c_label_46, "65%");
            // Using view component 'feed_rate_scale' with context CJSONObject([('unit', '/MIN'), ('label', 'SPEED'), ('letter', 'S'), ('ovr', 'Speed Ovr'), ('pad_bottom', 12)])
                c_obj_47 = lv_obj_create(c_obj_30);
                lv_obj_set_size(c_obj_47, lv_pct(100), lv_pct(100));
                lv_obj_add_style(c_obj_47, &container, 0);
                lv_obj_set_layout(c_obj_47, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_47, LV_FLEX_FLOW_ROW);
                lv_obj_set_height(c_obj_47, LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column
                lv_obj_set_style_pad_column(c_obj_47, 0, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_47, 12, 0);
                // Children of c_obj_47 (obj)
                    c_obj_48 = lv_obj_create(c_obj_47);
                    lv_obj_set_layout(c_obj_48, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_48, &container, 0);
                    lv_obj_set_flex_flow(c_obj_48, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_48, lv_pct(100));
                    lv_obj_set_height(c_obj_48, LV_SIZE_CONTENT);
                    lv_obj_set_flex_grow(c_obj_48, 1);
                    // Children of c_obj_48 (obj)
                        c_label_49 = lv_label_create(c_obj_48);
                        lv_label_set_text(c_label_49, "SPEED");
                        lv_obj_set_height(c_label_49, LV_SIZE_CONTENT);
                        lv_obj_set_width(c_label_49, lv_pct(100));
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_49, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_49, &border_top_btm, 0);
                        c_grid_obj_50 = lv_obj_create(c_obj_48);
                        lv_obj_set_layout(c_grid_obj_50, LV_LAYOUT_GRID);
                        lv_obj_add_style(c_grid_obj_50, &container, 0);
                        lv_obj_set_width(c_grid_obj_50, lv_pct(100));
                        lv_obj_set_height(c_grid_obj_50, LV_SIZE_CONTENT);
                        lv_obj_set_grid_dsc_array(c_grid_obj_50, c_coord_array_51, c_coord_array_52);
                        // Children of c_grid_obj_50 (grid)
                            c_label_53 = lv_label_create(c_grid_obj_50);
                            lv_label_set_text(c_label_53, "S");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_53, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_53, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
                            lv_obj_set_height(c_label_53, LV_SIZE_CONTENT);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left
                            lv_obj_set_style_pad_left(c_label_53, 10, 0);
                            c_label_54 = lv_label_create(c_grid_obj_50);
                            lv_obj_set_grid_cell(c_label_54, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_54, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_30", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_54, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_54, "1000");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                            lv_obj_set_style_pad_right(c_label_54, 10, 0);
                        c_obj_55 = lv_obj_create(c_obj_48);
                        lv_obj_set_layout(c_obj_55, LV_LAYOUT_FLEX);
                        lv_obj_set_flex_flow(c_obj_55, LV_FLEX_FLOW_COLUMN);
                        lv_obj_add_style(c_obj_55, &container, 0);
                        lv_obj_set_width(c_obj_55, lv_pct(100));
                        lv_obj_set_height(c_obj_55, LV_SIZE_CONTENT);
                        // Children of c_obj_55 (obj)
                            c_bar_56 = lv_bar_create(c_obj_55);
                            lv_obj_set_width(c_bar_56, lv_pct(100));
                            lv_obj_set_height(c_bar_56, 15);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_bar_56, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_bar_56, 15, 0);
                            lv_obj_add_style(c_bar_56, &bar_indicator, LV_PART_MAIN);
                            lv_obj_add_style(c_bar_56, &bar_indicator, LV_PART_INDICATOR);
                            lv_bar_set_value(c_bar_56, 65, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_color
                            lv_obj_set_style_bg_color(c_bar_56, lv_color_hex(0x5DD555), 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_bg_opa
                            lv_obj_set_style_bg_opa(c_bar_56, 255, 0);
                            c_scale_57 = lv_scale_create(c_obj_55);
                            lv_obj_set_width(c_scale_57, lv_pct(100));
                            lv_obj_set_height(c_scale_57, 18);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_left
                            lv_obj_set_style_margin_left(c_scale_57, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_margin_right
                            lv_obj_set_style_margin_right(c_scale_57, 15, 0);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_scale_57, ((lv_font_t *)lvgl_json_get_registered_ptr("font_montserrat_12", "lv_font_t *")), 0);
                    c_obj_58 = lv_obj_create(c_obj_47);
                    lv_obj_set_layout(c_obj_58, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_58, &container, 0);
                    lv_obj_set_flex_flow(c_obj_58, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_58, LV_SIZE_CONTENT);
                    lv_obj_set_height(c_obj_58, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                    lv_obj_set_style_pad_right(c_obj_58, 0, 0);
                    lv_obj_set_flex_align(c_obj_58, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER);
                    // Children of c_obj_58 (obj)
                        c_label_59 = lv_label_create(c_obj_58);
                        lv_label_set_text(c_label_59, "/MIN");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_59, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_59, &border_top_btm, 0);
                        c_label_60 = lv_label_create(c_obj_58);
                        lv_label_set_text(c_label_60, "Speed Ovr");
                        c_label_61 = lv_label_create(c_obj_58);
                        lv_label_set_text(c_label_61, "100%");
                        c_label_62 = lv_label_create(c_obj_58);
                        lv_label_set_text(c_label_62, "65%");
            // Using view component 'jog_feed' with context CJSONObject([('unit', 'MM'), ('label', 'JOG'), ('ovr', 'Jog Ovr'), ('pad_bottom', 0)])
                c_obj_63 = lv_obj_create(c_obj_30);
                lv_obj_set_size(c_obj_63, lv_pct(100), lv_pct(100));
                lv_obj_add_style(c_obj_63, &container, 0);
                lv_obj_set_layout(c_obj_63, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(c_obj_63, LV_FLEX_FLOW_ROW);
                lv_obj_set_height(c_obj_63, LV_SIZE_CONTENT);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_column
                lv_obj_set_style_pad_column(c_obj_63, 0, 0);
                // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_bottom
                lv_obj_set_style_pad_bottom(c_obj_63, 0, 0);
                // Children of c_obj_63 (obj)
                    c_obj_64 = lv_obj_create(c_obj_63);
                    lv_obj_set_layout(c_obj_64, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_64, &container, 0);
                    lv_obj_set_flex_flow(c_obj_64, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_64, lv_pct(100));
                    lv_obj_set_height(c_obj_64, LV_SIZE_CONTENT);
                    lv_obj_set_flex_grow(c_obj_64, 1);
                    // Children of c_obj_64 (obj)
                        c_label_65 = lv_label_create(c_obj_64);
                        lv_label_set_text(c_label_65, "JOG");
                        lv_obj_set_height(c_label_65, LV_SIZE_CONTENT);
                        lv_obj_set_width(c_label_65, lv_pct(100));
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_65, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_65, &border_top_btm, 0);
                        c_grid_obj_66 = lv_obj_create(c_obj_64);
                        lv_obj_set_layout(c_grid_obj_66, LV_LAYOUT_GRID);
                        lv_obj_add_style(c_grid_obj_66, &container, 0);
                        lv_obj_set_width(c_grid_obj_66, lv_pct(100));
                        lv_obj_set_height(c_grid_obj_66, LV_SIZE_CONTENT);
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_left
                        lv_obj_set_style_pad_left(c_grid_obj_66, 10, 0);
                        lv_obj_set_grid_dsc_array(c_grid_obj_66, c_coord_array_67, c_coord_array_68);
                        // Children of c_grid_obj_66 (grid)
                            c_label_69 = lv_label_create(c_grid_obj_66);
                            lv_label_set_text(c_label_69, "XY");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_69, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_69, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
                            lv_obj_set_height(c_label_69, LV_SIZE_CONTENT);
                            c_label_70 = lv_label_create(c_grid_obj_66);
                            lv_obj_set_grid_cell(c_label_70, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_70, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_70, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_70, "10");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                            lv_obj_set_style_pad_right(c_label_70, 10, 0);
                            lv_obj_add_style(c_label_70, &border_right, 0);
                            lv_obj_add_style(c_label_70, &indicator_yellow, 0);
                            c_label_71 = lv_label_create(c_grid_obj_66);
                            lv_label_set_text(c_label_71, "Z");
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_71, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_grid_cell(c_label_71, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 0, 1);
                            lv_obj_set_height(c_label_71, LV_SIZE_CONTENT);
                            c_label_72 = lv_label_create(c_grid_obj_66);
                            lv_obj_set_grid_cell(c_label_72, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 0, 1);
                            // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                            lv_obj_set_style_text_font(c_label_72, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_24", "lv_font_t *")), 0);
                            lv_obj_set_height(c_label_72, LV_SIZE_CONTENT);
                            lv_label_set_text(c_label_72, " 5");
                            lv_obj_add_style(c_label_72, &indicator_yellow, 0);
                    c_obj_73 = lv_obj_create(c_obj_63);
                    lv_obj_set_layout(c_obj_73, LV_LAYOUT_FLEX);
                    lv_obj_add_style(c_obj_73, &container, 0);
                    lv_obj_set_flex_flow(c_obj_73, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_width(c_obj_73, LV_SIZE_CONTENT);
                    lv_obj_set_height(c_obj_73, LV_SIZE_CONTENT);
                    // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_pad_right
                    lv_obj_set_style_pad_right(c_obj_73, 0, 0);
                    // Children of c_obj_73 (obj)
                        c_label_74 = lv_label_create(c_obj_73);
                        lv_label_set_text(c_label_74, "MM");
                        // INFO: Adding default selector LV_PART_MAIN (0) for lv_obj_set_style_text_font
                        lv_obj_set_style_text_font(c_label_74, ((lv_font_t *)lvgl_json_get_registered_ptr("font_kode_20", "lv_font_t *")), 0);
                        lv_obj_add_style(c_label_74, &border_top_btm, 0);
                        c_label_75 = lv_label_create(c_obj_73);
                        lv_label_set_text(c_label_75, "Jog Ovr");
                        c_label_76 = lv_label_create(c_obj_73);
                        lv_label_set_text(c_label_76, "100%");
    // Using view component 'nav_action_button' with context {}
        c_dropdown_77 = lv_dropdown_create(parent_obj);
        lv_obj_add_style(c_dropdown_77, &action_button, 0);
        lv_dropdown_set_options(c_dropdown_77, " Jog\n Probe\n Status\n X\n y\n Z\n Off");
        lv_obj_align(c_dropdown_77, LV_ALIGN_BOTTOM_LEFT, 20, -10);
        lv_obj_add_flag(c_dropdown_77, LV_OBJ_FLAG_FLOATING);
        lv_dropdown_set_text(c_dropdown_77, "");
        lv_dropdown_set_symbol(c_dropdown_77, NULL);
        lv_obj_move_foreground(c_dropdown_77);
        // WARNING: No setter function found for property 'with' on type 'dropdown'. Skipping.
    // Using view component 'jog_action_button' with context {}
        c_dropdown_78 = lv_dropdown_create(parent_obj);
        lv_dropdown_set_options(c_dropdown_78, " Home\n Zero\n G54\n G55\n G56\n G57\n G58");
        lv_obj_add_style(c_dropdown_78, &action_button, 0);
        lv_obj_align(c_dropdown_78, LV_ALIGN_BOTTOM_LEFT, 90, -10);
        lv_obj_add_flag(c_dropdown_78, LV_OBJ_FLAG_FLOATING);
        lv_dropdown_set_text(c_dropdown_78, "");
        lv_dropdown_set_symbol(c_dropdown_78, NULL);
        lv_obj_move_foreground(c_dropdown_78);
        lvgl_json_register_ptr("jog_action_button", "lv_dropdown_t", (void*)c_dropdown_78);
        // WARNING: No setter function found for property 'with' on type 'dropdown'. Skipping.
}