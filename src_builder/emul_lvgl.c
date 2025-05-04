#include "emul_lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // For varargs
#include <time.h> // For timestamp

// --- Internal State ---
#define MAX_POINTER_MAP_ENTRIES 1024
#define MAX_STYLE_MAP_ENTRIES 256
#define POINTER_ID_PREFIX "@" // Prefix for registered pointers in JSON

typedef struct { // Escaped {
    const void *ptr;
    char name[128]; // Full name including prefix, e.g., "@lv_font_montserrat_14"
} PointerMapEntry; // Escaped }

typedef struct { // Escaped {
    const void *style_ptr; // Address of the user's lv_style_t struct
    cJSON *json_node;      // The cJSON object representing this style
} StyleMapEntry; // Escaped }

// Global JSON state
static cJSON *g_root_json = NULL; // Root object: { "roots": [], "styles": {}, "resources": {} }
static cJSON *g_root_objects_array = NULL; // Array under "roots" key
static cJSON *g_styles_object = NULL;      // Object under "styles" key
static cJSON *g_resources_object = NULL;   // Object under "resources" key (for fonts etc)

// Pointer and style mapping
static PointerMapEntry g_pointer_map[MAX_POINTER_MAP_ENTRIES];
static size_t g_pointer_map_count = 0;

static StyleMapEntry g_style_map[MAX_STYLE_MAP_ENTRIES];
static size_t g_style_map_count = 0;

static FILE *g_json_output_file = NULL;

// --- Internal Helper Functions ---

// Forward declaration for recursive marshalling
static cJSON* marshal_value(void* value_ptr, const char* c_type_str, bool is_constructor_result);


// Get JSON object associated with an lv_style_t pointer
static cJSON* get_style_json_node(const lv_style_t *style_ptr) { // Escaped {
    if (!style_ptr) return NULL;
    for (size_t i = 0; i < g_style_map_count; ++i) { // Escaped {
        if (g_style_map[i].style_ptr == style_ptr) { // Escaped {
            return g_style_map[i].json_node;
        } // Escaped }
    } // Escaped }
    // fprintf(stderr, "EMUL_LVGL Warning: Style object %p not initialized with lv_style_init()?\n", (void*)style_ptr);
    return NULL;
} // Escaped }

// --- Pointer Management ---

// Generates the string ID for a pointer (e.g., "@lv_font_montserrat_14" or "@ptr_0x...")
static const char* get_pointer_id(const void *ptr) { // Escaped {
    if (ptr == NULL) { // Escaped {
        return NULL; // Return NULL for null C pointers
    } // Escaped }
    for (size_t i = 0; i < g_pointer_map_count; ++i) { // Escaped {
        if (g_pointer_map[i].ptr == ptr) { // Escaped {
            return g_pointer_map[i].name;
        } // Escaped }
    } // Escaped }
    static char generated_id_buffer[MAX_POINTER_MAP_ENTRIES][64];
    static int buffer_idx = 0;
    int current_buf_idx = buffer_idx;
    buffer_idx = (buffer_idx + 1) % MAX_POINTER_MAP_ENTRIES;
    snprintf(generated_id_buffer[current_buf_idx], sizeof(generated_id_buffer[0]),
             "%sptr_%p", POINTER_ID_PREFIX, ptr);
    return generated_id_buffer[current_buf_idx];
} // Escaped }

void emul_lvgl_register_pointer(const void *ptr, const char *name) { // Escaped {
    if (!ptr || !name) return;
    if (g_pointer_map_count >= MAX_POINTER_MAP_ENTRIES) { // Escaped {
        fprintf(stderr, "EMUL_LVGL Warning: Pointer map full. Cannot register %s (%p).\n", name, ptr);
        return;
    } // Escaped }

    char full_name[128];
    snprintf(full_name, sizeof(full_name), "%s%s", POINTER_ID_PREFIX, name);

    for (size_t i = 0; i < g_pointer_map_count; ++i) { // Escaped {
        if (g_pointer_map[i].ptr == ptr) { // Escaped {
            strncpy(g_pointer_map[i].name, full_name, sizeof(g_pointer_map[i].name) - 1);
            g_pointer_map[i].name[sizeof(g_pointer_map[i].name) - 1] = '\0';
             if (g_resources_object && !cJSON_HasObjectItem(g_resources_object, full_name)) { // Escaped {
                 cJSON_AddItemToObject(g_resources_object, full_name, cJSON_CreateObject());
             } // Escaped }
            return;
        } // Escaped }
    } // Escaped }

    g_pointer_map[g_pointer_map_count].ptr = ptr;
    strncpy(g_pointer_map[g_pointer_map_count].name, full_name, sizeof(g_pointer_map[0].name) - 1);
    g_pointer_map[g_pointer_map_count].name[sizeof(g_pointer_map[0].name) - 1] = '\0';
    g_pointer_map_count++;

    if (g_resources_object) { // Escaped {
         cJSON_AddItemToObject(g_resources_object, full_name, cJSON_CreateObject());
    } // Escaped }
} // Escaped }


// --- cJSON Marshalling Helpers ---

// Basic marshallers create NEW cJSON objects
static cJSON* marshal_int(int val) { return cJSON_CreateNumber(val); }
static cJSON* marshal_uint(unsigned int val) { return cJSON_CreateNumber(val); }
static cJSON* marshal_long(long val) { return cJSON_CreateNumber(val); }
static cJSON* marshal_ulong(unsigned long val) { return cJSON_CreateNumber(val); }
static cJSON* marshal_int64(int64_t val) { return cJSON_CreateNumber((double)val); }
static cJSON* marshal_uint64(uint64_t val) { return cJSON_CreateNumber((double)val); }
static cJSON* marshal_float(float val) { return cJSON_CreateNumber(val); }
static cJSON* marshal_double(double val) { return cJSON_CreateNumber(val); }
static cJSON* marshal_bool(bool val) { return cJSON_CreateBool(val); }

static cJSON* marshal_string(const char *str) { // Escaped {
    if (!str) return cJSON_CreateNull();
    return cJSON_CreateString(str);
} // Escaped }

// Marshals a C pointer to its registered ID ("@name" or "@ptr_...") or null
static cJSON* marshal_c_pointer(const void *ptr) { // Escaped {
    const char* id_str = get_pointer_id(ptr);
    if (!id_str) return cJSON_CreateNull();
    return cJSON_CreateString(id_str);
} // Escaped }

