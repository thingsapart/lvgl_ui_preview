#ifndef VIEW_DEFINE_MACROS_H
#define VIEW_DEFINE_MACROS_H

#include "lvgl.h"
#include "lv_vfl.h" // Need the view map definition

// --- Configuration ---
#ifndef dv_max_args_per_item // Max args for container, widget, or selector block
#define dv_max_args_per_item 64
#endif

// --- Internal Context Variables ---
// (Defined within view scope)
// static lv_obj_t* _dv_parent_obj = NULL;
// static lv_vfl_view_map_t* _dv_view_map_ptr = NULL;
// static size_t* _dv_view_map_count_ptr = NULL;
// static lv_obj_t* _dv_current_widget = NULL;

static lv_style_selector_t _dv_current_selector = LV_PART_MAIN | LV_STATE_DEFAULT;

#include "meta/macro_helpers.h"

// --- Parent Container Property Macros ---
// These operate on _dv_parent_obj
// Setters (prefixed with _)
#define _parent_size(w, h)              lv_obj_set_size(_dv_parent_obj, w, h);
#define _parent_align(align, x, y)      lv_obj_align(_dv_parent_obj, align, x, y);
#define _parent_style_border_width(w)   lv_obj_set_style_border_width(_dv_parent_obj, w, LV_PART_MAIN);
#define _parent_style_border_color(c)   lv_obj_set_style_border_color(_dv_parent_obj, c, LV_PART_MAIN);
#define _parent_style_pad_all(p)        lv_obj_set_style_pad_all(_dv_parent_obj, p, LV_PART_MAIN);
#define _parent_style_pad_ver(p)        lv_obj_set_style_pad_ver(_dv_parent_obj, p, LV_PART_MAIN);
#define _parent_style_pad_hor(p)        lv_obj_set_style_pad_hor(_dv_parent_obj, p, LV_PART_MAIN);
// Add more _parent_style_xxx as needed

// --- Widget Definition Macros ---
// These create a widget, add it to the map, set _dv_current_widget, reset _dv_current_selector,
// and process the remaining arguments (__VA_ARGS__) which include direct properties and styles.
#define _setup_widget(WidgetType, CreateFunc, VarName, ...) \
    VarName = CreateFunc(_dv_parent_obj); \
    _dv_current_widget = VarName; \
    _dv_current_selector = LV_PART_MAIN | LV_STATE_DEFAULT; /* Reset selector for this widget */ \
    __max_client_area(); \
    _process_args(__VA_ARGS__) /* Process properties/styles */

#define _place_widget(WidgetType, VarName, ...) \
    do { \
      _dv_current_widget = VarName; \
      _dv_current_selector = LV_PART_MAIN | LV_STATE_DEFAULT; /* Reset selector for this widget */ \
      __max_client_area(); \
      _process_args(__VA_ARGS__) /* Process properties/styles */ \
    } while (0);

#define obj(VarName, ...)     _setup_widget(lv_obj_t, lv_obj_create, VarName, __VA_ARGS__)
#define label(VarName, Text, ...)   _setup_widget(lv_obj_t, lv_label_create, VarName, _text(Text), __text_align(LV_TEXT_ALIGN_LEFT), __VA_ARGS__)
#define button(VarName, ...)  _setup_widget(lv_obj_t, lv_btn_create, VarName, __VA_ARGS__)
#define list(VarName, ...)  _setup_widget(lv_obj_t, lv_list_create, VarName, __VA_ARGS__)
#define textarea(VarName, ...) _setup_widget(lv_obj_t, lv_textarea_create, VarName, __VA_ARGS__)
#define sub_view(VarName, ...) _place_widget(lv_obj_t, VarName, __VA_ARGS__)

// ... Add other widget creation macros (switch, slider, image, etc.) following the same pattern.
// Example:
// #define switch(VarName, ...) _setup_widget(lv_obj_t, lv_switch_create, VarName, __VA_ARGS__)
// #define image(VarName, Src, ...) _setup_widget(lv_obj_t, lv_image_create, VarName, _src(Src), __VA_ARGS__)

// --- Widget Direct Property/Function Setter Macros (prefixed with _) ---
#define _size(w, h)                 lv_obj_set_size(_dv_current_widget, w, h);
#define _width(val)                 lv_obj_set_width(_dv_current_widget, val);
#define _height(val)                lv_obj_set_height(_dv_current_widget, val);
#define _align(align, x, y)         lv_obj_align(_dv_current_widget, align, x, y);
#define _add_flag(f)                lv_obj_add_flag(_dv_current_widget, f);
#define _clear_flag(f)              lv_obj_clear_flag(_dv_current_widget, f);
#define _add_state(s)               lv_obj_add_state(_dv_current_widget, s);
#define _clear_state(s)             lv_obj_clear_state(_dv_current_widget, s);

// Widget-specific direct property setters
#define _text(t)                    lv_label_set_text(_dv_current_widget, t); // Use specific setter if widget type known, otherwise generic
#define _label_text(t)              lv_label_set_text(_dv_current_widget, t);
#define _textarea_text(t)           lv_textarea_set_text(_dv_current_widget, t);
#define _placeholder_text(t)        lv_textarea_set_placeholder_text(_dv_current_widget, t);
#define _one_line(b)                lv_textarea_set_one_line(_dv_current_widget, b);
#define _image_src(s)               lv_image_set_src(_dv_current_widget, s);
// #define _button_label_text(t)    do { lv_obj_t* lbl = lv_obj_get_child(_dv_current_widget, 0); if(lbl && lv_obj_check_type(lbl, &lv_label_class)) lv_label_set_text(lbl, t); } while(0)


// --- Widget Direct Property/Function Getter Macros (suffixed with _) ---
// Note: Getters typically don't take arguments in this context, they return a value.
// They operate on _dv_current_widget.
#define size_()                     lv_obj_get_size(_dv_current_widget) // Returns lv_point_t
#define width_()                    lv_obj_get_width(_dv_current_widget)
#define height_()                   lv_obj_get_height(_dv_current_widget)
#define x_()                        lv_obj_get_x(_dv_current_widget)
#define y_()                        lv_obj_get_y(_dv_current_widget)
#define content_width_()            lv_obj_get_content_width(_dv_current_widget)
#define content_height_()           lv_obj_get_content_height(_dv_current_widget)
#define self_width_()               lv_obj_get_self_width(_dv_current_widget)
#define self_height_()              lv_obj_get_self_height(_dv_current_widget)
#define has_flag_(f)                lv_obj_has_flag(_dv_current_widget, f)
#define has_state_(s)               lv_obj_has_state(_dv_current_widget, s)
#define state_()                    lv_obj_get_state(_dv_current_widget)

// Widget-specific direct property getters
#define label_text_()               lv_label_get_text(_dv_current_widget)
#define textarea_text_()            lv_textarea_get_text(_dv_current_widget)
#define placeholder_text_()         lv_textarea_get_placeholder_text(_dv_current_widget)
#define one_line_()                 lv_textarea_get_one_line(_dv_current_widget)
#define image_src_()                lv_image_get_src(_dv_current_widget)


// --- Widget Direct Function Macros (no prefix/suffix) ---
// These call actions or setup functions that aren't simple setters/getters
#define center()                    lv_obj_center(_dv_current_widget);
#define scroll_to_view(anim)        lv_obj_scroll_to_view(_dv_current_widget, anim);
#define add_event_cb(cb, filter, data) lv_obj_add_event_cb(_dv_current_widget, cb, filter, data);


// --- Widget Style Setter Macros (prefixed with _) ---
// These call lv_obj_set_style_... functions using _dv_current_widget and the current _dv_current_selector.
// Size & Position Styles
#define _min_width(value)               lv_obj_set_style_min_width(_dv_current_widget, value, _dv_current_selector);
#define _max_width(value)               lv_obj_set_style_max_width(_dv_current_widget, value, _dv_current_selector);
#define _min_height(value)              lv_obj_set_style_min_height(_dv_current_widget, value, _dv_current_selector);
#define _max_height(value)              lv_obj_set_style_max_height(_dv_current_widget, value, _dv_current_selector);
#define _length(value)                  lv_obj_set_style_length(_dv_current_widget, value, _dv_current_selector);
#define _x(value)                       lv_obj_set_style_x(_dv_current_widget, value, _dv_current_selector);
#define _y(value)                       lv_obj_set_style_y(_dv_current_widget, value, _dv_current_selector);

// Transform Styles
#define _transform_width(value)         lv_obj_set_style_transform_width(_dv_current_widget, value, _dv_current_selector);
#define _transform_height(value)        lv_obj_set_style_transform_height(_dv_current_widget, value, _dv_current_selector);
#define _translate_x(value)             lv_obj_set_style_translate_x(_dv_current_widget, value, _dv_current_selector);
#define _translate_y(value)             lv_obj_set_style_translate_y(_dv_current_widget, value, _dv_current_selector);
#define _translate_radial(value)        lv_obj_set_style_translate_radial(_dv_current_widget, value, _dv_current_selector);
#define _transform_scale_x(value)       lv_obj_set_style_transform_scale_x(_dv_current_widget, value, _dv_current_selector);
#define _transform_scale_y(value)       lv_obj_set_style_transform_scale_y(_dv_current_widget, value, _dv_current_selector);
#define _transform_rotation(value)      lv_obj_set_style_transform_rotation(_dv_current_widget, value, _dv_current_selector);
#define _transform_pivot_x(value)       lv_obj_set_style_transform_pivot_x(_dv_current_widget, value, _dv_current_selector);
#define _transform_pivot_y(value)       lv_obj_set_style_transform_pivot_y(_dv_current_widget, value, _dv_current_selector);
#define _transform_skew_x(value)        lv_obj_set_style_transform_skew_x(_dv_current_widget, value, _dv_current_selector);
#define _transform_skew_y(value)        lv_obj_set_style_transform_skew_y(_dv_current_widget, value, _dv_current_selector);
// Padding Styles
#define _pad_top(value)                 lv_obj_set_style_pad_top(_dv_current_widget, value, _dv_current_selector);
#define _pad_bottom(value)              lv_obj_set_style_pad_bottom(_dv_current_widget, value, _dv_current_selector);
#define _pad_left(value)                lv_obj_set_style_pad_left(_dv_current_widget, value, _dv_current_selector);
#define _pad_right(value)               lv_obj_set_style_pad_right(_dv_current_widget, value, _dv_current_selector);
#define _pad_row(value)                 lv_obj_set_style_pad_row(_dv_current_widget, value, _dv_current_selector);
#define _pad_column(value)              lv_obj_set_style_pad_column(_dv_current_widget, value, _dv_current_selector);
#define _pad_radial(value)              lv_obj_set_style_pad_radial(_dv_current_widget, value, _dv_current_selector);
#define _pad_all(value)                 lv_obj_set_style_pad_all(_dv_current_widget, value, _dv_current_selector);
#define _pad_hor(value)                 lv_obj_set_style_pad_hor(_dv_current_widget, value, _dv_current_selector);
#define _pad_ver(value)                 lv_obj_set_style_pad_ver(_dv_current_widget, value, _dv_current_selector);
// Margin Styles
#define _margin_top(value)              lv_obj_set_style_margin_top(_dv_current_widget, value, _dv_current_selector);
#define _margin_bottom(value)           lv_obj_set_style_margin_bottom(_dv_current_widget, value, _dv_current_selector);
#define _margin_left(value)             lv_obj_set_style_margin_left(_dv_current_widget, value, _dv_current_selector);
#define _margin_right(value)            lv_obj_set_style_margin_right(_dv_current_widget, value, _dv_current_selector);
// Background Styles
#define _bg_color(value)                lv_obj_set_style_bg_color(_dv_current_widget, value, _dv_current_selector);
#define _bg_opa(value)                  lv_obj_set_style_bg_opa(_dv_current_widget, value, _dv_current_selector);
#define _bg_grad_color(value)           lv_obj_set_style_bg_grad_color(_dv_current_widget, value, _dv_current_selector);
#define _bg_grad_dir(value)             lv_obj_set_style_bg_grad_dir(_dv_current_widget, value, _dv_current_selector);
#define _bg_main_stop(value)            lv_obj_set_style_bg_main_stop(_dv_current_widget, value, _dv_current_selector);
#define _bg_grad_stop(value)            lv_obj_set_style_bg_grad_stop(_dv_current_widget, value, _dv_current_selector);
#define _bg_main_opa(value)             lv_obj_set_style_bg_main_opa(_dv_current_widget, value, _dv_current_selector);
#define _bg_grad_opa(value)             lv_obj_set_style_bg_grad_opa(_dv_current_widget, value, _dv_current_selector);
#define _bg_grad(value)                 lv_obj_set_style_bg_grad(_dv_current_widget, value, _dv_current_selector);
#define _bg_image_src(value)            lv_obj_set_style_bg_image_src(_dv_current_widget, value, _dv_current_selector);
#define _bg_image_opa(value)            lv_obj_set_style_bg_image_opa(_dv_current_widget, value, _dv_current_selector);
#define _bg_image_recolor(value)        lv_obj_set_style_bg_image_recolor(_dv_current_widget, value, _dv_current_selector);
#define _bg_image_recolor_opa(value)    lv_obj_set_style_bg_image_recolor_opa(_dv_current_widget, value, _dv_current_selector);
#define _bg_image_tiled(value)          lv_obj_set_style_bg_image_tiled(_dv_current_widget, value, _dv_current_selector);
// Border Styles
#define _border_color(value)            lv_obj_set_style_border_color(_dv_current_widget, value, _dv_current_selector);
#define _border_opa(value)              lv_obj_set_style_border_opa(_dv_current_widget, value, _dv_current_selector);
#define _border_width(value)            lv_obj_set_style_border_width(_dv_current_widget, value, _dv_current_selector);
#define _border_side(value)             lv_obj_set_style_border_side(_dv_current_widget, value, _dv_current_selector);
#define _border_post(value)             lv_obj_set_style_border_post(_dv_current_widget, value, _dv_current_selector);
// Outline Styles
#define _outline_width(value)           lv_obj_set_style_outline_width(_dv_current_widget, value, _dv_current_selector);
#define _outline_color(value)           lv_obj_set_style_outline_color(_dv_current_widget, value, _dv_current_selector);
#define _outline_opa(value)             lv_obj_set_style_outline_opa(_dv_current_widget, value, _dv_current_selector);
#define _outline_pad(value)             lv_obj_set_style_outline_pad(_dv_current_widget, value, _dv_current_selector);
// Shadow Styles
#define _shadow_width(value)            lv_obj_set_style_shadow_width(_dv_current_widget, value, _dv_current_selector);
#define _shadow_offset_x(value)         lv_obj_set_style_shadow_offset_x(_dv_current_widget, value, _dv_current_selector);
#define _shadow_offset_y(value)         lv_obj_set_style_shadow_offset_y(_dv_current_widget, value, _dv_current_selector);
#define _shadow_spread(value)           lv_obj_set_style_shadow_spread(_dv_current_widget, value, _dv_current_selector);
#define _shadow_color(value)            lv_obj_set_style_shadow_color(_dv_current_widget, value, _dv_current_selector);
#define _shadow_opa(value)              lv_obj_set_style_shadow_opa(_dv_current_widget, value, _dv_current_selector);
// Image Styles
#define _image_opa(value)               lv_obj_set_style_image_opa(_dv_current_widget, value, _dv_current_selector);
#define _image_recolor(value)           lv_obj_set_style_image_recolor(_dv_current_widget, value, _dv_current_selector);
#define _image_recolor_opa(value)       lv_obj_set_style_image_recolor_opa(_dv_current_widget, value, _dv_current_selector);
// Line Styles
#define _line_width(value)              lv_obj_set_style_line_width(_dv_current_widget, value, _dv_current_selector);
#define _line_dash_width(value)         lv_obj_set_style_line_dash_width(_dv_current_widget, value, _dv_current_selector);
#define _line_dash_gap(value)           lv_obj_set_style_line_dash_gap(_dv_current_widget, value, _dv_current_selector);
#define _line_rounded(value)            lv_obj_set_style_line_rounded(_dv_current_widget, value, _dv_current_selector);
#define _line_color(value)              lv_obj_set_style_line_color(_dv_current_widget, value, _dv_current_selector);
#define _line_opa(value)                lv_obj_set_style_line_opa(_dv_current_widget, value, _dv_current_selector);
// Arc Styles
#define _arc_width(value)               lv_obj_set_style_arc_width(_dv_current_widget, value, _dv_current_selector);
#define _arc_rounded(value)             lv_obj_set_style_arc_rounded(_dv_current_widget, value, _dv_current_selector);
#define _arc_color(value)               lv_obj_set_style_arc_color(_dv_current_widget, value, _dv_current_selector);
#define _arc_opa(value)                 lv_obj_set_style_arc_opa(_dv_current_widget, value, _dv_current_selector);
#define _arc_image_src(value)           lv_obj_set_style_arc_image_src(_dv_current_widget, value, _dv_current_selector);
// Text Styles
#define _text_color(value)              lv_obj_set_style_text_color(_dv_current_widget, value, _dv_current_selector);
#define _text_opa(value)                lv_obj_set_style_text_opa(_dv_current_widget, value, _dv_current_selector);
#define _text_font(value)               lv_obj_set_style_text_font(_dv_current_widget, value, _dv_current_selector);
#define _text_letter_space(value)       lv_obj_set_style_text_letter_space(_dv_current_widget, value, _dv_current_selector);
#define _text_line_space(value)         lv_obj_set_style_text_line_space(_dv_current_widget, value, _dv_current_selector);
#define _text_decor(value)              lv_obj_set_style_text_decor(_dv_current_widget, value, _dv_current_selector);
#define _text_align(value)              lv_obj_set_style_text_align(_dv_current_widget, value, _dv_current_selector);
#if LV_USE_FONT_SUBPX // Check LVGL config
#define _text_outline_stroke_color(value) lv_obj_set_style_text_outline_stroke_color(_dv_current_widget, value, _dv_current_selector);
#define _text_outline_stroke_width(value) lv_obj_set_style_text_outline_stroke_width(_dv_current_widget, value, _dv_current_selector);
#define _text_outline_stroke_opa(value)  lv_obj_set_style_text_outline_stroke_opa(_dv_current_widget, value, _dv_current_selector);
#endif // LV_USE_FONT_SUBPX
// Miscellaneous Styles
#define _radius(value)                  lv_obj_set_style_radius(_dv_current_widget, value, _dv_current_selector);
#define _radial_offset(value)           lv_obj_set_style_radial_offset(_dv_current_widget, value, _dv_current_selector);
#define _clip_corner(value)             lv_obj_set_style_clip_corner(_dv_current_widget, value, _dv_current_selector);
#define _opa(value)                     lv_obj_set_style_opa(_dv_current_widget, value, _dv_current_selector);
#define _opa_layered(value)             lv_obj_set_style_opa_layered(_dv_current_widget, value, _dv_current_selector);
#define _color_filter_dsc(value)        lv_obj_set_style_color_filter_dsc(_dv_current_widget, value, _dv_current_selector);
#define _color_filter_opa(value)        lv_obj_set_style_color_filter_opa(_dv_current_widget, value, _dv_current_selector);
#define _anim(value)                    lv_obj_set_style_anim(_dv_current_widget, value, _dv_current_selector);
#define _anim_duration(value)           lv_obj_set_style_anim_duration(_dv_current_widget, value, _dv_current_selector);
#define _transition(value)              lv_obj_set_style_transition(_dv_current_widget, value, _dv_current_selector);
#define _blend_mode(value)              lv_obj_set_style_blend_mode(_dv_current_widget, value, _dv_current_selector);
#define _layout(value)                  lv_obj_set_style_layout(_dv_current_widget, value, _dv_current_selector);
#define _base_dir(value)                lv_obj_set_style_base_dir(_dv_current_widget, value, _dv_current_selector);
#define _bitmap_mask_src(value)         lv_obj_set_style_bitmap_mask_src(_dv_current_widget, value, _dv_current_selector);
#define _rotary_sensitivity(value)      lv_obj_set_style_rotary_sensitivity(_dv_current_widget, value, _dv_current_selector);
// Flexbox Styles
#define _flex_flow(value)               lv_obj_set_style_flex_flow(_dv_current_widget, value, _dv_current_selector);
#define _flex_main_place(value)         lv_obj_set_style_flex_main_place(_dv_current_widget, value, _dv_current_selector);
#define _flex_cross_place(value)        lv_obj_set_style_flex_cross_place(_dv_current_widget, value, _dv_current_selector);
#define _flex_track_place(value)        lv_obj_set_style_flex_track_place(_dv_current_widget, value, _dv_current_selector);
#define _flex_grow(value)               lv_obj_set_style_flex_grow(_dv_current_widget, value, _dv_current_selector);
// Grid Styles
#define _grid_column_dsc_array(value)   lv_obj_set_style_grid_column_dsc_array(_dv_current_widget, value, _dv_current_selector);
#define _grid_column_align(value)       lv_obj_set_style_grid_column_align(_dv_current_widget, value, _dv_current_selector);
#define _grid_row_dsc_array(value)      lv_obj_set_style_grid_row_dsc_array(_dv_current_widget, value, _dv_current_selector);
#define _grid_row_align(value)          lv_obj_set_style_grid_row_align(_dv_current_widget, value, _dv_current_selector);
#define _grid_cell_column_pos(value)    lv_obj_set_style_grid_cell_column_pos(_dv_current_widget, value, _dv_current_selector);
#define _grid_cell_x_align(value)       lv_obj_set_style_grid_cell_x_align(_dv_current_widget, value, _dv_current_selector);
#define _grid_cell_column_span(value)   lv_obj_set_style_grid_cell_column_span(_dv_current_widget, value, _dv_current_selector);
#define _grid_cell_row_pos(value)       lv_obj_set_style_grid_cell_row_pos(_dv_current_widget, value, _dv_current_selector);
#define _grid_cell_y_align(value)       lv_obj_set_style_grid_cell_y_align(_dv_current_widget, value, _dv_current_selector);
#define _grid_cell_row_span(value)      lv_obj_set_style_grid_cell_row_span(_dv_current_widget, value, _dv_current_selector);
#define _grid_column_gap(value)         lv_obj_set_style_grid_column_gap(_dv_current_widget, value, _dv_current_selector);
#define _grid_row_gap(value)            lv_obj_set_style_grid_row_gap(_dv_current_widget, value, _dv_current_selector);


