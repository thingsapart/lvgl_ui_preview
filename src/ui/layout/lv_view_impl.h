#include "meta/macro_helpers.h"

#define layout(...) _process_args(__VA_ARGS__);

#define style(comp, ...) \
    do { \
        lv_obj_t *_dv_current_widget = comp; \
        lv_style_selector_t _dv_current_selector = LV_PART_MAIN | LV_STATE_DEFAULT; \
        _process_args(__VA_ARGS__); \
    } while (0);

#define debug_outline(name) \
        lv_obj_set_style_outline_width(name, 1, 0); \
        lv_obj_set_style_outline_color(name, lv_color_hex(0xffffff), 0); \
        lv_obj_set_style_outline_opa(name, LV_OPA_60, 0);

#define __create(name, constr, ...) \
    lv_obj_t *name = _PASTE(_PASTE(lv_, constr), _create)(__curr_parent); \
    do { \
        lv_obj_t *__curr_parent = name; \
        debug_outline(name); \
        __max_client_area(name); \
        _process_args(__VA_ARGS__); \
    } while (0);

#define container(name, ...) \
    __create(name, obj, self->name = name;, __VA_ARGS__) \
    __size(name, lv_pct(100), lv_pct(100)); \

#define component(name, constr, ...) \
    __create(name, constr, __VA_ARGS__) \
    self->name = name;

#define components(...) _process_args(__VA_ARGS__)

#define def_view(name, ...) \
INCLUDE(name.h) \
INCLUDE(string.h) \
INCLUDE(stdlib.h) \
INCLUDE(ui/layout/lv_vfl.h) \
INCLUDE(ui/layout/lv_views.h) \
\
static const char *TAG = #name; \
\
static void _PASTE(name,__impl__start)() {} \
\
_PASTE(name, _t) *_PASTE(name, _create)(lv_obj_t *parent) { \
    size_t self_sz = sizeof(_PASTE(name, _t)); \
    _PASTE(name, _t) *self = malloc(self_sz); \
    memset(self, 0, self_sz); \
    lv_obj_t *__curr_parent = self->main = lv_obj_create(parent); \
    debug_outline(__curr_parent); \
    _process_args(__VA_ARGS__); \
    return self; \
} 