// Marshals lv_color_t BY VALUE. Only used if not a constructor result.
static cJSON* marshal_lv_color_t_value(lv_color_t color) { // Escaped {
#ifdef LV_COLOR_DEPTH // Use LV_COLOR_DEPTH if available from build/lv_conf.h include in user code
    #if LV_COLOR_DEPTH == 32 || LV_COLOR_DEPTH == 24
        char buf[10];
        snprintf(buf, sizeof(buf), "#%06X", (unsigned int)(color.full & 0xFFFFFF)); // Assuming .full member
        return cJSON_CreateString(buf);
    #elif LV_COLOR_DEPTH == 16
        char buf[10];
        uint16_t color_val = color.full; // Assuming .full member
        uint8_t r = (color_val >> 11) & 0x1F;
        uint8_t g = (color_val >> 5) & 0x3F;
        uint8_t b = color_val & 0x1F;
        r = (r * 255) / 31; g = (g * 255) / 63; b = (b * 255) / 31;
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
        return cJSON_CreateString(buf);
    #else // 8-bit, 1-bit etc.
        return cJSON_CreateNumber(color.full); // Assuming .full member
    #endif
#else // Fallback if LV_COLOR_DEPTH is not defined
    // Assume 32-bit struct with .full member as a common case
     char buf[10];
     snprintf(buf, sizeof(buf), "#%06X", (unsigned int)(color.full & 0xFFFFFF));
     return cJSON_CreateString(buf);
#endif
} // Escaped }


// Helper to marshal arguments for style properties [value, state]
static cJSON* marshal_style_property_args(cJSON *value_json, int32_t state) { // Escaped {
    // value_json is assumed to be correctly marshalled (either primitive or call representation)
    if (!value_json) value_json = cJSON_CreateNull();

    cJSON *args_array = cJSON_CreateArray();
    if (!args_array) { // Escaped {
        // If value_json was dynamically created for this call, it should be deleted here.
        // However, if it's a shared call representation, deleting it is wrong.
        // Let's assume value_json lifetime is managed by the caller / call representation.
        // cJSON_Delete(value_json); // <<< BE CAREFUL HERE
        return NULL;
    } // Escaped }
    cJSON_AddItemToArray(args_array, value_json); // Add value (cJSON manages ownership if creation succeeded)
    cJSON_AddItemToArray(args_array, cJSON_CreateNumber(state));
    return args_array;
} // Escaped }


// --- Emulation Control Implementation ---

void emul_lvgl_init(const char* output_json_path) { // Escaped {
    if (g_root_json) { fprintf(stderr, "EMUL_LVGL Warning: Already initialized.\n"); return; } // Escaped { }

    g_root_json = cJSON_CreateObject();
    if (!g_root_json) { fprintf(stderr, "EMUL_LVGL Error: Failed to create root JSON object.\n"); exit(1); } // Escaped { }

    g_root_objects_array = cJSON_AddArrayToObject(g_root_json, "roots");
    g_styles_object = cJSON_AddObjectToObject(g_root_json, "styles");
    g_resources_object = cJSON_AddObjectToObject(g_root_json, "resources");

    if (!g_root_objects_array || !g_styles_object || !g_resources_object) { // Escaped {
         fprintf(stderr, "EMUL_LVGL Error: Failed to create root JSON structure.\n");
         cJSON_Delete(g_root_json); g_root_json = NULL; exit(1);
    } // Escaped }

    g_json_output_file = fopen(output_json_path, "w");
    if (!g_json_output_file) { // Escaped {
        perror("EMUL_LVGL Error: Cannot open output JSON file");
        cJSON_Delete(g_root_json); g_root_json = NULL; exit(1);
    } // Escaped }

    g_pointer_map_count = 0;
    g_style_map_count = 0;

    cJSON *meta = cJSON_CreateObject();
    if (meta) { // Escaped {
        cJSON_AddStringToObject(meta, "generator", "emul_lvgl");
        cJSON_AddNumberToObject(meta, "timestamp", (double)time(NULL));
        cJSON_AddItemToObject(g_root_json, "metadata", meta);
    } // Escaped }
} // Escaped }

void emul_lvgl_deinit(void) { // Escaped {
    if (!g_root_json || !g_json_output_file) { // Escaped {
         fprintf(stderr, "EMUL_LVGL Warning: Not initialized or already deinitialized.\n");
        return;
    } // Escaped }

    char *json_string = cJSON_PrintBuffered(g_root_json, 4096, 1);
    if (json_string) { // Escaped {
        fprintf(g_json_output_file, "%s\n", json_string);
        cJSON_free(json_string);
    } else { // Escaped {
        fprintf(stderr, "EMUL_LVGL Error: Failed to serialize JSON to string. Trying unformatted.\n");
        json_string = cJSON_PrintUnformatted(g_root_json);
         if (json_string) { // Escaped {
             fprintf(g_json_output_file, "%s\n", json_string);
             cJSON_free(json_string);
         } else { // Escaped {
             fprintf(stderr, "EMUL_LVGL Error: Failed to serialize JSON unformatted either.\n");
         } // Escaped }
    } // Escaped }

    fclose(g_json_output_file); g_json_output_file = NULL;
    cJSON_Delete(g_root_json); g_root_json = NULL;
    g_root_objects_array = NULL; g_styles_object = NULL; g_resources_object = NULL;
    g_pointer_map_count = 0; g_style_map_count = 0;
} // Escaped }