// --- Widget Style Getter Macros (suffixed with _) ---
// These call lv_obj_get_style_... functions using _dv_current_widget and the current _dv_current_selector.
// Note: Getters typically don't take arguments in this context, they return a value.
// Size & Position Styles
#define min_width_()                    lv_obj_get_style_min_width(_dv_current_widget, _dv_current_selector)
#define max_width_()                    lv_obj_get_style_max_width(_dv_current_widget, _dv_current_selector)
#define min_height_()                   lv_obj_get_style_min_height(_dv_current_widget, _dv_current_selector)
#define max_height_()                   lv_obj_get_style_max_height(_dv_current_widget, _dv_current_selector)
#define length_()                       lv_obj_get_style_length(_dv_current_widget, _dv_current_selector)
// Transform Styles
#define transform_width_()              lv_obj_get_style_transform_width(_dv_current_widget, _dv_current_selector)
#define transform_height_()             lv_obj_get_style_transform_height(_dv_current_widget, _dv_current_selector)
#define translate_x_()                 lv_obj_get_style_translate_x(_dv_current_widget, _dv_current_selector)
#define translate_y_()                 lv_obj_get_style_translate_y(_dv_current_widget, _dv_current_selector)
#define translate_radial_()             lv_obj_get_style_translate_radial(_dv_current_widget, _dv_current_selector)
#define transform_scale_x_()            lv_obj_get_style_transform_scale_x(_dv_current_widget, _dv_current_selector)
#define transform_scale_y_()            lv_obj_get_style_transform_scale_y(_dv_current_widget, _dv_current_selector)
#define transform_rotation_()           lv_obj_get_style_transform_rotation(_dv_current_widget, _dv_current_selector)
#define transform_pivot_x_()            lv_obj_get_style_transform_pivot_x(_dv_current_widget, _dv_current_selector)
#define transform_pivot_y_()            lv_obj_get_style_transform_pivot_y(_dv_current_widget, _dv_current_selector)
#define transform_skew_x_()             lv_obj_get_style_transform_skew_x(_dv_current_widget, _dv_current_selector)
#define transform_skew_y_()             lv_obj_get_style_transform_skew_y(_dv_current_widget, _dv_current_selector)
// Padding Styles
#define pad_top_()                      lv_obj_get_style_pad_top(_dv_current_widget, _dv_current_selector)
#define pad_bottom_()                   lv_obj_get_style_pad_bottom(_dv_current_widget, _dv_current_selector)
#define pad_left_()                     lv_obj_get_style_pad_left(_dv_current_widget, _dv_current_selector)
#define pad_right_()                    lv_obj_get_style_pad_right(_dv_current_widget, _dv_current_selector)
#define pad_row_()                      lv_obj_get_style_pad_row(_dv_current_widget, _dv_current_selector)
#define pad_column_()                   lv_obj_get_style_pad_column(_dv_current_widget, _dv_current_selector)
#define pad_radial_()                   lv_obj_get_style_pad_radial(_dv_current_widget, _dv_current_selector)
// No direct getters for pad_all, pad_hor, pad_ver
// Margin Styles
#define margin_top_()                   lv_obj_get_style_margin_top(_dv_current_widget, _dv_current_selector)
#define margin_bottom_()                lv_obj_get_style_margin_bottom(_dv_current_widget, _dv_current_selector)
#define margin_left_()                  lv_obj_get_style_margin_left(_dv_current_widget, _dv_current_selector)
#define margin_right_()                 lv_obj_get_style_margin_right(_dv_current_widget, _dv_current_selector)
// Background Styles
#define bg_color_()                     lv_obj_get_style_bg_color(_dv_current_widget, _dv_current_selector)
#define bg_opa_()                       lv_obj_get_style_bg_opa(_dv_current_widget, _dv_current_selector)
#define bg_grad_color_()                lv_obj_get_style_bg_grad_color(_dv_current_widget, _dv_current_selector)
#define bg_grad_dir_()                  lv_obj_get_style_bg_grad_dir(_dv_current_widget, _dv_current_selector)
#define bg_main_stop_()                 lv_obj_get_style_bg_main_stop(_dv_current_widget, _dv_current_selector)
#define bg_grad_stop_()                 lv_obj_get_style_bg_grad_stop(_dv_current_widget, _dv_current_selector)
#define bg_main_opa_()                  lv_obj_get_style_bg_main_opa(_dv_current_widget, _dv_current_selector)
#define bg_grad_opa_()                  lv_obj_get_style_bg_grad_opa(_dv_current_widget, _dv_current_selector)
#define bg_grad_()                      lv_obj_get_style_bg_grad(_dv_current_widget, _dv_current_selector) // Returns const lv_grad_dsc_t *
#define bg_image_src_()                 lv_obj_get_style_bg_image_src(_dv_current_widget, _dv_current_selector)
#define bg_image_opa_()                 lv_obj_get_style_bg_image_opa(_dv_current_widget, _dv_current_selector)
#define bg_image_recolor_()             lv_obj_get_style_bg_image_recolor(_dv_current_widget, _dv_current_selector)
#define bg_image_recolor_opa_()         lv_obj_get_style_bg_image_recolor_opa(_dv_current_widget, _dv_current_selector)
#define bg_image_tiled_()               lv_obj_get_style_bg_image_tiled(_dv_current_widget, _dv_current_selector)
// Border Styles
#define border_color_()                 lv_obj_get_style_border_color(_dv_current_widget, _dv_current_selector)
#define border_opa_()                   lv_obj_get_style_border_opa(_dv_current_widget, _dv_current_selector)
#define border_width_()                 lv_obj_get_style_border_width(_dv_current_widget, _dv_current_selector)
#define border_side_()                  lv_obj_get_style_border_side(_dv_current_widget, _dv_current_selector)
#define border_post_()                  lv_obj_get_style_border_post(_dv_current_widget, _dv_current_selector)
// Outline Styles
#define outline_width_()                lv_obj_get_style_outline_width(_dv_current_widget, _dv_current_selector)
#define outline_color_()                lv_obj_get_style_outline_color(_dv_current_widget, _dv_current_selector)
#define outline_opa_()                  lv_obj_get_style_outline_opa(_dv_current_widget, _dv_current_selector)
#define outline_pad_()                  lv_obj_get_style_outline_pad(_dv_current_widget, _dv_current_selector)
// Shadow Styles
#define shadow_width_()                 lv_obj_get_style_shadow_width(_dv_current_widget, _dv_current_selector)
#define shadow_offset_x_()              lv_obj_get_style_shadow_offset_x(_dv_current_widget, _dv_current_selector)
#define shadow_offset_y_()              lv_obj_get_style_shadow_offset_y(_dv_current_widget, _dv_current_selector)
#define shadow_spread_()                lv_obj_get_style_shadow_spread(_dv_current_widget, _dv_current_selector)
#define shadow_color_()                 lv_obj_get_style_shadow_color(_dv_current_widget, _dv_current_selector)
#define shadow_opa_()                   lv_obj_get_style_shadow_opa(_dv_current_widget, _dv_current_selector)
// Image Styles
#define image_opa_()                    lv_obj_get_style_image_opa(_dv_current_widget, _dv_current_selector)
#define image_recolor_()                lv_obj_get_style_image_recolor(_dv_current_widget, _dv_current_selector)
#define image_recolor_opa_()            lv_obj_get_style_image_recolor_opa(_dv_current_widget, _dv_current_selector)
// Line Styles
#define line_width_()                   lv_obj_get_style_line_width(_dv_current_widget, _dv_current_selector)
#define line_dash_width_()              lv_obj_get_style_line_dash_width(_dv_current_widget, _dv_current_selector)
#define line_dash_gap_()                lv_obj_get_style_line_dash_gap(_dv_current_widget, _dv_current_selector)
#define line_rounded_()                 lv_obj_get_style_line_rounded(_dv_current_widget, _dv_current_selector)
#define line_color_()                   lv_obj_get_style_line_color(_dv_current_widget, _dv_current_selector)
#define line_opa_()                     lv_obj_get_style_line_opa(_dv_current_widget, _dv_current_selector)
// Arc Styles
#define arc_width_()                    lv_obj_get_style_arc_width(_dv_current_widget, _dv_current_selector)
#define arc_rounded_()                  lv_obj_get_style_arc_rounded(_dv_current_widget, _dv_current_selector)
#define arc_color_()                    lv_obj_get_style_arc_color(_dv_current_widget, _dv_current_selector)
#define arc_opa_()                      lv_obj_get_style_arc_opa(_dv_current_widget, _dv_current_selector)
#define arc_image_src_()                lv_obj_get_style_arc_image_src(_dv_current_widget, _dv_current_selector)
// Text Styles
#define text_color_()                   lv_obj_get_style_text_color(_dv_current_widget, _dv_current_selector)
#define text_opa_()                     lv_obj_get_style_text_opa(_dv_current_widget, _dv_current_selector)
#define text_font_()                    lv_obj_get_style_text_font(_dv_current_widget, _dv_current_selector)
#define text_letter_space_()            lv_obj_get_style_text_letter_space(_dv_current_widget, _dv_current_selector)
#define text_line_space_()              lv_obj_get_style_text_line_space(_dv_current_widget, _dv_current_selector)
#define text_decor_()                   lv_obj_get_style_text_decor(_dv_current_widget, _dv_current_selector)
#define text_align_()                   lv_obj_get_style_text_align(_dv_current_widget, _dv_current_selector)
#if LV_USE_FONT_SUBPX // Check LVGL config
#define text_outline_stroke_color_()    lv_obj_get_style_text_outline_stroke_color(_dv_current_widget, _dv_current_selector);
#define text_outline_stroke_width_()    lv_obj_get_style_text_outline_stroke_width(_dv_current_widget, _dv_current_selector);
#define text_outline_stroke_opa_()     lv_obj_get_style_text_outline_stroke_opa(_dv_current_widget, _dv_current_selector);
#endif // LV_USE_FONT_SUBPX
// Miscellaneous Styles
#define radius_()                       lv_obj_get_style_radius(_dv_current_widget, _dv_current_selector)
#define radial_offset_()                lv_obj_get_style_radial_offset(_dv_current_widget, _dv_current_selector); // Added getter manually
#define clip_corner_()                  lv_obj_get_style_clip_corner(_dv_current_widget, _dv_current_selector)
#define opa_()                          lv_obj_get_style_opa(_dv_current_widget, _dv_current_selector)
#define opa_layered_()                  lv_obj_get_style_opa_layered(_dv_current_widget, _dv_current_selector)
#define color_filter_dsc_()             lv_obj_get_style_color_filter_dsc(_dv_current_widget, _dv_current_selector)
#define color_filter_opa_()             lv_obj_get_style_color_filter_opa(_dv_current_widget, _dv_current_selector)
#define anim_()                         lv_obj_get_style_anim(_dv_current_widget, _dv_current_selector)
#define anim_duration_()                lv_obj_get_style_anim_duration(_dv_current_widget, _dv_current_selector)
#define transition_()                   lv_obj_get_style_transition(_dv_current_widget, _dv_current_selector)
#define blend_mode_()                   lv_obj_get_style_blend_mode(_dv_current_widget, _dv_current_selector)
#define layout_()                       lv_obj_get_style_layout(_dv_current_widget, _dv_current_selector)
#define base_dir_()                     lv_obj_get_style_base_dir(_dv_current_widget, _dv_current_selector)
#define bitmap_mask_src_()              lv_obj_get_style_bitmap_mask_src(_dv_current_widget, _dv_current_selector)
#define rotary_sensitivity_()           lv_obj_get_style_rotary_sensitivity(_dv_current_widget, _dv_current_selector)
// Flexbox Styles
#define flex_flow_()                    lv_obj_get_style_flex_flow(_dv_current_widget, _dv_current_selector)
#define flex_main_place_()              lv_obj_get_style_flex_main_place(_dv_current_widget, _dv_current_selector)
#define flex_cross_place_()             lv_obj_get_style_flex_cross_place(_dv_current_widget, _dv_current_selector)
#define flex_track_place_()             lv_obj_get_style_flex_track_place(_dv_current_widget, _dv_current_selector)
#define flex_grow_()                    lv_obj_get_style_flex_grow(_dv_current_widget, _dv_current_selector)
// Grid Styles
#define grid_column_dsc_array_()        lv_obj_get_style_grid_column_dsc_array(_dv_current_widget, _dv_current_selector)
#define grid_column_align_()            lv_obj_get_style_grid_column_align(_dv_current_widget, _dv_current_selector)
#define grid_row_dsc_array_()           lv_obj_get_style_grid_row_dsc_array(_dv_current_widget, _dv_current_selector)
#define grid_row_align_()               lv_obj_get_style_grid_row_align(_dv_current_widget, _dv_current_selector)
#define grid_cell_column_pos_()         lv_obj_get_style_grid_cell_column_pos(_dv_current_widget, _dv_current_selector)
#define grid_cell_x_align_()            lv_obj_get_style_grid_cell_x_align(_dv_current_widget, _dv_current_selector)
#define grid_cell_column_span_()        lv_obj_get_style_grid_cell_column_span(_dv_current_widget, _dv_current_selector)
#define grid_cell_row_pos_()            lv_obj_get_style_grid_cell_row_pos(_dv_current_widget, _dv_current_selector)
#define grid_cell_y_align_()            lv_obj_get_style_grid_cell_y_align(_dv_current_widget, _dv_current_selector)
#define grid_cell_row_span_()           lv_obj_get_style_grid_cell_row_span(_dv_current_widget, _dv_current_selector)
#define grid_column_gap_()              lv_obj_get_style_grid_column_gap(_dv_current_widget, _dv_current_selector)
#define grid_row_gap_()                 lv_obj_get_style_grid_row_gap(_dv_current_widget, _dv_current_selector)


// --- Selector Block Macro ---
/**
 * @brief Creates a scope where subsequent style macros use the specified selector.
 *
 * @param selector_value The lv_style_selector_t value (e.g., LV_STATE_PRESSED, LV_PART_INDICATOR | LV_STATE_CHECKED).
 * @param ... Comma-separated list of style setter macros (e.g., _bg_color(...), _width(...)) to apply using this selector.
 */
#define selector(selector_value, ...) \
    do { \
        /* Save selector from the outer scope (widget default or previous selector) */ \
        lv_style_selector_t _dv_outer_selector = _dv_current_selector; \
        /* Set the new selector specifically for this block */ \
        _dv_current_selector = (selector_value); \
        /* Process the arguments passed to selector (...) using the new selector */ \
        _process_args(__VA_ARGS__) \
        /* Restore the outer selector once this block's arguments are processed */ \
        _dv_current_selector = _dv_outer_selector; \
    } while(0);

// --- Main Definition Macro ---
/**
 * @brief Defines LVGL objects, properties, styles (potentially scoped by selector), and populates a view map.
 *
 * Naming Convention:
 * - Macros are lower-case.
 * - Macros calling lv_obj_set_*(...) or lv_obj_set_style_*(...) are prefixed with '_'. (e.g., _width(100))
 * - Macros calling lv_obj_get_*(...) or lv_obj_get_style_*(...) are suffixed with '_'. (e.g., width_())
 * - Macros for actions or widget creation have no prefix/suffix. (e.g., center(), label(...))
 * - Parent-specific setters are prefixed with '_parent_'. (e.g., _parent_size(100, 50))
 *
 * Usage Example:
 *   view(map_array, map_count, container,
 *       _parent_style_pad_all(5),         // Apply style setter to container
 *       _layout(lv_layout_flex_create()), // Apply layout style setter to container
 *       _flex_flow(LV_FLEX_FLOW_ROW),     // Apply flex style setter to container
 *
 *       label(my_label, "Text",           // Define a label widget named my_label
 *           _width(100),                  // Style setter for default state
 *           _text_color(lv_color_black()),// Style setter for default state
 *           selector(LV_STATE_DISABLED,   // Start disabled state selector block
 *              _text_color(lv_color_grey()) // Style setter for disabled state
 *           ) // End selector block
 *       ), // End label block
 *
 *       button(my_button,                 // Define a button widget named my_button
 *           _add_flag(LV_OBJ_FLAG_CHECKABLE), // Direct property setter
 *           _width(LV_SIZE_CONTENT),      // Style setter for default state
 *           _height(30),                  // Style setter for default state
 *           _radius(4),                   // Style setter for default state
 *           selector(LV_PART_MAIN | LV_STATE_PRESSED, // Start pressed state selector block
 *              _bg_color(lv_palette_darken(LV_PALETTE_BLUE, 2)),
 *              _border_width(3)
 *           ), // End selector block
 *           selector(LV_PART_MAIN | LV_STATE_CHECKED, // Start checked state selector block
 *              _bg_color(lv_palette_main(LV_PALETTE_GREEN))
 *           ), // End selector block
 *
 *           // Add a label *inside* the button
 *           label(btn_label, "Click Me",   // Define nested label widget
 *               center(),                   // Action macro: center label in button
 *               _text_color(lv_color_white()), // Style setter for label's default state
 *               selector(LV_STATE_PRESSED,   // Selector block for label style when button is pressed
 *                  _text_color(lv_color_lighten(lv_color_white(), 50))
 *               ) // End selector block
 *           ) // End nested label block
 *       ) // End button block
 *   );
 *
 * @param ParentObj The lv_obj_t* container object where widgets defined at the top level will be created.
 * @param ... Comma-separated list of parent setters (_parent_...), layout setters (_layout, _flex_flow, etc.),
 *            widget creation (label, button, etc.), which can contain direct setters (_width, _add_flag),
 *            actions (center), style setters (_width, _bg_color, etc.), selector(...) blocks,
 *            and nested widget definitions. Getter macros (e.g., width_()) are generally not used
 *            within the view definition itself but might be used elsewhere to query properties.
 */
#define view(ParentObj, ...) \
    do { \
        lv_obj_t* _dv_parent_obj = (ParentObj); \
        lv_obj_t* _dv_current_widget = _dv_parent_obj; /* Track the widget being configured */ \
        lv_style_selector_t _dv_current_selector = LV_PART_MAIN | LV_STATE_DEFAULT; /* Track the selector */ \
        \
        /* Set the initial widget context to the parent for top-level setters */ \
        _dv_current_widget = _dv_parent_obj; \
        _dv_current_selector = LV_PART_MAIN | LV_STATE_DEFAULT; \
        \
        /* Process all top-level arguments */ \
        /* _parent_... macros use _dv_parent_obj directly */ \
        /* _layout, _flex_*, _grid_* etc. use _dv_current_widget (initially parent) */ \
        /* Widget macros (obj, label...) set _dv_current_widget internally and update _dv_parent_obj for nesting */ \
        _process_args(__VA_ARGS__) \
        \
    } while(0)

// --- Helper Macros for Overloads ---
#define _PASTE_IMPL(a, b) a##b
#define _PASTE(a, b) _PASTE_IMPL(a, b)

// --- Overloaded Property/Style Macros (double underscore prefix __) ---
// These macros can be called with or without an explicit lv_obj_t* target.
// If the target is omitted, _dv_current_widget is used.

