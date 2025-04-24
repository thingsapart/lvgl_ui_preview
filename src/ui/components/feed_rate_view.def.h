//#include "ui/layout/lv_view_impl.h"

def_view(
    feed_rate_view,

    components(
        style(self->main, __text_color(lv_color_white()), __width(lv_pct(50)), __height(lv_pct(50)) ),

        container(left_mid,
            container(header_cont,
                component(caption, label, style(caption, __text("SNEEED"), __max_client_area())),
            ),
            container(main_cont,
                component(identifier, label, style(identifier,__text("F"),__max_client_area(), __text_font(&lv_font_montserrat_24))),
                component(value, label, style(value, __text("7000"), __max_client_area(), __text_font(&lv_font_montserrat_24), __size(lv_pct(100), 40))),
                component(spacer_left, obj, style(spacer_left, __max_client_area(), __bg_opa(LV_OPA_0), __border_width(0))),
                container(bars, style(bars, __pad_left(10), __pad_right(10)),
                    component(feed_bar, bar, 
                        style(feed_bar, __min_height(20), lv_bar_set_range(feed_bar, 0, 10000); lv_bar_set_value(feed_bar, 7000, LV_ANIM_OFF);)
                    ),
                    component(feed_scale, scale, 
                        style(feed_scale, __min_height(10), lv_scale_set_mode(feed_scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM); lv_scale_set_major_tick_every(feed_scale, 2); lv_scale_set_range(feed_scale, 0, 10);)
                    ),

                    style(bars, __max_client_area())
                ),
            ),
        ),
        container(right,
            component(unit, label, style(unit, __text("MM/MIN"), __bg_color(lv_color_hex(0xff00ff)), __bg_opa(LV_OPA_100) ) ),
            component(override_lbl, label, style(override_lbl, __text("Override"))),
            component(override, label, style(override, __text("100%"))),
            component(load_lbl, label, style(load_lbl, __text("Load"))),
            component(load, label, style(load, __text("70%"))),
        ),
    ),

    layout(


        _layout_grid(self->main_cont, 
            _rows(LV_GRID_CONTENT, LV_GRID_FR(1)),
            _cols(LV_GRID_CONTENT, LV_GRID_CONTENT),

            _cell(self->identifier, 0, 0),
            _cell(self->value, 1, 0),
            //_cell(self->spacer_left, 0, 1),
            _cell(self->bars, 1, 1), //(_cell_opts(.row_span=1, .col_align=LV_GRID_ALIGN_START)),

            style(self->main_cont, __max_client_area(), __height(lv_pct(100)), __bg_opa(LV_OPA_0)),
            style(self->identifier, __bg_opa(LV_OPA_0)),
            style(self->value, __min_width(lv_pct(50))),
            style(self->bars, __bg_opa(LV_OPA_0), __min_width(20), __min_height(30)),
            style(self->spacer_left, __size(10, 10))
        ),

        _layout_v(self->bars, LV_FLEX_ALIGN_START,
            _sized(self->feed_bar, lv_pct(100), 30),
            _sized(self->feed_scale, lv_pct(100), 10)
        ),

        _layout_v(self->right, LV_FLEX_ALIGN_START,
            _content(self->unit),
            _content(self->override_lbl),
            _content(self->override),
            _content(self->load_lbl),
            _content(self->load),

            style(self->right, __size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)),
            style(self->unit, __width(LV_SIZE_CONTENT));
        ),

        _layout_v(self->left_mid, LV_FLEX_ALIGN_START,
            _content(self->header_cont),
            _flex(self->main_cont, 1),

            style(self->left_mid, __bg_opa(LV_OPA_0))
        ),

        _layout_grid(self->main,
            _rows(LV_GRID_FR(1), LV_GRID_CONTENT),
            _cols(LV_GRID_CONTENT),

            _cell(self->left_mid, 0, 0),
            _cell(self->right, 1, 0),
        ),

    ),
);