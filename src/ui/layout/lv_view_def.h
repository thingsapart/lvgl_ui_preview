#include "meta/macro_helpers.h"

#define layout(...)
#define style(...)

#define component(name, cons, ...) \
    lv_obj_t *name; \
    _process_args(__VA_ARGS__)

#define container(name, ...) component(name, obj, __VA_ARGS__)

#define components(...) _process_args(__VA_ARGS__)

#define def_view(name, ...) \
INCLUDE(lvgl.h) \
\
struct _PASTE(name, _t) { \
    lv_obj_t *main; \
    _process_args(__VA_ARGS__); \
}; \
\
typedef struct _PASTE(name, _t) _PASTE(name, _t); \
_PASTE(name, _t) *_PASTE(name, _create)(lv_obj_t *parent);