// --- Overloaded Direct Property/Function Setters ---
// _size(w, h) -> __size(...)
#define _APPLY_SIZE(obj, w, h)          lv_obj_set_size(obj, w, h);
#define __size_2(w, h)                  _APPLY_SIZE(_dv_current_widget, w, h)
#define __size_3(obj, w, h)             _APPLY_SIZE(obj, w, h)
#define __size(...)                     _PASTE(__size_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _width(val) -> __width(...)
#define _APPLY_WIDTH(obj, val)          lv_obj_set_width(obj, val);
#define __width_1(val)                  _APPLY_WIDTH(_dv_current_widget, val)
#define __width_2(obj, val)             _APPLY_WIDTH(obj, val)
#define __width(...)                    _PASTE(__width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _height(val) -> __height(...)
#define _APPLY_HEIGHT(obj, val)         lv_obj_set_height(obj, val);
#define __height_1(val)                 _APPLY_HEIGHT(_dv_current_widget, val)
#define __height_2(obj, val)            _APPLY_HEIGHT(obj, val)
#define __height(...)                   _PASTE(__height_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _align(align, x, y) -> __align(...)
#define _APPLY_ALIGN(obj, al, x, y)     lv_obj_align(obj, al, x, y);
#define __align_3(al, x, y)             _APPLY_ALIGN(_dv_current_widget, al, x, y)
#define __align_4(obj, al, x, y)        _APPLY_ALIGN(obj, al, x, y)
#define __align(...)                    _PASTE(__align_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _add_flag(f) -> __add_flag(...)
#define _APPLY_ADD_FLAG(obj, f)         lv_obj_add_flag(obj, f);
#define __add_flag_1(f)                 _APPLY_ADD_FLAG(_dv_current_widget, f)
#define __add_flag_2(obj, f)            _APPLY_ADD_FLAG(obj, f)
#define __add_flag(...)                 _PASTE(__add_flag_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _clear_flag(f) -> __clear_flag(...)
#define _APPLY_CLEAR_FLAG(obj, f)       lv_obj_clear_flag(obj, f);
#define __clear_flag_1(f)               _APPLY_CLEAR_FLAG(_dv_current_widget, f)
#define __clear_flag_2(obj, f)          _APPLY_CLEAR_FLAG(obj, f)
#define __clear_flag(...)               _PASTE(__clear_flag_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _add_state(s) -> __add_state(...)
#define _APPLY_ADD_STATE(obj, s)        lv_obj_add_state(obj, s);
#define __add_state_1(s)                _APPLY_ADD_STATE(_dv_current_widget, s)
#define __add_state_2(obj, s)           _APPLY_ADD_STATE(obj, s)
#define __add_state(...)                _PASTE(__add_state_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _clear_state(s) -> __clear_state(...)
#define _APPLY_CLEAR_STATE(obj, s)      lv_obj_clear_state(obj, s);
#define __clear_state_1(s)              _APPLY_CLEAR_STATE(_dv_current_widget, s)
#define __clear_state_2(obj, s)         _APPLY_CLEAR_STATE(obj, s)
#define __clear_state(...)              _PASTE(__clear_state_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text(t) -> __text(...)
#define _APPLY_TEXT(obj, t)             lv_label_set_text(obj, t);
#define __text_1(t)                     _APPLY_TEXT(_dv_current_widget, t)
#define __text_2(obj, t)                _APPLY_TEXT(obj, t)
#define __text(...)                     _PASTE(__text_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _label_text(t) -> __label_text(...)
#define _APPLY_LABEL_TEXT(obj, t)       lv_label_set_text(obj, t);
#define __label_text_1(t)               _APPLY_LABEL_TEXT(_dv_current_widget, t)
#define __label_text_2(obj, t)          _APPLY_LABEL_TEXT(obj, t)
#define __label_text(...)               _PASTE(__label_text_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _textarea_text(t) -> __textarea_text(...)
#define _APPLY_TEXTAREA_TEXT(obj, t)    lv_textarea_set_text(obj, t);
#define __textarea_text_1(t)            _APPLY_TEXTAREA_TEXT(_dv_current_widget, t)
#define __textarea_text_2(obj, t)       _APPLY_TEXTAREA_TEXT(obj, t)
#define __textarea_text(...)            _PASTE(__textarea_text_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _placeholder_text(t) -> __placeholder_text(...)
#define _APPLY_PLACEHOLDER_TEXT(obj, t) lv_textarea_set_placeholder_text(obj, t);
#define __placeholder_text_1(t)         _APPLY_PLACEHOLDER_TEXT(_dv_current_widget, t)
#define __placeholder_text_2(obj, t)    _APPLY_PLACEHOLDER_TEXT(obj, t)
#define __placeholder_text(...)         _PASTE(__placeholder_text_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _one_line(b) -> __one_line(...)
#define _APPLY_ONE_LINE(obj, b)         lv_textarea_set_one_line(obj, b);
#define __one_line_1(b)                 _APPLY_ONE_LINE(_dv_current_widget, b)
#define __one_line_2(obj, b)            _APPLY_ONE_LINE(obj, b)
#define __one_line(...)                 _PASTE(__one_line_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _image_src(s) -> __image_src(...)
#define _APPLY_IMAGE_SRC(obj, s)        lv_image_set_src(obj, s);
#define __image_src_1(s)                _APPLY_IMAGE_SRC(_dv_current_widget, s)
#define __image_src_2(obj, s)           _APPLY_IMAGE_SRC(obj, s)
#define __image_src(...)                _PASTE(__image_src_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// --- Overloaded Direct Property/Function Getters ---
// size_() -> __size_(...)
#define _GET_SIZE(obj)                  lv_obj_get_size(obj)
#define __size_0_()                     _GET_SIZE(_dv_current_widget)
#define __size_1_(obj)                  _GET_SIZE(obj)
#define __size_(...)                    _PASTE(__size_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// width_() -> __width_(...)
#define _GET_WIDTH(obj)                 lv_obj_get_width(obj)
#define __width_0_()                    _GET_WIDTH(_dv_current_widget)
#define __width_1_(obj)                 _GET_WIDTH(obj)
#define __width_(...)                   _PASTE(__width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// height_() -> __height_(...)
#define _GET_HEIGHT(obj)                lv_obj_get_height(obj)
#define __height_0_()                   _GET_HEIGHT(_dv_current_widget)
#define __height_1_(obj)                _GET_HEIGHT(obj)
#define __height_(...)                  _PASTE(__height_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// x_() -> __x_(...)
#define _GET_X(obj)                     lv_obj_get_x(obj)
#define __x_0_()                        _GET_X(_dv_current_widget)
#define __x_1_(obj)                     _GET_X(obj)
#define __x_(...)                       _PASTE(__x_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// y_() -> __y_(...)
#define _GET_Y(obj)                     lv_obj_get_y(obj)
#define __y_0_()                        _GET_Y(_dv_current_widget)
#define __y_1_(obj)                     _GET_Y(obj)
#define __y_(...)                       _PASTE(__y_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// content_width_() -> __content_width_(...)
#define _GET_CONTENT_WIDTH(obj)         lv_obj_get_content_width(obj)
#define __content_width_0_()            _GET_CONTENT_WIDTH(_dv_current_widget)
#define __content_width_1_(obj)         _GET_CONTENT_WIDTH(obj)
#define __content_width_(...)           _PASTE(__content_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// content_height_() -> __content_height_(...)
#define _GET_CONTENT_HEIGHT(obj)        lv_obj_get_content_height(obj)
#define __content_height_0_()           _GET_CONTENT_HEIGHT(_dv_current_widget)
#define __content_height_1_(obj)        _GET_CONTENT_HEIGHT(obj)
#define __content_height_(...)          _PASTE(__content_height_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// self_width_() -> __self_width_(...)
#define _GET_SELF_WIDTH(obj)            lv_obj_get_self_width(obj)
#define __self_width_0_()               _GET_SELF_WIDTH(_dv_current_widget)
#define __self_width_1_(obj)            _GET_SELF_WIDTH(obj)
#define __self_width_(...)              _PASTE(__self_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// self_height_() -> __self_height_(...)
#define _GET_SELF_HEIGHT(obj)           lv_obj_get_self_height(obj)
#define __self_height_0_()              _GET_SELF_HEIGHT(_dv_current_widget)
#define __self_height_1_(obj)           _GET_SELF_HEIGHT(obj)
#define __self_height_(...)             _PASTE(__self_height_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// has_flag_(f) -> __has_flag_(...)
#define _GET_HAS_FLAG(obj, f)           lv_obj_has_flag(obj, f)
#define __has_flag_1_(f)                _GET_HAS_FLAG(_dv_current_widget, f)
#define __has_flag_2_(obj, f)           _GET_HAS_FLAG(obj, f)
#define __has_flag_(...)                _PASTE(__has_flag_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// has_state_(s) -> __has_state_(...)
#define _GET_HAS_STATE(obj, s)          lv_obj_has_state(obj, s)
#define __has_state_1_(s)               _GET_HAS_STATE(_dv_current_widget, s)
#define __has_state_2_(obj, s)          _GET_HAS_STATE(obj, s)
#define __has_state_(...)               _PASTE(__has_state_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// state_() -> __state_(...)
#define _GET_STATE(obj)                 lv_obj_get_state(obj)
#define __state_0_()                    _GET_STATE(_dv_current_widget)
#define __state_1_(obj)                 _GET_STATE(obj)
#define __state_(...)                   _PASTE(__state_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// label_text_() -> __label_text_(...)
#define _GET_LABEL_TEXT(obj)            lv_label_get_text(obj)
#define __label_text_0_()               _GET_LABEL_TEXT(_dv_current_widget)
#define __label_text_1_(obj)            _GET_LABEL_TEXT(obj)
#define __label_text_(...)              _PASTE(__label_text_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// textarea_text_() -> __textarea_text_(...)
#define _GET_TEXTAREA_TEXT(obj)         lv_textarea_get_text(obj)
#define __textarea_text_0_()            _GET_TEXTAREA_TEXT(_dv_current_widget)
#define __textarea_text_1_(obj)         _GET_TEXTAREA_TEXT(obj)
#define __textarea_text_(...)           _PASTE(__textarea_text_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// placeholder_text_() -> __placeholder_text_(...)
#define _GET_PLACEHOLDER_TEXT(obj)      lv_textarea_get_placeholder_text(obj)
#define __placeholder_text_0_()         _GET_PLACEHOLDER_TEXT(_dv_current_widget)
#define __placeholder_text_1_(obj)      _GET_PLACEHOLDER_TEXT(obj)
#define __placeholder_text_(...)        _PASTE(__placeholder_text_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// one_line_() -> __one_line_(...)
#define _GET_ONE_LINE(obj)              lv_textarea_get_one_line(obj)
#define __one_line_0_()                 _GET_ONE_LINE(_dv_current_widget)
#define __one_line_1_(obj)              _GET_ONE_LINE(obj)
#define __one_line_(...)                _PASTE(__one_line_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// image_src_() -> __image_src_(...)
#define _GET_IMAGE_SRC(obj)             lv_image_get_src(obj)
#define __image_src_0_()                _GET_IMAGE_SRC(_dv_current_widget)
#define __image_src_1_(obj)             _GET_IMAGE_SRC(obj)
#define __image_src_(...)               _PASTE(__image_src_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// --- Overloaded Widget Direct Function Macros ---
// center() -> __center(...)
#define _APPLY_CENTER(obj)              lv_obj_center(obj);
#define __center_0()                    _APPLY_CENTER(_dv_current_widget)
#define __center_1(obj)                 _APPLY_CENTER(obj)
#define __center(...)                   _PASTE(__center_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// scroll_to_view(anim) -> __scroll_to_view(...)
#define _APPLY_SCROLL_TO_VIEW(obj, anim) lv_obj_scroll_to_view(obj, anim);
#define __scroll_to_view_1(anim)        _APPLY_SCROLL_TO_VIEW(_dv_current_widget, anim)
#define __scroll_to_view_2(obj, anim)   _APPLY_SCROLL_TO_VIEW(obj, anim)
#define __scroll_to_view(...)           _PASTE(__scroll_to_view_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// add_event_cb(cb, filter, data) -> __add_event_cb(...)
#define _APPLY_ADD_EVENT_CB(obj, cb, filter, data) lv_obj_add_event_cb(obj, cb, filter, data);
#define __add_event_cb_3(cb, filter, data)      _APPLY_ADD_EVENT_CB(_dv_current_widget, cb, filter, data)
#define __add_event_cb_4(obj, cb, filter, data) _APPLY_ADD_EVENT_CB(obj, cb, filter, data)
#define __add_event_cb(...)             _PASTE(__add_event_cb_, _nargs(__VA_ARGS__))(__VA_ARGS__)


// --- Overloaded Widget Style Setters ---
// (These use _dv_current_selector implicitly via the _APPLY macro)
// _min_width(value) -> __min_width(...)
#define _APPLY_MIN_WIDTH(obj, value)    lv_obj_set_style_min_width(obj, value, _dv_current_selector);
#define __min_width_1(value)            _APPLY_MIN_WIDTH(_dv_current_widget, value)
#define __min_width_2(obj, value)       _APPLY_MIN_WIDTH(obj, value)
#define __min_width(...)                _PASTE(__min_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _max_width(value) -> __max_width(...)
#define _APPLY_MAX_WIDTH(obj, value)    lv_obj_set_style_max_width(obj, value, _dv_current_selector);
#define __max_width_1(value)            _APPLY_MAX_WIDTH(_dv_current_widget, value)
#define __max_width_2(obj, value)       _APPLY_MAX_WIDTH(obj, value)
#define __max_width(...)                _PASTE(__max_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _min_height(value) -> __min_height(...)
#define _APPLY_MIN_HEIGHT(obj, value)   lv_obj_set_style_min_height(obj, value, _dv_current_selector);
#define __min_height_1(value)           _APPLY_MIN_HEIGHT(_dv_current_widget, value)
#define __min_height_2(obj, value)      _APPLY_MIN_HEIGHT(obj, value)
#define __min_height(...)               _PASTE(__min_height_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _max_height(value) -> __max_height(...)
#define _APPLY_MAX_HEIGHT(obj, value)   lv_obj_set_style_max_height(obj, value, _dv_current_selector);
#define __max_height_1(value)           _APPLY_MAX_HEIGHT(_dv_current_widget, value)
#define __max_height_2(obj, value)      _APPLY_MAX_HEIGHT(obj, value)
#define __max_height(...)               _PASTE(__max_height_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _length(value) -> __length(...)
#define _APPLY_LENGTH(obj, value)       lv_obj_set_style_length(obj, value, _dv_current_selector);
#define __length_1(value)               _APPLY_LENGTH(_dv_current_widget, value)
#define __length_2(obj, value)          _APPLY_LENGTH(obj, value)
#define __length(...)                   _PASTE(__length_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _x(value) -> __x(...)
#define _APPLY_X(obj, value)            lv_obj_set_style_x(obj, value, _dv_current_selector);
#define __x_1(value)                    _APPLY_X(_dv_current_widget, value)
#define __x_2(obj, value)               _APPLY_X(obj, value)
#define __x(...)                        _PASTE(__x_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _y(value) -> __y(...)
#define _APPLY_Y(obj, value)            lv_obj_set_style_y(obj, value, _dv_current_selector);
#define __y_1(value)                    _APPLY_Y(_dv_current_widget, value)
#define __y_2(obj, value)               _APPLY_Y(obj, value)
#define __y(...)                        _PASTE(__y_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_width(value) -> __transform_width(...)
#define _APPLY_TRANSFORM_WIDTH(obj, value) lv_obj_set_style_transform_width(obj, value, _dv_current_selector);
#define __transform_width_1(value)      _APPLY_TRANSFORM_WIDTH(_dv_current_widget, value)
#define __transform_width_2(obj, value) _APPLY_TRANSFORM_WIDTH(obj, value)
#define __transform_width(...)          _PASTE(__transform_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_height(value) -> __transform_height(...)
#define _APPLY_TRANSFORM_HEIGHT(obj, value) lv_obj_set_style_transform_height(obj, value, _dv_current_selector);
#define __transform_height_1(value)     _APPLY_TRANSFORM_HEIGHT(_dv_current_widget, value)
#define __transform_height_2(obj, value) _APPLY_TRANSFORM_HEIGHT(obj, value)
#define __transform_height(...)         _PASTE(__transform_height_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _translate_x(value) -> __translate_x(...)
#define _APPLY_TRANSLATE_X(obj, value)  lv_obj_set_style_translate_x(obj, value, _dv_current_selector);
#define __translate_x_1(value)          _APPLY_TRANSLATE_X(_dv_current_widget, value)
#define __translate_x_2(obj, value)     _APPLY_TRANSLATE_X(obj, value)
#define __translate_x(...)              _PASTE(__translate_x_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _translate_y(value) -> __translate_y(...)
#define _APPLY_TRANSLATE_Y(obj, value)  lv_obj_set_style_translate_y(obj, value, _dv_current_selector);
#define __translate_y_1(value)          _APPLY_TRANSLATE_Y(_dv_current_widget, value)
#define __translate_y_2(obj, value)     _APPLY_TRANSLATE_Y(obj, value)
#define __translate_y(...)              _PASTE(__translate_y_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _translate_radial(value) -> __translate_radial(...)
#define _APPLY_TRANSLATE_RADIAL(obj, value) lv_obj_set_style_translate_radial(obj, value, _dv_current_selector);
#define __translate_radial_1(value)     _APPLY_TRANSLATE_RADIAL(_dv_current_widget, value)
#define __translate_radial_2(obj, value) _APPLY_TRANSLATE_RADIAL(obj, value)
#define __translate_radial(...)         _PASTE(__translate_radial_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_scale_x(value) -> __transform_scale_x(...)
#define _APPLY_TRANSFORM_SCALE_X(obj, value) lv_obj_set_style_transform_scale_x(obj, value, _dv_current_selector);
#define __transform_scale_x_1(value)    _APPLY_TRANSFORM_SCALE_X(_dv_current_widget, value)
#define __transform_scale_x_2(obj, value) _APPLY_TRANSFORM_SCALE_X(obj, value)
#define __transform_scale_x(...)        _PASTE(__transform_scale_x_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_scale_y(value) -> __transform_scale_y(...)
#define _APPLY_TRANSFORM_SCALE_Y(obj, value) lv_obj_set_style_transform_scale_y(obj, value, _dv_current_selector);
#define __transform_scale_y_1(value)    _APPLY_TRANSFORM_SCALE_Y(_dv_current_widget, value)
#define __transform_scale_y_2(obj, value) _APPLY_TRANSFORM_SCALE_Y(obj, value)
#define __transform_scale_y(...)        _PASTE(__transform_scale_y_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_rotation(value) -> __transform_rotation(...)
#define _APPLY_TRANSFORM_ROTATION(obj, value) lv_obj_set_style_transform_rotation(obj, value, _dv_current_selector);
#define __transform_rotation_1(value)   _APPLY_TRANSFORM_ROTATION(_dv_current_widget, value)
#define __transform_rotation_2(obj, value) _APPLY_TRANSFORM_ROTATION(obj, value)
#define __transform_rotation(...)       _PASTE(__transform_rotation_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_pivot_x(value) -> __transform_pivot_x(...)
#define _APPLY_TRANSFORM_PIVOT_X(obj, value) lv_obj_set_style_transform_pivot_x(obj, value, _dv_current_selector);
#define __transform_pivot_x_1(value)    _APPLY_TRANSFORM_PIVOT_X(_dv_current_widget, value)
#define __transform_pivot_x_2(obj, value) _APPLY_TRANSFORM_PIVOT_X(obj, value)
#define __transform_pivot_x(...)        _PASTE(__transform_pivot_x_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_pivot_y(value) -> __transform_pivot_y(...)
#define _APPLY_TRANSFORM_PIVOT_Y(obj, value) lv_obj_set_style_transform_pivot_y(obj, value, _dv_current_selector);
#define __transform_pivot_y_1(value)    _APPLY_TRANSFORM_PIVOT_Y(_dv_current_widget, value)
#define __transform_pivot_y_2(obj, value) _APPLY_TRANSFORM_PIVOT_Y(obj, value)
#define __transform_pivot_y(...)        _PASTE(__transform_pivot_y_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_skew_x(value) -> __transform_skew_x(...)
#define _APPLY_TRANSFORM_SKEW_X(obj, value) lv_obj_set_style_transform_skew_x(obj, value, _dv_current_selector);
#define __transform_skew_x_1(value)     _APPLY_TRANSFORM_SKEW_X(_dv_current_widget, value)
#define __transform_skew_x_2(obj, value) _APPLY_TRANSFORM_SKEW_X(obj, value)
#define __transform_skew_x(...)         _PASTE(__transform_skew_x_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transform_skew_y(value) -> __transform_skew_y(...)
#define _APPLY_TRANSFORM_SKEW_Y(obj, value) lv_obj_set_style_transform_skew_y(obj, value, _dv_current_selector);
#define __transform_skew_y_1(value)     _APPLY_TRANSFORM_SKEW_Y(_dv_current_widget, value)
#define __transform_skew_y_2(obj, value) _APPLY_TRANSFORM_SKEW_Y(obj, value)
#define __transform_skew_y(...)         _PASTE(__transform_skew_y_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_top(value) -> __pad_top(...)
#define _APPLY_PAD_TOP(obj, value)      lv_obj_set_style_pad_top(obj, value, _dv_current_selector);
#define __pad_top_1(value)              _APPLY_PAD_TOP(_dv_current_widget, value)
#define __pad_top_2(obj, value)         _APPLY_PAD_TOP(obj, value)
#define __pad_top(...)                  _PASTE(__pad_top_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_bottom(value) -> __pad_bottom(...)
#define _APPLY_PAD_BOTTOM(obj, value)   lv_obj_set_style_pad_bottom(obj, value, _dv_current_selector);
#define __pad_bottom_1(value)           _APPLY_PAD_BOTTOM(_dv_current_widget, value)
#define __pad_bottom_2(obj, value)      _APPLY_PAD_BOTTOM(obj, value)
#define __pad_bottom(...)               _PASTE(__pad_bottom_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_left(value) -> __pad_left(...)
#define _APPLY_PAD_LEFT(obj, value)     lv_obj_set_style_pad_left(obj, value, _dv_current_selector);
#define __pad_left_1(value)             _APPLY_PAD_LEFT(_dv_current_widget, value)
#define __pad_left_2(obj, value)        _APPLY_PAD_LEFT(obj, value)
#define __pad_left(...)                 _PASTE(__pad_left_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_right(value) -> __pad_right(...)
#define _APPLY_PAD_RIGHT(obj, value)    lv_obj_set_style_pad_right(obj, value, _dv_current_selector);
#define __pad_right_1(value)            _APPLY_PAD_RIGHT(_dv_current_widget, value)
#define __pad_right_2(obj, value)       _APPLY_PAD_RIGHT(obj, value)
#define __pad_right(...)                _PASTE(__pad_right_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_row(value) -> __pad_row(...)
#define _APPLY_PAD_ROW(obj, value)      lv_obj_set_style_pad_row(obj, value, _dv_current_selector);
#define __pad_row_1(value)              _APPLY_PAD_ROW(_dv_current_widget, value)
#define __pad_row_2(obj, value)         _APPLY_PAD_ROW(obj, value)
#define __pad_row(...)                  _PASTE(__pad_row_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_column(value) -> __pad_column(...)
#define _APPLY_PAD_COLUMN(obj, value)   lv_obj_set_style_pad_column(obj, value, _dv_current_selector);
#define __pad_column_1(value)           _APPLY_PAD_COLUMN(_dv_current_widget, value)
#define __pad_column_2(obj, value)      _APPLY_PAD_COLUMN(obj, value)
#define __pad_column(...)               _PASTE(__pad_column_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_radial(value) -> __pad_radial(...)
#define _APPLY_PAD_RADIAL(obj, value)   lv_obj_set_style_pad_radial(obj, value, _dv_current_selector);
#define __pad_radial_1(value)           _APPLY_PAD_RADIAL(_dv_current_widget, value)
#define __pad_radial_2(obj, value)      _APPLY_PAD_RADIAL(obj, value)
#define __pad_radial(...)               _PASTE(__pad_radial_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_all(value) -> __pad_all(...)
#define _APPLY_PAD_ALL(obj, value)      lv_obj_set_style_pad_all(obj, value, _dv_current_selector);
#define __pad_all_1(value)              _APPLY_PAD_ALL(_dv_current_widget, value)
#define __pad_all_2(obj, value)         _APPLY_PAD_ALL(obj, value)
#define __pad_all(...)                  _PASTE(__pad_all_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_hor(value) -> __pad_hor(...)
#define _APPLY_PAD_HOR(obj, value)      lv_obj_set_style_pad_hor(obj, value, _dv_current_selector);
#define __pad_hor_1(value)              _APPLY_PAD_HOR(_dv_current_widget, value)
#define __pad_hor_2(obj, value)         _APPLY_PAD_HOR(obj, value)
#define __pad_hor(...)                  _PASTE(__pad_hor_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _pad_ver(value) -> __pad_ver(...)
#define _APPLY_PAD_VER(obj, value)      lv_obj_set_style_pad_ver(obj, value, _dv_current_selector);
#define __pad_ver_1(value)              _APPLY_PAD_VER(_dv_current_widget, value)
#define __pad_ver_2(obj, value)         _APPLY_PAD_VER(obj, value)
#define __pad_ver(...)                  _PASTE(__pad_ver_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _margin(value) -> __margin(...)
#define _APPLY_MARGIN(obj, value)   lv_obj_set_style_margin(obj, value, _dv_current_selector);
#define __margin_1(value)           _APPLY_MARGIN(_dv_current_widget, value)
#define __margin_2(obj, value)      _APPLY_MARGIN(obj, value)
#define __margin(...)               _PASTE(__margin_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _margin_top(value) -> __margin_top(...)
#define _APPLY_MARGIN_TOP(obj, value)   lv_obj_set_style_margin_top(obj, value, _dv_current_selector);
#define __margin_top_1(value)           _APPLY_MARGIN_TOP(_dv_current_widget, value)
#define __margin_top_2(obj, value)      _APPLY_MARGIN_TOP(obj, value)
#define __margin_top(...)               _PASTE(__margin_top_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _margin_bottom(value) -> __margin_bottom(...)
#define _APPLY_MARGIN_BOTTOM(obj, value) lv_obj_set_style_margin_bottom(obj, value, _dv_current_selector);
#define __margin_bottom_1(value)        _APPLY_MARGIN_BOTTOM(_dv_current_widget, value)
#define __margin_bottom_2(obj, value)   _APPLY_MARGIN_BOTTOM(obj, value)
#define __margin_bottom(...)            _PASTE(__margin_bottom_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _margin_left(value) -> __margin_left(...)
#define _APPLY_MARGIN_LEFT(obj, value)  lv_obj_set_style_margin_left(obj, value, _dv_current_selector);
#define __margin_left_1(value)          _APPLY_MARGIN_LEFT(_dv_current_widget, value)
#define __margin_left_2(obj, value)     _APPLY_MARGIN_LEFT(obj, value)
#define __margin_left(...)              _PASTE(__margin_left_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _margin_right(value) -> __margin_right(...)
#define _APPLY_MARGIN_RIGHT(obj, value) lv_obj_set_style_margin_right(obj, value, _dv_current_selector);
#define __margin_right_1(value)         _APPLY_MARGIN_RIGHT(_dv_current_widget, value)
#define __margin_right_2(obj, value)    _APPLY_MARGIN_RIGHT(obj, value)
#define __margin_right(...)             _PASTE(__margin_right_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_color(value) -> __bg_color(...)
#define _APPLY_BG_COLOR(obj, value)     lv_obj_set_style_bg_color(obj, value, _dv_current_selector);
#define __bg_color_1(value)             _APPLY_BG_COLOR(_dv_current_widget, value)
#define __bg_color_2(obj, value)        lv_obj_set_style_bg_color(obj, value, LV_PART_MAIN)
#define __bg_color(...)                 _PASTE(__bg_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