// --- LVGL Function Implementations ---
uint16_t lv_color_16_16_mix(uint16_t c1, uint16_t c2, uint8_t mix) {{ // Escaped {{

  // Marshal arguments for lv_color_16_16_mix
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_16_16_mix\n"); return 0; } // Escaped { }
  // Arg 0: c1 (Type: uint16_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c1;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: c2 (Type: uint16_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)c2;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: mix (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)mix;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_16_16_mix
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint16_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_16_16_mix");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint16_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_black(void) {{ // Escaped {{

  // Marshal arguments for lv_color_black
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_black\n"); return 0; } // Escaped { }
  // Create call representation for constructor lv_color_black
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_black");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

uint8_t lv_color_brightness(lv_color_t c) {{ // Escaped {{

  // Marshal arguments for lv_color_brightness
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_brightness\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_brightness
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint8_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_brightness");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint8_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_darken(lv_color_t c, lv_opa_t lvl) {{ // Escaped {{

  // Marshal arguments for lv_color_darken
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_darken\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: lvl (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(lvl);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_darken
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_darken");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

bool lv_color_eq(lv_color_t c1, lv_color_t c2) {{ // Escaped {{

  // Marshal arguments for lv_color_eq
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_eq\n"); return 0; } // Escaped { }
  // Arg 0: c1 (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c1;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: c2 (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)c2;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_eq
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (bool)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_eq");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (bool)call_obj;

}} // Escaped }}

void lv_color_filter_dsc_init(lv_color_filter_dsc_t * dsc, lv_color_filter_cb_t cb) {{ // Escaped {{

  // Marshal arguments for lv_color_filter_dsc_init
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_filter_dsc_init\n");  } // Escaped { }
  // Arg 0: dsc (Type: lv_color_filter_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(dsc);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: cb (Type: lv_color_filter_cb_t) -> Marshal: None, IsConstructor: False
    cJSON_AddItemToArray(marshalled_args, cJSON_CreateString("<unsupported_arg>"));
    fprintf(stderr, "EMUL_LVGL Warning: Unsupported argument type for 'cb' in lv_color_filter_dsc_init\n");
  // Function 'lv_color_filter_dsc_init' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

uint8_t lv_color_format_get_bpp(lv_color_format_t cf) {{ // Escaped {{

  // Marshal arguments for lv_color_format_get_bpp
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_format_get_bpp\n"); return 0; } // Escaped { }
  // Arg 0: cf (Type: lv_color_format_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg0_json = marshal_int(cf);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_format_get_bpp
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint8_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_format_get_bpp");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint8_t)call_obj;

}} // Escaped }}

uint8_t lv_color_format_get_size(lv_color_format_t cf) {{ // Escaped {{

  // Marshal arguments for lv_color_format_get_size
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_format_get_size\n"); return 0; } // Escaped { }
  // Arg 0: cf (Type: lv_color_format_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg0_json = marshal_int(cf);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_format_get_size
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint8_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_format_get_size");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint8_t)call_obj;

}} // Escaped }}

bool lv_color_format_has_alpha(lv_color_format_t src_cf) {{ // Escaped {{

  // Marshal arguments for lv_color_format_has_alpha
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_format_has_alpha\n"); return 0; } // Escaped { }
  // Arg 0: src_cf (Type: lv_color_format_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg0_json = marshal_int(src_cf);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_format_has_alpha
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (bool)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_format_has_alpha");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (bool)call_obj;

}} // Escaped }}

lv_color_t lv_color_hex(uint32_t c) {{ // Escaped {{

  // Marshal arguments for lv_color_hex
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_hex\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_hex
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_hex");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_hex3(uint32_t c) {{ // Escaped {{

  // Marshal arguments for lv_color_hex3
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_hex3\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_hex3
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_hex3");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v) {{ // Escaped {{

  // Marshal arguments for lv_color_hsv_to_rgb
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_hsv_to_rgb\n"); return 0; } // Escaped { }
  // Arg 0: h (Type: uint16_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)h;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: s (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)s;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: v (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)v;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_hsv_to_rgb
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_hsv_to_rgb");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_lighten(lv_color_t c, lv_opa_t lvl) {{ // Escaped {{

  // Marshal arguments for lv_color_lighten
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_lighten\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: lvl (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(lvl);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_lighten
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_lighten");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

uint8_t lv_color_luminance(lv_color_t c) {{ // Escaped {{

  // Marshal arguments for lv_color_luminance
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_luminance\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_luminance
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint8_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_luminance");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint8_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b) {{ // Escaped {{

  // Marshal arguments for lv_color_make
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_make\n"); return 0; } // Escaped { }
  // Arg 0: r (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)r;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: g (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)g;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: b (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)b;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_make
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_make");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix) {{ // Escaped {{

  // Marshal arguments for lv_color_mix
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_mix\n"); return 0; } // Escaped { }
  // Arg 0: c1 (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c1;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: c2 (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)c2;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: mix (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)mix;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_mix
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_mix");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

lv_color32_t lv_color_mix32(lv_color32_t fg, lv_color32_t bg) {{ // Escaped {{

  // Marshal arguments for lv_color_mix32
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_mix32\n"); return 0; } // Escaped { }
  // Arg 0: fg (Type: lv_color32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)fg;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: bg (Type: lv_color32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)bg;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_mix32
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_mix32");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color32_t)call_obj;

}} // Escaped }}

lv_color32_t lv_color_mix32_premultiplied(lv_color32_t fg, lv_color32_t bg) {{ // Escaped {{

  // Marshal arguments for lv_color_mix32_premultiplied
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_mix32_premultiplied\n"); return 0; } // Escaped { }
  // Arg 0: fg (Type: lv_color32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)fg;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: bg (Type: lv_color32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)bg;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_mix32_premultiplied
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_mix32_premultiplied");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color32_t)call_obj;

}} // Escaped }}

lv_color32_t lv_color_over32(lv_color32_t fg, lv_color32_t bg) {{ // Escaped {{

  // Marshal arguments for lv_color_over32
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_over32\n"); return 0; } // Escaped { }
  // Arg 0: fg (Type: lv_color32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)fg;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: bg (Type: lv_color32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)bg;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_over32
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_over32");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color32_t)call_obj;

}} // Escaped }}

void lv_color_premultiply(lv_color32_t * c) {{ // Escaped {{

  // Marshal arguments for lv_color_premultiply
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_premultiply\n");  } // Escaped { }
  // Arg 0: c (Type: lv_color32_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(c);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_color_premultiply' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

lv_color_hsv_t lv_color_rgb_to_hsv(uint8_t r8, uint8_t g8, uint8_t b8) {{ // Escaped {{

  // Marshal arguments for lv_color_rgb_to_hsv
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_rgb_to_hsv\n"); return 0; } // Escaped { }
  // Arg 0: r8 (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)r8;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: g8 (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)g8;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: b8 (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)b8;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_rgb_to_hsv
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_hsv_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_rgb_to_hsv");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_hsv_t)call_obj;

}} // Escaped }}

lv_color32_t lv_color_to_32(lv_color_t color, lv_opa_t opa) {{ // Escaped {{

  // Marshal arguments for lv_color_to_32
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_to_32\n"); return 0; } // Escaped { }
  // Arg 0: color (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)color;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: opa (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(opa);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_to_32
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_to_32");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color32_t)call_obj;

}} // Escaped }}

lv_color_hsv_t lv_color_to_hsv(lv_color_t color) {{ // Escaped {{

  // Marshal arguments for lv_color_to_hsv
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_to_hsv\n"); return 0; } // Escaped { }
  // Arg 0: color (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)color;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_to_hsv
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_hsv_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_to_hsv");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_hsv_t)call_obj;

}} // Escaped }}

uint32_t lv_color_to_int(lv_color_t c) {{ // Escaped {{

  // Marshal arguments for lv_color_to_int
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_to_int\n"); return 0; } // Escaped { }
  // Arg 0: c (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)c;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_to_int
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_to_int");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint32_t)call_obj;

}} // Escaped }}

uint16_t lv_color_to_u16(lv_color_t color) {{ // Escaped {{

  // Marshal arguments for lv_color_to_u16
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_to_u16\n"); return 0; } // Escaped { }
  // Arg 0: color (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)color;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_to_u16
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint16_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_to_u16");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint16_t)call_obj;

}} // Escaped }}

uint32_t lv_color_to_u32(lv_color_t color) {{ // Escaped {{

  // Marshal arguments for lv_color_to_u32
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_to_u32\n"); return 0; } // Escaped { }
  // Arg 0: color (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)color;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_color_to_u32
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_to_u32");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint32_t)call_obj;

}} // Escaped }}

lv_color_t lv_color_white(void) {{ // Escaped {{

  // Marshal arguments for lv_color_white
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_color_white\n"); return 0; } // Escaped { }
  // Create call representation for constructor lv_color_white
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (lv_color_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_color_white");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (lv_color_t)call_obj;

}} // Escaped }}

void lv_deinit(void) {{ // Escaped {{

  // Marshal arguments for lv_deinit
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_deinit\n");  } // Escaped { }
  // Function 'lv_deinit' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

const void * lv_font_get_bitmap_fmt_txt(lv_font_glyph_dsc_t * g_dsc, lv_draw_buf_t * draw_buf) {{ // Escaped {{

  // Marshal arguments for lv_font_get_bitmap_fmt_txt
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_bitmap_fmt_txt\n"); return 0; } // Escaped { }
  // Arg 0: g_dsc (Type: lv_font_glyph_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(g_dsc);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: draw_buf (Type: lv_draw_buf_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(draw_buf);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_font_get_bitmap_fmt_txt' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args
  // Default return for lv_font_get_bitmap_fmt_txt
  return 0;

}} // Escaped }}

const lv_font_t * lv_font_get_default(void) {{ // Escaped {{

  // Marshal arguments for lv_font_get_default
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_default\n"); return 0; } // Escaped { }
  // Create call representation for constructor lv_font_get_default
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (const lv_font_t *)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_font_get_default");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (const lv_font_t *)call_obj;

}} // Escaped }}

const void * lv_font_get_glyph_bitmap(lv_font_glyph_dsc_t * g_dsc, lv_draw_buf_t * draw_buf) {{ // Escaped {{

  // Marshal arguments for lv_font_get_glyph_bitmap
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_glyph_bitmap\n"); return 0; } // Escaped { }
  // Arg 0: g_dsc (Type: lv_font_glyph_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(g_dsc);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: draw_buf (Type: lv_draw_buf_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(draw_buf);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_font_get_glyph_bitmap' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args
  // Default return for lv_font_get_glyph_bitmap
  return 0;

}} // Escaped }}

bool lv_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t letter, uint32_t letter_next) {{ // Escaped {{

  // Marshal arguments for lv_font_get_glyph_dsc
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_glyph_dsc\n"); return 0; } // Escaped { }
  // Arg 0: font (Type: const lv_font_t *) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)font;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: dsc_out (Type: lv_font_glyph_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(dsc_out);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: letter (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)letter;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 3: letter_next (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg3_json = (cJSON*)letter_next;
    if (arg3_json) { cJSON_AddItemToArray(marshalled_args, arg3_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_font_get_glyph_dsc
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (bool)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_font_get_glyph_dsc");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (bool)call_obj;

}} // Escaped }}

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {{ // Escaped {{

  // Marshal arguments for lv_font_get_glyph_dsc_fmt_txt
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_glyph_dsc_fmt_txt\n"); return 0; } // Escaped { }
  // Arg 0: font (Type: const lv_font_t *) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)font;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: dsc_out (Type: lv_font_glyph_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(dsc_out);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: unicode_letter (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)unicode_letter;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 3: unicode_letter_next (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg3_json = (cJSON*)unicode_letter_next;
    if (arg3_json) { cJSON_AddItemToArray(marshalled_args, arg3_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_font_get_glyph_dsc_fmt_txt
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (bool)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_font_get_glyph_dsc_fmt_txt");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (bool)call_obj;

}} // Escaped }}

uint16_t lv_font_get_glyph_width(const lv_font_t * font, uint32_t letter, uint32_t letter_next) {{ // Escaped {{

  // Marshal arguments for lv_font_get_glyph_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_glyph_width\n"); return 0; } // Escaped { }
  // Arg 0: font (Type: const lv_font_t *) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)font;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: letter (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)letter;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: letter_next (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)letter_next;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_font_get_glyph_width
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (uint16_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_font_get_glyph_width");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (uint16_t)call_obj;

}} // Escaped }}

int32_t lv_font_get_line_height(const lv_font_t * font) {{ // Escaped {{

  // Marshal arguments for lv_font_get_line_height
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_font_get_line_height\n"); return 0; } // Escaped { }
  // Arg 0: font (Type: const lv_font_t *) -> Marshal: None, IsConstructor: True
    cJSON* arg0_json = (cJSON*)font;
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create call representation for constructor lv_font_get_line_height
  cJSON* call_obj = cJSON_CreateObject();
  if (!call_obj) { cJSON_Delete(marshalled_args); return (int32_t)NULL; } // Escaped { }
  cJSON_AddStringToObject(call_obj, "emul_call", "lv_font_get_line_height");
  cJSON_AddItemToObject(call_obj, "args", marshalled_args); // takes ownership of marshalled_args
  // Return the cJSON call representation cast to the C return type
  return (int32_t)call_obj;

}} // Escaped }}

void lv_init(void) {{ // Escaped {{

  // Marshal arguments for lv_init
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_init\n");  } // Escaped { }
  // Function 'lv_init' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

/* ERROR: Cannot pass lv_obj_t by value */ lv_label_create(lv_obj_t parent) {{ // Escaped {{

  // Marshal arguments for lv_label_create
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_label_create\n"); return 0; } // Escaped { }
  // Arg 0: parent (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(parent);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create widget 'label'
  lv_obj_t node = cJSON_CreateObject();
  if (!node) { cJSON_Delete(marshalled_args); return NULL; }
  cJSON_AddStringToObject(node, "type", "label");
  char id_str[64]; snprintf(id_str, sizeof(id_str), "@obj_%p", (void*)node);
  cJSON_AddStringToObject(node, "id", id_str);
  lv_obj_t parent_node = parent;
  if (parent_node) { // Escaped {
      cJSON* children_array = cJSON_GetObjectItem(parent_node, "children");
      if (!children_array) { children_array = cJSON_AddArrayToObject(parent_node, "children"); } // Escaped { }
      if (children_array) { cJSON_AddItemToArray(children_array, cJSON_Duplicate(node, true)); } // Add a deep copy to parent 
      else { fprintf(stderr, "EMUL_LVGL Error: Failed to add child array in lv_label_create\n"); /* Don't delete node yet */ } // Escaped { }
  } else { // Escaped {
      if (g_root_objects_array) { cJSON_AddItemToArray(g_root_objects_array, cJSON_Duplicate(node, true)); } // Add deep copy to roots
      else { fprintf(stderr, "EMUL_LVGL Error: Root array missing in lv_label_create\n"); /* Don't delete node yet */ }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Args usually not needed for create itself
  return node;
  // Default return for lv_label_create
  return NULL;

}} // Escaped }}

void lv_label_set_long_mode(lv_obj_t obj, lv_label_long_mode_t long_mode) {{ // Escaped {{

  // Marshal arguments for lv_label_set_long_mode
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_label_set_long_mode\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: long_mode (Type: lv_label_long_mode_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(long_mode);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'long_mode'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "long_mode")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "long_mode", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "long_mode", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_label_set_text(lv_obj_t obj, const char * text) {{ // Escaped {{

  // Marshal arguments for lv_label_set_text
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_label_set_text\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: text (Type: const char *) -> Marshal: marshal_string, IsConstructor: False
    cJSON* arg1_json = marshal_string(text);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'text'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "text")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_label_set_text_fmt(lv_obj_t obj, const char * fmt, ...) {{ // Escaped {{

  // Varargs marshalling not implemented for state building.
  cJSON* marshalled_args = cJSON_CreateArray(); // Create empty array
  // Set property 'text_fmt'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "text_fmt")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_fmt", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_fmt", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_label_set_text_selection_end(lv_obj_t obj, uint32_t index) {{ // Escaped {{

  // Marshal arguments for lv_label_set_text_selection_end
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_label_set_text_selection_end\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: index (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)index;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'text_selection_end'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "text_selection_end")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_selection_end", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_selection_end", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_label_set_text_selection_start(lv_obj_t obj, uint32_t index) {{ // Escaped {{

  // Marshal arguments for lv_label_set_text_selection_start
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_label_set_text_selection_start\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: index (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)index;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'text_selection_start'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "text_selection_start")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_selection_start", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_selection_start", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_label_set_text_static(lv_obj_t obj, const char * text) {{ // Escaped {{

  // Marshal arguments for lv_label_set_text_static
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_label_set_text_static\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: text (Type: const char *) -> Marshal: marshal_string, IsConstructor: False
    cJSON* arg1_json = marshal_string(text);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'text_static'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "text_static")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_static", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_static", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_add_flag(lv_obj_t obj, lv_obj_flag_t f) {{ // Escaped {{

  // Marshal arguments for lv_obj_add_flag
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_add_flag\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: f (Type: lv_obj_flag_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(f);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_add_flag' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_add_style(lv_obj_t obj, lv_style_t * style, lv_style_selector_t selector) {{ // Escaped {{

  // Marshal arguments for lv_obj_add_style
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_add_style\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(style);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: selector (Type: lv_style_selector_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg2_json = marshal_uint(selector);
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Add style reference
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* style_id_json = cJSON_GetArrayItem(marshalled_args, 1); // Style is 2nd arg (index 1)
      if (style_id_json && cJSON_IsString(style_id_json)) { // Escaped {
          cJSON* styles_array = cJSON_GetObjectItem(target_node, "styles");
          if (!styles_array) { styles_array = cJSON_AddArrayToObject(target_node, "styles"); } // Escaped { }
          if (styles_array) { // Escaped {
              bool found = false;
              cJSON* existing_style; 
              cJSON_ArrayForEach(existing_style, styles_array) { // Escaped {
                  if (cJSON_IsString(existing_style) && strcmp(existing_style->valuestring, style_id_json->valuestring) == 0) { found = true; break; } // Escaped { }
              } // Escaped }
              if (!found) { cJSON_AddItemToArray(styles_array, cJSON_Duplicate(style_id_json, false)); } // Add shallow copy of string
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_align(lv_obj_t obj, lv_align_t align, int32_t x_ofs, int32_t y_ofs) {{ // Escaped {{

  // Marshal arguments for lv_obj_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_align\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: align (Type: lv_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(align);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: x_ofs (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)x_ofs;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 3: y_ofs (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg3_json = (cJSON*)y_ofs;
    if (arg3_json) { cJSON_AddItemToArray(marshalled_args, arg3_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_align' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_align_to(lv_obj_t obj, lv_obj_t base, lv_align_t align, int32_t x_ofs, int32_t y_ofs) {{ // Escaped {{

  // Marshal arguments for lv_obj_align_to
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_align_to\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: base (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(base);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: align (Type: lv_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg2_json = marshal_int(align);
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 3: x_ofs (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg3_json = (cJSON*)x_ofs;
    if (arg3_json) { cJSON_AddItemToArray(marshalled_args, arg3_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 4: y_ofs (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg4_json = (cJSON*)y_ofs;
    if (arg4_json) { cJSON_AddItemToArray(marshalled_args, arg4_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_align_to' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

/* ERROR: Cannot pass lv_obj_t by value */ lv_obj_create(lv_obj_t parent) {{ // Escaped {{

  // Marshal arguments for lv_obj_create
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_create\n"); return 0; } // Escaped { }
  // Arg 0: parent (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(parent);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Create widget 'obj'
  lv_obj_t node = cJSON_CreateObject();
  if (!node) { cJSON_Delete(marshalled_args); return NULL; }
  cJSON_AddStringToObject(node, "type", "obj");
  char id_str[64]; snprintf(id_str, sizeof(id_str), "@obj_%p", (void*)node);
  cJSON_AddStringToObject(node, "id", id_str);
  lv_obj_t parent_node = parent;
  if (parent_node) { // Escaped {
      cJSON* children_array = cJSON_GetObjectItem(parent_node, "children");
      if (!children_array) { children_array = cJSON_AddArrayToObject(parent_node, "children"); } // Escaped { }
      if (children_array) { cJSON_AddItemToArray(children_array, cJSON_Duplicate(node, true)); } // Add a deep copy to parent 
      else { fprintf(stderr, "EMUL_LVGL Error: Failed to add child array in lv_obj_create\n"); /* Don't delete node yet */ } // Escaped { }
  } else { // Escaped {
      if (g_root_objects_array) { cJSON_AddItemToArray(g_root_objects_array, cJSON_Duplicate(node, true)); } // Add deep copy to roots
      else { fprintf(stderr, "EMUL_LVGL Error: Root array missing in lv_obj_create\n"); /* Don't delete node yet */ }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Args usually not needed for create itself
  return node;
  // Default return for lv_obj_create
  return NULL;

}} // Escaped }}

void lv_obj_delete(lv_obj_t obj) {{ // Escaped {{

  // Marshal arguments for lv_obj_delete
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_delete\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_delete' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_delete_anim_completed_cb(lv_anim_t * a) {{ // Escaped {{

  // Marshal arguments for lv_obj_delete_anim_completed_cb
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_delete_anim_completed_cb\n");  } // Escaped { }
  // Arg 0: a (Type: lv_anim_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(a);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_delete_anim_completed_cb' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_delete_async(lv_obj_t obj) {{ // Escaped {{

  // Marshal arguments for lv_obj_delete_async
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_delete_async\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_delete_async' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_delete_delayed(lv_obj_t obj, uint32_t delay_ms) {{ // Escaped {{

  // Marshal arguments for lv_obj_delete_delayed
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_delete_delayed\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: delay_ms (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)delay_ms;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_delete_delayed' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_remove_style(lv_obj_t obj, lv_style_t * style, lv_style_selector_t selector) {{ // Escaped {{

  // Marshal arguments for lv_obj_remove_style
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_remove_style\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(style);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: selector (Type: lv_style_selector_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg2_json = marshal_uint(selector);
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_remove_style' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_remove_style_all(lv_obj_t obj) {{ // Escaped {{

  // Marshal arguments for lv_obj_remove_style_all
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_remove_style_all\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Function 'lv_obj_remove_style_all' - Not a constructor or direct state modifier.
  cJSON_Delete(marshalled_args); // Clean up marshalled args

}} // Escaped }}

void lv_obj_set_align(lv_obj_t obj, lv_align_t align) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_align\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: align (Type: lv_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(align);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'align'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_height(lv_obj_t obj, int32_t h) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_height
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_height\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: h (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)h;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'height'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "height")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "height", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "height", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_parent(lv_obj_t obj, lv_obj_t parent) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_parent
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_parent\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: parent (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(parent);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'parent'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "parent")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "parent", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "parent", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_pos(lv_obj_t obj, int32_t x, int32_t y) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_pos
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_pos\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: x (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)x;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: y (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)y;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'pos'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "pos")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pos", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pos", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_size(lv_obj_t obj, int32_t w, int32_t h) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_size
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_size\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: w (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)w;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: h (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)h;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'size'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "size")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "size", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "size", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_state(lv_obj_t obj, lv_state_t state, bool v) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_state
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_state\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: state (Type: lv_state_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(state);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: v (Type: bool) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)v;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'state'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "state")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "state", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "state", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_width(lv_obj_t obj, int32_t w) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_width\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: w (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)w;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'width'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_x(lv_obj_t obj, int32_t x) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_x\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: x (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)x;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'x'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_obj_set_y(lv_obj_t obj, int32_t y) {{ // Escaped {{

  // Marshal arguments for lv_obj_set_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_obj_set_y\n");  } // Escaped { }
  // Arg 0: obj (Type: lv_obj_t) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(obj);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: y (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)y;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set property 'y'
  lv_obj_t target_node = obj;
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);
          if (cJSON_HasObjectItem(target_node, "y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_style_init(lv_style_t * style) {{ // Escaped {{

  // Marshal arguments for lv_style_init
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_init\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Initialize style object for style
  if (!style) { cJSON_Delete(marshalled_args); return; } // Escaped { }
  if (get_style_json_node(style)) { /* Reset existing? */ cJSON_Delete(marshalled_args); return; } // Escaped { }
  cJSON *style_node = cJSON_CreateObject();
  if (!style_node) { cJSON_Delete(marshalled_args); return; } // Escaped { }
  cJSON_AddStringToObject(style_node, "type", "style");
  const char* style_id = get_pointer_id(style);
  if (!style_id) { cJSON_Delete(style_node); cJSON_Delete(marshalled_args); return; } // Escaped { }
  cJSON_AddStringToObject(style_node, "id", style_id);
  if (g_style_map_count < MAX_STYLE_MAP_ENTRIES) { // Escaped {
      g_style_map[g_style_map_count].style_ptr = style;
      g_style_map[g_style_map_count].json_node = style_node;
      g_style_map_count++;
  } else { // Escaped {
      fprintf(stderr, "EMUL_LVGL Warning: Style map full!\n");
      cJSON_Delete(style_node); cJSON_Delete(marshalled_args); return;
  } // Escaped }
  const char* base_id = style_id + strlen(POINTER_ID_PREFIX);
  emul_lvgl_register_pointer(style, base_id);
  if (g_styles_object) { // Escaped {
      cJSON_AddItemToObject(g_styles_object, style_id, style_node); // Transfer ownership
  } else { // Escaped {
      fprintf(stderr, "EMUL_LVGL Error: Global styles object missing! Style node will leak.\n");
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_style_reset(lv_style_t * style) {{ // Escaped {{

  // Marshal arguments for lv_style_reset
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_reset\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Reset style object style
  cJSON* style_node = get_style_json_node(style);
  if (style_node) { // Escaped {
     cJSON *child = style_node->child, *next_child = NULL;
     while (child) { // Escaped {
         next_child = child->next;
         if (child->string && strcmp(child->string, "type") != 0 && strcmp(child->string, "id") != 0) { // Escaped {
             cJSON_DeleteItemFromObjectCaseSensitive(style_node, child->string);
         } // Escaped }
         child = next_child;
     } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args);

}} // Escaped }}

void lv_style_set_align(lv_style_t * style, lv_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_align\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'align'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_anim(lv_style_t * style, const lv_anim_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_anim
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_anim\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const lv_anim_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'anim'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "anim")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "anim", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "anim", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_anim_duration(lv_style_t * style, uint32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_anim_duration
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_anim_duration\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'anim_duration'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "anim_duration")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "anim_duration", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "anim_duration", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_arc_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_arc_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_arc_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'arc_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "arc_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "arc_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "arc_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_arc_image_src(lv_style_t * style, const void * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_arc_image_src
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_arc_image_src\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const void *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'arc_image_src'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "arc_image_src")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "arc_image_src", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "arc_image_src", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_arc_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_arc_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_arc_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'arc_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "arc_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "arc_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "arc_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_arc_rounded(lv_style_t * style, bool value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_arc_rounded
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_arc_rounded\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: bool) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'arc_rounded'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "arc_rounded")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "arc_rounded", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "arc_rounded", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_arc_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_arc_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_arc_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'arc_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "arc_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "arc_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "arc_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_base_dir(lv_style_t * style, lv_base_dir_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_base_dir
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_base_dir\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_base_dir_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'base_dir'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "base_dir")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "base_dir", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "base_dir", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_grad(lv_style_t * style, const lv_grad_dsc_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_grad
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_grad\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const lv_grad_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_grad'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_grad")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_grad", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_grad", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_grad_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_grad_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_grad_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_grad_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_grad_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_grad_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_grad_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_grad_dir(lv_style_t * style, lv_grad_dir_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_grad_dir
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_grad_dir\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_grad_dir_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_grad_dir'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_grad_dir")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_grad_dir", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_grad_dir", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_grad_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_grad_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_grad_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_grad_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_grad_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_grad_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_grad_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_grad_stop(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_grad_stop
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_grad_stop\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_grad_stop'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_grad_stop")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_grad_stop", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_grad_stop", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_image_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_image_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_image_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_image_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_image_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_image_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_image_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_image_recolor(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_image_recolor
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_image_recolor\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_image_recolor'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_image_recolor")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_image_recolor", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_image_recolor", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_image_recolor_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_image_recolor_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_image_recolor_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_image_recolor_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_image_recolor_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_image_recolor_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_image_recolor_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_image_src(lv_style_t * style, const void * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_image_src
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_image_src\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const void *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_image_src'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_image_src")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_image_src", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_image_src", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_image_tiled(lv_style_t * style, bool value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_image_tiled
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_image_tiled\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: bool) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_image_tiled'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_image_tiled")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_image_tiled", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_image_tiled", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_main_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_main_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_main_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_main_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_main_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_main_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_main_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_main_stop(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_main_stop
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_main_stop\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_main_stop'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_main_stop")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_main_stop", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_main_stop", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bg_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bg_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bg_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bg_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bg_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bg_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bg_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_bitmap_mask_src(lv_style_t * style, const void * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_bitmap_mask_src
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_bitmap_mask_src\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const void *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'bitmap_mask_src'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "bitmap_mask_src")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "bitmap_mask_src", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "bitmap_mask_src", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_blend_mode(lv_style_t * style, lv_blend_mode_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_blend_mode
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_blend_mode\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_blend_mode_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'blend_mode'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "blend_mode")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "blend_mode", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "blend_mode", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_border_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_border_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_border_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'border_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "border_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "border_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "border_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_border_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_border_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_border_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'border_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "border_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "border_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "border_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_border_post(lv_style_t * style, bool value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_border_post
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_border_post\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: bool) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'border_post'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "border_post")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "border_post", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "border_post", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_border_side(lv_style_t * style, lv_border_side_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_border_side
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_border_side\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_border_side_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'border_side'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "border_side")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "border_side", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "border_side", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_border_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_border_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_border_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'border_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "border_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "border_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "border_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_clip_corner(lv_style_t * style, bool value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_clip_corner
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_clip_corner\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: bool) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'clip_corner'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "clip_corner")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "clip_corner", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "clip_corner", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_color_filter_dsc(lv_style_t * style, const lv_color_filter_dsc_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_color_filter_dsc
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_color_filter_dsc\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const lv_color_filter_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'color_filter_dsc'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "color_filter_dsc")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "color_filter_dsc", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "color_filter_dsc", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_color_filter_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_color_filter_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_color_filter_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'color_filter_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "color_filter_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "color_filter_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "color_filter_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_flex_cross_place(lv_style_t * style, lv_flex_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_flex_cross_place
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_flex_cross_place\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_flex_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'flex_cross_place'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "flex_cross_place")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "flex_cross_place", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "flex_cross_place", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_flex_flow(lv_style_t * style, lv_flex_flow_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_flex_flow
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_flex_flow\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_flex_flow_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'flex_flow'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "flex_flow")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "flex_flow", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "flex_flow", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_flex_grow(lv_style_t * style, uint8_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_flex_grow
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_flex_grow\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: uint8_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'flex_grow'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "flex_grow")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "flex_grow", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "flex_grow", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_flex_main_place(lv_style_t * style, lv_flex_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_flex_main_place
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_flex_main_place\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_flex_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'flex_main_place'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "flex_main_place")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "flex_main_place", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "flex_main_place", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_flex_track_place(lv_style_t * style, lv_flex_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_flex_track_place
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_flex_track_place\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_flex_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'flex_track_place'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "flex_track_place")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "flex_track_place", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "flex_track_place", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_cell_column_pos(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_cell_column_pos
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_cell_column_pos\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_cell_column_pos'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_cell_column_pos")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_cell_column_pos", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_cell_column_pos", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_cell_column_span(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_cell_column_span
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_cell_column_span\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_cell_column_span'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_cell_column_span")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_cell_column_span", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_cell_column_span", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_cell_row_pos(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_cell_row_pos
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_cell_row_pos\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_cell_row_pos'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_cell_row_pos")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_cell_row_pos", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_cell_row_pos", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_cell_row_span(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_cell_row_span
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_cell_row_span\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_cell_row_span'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_cell_row_span")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_cell_row_span", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_cell_row_span", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_cell_x_align(lv_style_t * style, lv_grid_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_cell_x_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_cell_x_align\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_grid_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_cell_x_align'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_cell_x_align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_cell_x_align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_cell_x_align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_cell_y_align(lv_style_t * style, lv_grid_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_cell_y_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_cell_y_align\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_grid_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_cell_y_align'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_cell_y_align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_cell_y_align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_cell_y_align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_column_align(lv_style_t * style, lv_grid_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_column_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_column_align\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_grid_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_column_align'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_column_align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_column_align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_column_align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_column_dsc_array(lv_style_t * style, const int32_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_column_dsc_array
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_column_dsc_array\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const int32_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_column_dsc_array'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_column_dsc_array")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_column_dsc_array", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_column_dsc_array", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_row_align(lv_style_t * style, lv_grid_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_row_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_row_align\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_grid_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_row_align'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_row_align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_row_align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_row_align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_grid_row_dsc_array(lv_style_t * style, const int32_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_grid_row_dsc_array
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_grid_row_dsc_array\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const int32_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'grid_row_dsc_array'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "grid_row_dsc_array")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "grid_row_dsc_array", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "grid_row_dsc_array", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_height(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_height
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_height\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'height'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "height")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "height", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "height", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_image_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_image_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_image_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'image_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "image_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "image_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "image_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_image_recolor(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_image_recolor
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_image_recolor\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'image_recolor'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "image_recolor")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "image_recolor", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "image_recolor", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_image_recolor_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_image_recolor_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_image_recolor_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'image_recolor_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "image_recolor_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "image_recolor_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "image_recolor_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_layout(lv_style_t * style, uint16_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_layout
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_layout\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: uint16_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'layout'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "layout")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "layout", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "layout", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_length(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_length
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_length\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'length'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "length")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "length", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "length", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_line_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_line_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_line_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'line_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "line_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "line_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "line_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_line_dash_gap(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_line_dash_gap
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_line_dash_gap\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'line_dash_gap'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "line_dash_gap")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "line_dash_gap", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "line_dash_gap", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_line_dash_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_line_dash_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_line_dash_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'line_dash_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "line_dash_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "line_dash_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "line_dash_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_line_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_line_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_line_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'line_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "line_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "line_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "line_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_line_rounded(lv_style_t * style, bool value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_line_rounded
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_line_rounded\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: bool) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'line_rounded'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "line_rounded")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "line_rounded", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "line_rounded", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_line_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_line_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_line_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'line_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "line_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "line_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "line_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_all(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_all
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_all\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_all'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_all")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_all", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_all", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_bottom(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_bottom
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_bottom\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_bottom'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_bottom")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_bottom", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_bottom", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_hor(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_hor
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_hor\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_hor'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_hor")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_hor", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_hor", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_left(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_left
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_left\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_left'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_left")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_left", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_left", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_right(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_right
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_right\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_right'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_right")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_right", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_right", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_top(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_top
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_top\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_top'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_top")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_top", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_top", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_margin_ver(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_margin_ver
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_margin_ver\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'margin_ver'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "margin_ver")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "margin_ver", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "margin_ver", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_max_height(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_max_height
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_max_height\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'max_height'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "max_height")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "max_height", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "max_height", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_max_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_max_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_max_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'max_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "max_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "max_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "max_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_min_height(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_min_height
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_min_height\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'min_height'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "min_height")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "min_height", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "min_height", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_min_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_min_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_min_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'min_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "min_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "min_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "min_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_opa_layered(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_opa_layered
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_opa_layered\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'opa_layered'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "opa_layered")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "opa_layered", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "opa_layered", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_outline_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_outline_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_outline_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'outline_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "outline_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "outline_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "outline_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_outline_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_outline_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_outline_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'outline_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "outline_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "outline_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "outline_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_outline_pad(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_outline_pad
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_outline_pad\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'outline_pad'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "outline_pad")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "outline_pad", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "outline_pad", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_outline_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_outline_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_outline_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'outline_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "outline_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "outline_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "outline_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_all(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_all
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_all\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_all'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_all")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_all", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_all", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_bottom(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_bottom
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_bottom\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_bottom'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_bottom")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_bottom", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_bottom", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_column(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_column
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_column\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_column'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_column")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_column", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_column", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_gap(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_gap
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_gap\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_gap'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_gap")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_gap", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_gap", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_hor(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_hor
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_hor\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_hor'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_hor")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_hor", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_hor", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_left(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_left
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_left\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_left'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_left")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_left", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_left", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_radial(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_radial
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_radial\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_radial'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_radial")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_radial", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_radial", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_right(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_right
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_right\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_right'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_right")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_right", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_right", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_row(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_row
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_row\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_row'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_row")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_row", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_row", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_top(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_top
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_top\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_top'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_top")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_top", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_top", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_pad_ver(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_pad_ver
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_pad_ver\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'pad_ver'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "pad_ver")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "pad_ver", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "pad_ver", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_prop(lv_style_t * style, lv_style_prop_t prop, lv_style_value_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_prop
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_prop\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: prop (Type: lv_style_prop_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(prop);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: value (Type: lv_style_value_t) -> Marshal: None, IsConstructor: False
    cJSON_AddItemToArray(marshalled_args, cJSON_CreateString("<unsupported_arg>"));
    fprintf(stderr, "EMUL_LVGL Warning: Unsupported argument type for 'value' in lv_style_set_prop\n");
  // Set style property 'prop'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "prop")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "prop", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "prop", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_radial_offset(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_radial_offset
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_radial_offset\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'radial_offset'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "radial_offset")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "radial_offset", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "radial_offset", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_radius(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_radius
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_radius\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'radius'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "radius")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "radius", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "radius", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_recolor(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_recolor
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_recolor\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'recolor'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "recolor")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "recolor", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "recolor", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_recolor_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_recolor_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_recolor_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'recolor_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "recolor_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "recolor_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "recolor_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_rotary_sensitivity(lv_style_t * style, uint32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_rotary_sensitivity
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_rotary_sensitivity\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: uint32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'rotary_sensitivity'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "rotary_sensitivity")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "rotary_sensitivity", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "rotary_sensitivity", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_shadow_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_shadow_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_shadow_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'shadow_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "shadow_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "shadow_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "shadow_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_shadow_offset_x(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_shadow_offset_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_shadow_offset_x\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'shadow_offset_x'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "shadow_offset_x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "shadow_offset_x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "shadow_offset_x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_shadow_offset_y(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_shadow_offset_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_shadow_offset_y\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'shadow_offset_y'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "shadow_offset_y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "shadow_offset_y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "shadow_offset_y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_shadow_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_shadow_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_shadow_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'shadow_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "shadow_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "shadow_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "shadow_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_shadow_spread(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_shadow_spread
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_shadow_spread\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'shadow_spread'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "shadow_spread")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "shadow_spread", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "shadow_spread", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_shadow_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_shadow_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_shadow_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'shadow_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "shadow_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "shadow_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "shadow_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_size(lv_style_t * style, int32_t width, int32_t height) {{ // Escaped {{

  // Marshal arguments for lv_style_set_size
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_size\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: width (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)width;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 2: height (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg2_json = (cJSON*)height;
    if (arg2_json) { cJSON_AddItemToArray(marshalled_args, arg2_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'size'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "size")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "size", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "size", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_align(lv_style_t * style, lv_text_align_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_align
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_align\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_text_align_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_align'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_align")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_align", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_align", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_decor(lv_style_t * style, lv_text_decor_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_decor
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_decor\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_text_decor_t) -> Marshal: marshal_int, IsConstructor: False
    cJSON* arg1_json = marshal_int(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_decor'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_decor")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_decor", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_decor", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_font(lv_style_t * style, const lv_font_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_font
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_font\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const lv_font_t *) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_font'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_font")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_font", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_font", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_letter_space(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_letter_space
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_letter_space\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_letter_space'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_letter_space")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_letter_space", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_letter_space", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_line_space(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_line_space
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_line_space\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_line_space'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_line_space")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_line_space", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_line_space", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_outline_stroke_color(lv_style_t * style, lv_color_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_outline_stroke_color
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_outline_stroke_color\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_color_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_outline_stroke_color'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_outline_stroke_color")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_outline_stroke_color", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_outline_stroke_color", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_outline_stroke_opa(lv_style_t * style, lv_opa_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_outline_stroke_opa
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_outline_stroke_opa\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: lv_opa_t) -> Marshal: marshal_uint, IsConstructor: False
    cJSON* arg1_json = marshal_uint(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_outline_stroke_opa'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_outline_stroke_opa")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_outline_stroke_opa", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_outline_stroke_opa", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_text_outline_stroke_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_text_outline_stroke_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_text_outline_stroke_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'text_outline_stroke_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "text_outline_stroke_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "text_outline_stroke_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "text_outline_stroke_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_height(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_height
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_height\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_height'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_height")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_height", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_height", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_pivot_x(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_pivot_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_pivot_x\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_pivot_x'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_pivot_x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_pivot_x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_pivot_x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_pivot_y(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_pivot_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_pivot_y\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_pivot_y'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_pivot_y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_pivot_y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_pivot_y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_rotation(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_rotation
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_rotation\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_rotation'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_rotation")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_rotation", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_rotation", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_scale(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_scale
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_scale\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_scale'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_scale")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_scale", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_scale", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_scale_x(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_scale_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_scale_x\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_scale_x'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_scale_x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_scale_x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_scale_x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_scale_y(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_scale_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_scale_y\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_scale_y'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_scale_y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_scale_y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_scale_y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_skew_x(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_skew_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_skew_x\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_skew_x'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_skew_x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_skew_x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_skew_x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_skew_y(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_skew_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_skew_y\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_skew_y'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_skew_y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_skew_y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_skew_y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transform_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transform_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transform_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transform_width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transform_width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transform_width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transform_width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_transition
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_transition\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: const lv_style_transition_dsc_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg1_json = marshal_c_pointer(value);
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'transition'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "transition")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "transition", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "transition", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_translate_radial(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_translate_radial
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_translate_radial\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'translate_radial'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "translate_radial")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "translate_radial", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "translate_radial", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_translate_x(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_translate_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_translate_x\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'translate_x'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "translate_x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "translate_x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "translate_x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_translate_y(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_translate_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_translate_y\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'translate_y'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "translate_y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "translate_y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "translate_y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_width(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_width
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_width\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'width'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "width")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "width", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "width", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_x(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_x
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_x\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'x'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "x")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "x", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "x", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }}

void lv_style_set_y(lv_style_t * style, int32_t value) {{ // Escaped {{

  // Marshal arguments for lv_style_set_y
  cJSON* marshalled_args = cJSON_CreateArray();
  if (!marshalled_args) { fprintf(stderr, "EMUL_LVGL Error: Failed to create args array for lv_style_set_y\n");  } // Escaped { }
  // Arg 0: style (Type: lv_style_t *) -> Marshal: marshal_c_pointer, IsConstructor: False
    cJSON* arg0_json = marshal_c_pointer(style);
    if (arg0_json) { cJSON_AddItemToArray(marshalled_args, arg0_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Arg 1: value (Type: int32_t) -> Marshal: None, IsConstructor: True
    cJSON* arg1_json = (cJSON*)value;
    if (arg1_json) { cJSON_AddItemToArray(marshalled_args, arg1_json); } else { cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }
  // Set style property 'y'
  cJSON* target_node = get_style_json_node(style);
  if (target_node) { // Escaped {
      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)
      if (value_json) { // Escaped {
          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding
          if (cJSON_HasObjectItem(target_node, "y")) { // Escaped {
              cJSON_ReplaceItemInObjectCaseSensitive(target_node, "y", detached_value);
          } else { // Escaped {
              cJSON_AddItemToObject(target_node, "y", detached_value);
          } // Escaped }
      } // Escaped }
  } // Escaped }
  cJSON_Delete(marshalled_args); // Clean up array and remaining items

}} // Escaped }} // <<< UNESCAPED PLACEHOLDER

