#ifndef EMUL_LVGL_CONFIG_H
#define EMUL_LVGL_CONFIG_H

// Define if you want logging within the emulation library
#define EMUL_LVGL_USE_LOG 1

#if EMUL_LVGL_USE_LOG
#include <stdio.h>
#define EMUL_LOG(...) printf(__VA_ARGS__)
#else
#define EMUL_LOG(...)
#endif

// Define max number of objects, properties, styles if not using dynamic allocation fully
// #define EMUL_MAX_OBJECTS 100
// #define EMUL_MAX_PROPERTIES 20
// #define EMUL_MAX_STYLES 30

#endif // EMUL_LVGL_CONFIG_H