#define _APPLY_BG_COLOR_HEX(obj, value)     lv_obj_set_style_bg_color(obj, lv_color_hex(value), _dv_current_selector);
#define __bg_color_hex_1(value)             _APPLY_BG_COLOR_HEX(_dv_current_widget, value)
#define __bg_color_hex_2(obj, value)        lv_obj_set_style_bg_color(obj, lv_color_hex(value), LV_PART_MAIN)
#define __bg_color_hex(...)                 _PASTE(__bg_color_hex_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_opa(value) -> __bg_opa(...)
#define _APPLY_BG_OPA(obj, value)       lv_obj_set_style_bg_opa(obj, value, _dv_current_selector);
#define __bg_opa_1(value)               _APPLY_BG_OPA(_dv_current_widget, value)
#define __bg_opa_2(obj, value)          _APPLY_BG_OPA(obj, value)
#define __bg_opa(...)                   _PASTE(__bg_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_grad_color(value) -> __bg_grad_color(...)
#define _APPLY_BG_GRAD_COLOR(obj, value) lv_obj_set_style_bg_grad_color(obj, value, _dv_current_selector);
#define __bg_grad_color_1(value)        _APPLY_BG_GRAD_COLOR(_dv_current_widget, value)
#define __bg_grad_color_2(obj, value)   _APPLY_BG_GRAD_COLOR(obj, value)
#define __bg_grad_color(...)            _PASTE(__bg_grad_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_grad_dir(value) -> __bg_grad_dir(...)
#define _APPLY_BG_GRAD_DIR(obj, value)  lv_obj_set_style_bg_grad_dir(obj, value, _dv_current_selector);
#define __bg_grad_dir_1(value)          _APPLY_BG_GRAD_DIR(_dv_current_widget, value)
#define __bg_grad_dir_2(obj, value)     _APPLY_BG_GRAD_DIR(obj, value)
#define __bg_grad_dir(...)              _PASTE(__bg_grad_dir_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_main_stop(value) -> __bg_main_stop(...)
#define _APPLY_BG_MAIN_STOP(obj, value) lv_obj_set_style_bg_main_stop(obj, value, _dv_current_selector);
#define __bg_main_stop_1(value)         _APPLY_BG_MAIN_STOP(_dv_current_widget, value)
#define __bg_main_stop_2(obj, value)    _APPLY_BG_MAIN_STOP(obj, value)
#define __bg_main_stop(...)             _PASTE(__bg_main_stop_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_grad_stop(value) -> __bg_grad_stop(...)
#define _APPLY_BG_GRAD_STOP(obj, value) lv_obj_set_style_bg_grad_stop(obj, value, _dv_current_selector);
#define __bg_grad_stop_1(value)         _APPLY_BG_GRAD_STOP(_dv_current_widget, value)
#define __bg_grad_stop_2(obj, value)    _APPLY_BG_GRAD_STOP(obj, value)
#define __bg_grad_stop(...)             _PASTE(__bg_grad_stop_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_main_opa(value) -> __bg_main_opa(...)
#define _APPLY_BG_MAIN_OPA(obj, value)  lv_obj_set_style_bg_main_opa(obj, value, _dv_current_selector);
#define __bg_main_opa_1(value)          _APPLY_BG_MAIN_OPA(_dv_current_widget, value)
#define __bg_main_opa_2(obj, value)     _APPLY_BG_MAIN_OPA(obj, value)
#define __bg_main_opa(...)              _PASTE(__bg_main_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_grad_opa(value) -> __bg_grad_opa(...)
#define _APPLY_BG_GRAD_OPA(obj, value)  lv_obj_set_style_bg_grad_opa(obj, value, _dv_current_selector);
#define __bg_grad_opa_1(value)          _APPLY_BG_GRAD_OPA(_dv_current_widget, value)
#define __bg_grad_opa_2(obj, value)     _APPLY_BG_GRAD_OPA(obj, value)
#define __bg_grad_opa(...)              _PASTE(__bg_grad_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_grad(value) -> __bg_grad(...)
#define _APPLY_BG_GRAD(obj, value)      lv_obj_set_style_bg_grad(obj, value, _dv_current_selector);
#define __bg_grad_1(value)              _APPLY_BG_GRAD(_dv_current_widget, value)
#define __bg_grad_2(obj, value)         _APPLY_BG_GRAD(obj, value)
#define __bg_grad(...)                  _PASTE(__bg_grad_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_image_src(value) -> __bg_image_src(...)
#define _APPLY_BG_IMAGE_SRC(obj, value) lv_obj_set_style_bg_image_src(obj, value, _dv_current_selector);
#define __bg_image_src_1(value)         _APPLY_BG_IMAGE_SRC(_dv_current_widget, value)
#define __bg_image_src_2(obj, value)    _APPLY_BG_IMAGE_SRC(obj, value)
#define __bg_image_src(...)             _PASTE(__bg_image_src_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_image_opa(value) -> __bg_image_opa(...)
#define _APPLY_BG_IMAGE_OPA(obj, value) lv_obj_set_style_bg_image_opa(obj, value, _dv_current_selector);
#define __bg_image_opa_1(value)         _APPLY_BG_IMAGE_OPA(_dv_current_widget, value)
#define __bg_image_opa_2(obj, value)    _APPLY_BG_IMAGE_OPA(obj, value)
#define __bg_image_opa(...)             _PASTE(__bg_image_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_image_recolor(value) -> __bg_image_recolor(...)
#define _APPLY_BG_IMAGE_RECOLOR(obj, value) lv_obj_set_style_bg_image_recolor(obj, value, _dv_current_selector);
#define __bg_image_recolor_1(value)     _APPLY_BG_IMAGE_RECOLOR(_dv_current_widget, value)
#define __bg_image_recolor_2(obj, value) _APPLY_BG_IMAGE_RECOLOR(obj, value)
#define __bg_image_recolor(...)         _PASTE(__bg_image_recolor_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_image_recolor_opa(value) -> __bg_image_recolor_opa(...)
#define _APPLY_BG_IMAGE_RECOLOR_OPA(obj, value) lv_obj_set_style_bg_image_recolor_opa(obj, value, _dv_current_selector);
#define __bg_image_recolor_opa_1(value) _APPLY_BG_IMAGE_RECOLOR_OPA(_dv_current_widget, value)
#define __bg_image_recolor_opa_2(obj, value) _APPLY_BG_IMAGE_RECOLOR_OPA(obj, value)
#define __bg_image_recolor_opa(...)     _PASTE(__bg_image_recolor_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bg_image_tiled(value) -> __bg_image_tiled(...)
#define _APPLY_BG_IMAGE_TILED(obj, value) lv_obj_set_style_bg_image_tiled(obj, value, _dv_current_selector);
#define __bg_image_tiled_1(value)       _APPLY_BG_IMAGE_TILED(_dv_current_widget, value)
#define __bg_image_tiled_2(obj, value)  _APPLY_BG_IMAGE_TILED(obj, value)
#define __bg_image_tiled(...)           _PASTE(__bg_image_tiled_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _border_color(value) -> __border_color(...)
#define _APPLY_BORDER_COLOR(obj, value) lv_obj_set_style_border_color(obj, value, _dv_current_selector);
#define __border_color_1(value)         _APPLY_BORDER_COLOR(_dv_current_widget, value)
#define __border_color_2(obj, value)    _APPLY_BORDER_COLOR(obj, value)
#define __border_color(...)             _PASTE(__border_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _border_opa(value) -> __border_opa(...)
#define _APPLY_BORDER_OPA(obj, value)   lv_obj_set_style_border_opa(obj, value, _dv_current_selector);
#define __border_opa_1(value)           _APPLY_BORDER_OPA(_dv_current_widget, value)
#define __border_opa_2(obj, value)      _APPLY_BORDER_OPA(obj, value)
#define __border_opa(...)               _PASTE(__border_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _border_width(value) -> __border_width(...)
#define _APPLY_BORDER_WIDTH(obj, value) lv_obj_set_style_border_width(obj, value, _dv_current_selector);
#define __border_width_1(value)         _APPLY_BORDER_WIDTH(_dv_current_widget, value)
#define __border_width_2(obj, value)    _APPLY_BORDER_WIDTH(obj, value)
#define __border_width(...)             _PASTE(__border_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _border_side(value) -> __border_side(...)
#define _APPLY_BORDER_SIDE(obj, value)  lv_obj_set_style_border_side(obj, value, _dv_current_selector);
#define __border_side_1(value)          _APPLY_BORDER_SIDE(_dv_current_widget, value)
#define __border_side_2(obj, value)     _APPLY_BORDER_SIDE(obj, value)
#define __border_side(...)              _PASTE(__border_side_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _border_post(value) -> __border_post(...)
#define _APPLY_BORDER_POST(obj, value)  lv_obj_set_style_border_post(obj, value, _dv_current_selector);
#define __border_post_1(value)          _APPLY_BORDER_POST(_dv_current_widget, value)
#define __border_post_2(obj, value)     _APPLY_BORDER_POST(obj, value)
#define __border_post(...)              _PASTE(__border_post_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _outline_width(value) -> __outline_width(...)
#define _APPLY_OUTLINE_WIDTH(obj, value) lv_obj_set_style_outline_width(obj, value, _dv_current_selector);
#define __outline_width_1(value)        _APPLY_OUTLINE_WIDTH(_dv_current_widget, value)
#define __outline_width_2(obj, value)   _APPLY_OUTLINE_WIDTH(obj, value)
#define __outline_width(...)            _PASTE(__outline_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _outline_color(value) -> __outline_color(...)
#define _APPLY_OUTLINE_COLOR(obj, value) lv_obj_set_style_outline_color(obj, value, _dv_current_selector);
#define __outline_color_1(value)        _APPLY_OUTLINE_COLOR(_dv_current_widget, value)
#define __outline_color_2(obj, value)   _APPLY_OUTLINE_COLOR(obj, value)
#define __outline_color(...)            _PASTE(__outline_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _outline_opa(value) -> __outline_opa(...)
#define _APPLY_OUTLINE_OPA(obj, value)  lv_obj_set_style_outline_opa(obj, value, _dv_current_selector);
#define __outline_opa_1(value)          _APPLY_OUTLINE_OPA(_dv_current_widget, value)
#define __outline_opa_2(obj, value)     _APPLY_OUTLINE_OPA(obj, value)
#define __outline_opa(...)              _PASTE(__outline_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _outline_pad(value) -> __outline_pad(...)
#define _APPLY_OUTLINE_PAD(obj, value)  lv_obj_set_style_outline_pad(obj, value, _dv_current_selector);
#define __outline_pad_1(value)          _APPLY_OUTLINE_PAD(_dv_current_widget, value)
#define __outline_pad_2(obj, value)     _APPLY_OUTLINE_PAD(obj, value)
#define __outline_pad(...)              _PASTE(__outline_pad_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _shadow_width(value) -> __shadow_width(...)
#define _APPLY_SHADOW_WIDTH(obj, value) lv_obj_set_style_shadow_width(obj, value, _dv_current_selector);
#define __shadow_width_1(value)         _APPLY_SHADOW_WIDTH(_dv_current_widget, value)
#define __shadow_width_2(obj, value)    _APPLY_SHADOW_WIDTH(obj, value)
#define __shadow_width(...)             _PASTE(__shadow_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _shadow_offset_x(value) -> __shadow_offset_x(...)
#define _APPLY_SHADOW_OFFSET_X(obj, value) lv_obj_set_style_shadow_offset_x(obj, value, _dv_current_selector);
#define __shadow_offset_x_1(value)      _APPLY_SHADOW_OFFSET_X(_dv_current_widget, value)
#define __shadow_offset_x_2(obj, value) _APPLY_SHADOW_OFFSET_X(obj, value)
#define __shadow_offset_x(...)          _PASTE(__shadow_offset_x_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _shadow_offset_y(value) -> __shadow_offset_y(...)
#define _APPLY_SHADOW_OFFSET_Y(obj, value) lv_obj_set_style_shadow_offset_y(obj, value, _dv_current_selector);
#define __shadow_offset_y_1(value)      _APPLY_SHADOW_OFFSET_Y(_dv_current_widget, value)
#define __shadow_offset_y_2(obj, value) _APPLY_SHADOW_OFFSET_Y(obj, value)
#define __shadow_offset_y(...)          _PASTE(__shadow_offset_y_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _shadow_spread(value) -> __shadow_spread(...)
#define _APPLY_SHADOW_SPREAD(obj, value) lv_obj_set_style_shadow_spread(obj, value, _dv_current_selector);
#define __shadow_spread_1(value)        _APPLY_SHADOW_SPREAD(_dv_current_widget, value)
#define __shadow_spread_2(obj, value)   _APPLY_SHADOW_SPREAD(obj, value)
#define __shadow_spread(...)            _PASTE(__shadow_spread_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _shadow_color(value) -> __shadow_color(...)
#define _APPLY_SHADOW_COLOR(obj, value) lv_obj_set_style_shadow_color(obj, value, _dv_current_selector);
#define __shadow_color_1(value)         _APPLY_SHADOW_COLOR(_dv_current_widget, value)
#define __shadow_color_2(obj, value)    _APPLY_SHADOW_COLOR(obj, value)
#define __shadow_color(...)             _PASTE(__shadow_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _shadow_opa(value) -> __shadow_opa(...)
#define _APPLY_SHADOW_OPA(obj, value)   lv_obj_set_style_shadow_opa(obj, value, _dv_current_selector);
#define __shadow_opa_1(value)           _APPLY_SHADOW_OPA(_dv_current_widget, value)
#define __shadow_opa_2(obj, value)      _APPLY_SHADOW_OPA(obj, value)
#define __shadow_opa(...)               _PASTE(__shadow_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _image_opa(value) -> __image_opa(...)
#define _APPLY_IMAGE_OPA(obj, value)    lv_obj_set_style_image_opa(obj, value, _dv_current_selector);
#define __image_opa_1(value)            _APPLY_IMAGE_OPA(_dv_current_widget, value)
#define __image_opa_2(obj, value)       _APPLY_IMAGE_OPA(obj, value)
#define __image_opa(...)                _PASTE(__image_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _image_recolor(value) -> __image_recolor(...)
#define _APPLY_IMAGE_RECOLOR(obj, value) lv_obj_set_style_image_recolor(obj, value, _dv_current_selector);
#define __image_recolor_1(value)        _APPLY_IMAGE_RECOLOR(_dv_current_widget, value)
#define __image_recolor_2(obj, value)   _APPLY_IMAGE_RECOLOR(obj, value)
#define __image_recolor(...)            _PASTE(__image_recolor_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _image_recolor_opa(value) -> __image_recolor_opa(...)
#define _APPLY_IMAGE_RECOLOR_OPA(obj, value) lv_obj_set_style_image_recolor_opa(obj, value, _dv_current_selector);
#define __image_recolor_opa_1(value)    _APPLY_IMAGE_RECOLOR_OPA(_dv_current_widget, value)
#define __image_recolor_opa_2(obj, value) _APPLY_IMAGE_RECOLOR_OPA(obj, value)
#define __image_recolor_opa(...)        _PASTE(__image_recolor_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _line_width(value) -> __line_width(...)
#define _APPLY_LINE_WIDTH(obj, value)   lv_obj_set_style_line_width(obj, value, _dv_current_selector);
#define __line_width_1(value)           _APPLY_LINE_WIDTH(_dv_current_widget, value)
#define __line_width_2(obj, value)      _APPLY_LINE_WIDTH(obj, value)
#define __line_width(...)               _PASTE(__line_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _line_dash_width(value) -> __line_dash_width(...)
#define _APPLY_LINE_DASH_WIDTH(obj, value) lv_obj_set_style_line_dash_width(obj, value, _dv_current_selector);
#define __line_dash_width_1(value)      _APPLY_LINE_DASH_WIDTH(_dv_current_widget, value)
#define __line_dash_width_2(obj, value) _APPLY_LINE_DASH_WIDTH(obj, value)
#define __line_dash_width(...)          _PASTE(__line_dash_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _line_dash_gap(value) -> __line_dash_gap(...)
#define _APPLY_LINE_DASH_GAP(obj, value) lv_obj_set_style_line_dash_gap(obj, value, _dv_current_selector);
#define __line_dash_gap_1(value)        _APPLY_LINE_DASH_GAP(_dv_current_widget, value)
#define __line_dash_gap_2(obj, value)   _APPLY_LINE_DASH_GAP(obj, value)
#define __line_dash_gap(...)            _PASTE(__line_dash_gap_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _line_rounded(value) -> __line_rounded(...)
#define _APPLY_LINE_ROUNDED(obj, value) lv_obj_set_style_line_rounded(obj, value, _dv_current_selector);
#define __line_rounded_1(value)         _APPLY_LINE_ROUNDED(_dv_current_widget, value)
#define __line_rounded_2(obj, value)    _APPLY_LINE_ROUNDED(obj, value)
#define __line_rounded(...)             _PASTE(__line_rounded_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _line_color(value) -> __line_color(...)
#define _APPLY_LINE_COLOR(obj, value)   lv_obj_set_style_line_color(obj, value, _dv_current_selector);
#define __line_color_1(value)           _APPLY_LINE_COLOR(_dv_current_widget, value)
#define __line_color_2(obj, value)      _APPLY_LINE_COLOR(obj, value)
#define __line_color(...)               _PASTE(__line_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _line_opa(value) -> __line_opa(...)
#define _APPLY_LINE_OPA(obj, value)     lv_obj_set_style_line_opa(obj, value, _dv_current_selector);
#define __line_opa_1(value)             _APPLY_LINE_OPA(_dv_current_widget, value)
#define __line_opa_2(obj, value)        _APPLY_LINE_OPA(obj, value)
#define __line_opa(...)                 _PASTE(__line_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _arc_width(value) -> __arc_width(...)
#define _APPLY_ARC_WIDTH(obj, value)    lv_obj_set_style_arc_width(obj, value, _dv_current_selector);
#define __arc_width_1(value)            _APPLY_ARC_WIDTH(_dv_current_widget, value)
#define __arc_width_2(obj, value)       _APPLY_ARC_WIDTH(obj, value)
#define __arc_width(...)                _PASTE(__arc_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _arc_rounded(value) -> __arc_rounded(...)
#define _APPLY_ARC_ROUNDED(obj, value)  lv_obj_set_style_arc_rounded(obj, value, _dv_current_selector);
#define __arc_rounded_1(value)          _APPLY_ARC_ROUNDED(_dv_current_widget, value)
#define __arc_rounded_2(obj, value)     _APPLY_ARC_ROUNDED(obj, value)
#define __arc_rounded(...)              _PASTE(__arc_rounded_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _arc_color(value) -> __arc_color(...)
#define _APPLY_ARC_COLOR(obj, value)    lv_obj_set_style_arc_color(obj, value, _dv_current_selector);
#define __arc_color_1(value)            _APPLY_ARC_COLOR(_dv_current_widget, value)
#define __arc_color_2(obj, value)       _APPLY_ARC_COLOR(obj, value)
#define __arc_color(...)                _PASTE(__arc_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _arc_opa(value) -> __arc_opa(...)
#define _APPLY_ARC_OPA(obj, value)      lv_obj_set_style_arc_opa(obj, value, _dv_current_selector);
#define __arc_opa_1(value)              _APPLY_ARC_OPA(_dv_current_widget, value)
#define __arc_opa_2(obj, value)         _APPLY_ARC_OPA(obj, value)
#define __arc_opa(...)                  _PASTE(__arc_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _arc_image_src(value) -> __arc_image_src(...)
#define _APPLY_ARC_IMAGE_SRC(obj, value) lv_obj_set_style_arc_image_src(obj, value, _dv_current_selector);
#define __arc_image_src_1(value)        _APPLY_ARC_IMAGE_SRC(_dv_current_widget, value)
#define __arc_image_src_2(obj, value)   _APPLY_ARC_IMAGE_SRC(obj, value)
#define __arc_image_src(...)            _PASTE(__arc_image_src_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_color(value) -> __text_color(...)
#define _APPLY_TEXT_COLOR(obj, value)   lv_obj_set_style_text_color(obj, value, _dv_current_selector);
#define __text_color_1(value)           _APPLY_TEXT_COLOR(_dv_current_widget, value)
#define __text_color_2(obj, value)      _APPLY_TEXT_COLOR(obj, value)
#define __text_color(...)               _PASTE(__text_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_opa(value) -> __text_opa(...)
#define _APPLY_TEXT_OPA(obj, value)     lv_obj_set_style_text_opa(obj, value, _dv_current_selector);
#define __text_opa_1(value)             _APPLY_TEXT_OPA(_dv_current_widget, value)
#define __text_opa_2(obj, value)        _APPLY_TEXT_OPA(obj, value)
#define __text_opa(...)                 _PASTE(__text_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_font(value) -> __text_font(...)
#define _APPLY_TEXT_FONT(obj, value)    lv_obj_set_style_text_font(obj, value, _dv_current_selector);
#define __text_font_1(value)            _APPLY_TEXT_FONT(_dv_current_widget, value)
#define __text_font_2(obj, value)       _APPLY_TEXT_FONT(obj, value)
#define __text_font(...)                _PASTE(__text_font_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_letter_space(value) -> __text_letter_space(...)
#define _APPLY_TEXT_LETTER_SPACE(obj, value) lv_obj_set_style_text_letter_space(obj, value, _dv_current_selector);
#define __text_letter_space_1(value)    _APPLY_TEXT_LETTER_SPACE(_dv_current_widget, value)
#define __text_letter_space_2(obj, value) _APPLY_TEXT_LETTER_SPACE(obj, value)
#define __text_letter_space(...)        _PASTE(__text_letter_space_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_line_space(value) -> __text_line_space(...)
#define _APPLY_TEXT_LINE_SPACE(obj, value) lv_obj_set_style_text_line_space(obj, value, _dv_current_selector);
#define __text_line_space_1(value)      _APPLY_TEXT_LINE_SPACE(_dv_current_widget, value)
#define __text_line_space_2(obj, value) _APPLY_TEXT_LINE_SPACE(obj, value)
#define __text_line_space(...)          _PASTE(__text_line_space_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_decor(value) -> __text_decor(...)
#define _APPLY_TEXT_DECOR(obj, value)   lv_obj_set_style_text_decor(obj, value, _dv_current_selector);
#define __text_decor_1(value)           _APPLY_TEXT_DECOR(_dv_current_widget, value)
#define __text_decor_2(obj, value)      _APPLY_TEXT_DECOR(obj, value)
#define __text_decor(...)               _PASTE(__text_decor_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_align(value) -> __text_align(...)
#define _APPLY_TEXT_ALIGN(obj, value)   lv_obj_set_style_text_align(obj, value, _dv_current_selector);
#define __text_align_1(value)           _APPLY_TEXT_ALIGN(_dv_current_widget, value)
#define __text_align_2(obj, value)      _APPLY_TEXT_ALIGN(obj, value)
#define __text_align(...)               _PASTE(__text_align_, _nargs(__VA_ARGS__))(__VA_ARGS__)

