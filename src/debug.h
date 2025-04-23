#ifndef __DEBUG_H_
#define __DEBUG_H_

#define D_INFO 0
#define D_WARN 1
#define D_ERROR 2

#ifndef UI_DEBUG_LOG
#define UI_DEBUG_LOG D_INFO
#endif

#undef ESP_LOGE
#define LOGE(tag, fmt, ...) _df(2, "[%s] " fmt, tag __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGE LOGE

#undef ESP_LOGW
#define LOGW(tag, fmt, ...) _df(1, "[%s] " fmt, tag __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGW LOGW

#undef ESP_LOGI
#define LOGI(tag, fmt, ...) _df(0, "[%s] " fmt, tag __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGI LOGI

#undef ESP_LOGD
#define LOGD(tag, fmt, ...) _df(-1, "[%s] " fmt, tag __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGD LOGD

// Verbose.
#define LOGV(tag, fmt, ...) _df(-2, "[%s] " fmt, tag __VA_OPT__(, ) __VA_ARGS__)

// Temporary debug override, always show.
#define LOGT(tag, fmt, ...) _df(100, "[%s] " fmt, tag __VA_OPT__(, ) __VA_ARGS__)

#ifndef UI_DEBUG_LOG
#define _d(lvl, s)
#define _df(lvl, format, ...)
#else

#include <stdio.h>
#define _d(lvl, s)                                                             \
  do {                                                                         \
    if (lvl >= UI_DEBUG_LOG) {                                                 \
      printf("%s\n", s);                                                       \
      fflush(stdout);                                                          \
    }                                                                          \
  } while (false)
#define _df(lvl, format, ...)                                                  \
  do {                                                                         \
    if (lvl >= UI_DEBUG_LOG) {                                                 \
      printf(format, __VA_ARGS__);                                             \
      printf("\n");                                                            \
      fflush(stdout);                                                          \
    }                                                                          \
  } while (0)

#endif

#endif // __DEBUG_H_