#if LV_USE_FONT_SUBPX // Check LVGL config
// _text_outline_stroke_color(value) -> __text_outline_stroke_color(...)
#define _APPLY_TEXT_OUTLINE_STROKE_COLOR(obj, value) lv_obj_set_style_text_outline_stroke_color(obj, value, _dv_current_selector);
#define __text_outline_stroke_color_1(value) _APPLY_TEXT_OUTLINE_STROKE_COLOR(_dv_current_widget, value)
#define __text_outline_stroke_color_2(obj, value) _APPLY_TEXT_OUTLINE_STROKE_COLOR(obj, value)
#define __text_outline_stroke_color(...) _PASTE(__text_outline_stroke_color_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_outline_stroke_width(value) -> __text_outline_stroke_width(...)
#define _APPLY_TEXT_OUTLINE_STROKE_WIDTH(obj, value) lv_obj_set_style_text_outline_stroke_width(obj, value, _dv_current_selector);
#define __text_outline_stroke_width_1(value) _APPLY_TEXT_OUTLINE_STROKE_WIDTH(_dv_current_widget, value)
#define __text_outline_stroke_width_2(obj, value) _APPLY_TEXT_OUTLINE_STROKE_WIDTH(obj, value)
#define __text_outline_stroke_width(...) _PASTE(__text_outline_stroke_width_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _text_outline_stroke_opa(value) -> __text_outline_stroke_opa(...)
#define _APPLY_TEXT_OUTLINE_STROKE_OPA(obj, value) lv_obj_set_style_text_outline_stroke_opa(obj, value, _dv_current_selector);
#define __text_outline_stroke_opa_1(value) _APPLY_TEXT_OUTLINE_STROKE_OPA(_dv_current_widget, value)
#define __text_outline_stroke_opa_2(obj, value) _APPLY_TEXT_OUTLINE_STROKE_OPA(obj, value)
#define __text_outline_stroke_opa(...)  _PASTE(__text_outline_stroke_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)
#endif // LV_USE_FONT_SUBPX

// _radius(value) -> __radius(...)
#define _APPLY_RADIUS(obj, value)       lv_obj_set_style_radius(obj, value, _dv_current_selector);
#define __radius_1(value)               _APPLY_RADIUS(_dv_current_widget, value)
#define __radius_2(obj, value)          _APPLY_RADIUS(obj, value)
#define __radius(...)                   _PASTE(__radius_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _radial_offset(value) -> __radial_offset(...)
#define _APPLY_RADIAL_OFFSET(obj, value) lv_obj_set_style_radial_offset(obj, value, _dv_current_selector);
#define __radial_offset_1(value)        _APPLY_RADIAL_OFFSET(_dv_current_widget, value)
#define __radial_offset_2(obj, value)   _APPLY_RADIAL_OFFSET(obj, value)
#define __radial_offset(...)            _PASTE(__radial_offset_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _clip_corner(value) -> __clip_corner(...)
#define _APPLY_CLIP_CORNER(obj, value)  lv_obj_set_style_clip_corner(obj, value, _dv_current_selector);
#define __clip_corner_1(value)          _APPLY_CLIP_CORNER(_dv_current_widget, value)
#define __clip_corner_2(obj, value)     _APPLY_CLIP_CORNER(obj, value)
#define __clip_corner(...)              _PASTE(__clip_corner_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _opa(value) -> __opa(...)
#define _APPLY_OPA(obj, value)          lv_obj_set_style_opa(obj, value, _dv_current_selector);
#define __opa_1(value)                  _APPLY_OPA(_dv_current_widget, value)
#define __opa_2(obj, value)             _APPLY_OPA(obj, value)
#define __opa(...)                      _PASTE(__opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _opa_layered(value) -> __opa_layered(...)
#define _APPLY_OPA_LAYERED(obj, value)  lv_obj_set_style_opa_layered(obj, value, _dv_current_selector);
#define __opa_layered_1(value)          _APPLY_OPA_LAYERED(_dv_current_widget, value)
#define __opa_layered_2(obj, value)     _APPLY_OPA_LAYERED(obj, value)
#define __opa_layered(...)              _PASTE(__opa_layered_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _color_filter_dsc(value) -> __color_filter_dsc(...)
#define _APPLY_COLOR_FILTER_DSC(obj, value) lv_obj_set_style_color_filter_dsc(obj, value, _dv_current_selector);
#define __color_filter_dsc_1(value)     _APPLY_COLOR_FILTER_DSC(_dv_current_widget, value)
#define __color_filter_dsc_2(obj, value) _APPLY_COLOR_FILTER_DSC(obj, value)
#define __color_filter_dsc(...)         _PASTE(__color_filter_dsc_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _color_filter_opa(value) -> __color_filter_opa(...)
#define _APPLY_COLOR_FILTER_OPA(obj, value) lv_obj_set_style_color_filter_opa(obj, value, _dv_current_selector);
#define __color_filter_opa_1(value)     _APPLY_COLOR_FILTER_OPA(_dv_current_widget, value)
#define __color_filter_opa_2(obj, value) _APPLY_COLOR_FILTER_OPA(obj, value)
#define __color_filter_opa(...)         _PASTE(__color_filter_opa_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _anim(value) -> __anim(...)
#define _APPLY_ANIM(obj, value)         lv_obj_set_style_anim(obj, value, _dv_current_selector);
#define __anim_1(value)                 _APPLY_ANIM(_dv_current_widget, value)
#define __anim_2(obj, value)            _APPLY_ANIM(obj, value)
#define __anim(...)                     _PASTE(__anim_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _anim_duration(value) -> __anim_duration(...)
#define _APPLY_ANIM_DURATION(obj, value) lv_obj_set_style_anim_duration(obj, value, _dv_current_selector);
#define __anim_duration_1(value)        _APPLY_ANIM_DURATION(_dv_current_widget, value)
#define __anim_duration_2(obj, value)   _APPLY_ANIM_DURATION(obj, value)
#define __anim_duration(...)            _PASTE(__anim_duration_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _transition(value) -> __transition(...)
#define _APPLY_TRANSITION(obj, value)   lv_obj_set_style_transition(obj, value, _dv_current_selector);
#define __transition_1(value)           _APPLY_TRANSITION(_dv_current_widget, value)
#define __transition_2(obj, value)      _APPLY_TRANSITION(obj, value)
#define __transition(...)               _PASTE(__transition_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _blend_mode(value) -> __blend_mode(...)
#define _APPLY_BLEND_MODE(obj, value)   lv_obj_set_style_blend_mode(obj, value, _dv_current_selector);
#define __blend_mode_1(value)           _APPLY_BLEND_MODE(_dv_current_widget, value)
#define __blend_mode_2(obj, value)      _APPLY_BLEND_MODE(obj, value)
#define __blend_mode(...)               _PASTE(__blend_mode_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _layout(value) -> __layout(...)
#define _APPLY_LAYOUT(obj, value)       lv_obj_set_style_layout(obj, value, _dv_current_selector);
#define __layout_1(value)               _APPLY_LAYOUT(_dv_current_widget, value)
#define __layout_2(obj, value)          _APPLY_LAYOUT(obj, value)
#define __layout(...)                   _PASTE(__layout_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _base_dir(value) -> __base_dir(...)
#define _APPLY_BASE_DIR(obj, value)     lv_obj_set_style_base_dir(obj, value, _dv_current_selector);
#define __base_dir_1(value)             _APPLY_BASE_DIR(_dv_current_widget, value)
#define __base_dir_2(obj, value)        _APPLY_BASE_DIR(obj, value)
#define __base_dir(...)                 _PASTE(__base_dir_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bitmap_mask_src(value) -> __bitmap_mask_src(...)
#define _APPLY_BITMAP_MASK_SRC(obj, value) lv_obj_set_style_bitmap_mask_src(obj, value, _dv_current_selector);
#define __bitmap_mask_src_1(value)      _APPLY_BITMAP_MASK_SRC(_dv_current_widget, value)
#define __bitmap_mask_src_2(obj, value) _APPLY_BITMAP_MASK_SRC(obj, value)
#define __bitmap_mask_src(...)          _PASTE(__bitmap_mask_src_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _rotary_sensitivity(value) -> __rotary_sensitivity(...)
#define _APPLY_ROTARY_SENSITIVITY(obj, value) lv_obj_set_style_rotary_sensitivity(obj, value, _dv_current_selector);
#define __rotary_sensitivity_1(value)   _APPLY_ROTARY_SENSITIVITY(_dv_current_widget, value)
#define __rotary_sensitivity_2(obj, value) _APPLY_ROTARY_SENSITIVITY(obj, value)
#define __rotary_sensitivity(...)       _PASTE(__rotary_sensitivity_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _flex_flow(value) -> __flex_flow(...)
#define _APPLY_FLEX_FLOW(obj, value)    lv_obj_set_style_flex_flow(obj, value, _dv_current_selector);
#define __flex_flow_1(value)            _APPLY_FLEX_FLOW(_dv_current_widget, value)
#define __flex_flow_2(obj, value)       _APPLY_FLEX_FLOW(obj, value)
#define __flex_flow(...)                _PASTE(__flex_flow_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _flex_main_place(value) -> __flex_main_place(...)
#define _APPLY_FLEX_MAIN_PLACE(obj, value) lv_obj_set_style_flex_main_place(obj, value, _dv_current_selector);
#define __flex_main_place_1(value)      _APPLY_FLEX_MAIN_PLACE(_dv_current_widget, value)
#define __flex_main_place_2(obj, value) _APPLY_FLEX_MAIN_PLACE(obj, value)
#define __flex_main_place(...)          _PASTE(__flex_main_place_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _flex_cross_place(value) -> __flex_cross_place(...)
#define _APPLY_FLEX_CROSS_PLACE(obj, value) lv_obj_set_style_flex_cross_place(obj, value, _dv_current_selector);
#define __flex_cross_place_1(value)     _APPLY_FLEX_CROSS_PLACE(_dv_current_widget, value)
#define __flex_cross_place_2(obj, value) _APPLY_FLEX_CROSS_PLACE(obj, value)
#define __flex_cross_place(...)         _PASTE(__flex_cross_place_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _flex_track_place(value) -> __flex_track_place(...)
#define _APPLY_FLEX_TRACK_PLACE(obj, value) lv_obj_set_style_flex_track_place(obj, value, _dv_current_selector);
#define __flex_track_place_1(value)     _APPLY_FLEX_TRACK_PLACE(_dv_current_widget, value)
#define __flex_track_place_2(obj, value) _APPLY_FLEX_TRACK_PLACE(obj, value)
#define __flex_track_place(...)         _PASTE(__flex_track_place_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _flex_grow(value) -> __flex_grow(...)
#define _APPLY_FLEX_GROW(obj, value)    lv_obj_set_style_flex_grow(obj, value, _dv_current_selector);
#define __flex_grow_1(value)            _APPLY_FLEX_GROW(_dv_current_widget, value)
#define __flex_grow_2(obj, value)       _APPLY_FLEX_GROW(obj, value)
#define __flex_grow(...)                _PASTE(__flex_grow_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_column_dsc_array(value) -> __grid_column_dsc_array(...)
#define _APPLY_GRID_COLUMN_DSC_ARRAY(obj, value) lv_obj_set_style_grid_column_dsc_array(obj, value, _dv_current_selector);
#define __grid_column_dsc_array_1(value) _APPLY_GRID_COLUMN_DSC_ARRAY(_dv_current_widget, value)
#define __grid_column_dsc_array_2(obj, value) _APPLY_GRID_COLUMN_DSC_ARRAY(obj, value)
#define __grid_column_dsc_array(...)    _PASTE(__grid_column_dsc_array_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_column_align(value) -> __grid_column_align(...)
#define _APPLY_GRID_COLUMN_ALIGN(obj, value) lv_obj_set_style_grid_column_align(obj, value, _dv_current_selector);
#define __grid_column_align_1(value)    _APPLY_GRID_COLUMN_ALIGN(_dv_current_widget, value)
#define __grid_column_align_2(obj, value) _APPLY_GRID_COLUMN_ALIGN(obj, value)
#define __grid_column_align(...)        _PASTE(__grid_column_align_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_row_dsc_array(value) -> __grid_row_dsc_array(...)
#define _APPLY_GRID_ROW_DSC_ARRAY(obj, value) lv_obj_set_style_grid_row_dsc_array(obj, value, _dv_current_selector);
#define __grid_row_dsc_array_1(value)   _APPLY_GRID_ROW_DSC_ARRAY(_dv_current_widget, value)
#define __grid_row_dsc_array_2(obj, value) _APPLY_GRID_ROW_DSC_ARRAY(obj, value)
#define __grid_row_dsc_array(...)       _PASTE(__grid_row_dsc_array_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_row_align(value) -> __grid_row_align(...)
#define _APPLY_GRID_ROW_ALIGN(obj, value) lv_obj_set_style_grid_row_align(obj, value, _dv_current_selector);
#define __grid_row_align_1(value)       _APPLY_GRID_ROW_ALIGN(_dv_current_widget, value)
#define __grid_row_align_2(obj, value)  _APPLY_GRID_ROW_ALIGN(obj, value)
#define __grid_row_align(...)           _PASTE(__grid_row_align_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_cell_column_pos(value) -> __grid_cell_column_pos(...)
#define _APPLY_GRID_CELL_COLUMN_POS(obj, value) lv_obj_set_style_grid_cell_column_pos(obj, value, _dv_current_selector);
#define __grid_cell_column_pos_1(value) _APPLY_GRID_CELL_COLUMN_POS(_dv_current_widget, value)
#define __grid_cell_column_pos_2(obj, value) _APPLY_GRID_CELL_COLUMN_POS(obj, value)
#define __grid_cell_column_pos(...)     _PASTE(__grid_cell_column_pos_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_cell_x_align(value) -> __grid_cell_x_align(...)
#define _APPLY_GRID_CELL_X_ALIGN(obj, value) lv_obj_set_style_grid_cell_x_align(obj, value, _dv_current_selector);
#define __grid_cell_x_align_1(value)    _APPLY_GRID_CELL_X_ALIGN(_dv_current_widget, value)
#define __grid_cell_x_align_2(obj, value) _APPLY_GRID_CELL_X_ALIGN(obj, value)
#define __grid_cell_x_align(...)        _PASTE(__grid_cell_x_align_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_cell_column_span(value) -> __grid_cell_column_span(...)
#define _APPLY_GRID_CELL_COLUMN_SPAN(obj, value) lv_obj_set_style_grid_cell_column_span(obj, value, _dv_current_selector);
#define __grid_cell_column_span_1(value) _APPLY_GRID_CELL_COLUMN_SPAN(_dv_current_widget, value)
#define __grid_cell_column_span_2(obj, value) _APPLY_GRID_CELL_COLUMN_SPAN(obj, value)
#define __grid_cell_column_span(...)    _PASTE(__grid_cell_column_span_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_cell_row_pos(value) -> __grid_cell_row_pos(...)
#define _APPLY_GRID_CELL_ROW_POS(obj, value) lv_obj_set_style_grid_cell_row_pos(obj, value, _dv_current_selector);
#define __grid_cell_row_pos_1(value)    _APPLY_GRID_CELL_ROW_POS(_dv_current_widget, value)
#define __grid_cell_row_pos_2(obj, value) _APPLY_GRID_CELL_ROW_POS(obj, value)
#define __grid_cell_row_pos(...)        _PASTE(__grid_cell_row_pos_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_cell_y_align(value) -> __grid_cell_y_align(...)
#define _APPLY_GRID_CELL_Y_ALIGN(obj, value) lv_obj_set_style_grid_cell_y_align(obj, value, _dv_current_selector);
#define __grid_cell_y_align_1(value)    _APPLY_GRID_CELL_Y_ALIGN(_dv_current_widget, value)
#define __grid_cell_y_align_2(obj, value) _APPLY_GRID_CELL_Y_ALIGN(obj, value)
#define __grid_cell_y_align(...)        _PASTE(__grid_cell_y_align_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_cell_row_span(value) -> __grid_cell_row_span(...)
#define _APPLY_GRID_CELL_ROW_SPAN(obj, value) lv_obj_set_style_grid_cell_row_span(obj, value, _dv_current_selector);
#define __grid_cell_row_span_1(value)   _APPLY_GRID_CELL_ROW_SPAN(_dv_current_widget, value)
#define __grid_cell_row_span_2(obj, value) _APPLY_GRID_CELL_ROW_SPAN(obj, value)
#define __grid_cell_row_span(...)       _PASTE(__grid_cell_row_span_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_column_gap(value) -> __grid_column_gap(...)
#define _APPLY_GRID_COLUMN_GAP(obj, value) lv_obj_set_style_grid_column_gap(obj, value, _dv_current_selector);
#define __grid_column_gap_1(value)      _APPLY_GRID_COLUMN_GAP(_dv_current_widget, value)
#define __grid_column_gap_2(obj, value) _APPLY_GRID_COLUMN_GAP(obj, value)
#define __grid_column_gap(...)          _PASTE(__grid_column_gap_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _grid_row_gap(value) -> __grid_row_gap(...)
#define _APPLY_GRID_ROW_GAP(obj, value) lv_obj_set_style_grid_row_gap(obj, value, _dv_current_selector);
#define __grid_row_gap_1(value)         _APPLY_GRID_ROW_GAP(_dv_current_widget, value)
#define __grid_row_gap_2(obj, value)    _APPLY_GRID_ROW_GAP(obj, value)
#define __grid_row_gap(...)             _PASTE(__grid_row_gap_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// Add near other widget creation macros like label, button...
#define bar(VarName, ...)  _setup_widget(lv_obj_t, lv_bar_create, VarName, __VA_ARGS__)

// Add near other widget-specific direct property setters like _label_text...
#define _bar_range(min, max)       lv_bar_set_range(_dv_current_widget, min, max);
#define _bar_value(val, anim)      lv_bar_set_value(_dv_current_widget, val, anim);
#define _bar_mode(mode)            lv_bar_set_mode(_dv_current_widget, mode);

// Add near other widget-specific direct property getters like label_text_...
#define bar_min_value_()           lv_bar_get_min_value(_dv_current_widget)
#define bar_max_value_()           lv_bar_get_max_value(_dv_current_widget)
#define bar_value_()               lv_bar_get_value(_dv_current_widget)
#define bar_mode_()                lv_bar_get_mode(_dv_current_widget)

// Add overloaded versions near __label_text, etc.
// _bar_range(min, max) -> __bar_range(...)
#define _APPLY_BAR_RANGE(obj, min, max) lv_bar_set_range(obj, min, max);
#define __bar_range_2(min, max)         _APPLY_BAR_RANGE(_dv_current_widget, min, max)
#define __bar_range_3(obj, min, max)    _APPLY_BAR_RANGE(obj, min, max)
#define __bar_range(...)                _PASTE(__bar_range_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bar_value(val, anim) -> __bar_value(...)
#define _APPLY_BAR_VALUE(obj, val, anim) lv_bar_set_value(obj, val, anim);
#define __bar_value_2(val, anim)        _APPLY_BAR_VALUE(_dv_current_widget, val, anim)
#define __bar_value_3(obj, val, anim)   _APPLY_BAR_VALUE(obj, val, anim)
#define __bar_value(...)                _PASTE(__bar_value_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// _bar_mode(mode) -> __bar_mode(...)
#define _APPLY_BAR_MODE(obj, mode)      lv_bar_set_mode(obj, mode);
#define __bar_mode_1(mode)              _APPLY_BAR_MODE(_dv_current_widget, mode)
#define __bar_mode_2(obj, mode)         _APPLY_BAR_MODE(obj, mode)
#define __bar_mode(...)                 _PASTE(__bar_mode_, _nargs(__VA_ARGS__))(__VA_ARGS__)

// Add overloaded getters
#define _GET_BAR_MIN_VALUE(obj)         lv_bar_get_min_value(obj)
#define __bar_min_value_0_()            _GET_BAR_MIN_VALUE(_dv_current_widget)
#define __bar_min_value_1_(obj)         _GET_BAR_MIN_VALUE(obj)
#define __bar_min_value_(...)           _PASTE(__bar_min_value_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)
// ... add similar for max_value, value, mode ...

// --- Overloaded Widget Style Getters ---
// (These use _dv_current_selector implicitly via the _GET macro)
// min_width_() -> __min_width_(...)
#define _GET_MIN_WIDTH(obj)             lv_obj_get_style_min_width(obj, _dv_current_selector)
#define __min_width_0_()                _GET_MIN_WIDTH(_dv_current_widget)
#define __min_width_1_(obj)             _GET_MIN_WIDTH(obj)
#define __min_width_(...)               _PASTE(__min_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// max_width_() -> __max_width_(...)
#define _GET_MAX_WIDTH(obj)             lv_obj_get_style_max_width(obj, _dv_current_selector)
#define __max_width_0_()                _GET_MAX_WIDTH(_dv_current_widget)
#define __max_width_1_(obj)             _GET_MAX_WIDTH(obj)
#define __max_width_(...)               _PASTE(__max_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// min_height_() -> __min_height_(...)
#define _GET_MIN_HEIGHT(obj)            lv_obj_get_style_min_height(obj, _dv_current_selector)
#define __min_height_0_()               _GET_MIN_HEIGHT(_dv_current_widget)
#define __min_height_1_(obj)            _GET_MIN_HEIGHT(obj)
#define __min_height_(...)              _PASTE(__min_height_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// max_height_() -> __max_height_(...)
#define _GET_MAX_HEIGHT(obj)            lv_obj_get_style_max_height(obj, _dv_current_selector)
#define __max_height_0_()               _GET_MAX_HEIGHT(_dv_current_widget)
#define __max_height_1_(obj)            _GET_MAX_HEIGHT(obj)
#define __max_height_(...)              _PASTE(__max_height_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// length_() -> __length_(...)
#define _GET_LENGTH(obj)                lv_obj_get_style_length(obj, _dv_current_selector)
#define __length_0_()                   _GET_LENGTH(_dv_current_widget)
#define __length_1_(obj)                _GET_LENGTH(obj)
#define __length_(...)                  _PASTE(__length_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_width_() -> __transform_width_(...)
#define _GET_TRANSFORM_WIDTH(obj)       lv_obj_get_style_transform_width(obj, _dv_current_selector)
#define __transform_width_0_()          _GET_TRANSFORM_WIDTH(_dv_current_widget)
#define __transform_width_1_(obj)       _GET_TRANSFORM_WIDTH(obj)
#define __transform_width_(...)         _PASTE(__transform_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_height_() -> __transform_height_(...)
#define _GET_TRANSFORM_HEIGHT(obj)      lv_obj_get_style_transform_height(obj, _dv_current_selector)
#define __transform_height_0_()         _GET_TRANSFORM_HEIGHT(_dv_current_widget)
#define __transform_height_1_(obj)      _GET_TRANSFORM_HEIGHT(obj)
#define __transform_height_(...)        _PASTE(__transform_height_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// translate_x_() -> __translate_x_(...)
#define _GET_TRANSLATE_X(obj)           lv_obj_get_style_translate_x(obj, _dv_current_selector)
#define __translate_x_0_()              _GET_TRANSLATE_X(_dv_current_widget)
#define __translate_x_1_(obj)           _GET_TRANSLATE_X(obj)
#define __translate_x_(...)             _PASTE(__translate_x_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// translate_y_() -> __translate_y_(...)
#define _GET_TRANSLATE_Y(obj)           lv_obj_get_style_translate_y(obj, _dv_current_selector)
#define __translate_y_0_()              _GET_TRANSLATE_Y(_dv_current_widget)
#define __translate_y_1_(obj)           _GET_TRANSLATE_Y(obj)
#define __translate_y_(...)             _PASTE(__translate_y_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// translate_radial_() -> __translate_radial_(...)
#define _GET_TRANSLATE_RADIAL(obj)      lv_obj_get_style_translate_radial(obj, _dv_current_selector)
#define __translate_radial_0_()         _GET_TRANSLATE_RADIAL(_dv_current_widget)
#define __translate_radial_1_(obj)      _GET_TRANSLATE_RADIAL(obj)
#define __translate_radial_(...)        _PASTE(__translate_radial_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_scale_x_() -> __transform_scale_x_(...)
#define _GET_TRANSFORM_SCALE_X(obj)     lv_obj_get_style_transform_scale_x(obj, _dv_current_selector)
#define __transform_scale_x_0_()        _GET_TRANSFORM_SCALE_X(_dv_current_widget)
#define __transform_scale_x_1_(obj)     _GET_TRANSFORM_SCALE_X(obj)
#define __transform_scale_x_(...)       _PASTE(__transform_scale_x_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_scale_y_() -> __transform_scale_y_(...)
#define _GET_TRANSFORM_SCALE_Y(obj)     lv_obj_get_style_transform_scale_y(obj, _dv_current_selector)
#define __transform_scale_y_0_()        _GET_TRANSFORM_SCALE_Y(_dv_current_widget)
#define __transform_scale_y_1_(obj)     _GET_TRANSFORM_SCALE_Y(obj)
#define __transform_scale_y_(...)       _PASTE(__transform_scale_y_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_rotation_() -> __transform_rotation_(...)
#define _GET_TRANSFORM_ROTATION(obj)    lv_obj_get_style_transform_rotation(obj, _dv_current_selector)
#define __transform_rotation_0_()       _GET_TRANSFORM_ROTATION(_dv_current_widget)
#define __transform_rotation_1_(obj)    _GET_TRANSFORM_ROTATION(obj)
#define __transform_rotation_(...)      _PASTE(__transform_rotation_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_pivot_x_() -> __transform_pivot_x_(...)
#define _GET_TRANSFORM_PIVOT_X(obj)     lv_obj_get_style_transform_pivot_x(obj, _dv_current_selector)
#define __transform_pivot_x_0_()        _GET_TRANSFORM_PIVOT_X(_dv_current_widget)
#define __transform_pivot_x_1_(obj)     _GET_TRANSFORM_PIVOT_X(obj)
#define __transform_pivot_x_(...)       _PASTE(__transform_pivot_x_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_pivot_y_() -> __transform_pivot_y_(...)
#define _GET_TRANSFORM_PIVOT_Y(obj)     lv_obj_get_style_transform_pivot_y(obj, _dv_current_selector)
#define __transform_pivot_y_0_()        _GET_TRANSFORM_PIVOT_Y(_dv_current_widget)
#define __transform_pivot_y_1_(obj)     _GET_TRANSFORM_PIVOT_Y(obj)
#define __transform_pivot_y_(...)       _PASTE(__transform_pivot_y_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_skew_x_() -> __transform_skew_x_(...)
#define _GET_TRANSFORM_SKEW_X(obj)      lv_obj_get_style_transform_skew_x(obj, _dv_current_selector)
#define __transform_skew_x_0_()         _GET_TRANSFORM_SKEW_X(_dv_current_widget)
#define __transform_skew_x_1_(obj)      _GET_TRANSFORM_SKEW_X(obj)
#define __transform_skew_x_(...)        _PASTE(__transform_skew_x_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transform_skew_y_() -> __transform_skew_y_(...)
#define _GET_TRANSFORM_SKEW_Y(obj)      lv_obj_get_style_transform_skew_y(obj, _dv_current_selector)
#define __transform_skew_y_0_()         _GET_TRANSFORM_SKEW_Y(_dv_current_widget)
#define __transform_skew_y_1_(obj)      _GET_TRANSFORM_SKEW_Y(obj)
#define __transform_skew_y_(...)        _PASTE(__transform_skew_y_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_top_() -> __pad_top_(...)
#define _GET_PAD_TOP(obj)               lv_obj_get_style_pad_top(obj, _dv_current_selector)
#define __pad_top_0_()                  _GET_PAD_TOP(_dv_current_widget)
#define __pad_top_1_(obj)               _GET_PAD_TOP(obj)
#define __pad_top_(...)                 _PASTE(__pad_top_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_bottom_() -> __pad_bottom_(...)
#define _GET_PAD_BOTTOM(obj)            lv_obj_get_style_pad_bottom(obj, _dv_current_selector)
#define __pad_bottom_0_()               _GET_PAD_BOTTOM(_dv_current_widget)
#define __pad_bottom_1_(obj)            _GET_PAD_BOTTOM(obj)
#define __pad_bottom_(...)              _PASTE(__pad_bottom_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_left_() -> __pad_left_(...)
#define _GET_PAD_LEFT(obj)              lv_obj_get_style_pad_left(obj, _dv_current_selector)
#define __pad_left_0_()                 _GET_PAD_LEFT(_dv_current_widget)
#define __pad_left_1_(obj)              _GET_PAD_LEFT(obj)
#define __pad_left_(...)                _PASTE(__pad_left_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_right_() -> __pad_right_(...)
#define _GET_PAD_RIGHT(obj)             lv_obj_get_style_pad_right(obj, _dv_current_selector)
#define __pad_right_0_()                _GET_PAD_RIGHT(_dv_current_widget)
#define __pad_right_1_(obj)             _GET_PAD_RIGHT(obj)
#define __pad_right_(...)               _PASTE(__pad_right_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_row_() -> __pad_row_(...)
#define _GET_PAD_ROW(obj)               lv_obj_get_style_pad_row(obj, _dv_current_selector)
#define __pad_row_0_()                  _GET_PAD_ROW(_dv_current_widget)
#define __pad_row_1_(obj)               _GET_PAD_ROW(obj)
#define __pad_row_(...)                 _PASTE(__pad_row_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_column_() -> __pad_column_(...)
#define _GET_PAD_COLUMN(obj)            lv_obj_get_style_pad_column(obj, _dv_current_selector)
#define __pad_column_0_()               _GET_PAD_COLUMN(_dv_current_widget)
#define __pad_column_1_(obj)            _GET_PAD_COLUMN(obj)
#define __pad_column_(...)              _PASTE(__pad_column_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// pad_radial_() -> __pad_radial_(...)
#define _GET_PAD_RADIAL(obj)            lv_obj_get_style_pad_radial(obj, _dv_current_selector)
#define __pad_radial_0_()               _GET_PAD_RADIAL(_dv_current_widget)
#define __pad_radial_1_(obj)            _GET_PAD_RADIAL(obj)
#define __pad_radial_(...)              _PASTE(__pad_radial_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// margin_top_() -> __margin_top_(...)
#define _GET_MARGIN_TOP(obj)            lv_obj_get_style_margin_top(obj, _dv_current_selector)
#define __margin_top_0_()               _GET_MARGIN_TOP(_dv_current_widget)
#define __margin_top_1_(obj)            _GET_MARGIN_TOP(obj)
#define __margin_top_(...)              _PASTE(__margin_top_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// margin_bottom_() -> __margin_bottom_(...)
#define _GET_MARGIN_BOTTOM(obj)         lv_obj_get_style_margin_bottom(obj, _dv_current_selector)
#define __margin_bottom_0_()            _GET_MARGIN_BOTTOM(_dv_current_widget)
#define __margin_bottom_1_(obj)         _GET_MARGIN_BOTTOM(obj)
#define __margin_bottom_(...)           _PASTE(__margin_bottom_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// margin_left_() -> __margin_left_(...)
#define _GET_MARGIN_LEFT(obj)           lv_obj_get_style_margin_left(obj, _dv_current_selector)
#define __margin_left_0_()              _GET_MARGIN_LEFT(_dv_current_widget)
#define __margin_left_1_(obj)           _GET_MARGIN_LEFT(obj)
#define __margin_left_(...)             _PASTE(__margin_left_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// margin_right_() -> __margin_right_(...)
#define _GET_MARGIN_RIGHT(obj)          lv_obj_get_style_margin_right(obj, _dv_current_selector)
#define __margin_right_0_()             _GET_MARGIN_RIGHT(_dv_current_widget)
#define __margin_right_1_(obj)          _GET_MARGIN_RIGHT(obj)
#define __margin_right_(...)            _PASTE(__margin_right_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_color_() -> __bg_color_(...)
#define _GET_BG_COLOR(obj)              lv_obj_get_style_bg_color(obj, _dv_current_selector)
#define __bg_color_0_()                 _GET_BG_COLOR(_dv_current_widget)
#define __bg_color_1_(obj)              _GET_BG_COLOR(obj)
#define __bg_color_(...)                _PASTE(__bg_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_opa_() -> __bg_opa_(...)
#define _GET_BG_OPA(obj)                lv_obj_get_style_bg_opa(obj, _dv_current_selector)
#define __bg_opa_0_()                   _GET_BG_OPA(_dv_current_widget)
#define __bg_opa_1_(obj)                _GET_BG_OPA(obj)
#define __bg_opa_(...)                  _PASTE(__bg_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_grad_color_() -> __bg_grad_color_(...)
#define _GET_BG_GRAD_COLOR(obj)         lv_obj_get_style_bg_grad_color(obj, _dv_current_selector)
#define __bg_grad_color_0_()            _GET_BG_GRAD_COLOR(_dv_current_widget)
#define __bg_grad_color_1_(obj)         _GET_BG_GRAD_COLOR(obj)
#define __bg_grad_color_(...)           _PASTE(__bg_grad_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_grad_dir_() -> __bg_grad_dir_(...)
#define _GET_BG_GRAD_DIR(obj)           lv_obj_get_style_bg_grad_dir(obj, _dv_current_selector)
#define __bg_grad_dir_0_()              _GET_BG_GRAD_DIR(_dv_current_widget)
#define __bg_grad_dir_1_(obj)           _GET_BG_GRAD_DIR(obj)
#define __bg_grad_dir_(...)             _PASTE(__bg_grad_dir_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_main_stop_() -> __bg_main_stop_(...)
#define _GET_BG_MAIN_STOP(obj)          lv_obj_get_style_bg_main_stop(obj, _dv_current_selector)
#define __bg_main_stop_0_()             _GET_BG_MAIN_STOP(_dv_current_widget)
#define __bg_main_stop_1_(obj)          _GET_BG_MAIN_STOP(obj)
#define __bg_main_stop_(...)            _PASTE(__bg_main_stop_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_grad_stop_() -> __bg_grad_stop_(...)
#define _GET_BG_GRAD_STOP(obj)          lv_obj_get_style_bg_grad_stop(obj, _dv_current_selector)
#define __bg_grad_stop_0_()             _GET_BG_GRAD_STOP(_dv_current_widget)
#define __bg_grad_stop_1_(obj)          _GET_BG_GRAD_STOP(obj)
#define __bg_grad_stop_(...)            _PASTE(__bg_grad_stop_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_main_opa_() -> __bg_main_opa_(...)
#define _GET_BG_MAIN_OPA(obj)           lv_obj_get_style_bg_main_opa(obj, _dv_current_selector)
#define __bg_main_opa_0_()              _GET_BG_MAIN_OPA(_dv_current_widget)
#define __bg_main_opa_1_(obj)           _GET_BG_MAIN_OPA(obj)
#define __bg_main_opa_(...)             _PASTE(__bg_main_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_grad_opa_() -> __bg_grad_opa_(...)
#define _GET_BG_GRAD_OPA(obj)           lv_obj_get_style_bg_grad_opa(obj, _dv_current_selector)
#define __bg_grad_opa_0_()              _GET_BG_GRAD_OPA(_dv_current_widget)
#define __bg_grad_opa_1_(obj)           _GET_BG_GRAD_OPA(obj)
#define __bg_grad_opa_(...)             _PASTE(__bg_grad_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_grad_() -> __bg_grad_(...)
#define _GET_BG_GRAD(obj)               lv_obj_get_style_bg_grad(obj, _dv_current_selector)
#define __bg_grad_0_()                  _GET_BG_GRAD(_dv_current_widget)
#define __bg_grad_1_(obj)               _GET_BG_GRAD(obj)
#define __bg_grad_(...)                 _PASTE(__bg_grad_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_image_src_() -> __bg_image_src_(...)
#define _GET_BG_IMAGE_SRC(obj)          lv_obj_get_style_bg_image_src(obj, _dv_current_selector)
#define __bg_image_src_0_()             _GET_BG_IMAGE_SRC(_dv_current_widget)
#define __bg_image_src_1_(obj)          _GET_BG_IMAGE_SRC(obj)
#define __bg_image_src_(...)            _PASTE(__bg_image_src_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_image_opa_() -> __bg_image_opa_(...)
#define _GET_BG_IMAGE_OPA(obj)          lv_obj_get_style_bg_image_opa(obj, _dv_current_selector)
#define __bg_image_opa_0_()             _GET_BG_IMAGE_OPA(_dv_current_widget)
#define __bg_image_opa_1_(obj)          _GET_BG_IMAGE_OPA(obj)
#define __bg_image_opa_(...)            _PASTE(__bg_image_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_image_recolor_() -> __bg_image_recolor_(...)
#define _GET_BG_IMAGE_RECOLOR(obj)      lv_obj_get_style_bg_image_recolor(obj, _dv_current_selector)
#define __bg_image_recolor_0_()         _GET_BG_IMAGE_RECOLOR(_dv_current_widget)
#define __bg_image_recolor_1_(obj)      _GET_BG_IMAGE_RECOLOR(obj)
#define __bg_image_recolor_(...)        _PASTE(__bg_image_recolor_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_image_recolor_opa_() -> __bg_image_recolor_opa_(...)
#define _GET_BG_IMAGE_RECOLOR_OPA(obj)  lv_obj_get_style_bg_image_recolor_opa(obj, _dv_current_selector)
#define __bg_image_recolor_opa_0_()     _GET_BG_IMAGE_RECOLOR_OPA(_dv_current_widget)
#define __bg_image_recolor_opa_1_(obj)  _GET_BG_IMAGE_RECOLOR_OPA(obj)
#define __bg_image_recolor_opa_(...)    _PASTE(__bg_image_recolor_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bg_image_tiled_() -> __bg_image_tiled_(...)
#define _GET_BG_IMAGE_TILED(obj)        lv_obj_get_style_bg_image_tiled(obj, _dv_current_selector)
#define __bg_image_tiled_0_()           _GET_BG_IMAGE_TILED(_dv_current_widget)
#define __bg_image_tiled_1_(obj)        _GET_BG_IMAGE_TILED(obj)
#define __bg_image_tiled_(...)          _PASTE(__bg_image_tiled_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// border_color_() -> __border_color_(...)
#define _GET_BORDER_COLOR(obj)          lv_obj_get_style_border_color(obj, _dv_current_selector)
#define __border_color_0_()             _GET_BORDER_COLOR(_dv_current_widget)
#define __border_color_1_(obj)          _GET_BORDER_COLOR(obj)
#define __border_color_(...)            _PASTE(__border_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// border_opa_() -> __border_opa_(...)
#define _GET_BORDER_OPA(obj)            lv_obj_get_style_border_opa(obj, _dv_current_selector)
#define __border_opa_0_()               _GET_BORDER_OPA(_dv_current_widget)
#define __border_opa_1_(obj)            _GET_BORDER_OPA(obj)
#define __border_opa_(...)              _PASTE(__border_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// border_width_() -> __border_width_(...)
#define _GET_BORDER_WIDTH(obj)          lv_obj_get_style_border_width(obj, _dv_current_selector)
#define __border_width_0_()             _GET_BORDER_WIDTH(_dv_current_widget)
#define __border_width_1_(obj)          _GET_BORDER_WIDTH(obj)
#define __border_width_(...)            _PASTE(__border_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// border_side_() -> __border_side_(...)
#define _GET_BORDER_SIDE(obj)           lv_obj_get_style_border_side(obj, _dv_current_selector)
#define __border_side_0_()              _GET_BORDER_SIDE(_dv_current_widget)
#define __border_side_1_(obj)           _GET_BORDER_SIDE(obj)
#define __border_side_(...)             _PASTE(__border_side_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// border_post_() -> __border_post_(...)
#define _GET_BORDER_POST(obj)           lv_obj_get_style_border_post(obj, _dv_current_selector)
#define __border_post_0_()              _GET_BORDER_POST(_dv_current_widget)
#define __border_post_1_(obj)           _GET_BORDER_POST(obj)
#define __border_post_(...)             _PASTE(__border_post_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// outline_width_() -> __outline_width_(...)
#define _GET_OUTLINE_WIDTH(obj)         lv_obj_get_style_outline_width(obj, _dv_current_selector)
#define __outline_width_0_()            _GET_OUTLINE_WIDTH(_dv_current_widget)
#define __outline_width_1_(obj)         _GET_OUTLINE_WIDTH(obj)
#define __outline_width_(...)           _PASTE(__outline_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// outline_color_() -> __outline_color_(...)
#define _GET_OUTLINE_COLOR(obj)         lv_obj_get_style_outline_color(obj, _dv_current_selector)
#define __outline_color_0_()            _GET_OUTLINE_COLOR(_dv_current_widget)
#define __outline_color_1_(obj)         _GET_OUTLINE_COLOR(obj)
#define __outline_color_(...)           _PASTE(__outline_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// outline_opa_() -> __outline_opa_(...)
#define _GET_OUTLINE_OPA(obj)           lv_obj_get_style_outline_opa(obj, _dv_current_selector)
#define __outline_opa_0_()              _GET_OUTLINE_OPA(_dv_current_widget)
#define __outline_opa_1_(obj)           _GET_OUTLINE_OPA(obj)
#define __outline_opa_(...)             _PASTE(__outline_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// outline_pad_() -> __outline_pad_(...)
#define _GET_OUTLINE_PAD(obj)           lv_obj_get_style_outline_pad(obj, _dv_current_selector)
#define __outline_pad_0_()              _GET_OUTLINE_PAD(_dv_current_widget)
#define __outline_pad_1_(obj)           _GET_OUTLINE_PAD(obj)
#define __outline_pad_(...)             _PASTE(__outline_pad_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// shadow_width_() -> __shadow_width_(...)
#define _GET_SHADOW_WIDTH(obj)          lv_obj_get_style_shadow_width(obj, _dv_current_selector)
#define __shadow_width_0_()             _GET_SHADOW_WIDTH(_dv_current_widget)
#define __shadow_width_1_(obj)          _GET_SHADOW_WIDTH(obj)
#define __shadow_width_(...)            _PASTE(__shadow_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// shadow_offset_x_() -> __shadow_offset_x_(...)
#define _GET_SHADOW_OFFSET_X(obj)       lv_obj_get_style_shadow_offset_x(obj, _dv_current_selector)
#define __shadow_offset_x_0_()          _GET_SHADOW_OFFSET_X(_dv_current_widget)
#define __shadow_offset_x_1_(obj)       _GET_SHADOW_OFFSET_X(obj)
#define __shadow_offset_x_(...)         _PASTE(__shadow_offset_x_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// shadow_offset_y_() -> __shadow_offset_y_(...)
#define _GET_SHADOW_OFFSET_Y(obj)       lv_obj_get_style_shadow_offset_y(obj, _dv_current_selector)
#define __shadow_offset_y_0_()          _GET_SHADOW_OFFSET_Y(_dv_current_widget)
#define __shadow_offset_y_1_(obj)       _GET_SHADOW_OFFSET_Y(obj)
#define __shadow_offset_y_(...)         _PASTE(__shadow_offset_y_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// shadow_spread_() -> __shadow_spread_(...)
#define _GET_SHADOW_SPREAD(obj)         lv_obj_get_style_shadow_spread(obj, _dv_current_selector)
#define __shadow_spread_0_()            _GET_SHADOW_SPREAD(_dv_current_widget)
#define __shadow_spread_1_(obj)         _GET_SHADOW_SPREAD(obj)
#define __shadow_spread_(...)           _PASTE(__shadow_spread_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// shadow_color_() -> __shadow_color_(...)
#define _GET_SHADOW_COLOR(obj)          lv_obj_get_style_shadow_color(obj, _dv_current_selector)
#define __shadow_color_0_()             _GET_SHADOW_COLOR(_dv_current_widget)
#define __shadow_color_1_(obj)          _GET_SHADOW_COLOR(obj)
#define __shadow_color_(...)            _PASTE(__shadow_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// shadow_opa_() -> __shadow_opa_(...)
#define _GET_SHADOW_OPA(obj)            lv_obj_get_style_shadow_opa(obj, _dv_current_selector)
#define __shadow_opa_0_()               _GET_SHADOW_OPA(_dv_current_widget)
#define __shadow_opa_1_(obj)            _GET_SHADOW_OPA(obj)
#define __shadow_opa_(...)              _PASTE(__shadow_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// image_opa_() -> __image_opa_(...)
#define _GET_IMAGE_OPA(obj)             lv_obj_get_style_image_opa(obj, _dv_current_selector)
#define __image_opa_0_()                _GET_IMAGE_OPA(_dv_current_widget)
#define __image_opa_1_(obj)             _GET_IMAGE_OPA(obj)
#define __image_opa_(...)               _PASTE(__image_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// image_recolor_() -> __image_recolor_(...)
#define _GET_IMAGE_RECOLOR(obj)         lv_obj_get_style_image_recolor(obj, _dv_current_selector)
#define __image_recolor_0_()            _GET_IMAGE_RECOLOR(_dv_current_widget)
#define __image_recolor_1_(obj)         _GET_IMAGE_RECOLOR(obj)
#define __image_recolor_(...)           _PASTE(__image_recolor_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// image_recolor_opa_() -> __image_recolor_opa_(...)
#define _GET_IMAGE_RECOLOR_OPA(obj)     lv_obj_get_style_image_recolor_opa(obj, _dv_current_selector)
#define __image_recolor_opa_0_()        _GET_IMAGE_RECOLOR_OPA(_dv_current_widget)
#define __image_recolor_opa_1_(obj)     _GET_IMAGE_RECOLOR_OPA(obj)
#define __image_recolor_opa_(...)       _PASTE(__image_recolor_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// line_width_() -> __line_width_(...)
#define _GET_LINE_WIDTH(obj)            lv_obj_get_style_line_width(obj, _dv_current_selector)
#define __line_width_0_()               _GET_LINE_WIDTH(_dv_current_widget)
#define __line_width_1_(obj)            _GET_LINE_WIDTH(obj)
#define __line_width_(...)              _PASTE(__line_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// line_dash_width_() -> __line_dash_width_(...)
#define _GET_LINE_DASH_WIDTH(obj)       lv_obj_get_style_line_dash_width(obj, _dv_current_selector)
#define __line_dash_width_0_()          _GET_LINE_DASH_WIDTH(_dv_current_widget)
#define __line_dash_width_1_(obj)       _GET_LINE_DASH_WIDTH(obj)
#define __line_dash_width_(...)         _PASTE(__line_dash_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// line_dash_gap_() -> __line_dash_gap_(...)
#define _GET_LINE_DASH_GAP(obj)         lv_obj_get_style_line_dash_gap(obj, _dv_current_selector)
#define __line_dash_gap_0_()            _GET_LINE_DASH_GAP(_dv_current_widget)
#define __line_dash_gap_1_(obj)         _GET_LINE_DASH_GAP(obj)
#define __line_dash_gap_(...)           _PASTE(__line_dash_gap_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// line_rounded_() -> __line_rounded_(...)
#define _GET_LINE_ROUNDED(obj)          lv_obj_get_style_line_rounded(obj, _dv_current_selector)
#define __line_rounded_0_()             _GET_LINE_ROUNDED(_dv_current_widget)
#define __line_rounded_1_(obj)          _GET_LINE_ROUNDED(obj)
#define __line_rounded_(...)            _PASTE(__line_rounded_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// line_color_() -> __line_color_(...)
#define _GET_LINE_COLOR(obj)            lv_obj_get_style_line_color(obj, _dv_current_selector)
#define __line_color_0_()               _GET_LINE_COLOR(_dv_current_widget)
#define __line_color_1_(obj)            _GET_LINE_COLOR(obj)
#define __line_color_(...)              _PASTE(__line_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// line_opa_() -> __line_opa_(...)
#define _GET_LINE_OPA(obj)              lv_obj_get_style_line_opa(obj, _dv_current_selector)
#define __line_opa_0_()                 _GET_LINE_OPA(_dv_current_widget)
#define __line_opa_1_(obj)              _GET_LINE_OPA(obj)
#define __line_opa_(...)                _PASTE(__line_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// arc_width_() -> __arc_width_(...)
#define _GET_ARC_WIDTH(obj)             lv_obj_get_style_arc_width(obj, _dv_current_selector)
#define __arc_width_0_()                _GET_ARC_WIDTH(_dv_current_widget)
#define __arc_width_1_(obj)             _GET_ARC_WIDTH(obj)
#define __arc_width_(...)               _PASTE(__arc_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// arc_rounded_() -> __arc_rounded_(...)
#define _GET_ARC_ROUNDED(obj)           lv_obj_get_style_arc_rounded(obj, _dv_current_selector)
#define __arc_rounded_0_()              _GET_ARC_ROUNDED(_dv_current_widget)
#define __arc_rounded_1_(obj)           _GET_ARC_ROUNDED(obj)
#define __arc_rounded_(...)             _PASTE(__arc_rounded_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// arc_color_() -> __arc_color_(...)
#define _GET_ARC_COLOR(obj)             lv_obj_get_style_arc_color(obj, _dv_current_selector)
#define __arc_color_0_()                _GET_ARC_COLOR(_dv_current_widget)
#define __arc_color_1_(obj)             _GET_ARC_COLOR(obj)
#define __arc_color_(...)               _PASTE(__arc_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// arc_opa_() -> __arc_opa_(...)
#define _GET_ARC_OPA(obj)               lv_obj_get_style_arc_opa(obj, _dv_current_selector)
#define __arc_opa_0_()                  _GET_ARC_OPA(_dv_current_widget)
#define __arc_opa_1_(obj)               _GET_ARC_OPA(obj)
#define __arc_opa_(...)                 _PASTE(__arc_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// arc_image_src_() -> __arc_image_src_(...)
#define _GET_ARC_IMAGE_SRC(obj)         lv_obj_get_style_arc_image_src(obj, _dv_current_selector)
#define __arc_image_src_0_()            _GET_ARC_IMAGE_SRC(_dv_current_widget)
#define __arc_image_src_1_(obj)         _GET_ARC_IMAGE_SRC(obj)
#define __arc_image_src_(...)           _PASTE(__arc_image_src_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_color_() -> __text_color_(...)
#define _GET_TEXT_COLOR(obj)            lv_obj_get_style_text_color(obj, _dv_current_selector)
#define __text_color_0_()               _GET_TEXT_COLOR(_dv_current_widget)
#define __text_color_1_(obj)            _GET_TEXT_COLOR(obj)
#define __text_color_(...)              _PASTE(__text_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_opa_() -> __text_opa_(...)
#define _GET_TEXT_OPA(obj)              lv_obj_get_style_text_opa(obj, _dv_current_selector)
#define __text_opa_0_()                 _GET_TEXT_OPA(_dv_current_widget)
#define __text_opa_1_(obj)              _GET_TEXT_OPA(obj)
#define __text_opa_(...)                _PASTE(__text_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_font_() -> __text_font_(...)
#define _GET_TEXT_FONT(obj)             lv_obj_get_style_text_font(obj, _dv_current_selector)
#define __text_font_0_()                _GET_TEXT_FONT(_dv_current_widget)
#define __text_font_1_(obj)             _GET_TEXT_FONT(obj)
#define __text_font_(...)               _PASTE(__text_font_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_letter_space_() -> __text_letter_space_(...)
#define _GET_TEXT_LETTER_SPACE(obj)     lv_obj_get_style_text_letter_space(obj, _dv_current_selector)
#define __text_letter_space_0_()        _GET_TEXT_LETTER_SPACE(_dv_current_widget)
#define __text_letter_space_1_(obj)     _GET_TEXT_LETTER_SPACE(obj)
#define __text_letter_space_(...)       _PASTE(__text_letter_space_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_line_space_() -> __text_line_space_(...)
#define _GET_TEXT_LINE_SPACE(obj)       lv_obj_get_style_text_line_space(obj, _dv_current_selector)
#define __text_line_space_0_()          _GET_TEXT_LINE_SPACE(_dv_current_widget)
#define __text_line_space_1_(obj)       _GET_TEXT_LINE_SPACE(obj)
#define __text_line_space_(...)         _PASTE(__text_line_space_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_decor_() -> __text_decor_(...)
#define _GET_TEXT_DECOR(obj)            lv_obj_get_style_text_decor(obj, _dv_current_selector)
#define __text_decor_0_()               _GET_TEXT_DECOR(_dv_current_widget)
#define __text_decor_1_(obj)            _GET_TEXT_DECOR(obj)
#define __text_decor_(...)              _PASTE(__text_decor_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_align_() -> __text_align_(...)
#define _GET_TEXT_ALIGN(obj)            lv_obj_get_style_text_align(obj, _dv_current_selector)
#define __text_align_0_()               _GET_TEXT_ALIGN(_dv_current_widget)
#define __text_align_1_(obj)            _GET_TEXT_ALIGN(obj)
#define __text_align_(...)              _PASTE(__text_align_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

#if LV_USE_FONT_SUBPX // Check LVGL config
// text_outline_stroke_color_() -> __text_outline_stroke_color_(...)
#define _GET_TEXT_OUTLINE_STROKE_COLOR(obj) lv_obj_get_style_text_outline_stroke_color(obj, _dv_current_selector)
#define __text_outline_stroke_color_0_() _GET_TEXT_OUTLINE_STROKE_COLOR(_dv_current_widget)
#define __text_outline_stroke_color_1_(obj) _GET_TEXT_OUTLINE_STROKE_COLOR(obj)
#define __text_outline_stroke_color_(...) _PASTE(__text_outline_stroke_color_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_outline_stroke_width_() -> __text_outline_stroke_width_(...)
#define _GET_TEXT_OUTLINE_STROKE_WIDTH(obj) lv_obj_get_style_text_outline_stroke_width(obj, _dv_current_selector)
#define __text_outline_stroke_width_0_() _GET_TEXT_OUTLINE_STROKE_WIDTH(_dv_current_widget)
#define __text_outline_stroke_width_1_(obj) _GET_TEXT_OUTLINE_STROKE_WIDTH(obj)
#define __text_outline_stroke_width_(...) _PASTE(__text_outline_stroke_width_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// text_outline_stroke_opa_() -> __text_outline_stroke_opa_(...)
#define _GET_TEXT_OUTLINE_STROKE_OPA(obj) lv_obj_get_style_text_outline_stroke_opa(obj, _dv_current_selector)
#define __text_outline_stroke_opa_0_()  _GET_TEXT_OUTLINE_STROKE_OPA(_dv_current_widget)
#define __text_outline_stroke_opa_1_(obj) _GET_TEXT_OUTLINE_STROKE_OPA(obj)
#define __text_outline_stroke_opa_(...) _PASTE(__text_outline_stroke_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)
#endif // LV_USE_FONT_SUBPX

// radius_() -> __radius_(...)
#define _GET_RADIUS(obj)                lv_obj_get_style_radius(obj, _dv_current_selector)
#define __radius_0_()                   _GET_RADIUS(_dv_current_widget)
#define __radius_1_(obj)                _GET_RADIUS(obj)
#define __radius_(...)                  _PASTE(__radius_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// radial_offset_() -> __radial_offset_(...)
#define _GET_RADIAL_OFFSET(obj)         lv_obj_get_style_radial_offset(obj, _dv_current_selector)
#define __radial_offset_0_()            _GET_RADIAL_OFFSET(_dv_current_widget)
#define __radial_offset_1_(obj)         _GET_RADIAL_OFFSET(obj)
#define __radial_offset_(...)           _PASTE(__radial_offset_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// clip_corner_() -> __clip_corner_(...)
#define _GET_CLIP_CORNER(obj)           lv_obj_get_style_clip_corner(obj, _dv_current_selector)
#define __clip_corner_0_()              _GET_CLIP_CORNER(_dv_current_widget)
#define __clip_corner_1_(obj)           _GET_CLIP_CORNER(obj)
#define __clip_corner_(...)             _PASTE(__clip_corner_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// opa_() -> __opa_(...)
#define _GET_OPA(obj)                   lv_obj_get_style_opa(obj, _dv_current_selector)
#define __opa_0_()                      _GET_OPA(_dv_current_widget)
#define __opa_1_(obj)                   _GET_OPA(obj)
#define __opa_(...)                     _PASTE(__opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// opa_layered_() -> __opa_layered_(...)
#define _GET_OPA_LAYERED(obj)           lv_obj_get_style_opa_layered(obj, _dv_current_selector)
#define __opa_layered_0_()              _GET_OPA_LAYERED(_dv_current_widget)
#define __opa_layered_1_(obj)           _GET_OPA_LAYERED(obj)
#define __opa_layered_(...)             _PASTE(__opa_layered_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// color_filter_dsc_() -> __color_filter_dsc_(...)
#define _GET_COLOR_FILTER_DSC(obj)      lv_obj_get_style_color_filter_dsc(obj, _dv_current_selector)
#define __color_filter_dsc_0_()         _GET_COLOR_FILTER_DSC(_dv_current_widget)
#define __color_filter_dsc_1_(obj)      _GET_COLOR_FILTER_DSC(obj)
#define __color_filter_dsc_(...)        _PASTE(__color_filter_dsc_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// color_filter_opa_() -> __color_filter_opa_(...)
#define _GET_COLOR_FILTER_OPA(obj)      lv_obj_get_style_color_filter_opa(obj, _dv_current_selector)
#define __color_filter_opa_0_()         _GET_COLOR_FILTER_OPA(_dv_current_widget)
#define __color_filter_opa_1_(obj)      _GET_COLOR_FILTER_OPA(obj)
#define __color_filter_opa_(...)        _PASTE(__color_filter_opa_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// anim_() -> __anim_(...)
#define _GET_ANIM(obj)                  lv_obj_get_style_anim(obj, _dv_current_selector)
#define __anim_0_()                     _GET_ANIM(_dv_current_widget)
#define __anim_1_(obj)                  _GET_ANIM(obj)
#define __anim_(...)                    _PASTE(__anim_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// anim_duration_() -> __anim_duration_(...)
#define _GET_ANIM_DURATION(obj)         lv_obj_get_style_anim_duration(obj, _dv_current_selector)
#define __anim_duration_0_()            _GET_ANIM_DURATION(_dv_current_widget)
#define __anim_duration_1_(obj)         _GET_ANIM_DURATION(obj)
#define __anim_duration_(...)           _PASTE(__anim_duration_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// transition_() -> __transition_(...)
#define _GET_TRANSITION(obj)            lv_obj_get_style_transition(obj, _dv_current_selector)
#define __transition_0_()               _GET_TRANSITION(_dv_current_widget)
#define __transition_1_(obj)            _GET_TRANSITION(obj)
#define __transition_(...)              _PASTE(__transition_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// blend_mode_() -> __blend_mode_(...)
#define _GET_BLEND_MODE(obj)            lv_obj_get_style_blend_mode(obj, _dv_current_selector)
#define __blend_mode_0_()               _GET_BLEND_MODE(_dv_current_widget)
#define __blend_mode_1_(obj)            _GET_BLEND_MODE(obj)
#define __blend_mode_(...)              _PASTE(__blend_mode_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// layout_() -> __layout_(...)
#define _GET_LAYOUT(obj)                lv_obj_get_style_layout(obj, _dv_current_selector)
#define __layout_0_()                   _GET_LAYOUT(_dv_current_widget)
#define __layout_1_(obj)                _GET_LAYOUT(obj)
#define __layout_(...)                  _PASTE(__layout_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// base_dir_() -> __base_dir_(...)
#define _GET_BASE_DIR(obj)              lv_obj_get_style_base_dir(obj, _dv_current_selector)
#define __base_dir_0_()                 _GET_BASE_DIR(_dv_current_widget)
#define __base_dir_1_(obj)              _GET_BASE_DIR(obj)
#define __base_dir_(...)                _PASTE(__base_dir_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// bitmap_mask_src_() -> __bitmap_mask_src_(...)
#define _GET_BITMAP_MASK_SRC(obj)       lv_obj_get_style_bitmap_mask_src(obj, _dv_current_selector)
#define __bitmap_mask_src_0_()          _GET_BITMAP_MASK_SRC(_dv_current_widget)
#define __bitmap_mask_src_1_(obj)       _GET_BITMAP_MASK_SRC(obj)
#define __bitmap_mask_src_(...)         _PASTE(__bitmap_mask_src_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// rotary_sensitivity_() -> __rotary_sensitivity_(...)
#define _GET_ROTARY_SENSITIVITY(obj)    lv_obj_get_style_rotary_sensitivity(obj, _dv_current_selector)
#define __rotary_sensitivity_0_()       _GET_ROTARY_SENSITIVITY(_dv_current_widget)
#define __rotary_sensitivity_1_(obj)    _GET_ROTARY_SENSITIVITY(obj)
#define __rotary_sensitivity_(...)      _PASTE(__rotary_sensitivity_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// flex_flow_() -> __flex_flow_(...)
#define _GET_FLEX_FLOW(obj)             lv_obj_get_style_flex_flow(obj, _dv_current_selector)
#define __flex_flow_0_()                _GET_FLEX_FLOW(_dv_current_widget)
#define __flex_flow_1_(obj)             _GET_FLEX_FLOW(obj)
#define __flex_flow_(...)               _PASTE(__flex_flow_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// flex_main_place_() -> __flex_main_place_(...)
#define _GET_FLEX_MAIN_PLACE(obj)       lv_obj_get_style_flex_main_place(obj, _dv_current_selector)
#define __flex_main_place_0_()          _GET_FLEX_MAIN_PLACE(_dv_current_widget)
#define __flex_main_place_1_(obj)       _GET_FLEX_MAIN_PLACE(obj)
#define __flex_main_place_(...)         _PASTE(__flex_main_place_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// flex_cross_place_() -> __flex_cross_place_(...)
#define _GET_FLEX_CROSS_PLACE(obj)      lv_obj_get_style_flex_cross_place(obj, _dv_current_selector)
#define __flex_cross_place_0_()         _GET_FLEX_CROSS_PLACE(_dv_current_widget)
#define __flex_cross_place_1_(obj)      _GET_FLEX_CROSS_PLACE(obj)
#define __flex_cross_place_(...)        _PASTE(__flex_cross_place_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// flex_track_place_() -> __flex_track_place_(...)
#define _GET_FLEX_TRACK_PLACE(obj)      lv_obj_get_style_flex_track_place(obj, _dv_current_selector)
#define __flex_track_place_0_()         _GET_FLEX_TRACK_PLACE(_dv_current_widget)
#define __flex_track_place_1_(obj)      _GET_FLEX_TRACK_PLACE(obj)
#define __flex_track_place_(...)        _PASTE(__flex_track_place_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// flex_grow_() -> __flex_grow_(...)
#define _GET_FLEX_GROW(obj)             lv_obj_get_style_flex_grow(obj, _dv_current_selector)
#define __flex_grow_0_()                _GET_FLEX_GROW(_dv_current_widget)
#define __flex_grow_1_(obj)             _GET_FLEX_GROW(obj)
#define __flex_grow_(...)               _PASTE(__flex_grow_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_column_dsc_array_() -> __grid_column_dsc_array_(...)
#define _GET_GRID_COLUMN_DSC_ARRAY(obj) lv_obj_get_style_grid_column_dsc_array(obj, _dv_current_selector)
#define __grid_column_dsc_array_0_()    _GET_GRID_COLUMN_DSC_ARRAY(_dv_current_widget)
#define __grid_column_dsc_array_1_(obj) _GET_GRID_COLUMN_DSC_ARRAY(obj)
#define __grid_column_dsc_array_(...)   _PASTE(__grid_column_dsc_array_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_column_align_() -> __grid_column_align_(...)
#define _GET_GRID_COLUMN_ALIGN(obj)     lv_obj_get_style_grid_column_align(obj, _dv_current_selector)
#define __grid_column_align_0_()        _GET_GRID_COLUMN_ALIGN(_dv_current_widget)
#define __grid_column_align_1_(obj)     _GET_GRID_COLUMN_ALIGN(obj)
#define __grid_column_align_(...)       _PASTE(__grid_column_align_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_row_dsc_array_() -> __grid_row_dsc_array_(...)
#define _GET_GRID_ROW_DSC_ARRAY(obj)    lv_obj_get_style_grid_row_dsc_array(obj, _dv_current_selector)
#define __grid_row_dsc_array_0_()       _GET_GRID_ROW_DSC_ARRAY(_dv_current_widget)
#define __grid_row_dsc_array_1_(obj)    _GET_GRID_ROW_DSC_ARRAY(obj)
#define __grid_row_dsc_array_(...)      _PASTE(__grid_row_dsc_array_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_row_align_() -> __grid_row_align_(...)
#define _GET_GRID_ROW_ALIGN(obj)        lv_obj_get_style_grid_row_align(obj, _dv_current_selector)
#define __grid_row_align_0_()           _GET_GRID_ROW_ALIGN(_dv_current_widget)
#define __grid_row_align_1_(obj)        _GET_GRID_ROW_ALIGN(obj)
#define __grid_row_align_(...)          _PASTE(__grid_row_align_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_cell_column_pos_() -> __grid_cell_column_pos_(...)
#define _GET_GRID_CELL_COLUMN_POS(obj)  lv_obj_get_style_grid_cell_column_pos(obj, _dv_current_selector)
#define __grid_cell_column_pos_0_()     _GET_GRID_CELL_COLUMN_POS(_dv_current_widget)
#define __grid_cell_column_pos_1_(obj)  _GET_GRID_CELL_COLUMN_POS(obj)
#define __grid_cell_column_pos_(...)    _PASTE(__grid_cell_column_pos_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_cell_x_align_() -> __grid_cell_x_align_(...)
#define _GET_GRID_CELL_X_ALIGN(obj)     lv_obj_get_style_grid_cell_x_align(obj, _dv_current_selector)
#define __grid_cell_x_align_0_()        _GET_GRID_CELL_X_ALIGN(_dv_current_widget)
#define __grid_cell_x_align_1_(obj)     _GET_GRID_CELL_X_ALIGN(obj)
#define __grid_cell_x_align_(...)       _PASTE(__grid_cell_x_align_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_cell_column_span_() -> __grid_cell_column_span_(...)
#define _GET_GRID_CELL_COLUMN_SPAN(obj) lv_obj_get_style_grid_cell_column_span(obj, _dv_current_selector)
#define __grid_cell_column_span_0_()    _GET_GRID_CELL_COLUMN_SPAN(_dv_current_widget)
#define __grid_cell_column_span_1_(obj) _GET_GRID_CELL_COLUMN_SPAN(obj)
#define __grid_cell_column_span_(...)   _PASTE(__grid_cell_column_span_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_cell_row_pos_() -> __grid_cell_row_pos_(...)
#define _GET_GRID_CELL_ROW_POS(obj)     lv_obj_get_style_grid_cell_row_pos(obj, _dv_current_selector)
#define __grid_cell_row_pos_0_()        _GET_GRID_CELL_ROW_POS(_dv_current_widget)
#define __grid_cell_row_pos_1_(obj)     _GET_GRID_CELL_ROW_POS(obj)
#define __grid_cell_row_pos_(...)       _PASTE(__grid_cell_row_pos_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_cell_y_align_() -> __grid_cell_y_align_(...)
#define _GET_GRID_CELL_Y_ALIGN(obj)     lv_obj_get_style_grid_cell_y_align(obj, _dv_current_selector)
#define __grid_cell_y_align_0_()        _GET_GRID_CELL_Y_ALIGN(_dv_current_widget)
#define __grid_cell_y_align_1_(obj)     _GET_GRID_CELL_Y_ALIGN(obj)
#define __grid_cell_y_align_(...)       _PASTE(__grid_cell_y_align_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_cell_row_span_() -> __grid_cell_row_span_(...)
#define _GET_GRID_CELL_ROW_SPAN(obj)    lv_obj_get_style_grid_cell_row_span(obj, _dv_current_selector)
#define __grid_cell_row_span_0_()       _GET_GRID_CELL_ROW_SPAN(_dv_current_widget)
#define __grid_cell_row_span_1_(obj)    _GET_GRID_CELL_ROW_SPAN(obj)
#define __grid_cell_row_span_(...)      _PASTE(__grid_cell_row_span_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_column_gap_() -> __grid_column_gap_(...)
#define _GET_GRID_COLUMN_GAP(obj)       lv_obj_get_style_grid_column_gap(obj, _dv_current_selector)
#define __grid_column_gap_0_()          _GET_GRID_COLUMN_GAP(_dv_current_widget)
#define __grid_column_gap_1_(obj)       _GET_GRID_COLUMN_GAP(obj)
#define __grid_column_gap_(...)         _PASTE(__grid_column_gap_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

// grid_row_gap_() -> __grid_row_gap_(...)
#define _GET_GRID_ROW_GAP(obj)          lv_obj_get_style_grid_row_gap(obj, _dv_current_selector)
#define __grid_row_gap_0_()             _GET_GRID_ROW_GAP(_dv_current_widget)
#define __grid_row_gap_1_(obj)          _GET_GRID_ROW_GAP(obj)
#define __grid_row_gap_(...)            _PASTE(__grid_row_gap_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)

#define __label_text_align_2_(al, w)    __label_text_align_3_(_dv_current_widget, al, w)
#define __label_text_align_3_(obj, al, w)  do { lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP); lv_obj_set_width(obj, w); lv_label_(obj, al, 0, 0); } while (0);
#define __label_text_align(...)         _PASTE(__label_text_align_, _PASTE(_nargs(__VA_ARGS__), _))(__VA_ARGS__)


lv_obj_t *_maximize_client_area(lv_obj_t *obj);
lv_obj_t *_fill_parent(lv_obj_t *obj);

#define _APPLY_MAX_CLIENT_AREA(__obj)   _maximize_client_area(__obj);
#define __max_client_area_()            _APPLY_MAX_CLIENT_AREA(_dv_current_widget)
#define __max_client_area_1(obj)        _APPLY_MAX_CLIENT_AREA(obj)
#define __max_client_area(...)          _PASTE(__max_client_area_, __VA_OPT__(1))(__VA_ARGS__)

#define _APPLY_EXPAND_CLIENT_AREA(__obj)   _fill_parent(__obj);
#define __expand_client_area_()         _APPLY_EXPAND_CLIENT_AREA(_dv_current_widget)
#define __expand_client_area_1(obj)     _APPLY_EXPAND_CLIENT_AREA(obj)
#define __expand_client_area(...)       _PASTE(__expand_client_area_, __VA_OPT__(1))(__VA_ARGS__)

#define __flag(obj, flag, enabled)                                              \
  ((enabled) ? lv_obj_add_flag(obj, flag) : lv_obj_clear_flag(obj, flag))
#define __hide(obj, hidden)                                                   \
  lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN) // Shortcut for hiding
#define __clickable(obj, enabled) __flag(obj, LV_OBJ_FLAG_CLICKABLE, enabled)
#define __scrollable(obj, enabled) __flag(obj, LV_OBJ_FLAG_SCROLLABLE, enabled)
#define __use_layout(obj, enabled)                                              \
  __flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT, !enabled)
//#define __layout(obj, layout) lv_obj_set_layout(obj, layout)
#define __update_layout(obj) lv_obj_update_layout(obj)



#define COMBINE1(X, Y, Z) X##Y##Z
#define TEMP_VAR(X, Y, Z) COMBINE1(X, Y, Z)

#define _bar_indicator(bar, name, bg_oa, bg_color, bg_grad_color, bg_grad_dir, \
                       bg_main_stop, radius)                                   \
  do {                                                                         \
    static lv_style_t TEMP_VAR(style_indic_, __func__, name);                  \
    lv_style_init(&TEMP_VAR(style_indic_, __func__, name));                    \
    lv_style_set_bg_opa(&TEMP_VAR(style_indic_, __func__, name),               \
                        LV_OPA_COVER);                                         \
    lv_style_set_bg_color(&TEMP_VAR(style_indic_, __func__, name),             \
                          lv_color_hex(0x00DD00));                             \
    lv_style_set_bg_grad_color(&TEMP_VAR(style_indic_, __func__, name),        \
                               lv_color_hex(0x0000DD));                        \
    lv_style_set_bg_grad_dir(&TEMP_VAR(style_indic_, __func__, name),          \
                             LV_GRAD_DIR_HOR);                                 \
    lv_style_set_bg_main_stop(&TEMP_VAR(style_indic_, __func__, name), 175);   \
    lv_style_set_radius(&TEMP_VAR(style_indic_, __func__, name), 3);           \
    lv_obj_add_style(msm->bar_feed, &TEMP_VAR(style_indic_, __func__, name),   \
                     LV_PART_INDICATOR);                                       \
  } while (0);

#define _style_gradient(obj, main_color, grad_color, grad_dir, main_stop,      \
                        border_width, border_color, shadow_w, shadow_color,    \
                        line_color, radius)                                    \
  do {                                                                         \
    if (!lv_color_eq(main_color, lv_color_hex(0x00000000)) &&                  \
        lv_color_eq(grad_color, lv_color_hex(0x00000000))) {                   \
      _bg_opa(obj, LV_OPA_COVER, _M);                                          \
      _bg_color(obj, main_color, _M);                                          \
      lv_obj_set_style_bg_grad_color(obj, grad_color, _M);                     \
      lv_obj_set_style_bg_grad_dir(obj, grad_dir, _M);                         \
      lv_obj_set_style_bg_main_stop(obj, main_stop, _M);                       \
    } else {                                                                   \
      _bg_opa(obj, LV_OPA_0, _M);                                              \
    }                                                                          \
    _border_width(obj, border_width, _M);                                      \
    _border_color(obj, border_color, _M);                                      \
    lv_obj_set_style_shadow_width(obj, shadow_w, _M);                          \
    lv_obj_set_style_shadow_color(obj, shadow_color, _M);                      \
    lv_obj_set_style_line_color(obj, line_color, _M);                          \
    _radius(obj, radius, _M);                                                  \
  } while (0);

#endif // VIEW_DEFINE_MACROS_H