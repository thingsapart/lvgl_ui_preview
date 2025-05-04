#!/usr/bin/env python3

import json
import argparse
import os
import sys
import re
import time # Needed for timestamp
from collections import defaultdict
from typing import List, Dict, Set, Any, Optional, Tuple

_debug = print

# --- Default Include/Exclude Lists (Adjusted for state building) ---
# (Keep defaults as they are, users can override)
DEFAULT_INCLUDES = {
    "functions": [
        "lv_init", "lv_deinit", # Core lifecycle
        "lv_obj_create", "lv_obj_del", # Base object
        "lv_obj_set_parent",
        "lv_obj_set_pos", "lv_obj_set_x", "lv_obj_set_y",
        "lv_obj_set_size", "lv_obj_set_width", "lv_obj_set_height",
        "lv_obj_set_align", "lv_obj_align", "lv_obj_align_to", # Alignment needs careful state mapping
        "lv_obj_add_style", "lv_obj_remove_style", # Referencing styles
        "lv_obj_add_flag", "lv_obj_clear_flag", "lv_obj_set_state", # State flags
        "lv_obj_set_style_local_", # Local style properties
        "lv_style_init", "lv_style_reset", "lv_style_set_", # Style definition functions
        # Common Widgets
        "lv_label_create", "lv_label_set_text", "lv_label_set_long_mode",
        "lv_btn_create",
        "lv_img_create", "lv_img_set_src",
        # Color helpers (needed by callers AND for nesting)
        "lv_color_",
        # Font helpers (needed for nesting if used as args)
        "lv_font_get",
        # Screen management (optional but useful context)
        "lv_disp_get_default", "lv_disp_load_scr", "lv_scr_act",
    ],
    "enums": ["lv_", "LV_"], # Need states, aligns, etc.
    "structs": ["lv_color_t", "lv_font_t", "lv_img_dsc_t"], # Only structs passed by value or whose members are accessed
    "typedefs": ["lv_", "LV_", "bool", "int", "uint", "char", "float", "double", "size_t", "intptr_t", "uintptr_t"], # Base types
    "macros": ["LV_"], # Colors, constants etc.
}

DEFAULT_EXCLUDES: Dict[str, List[str]] = {
    "functions": [
         # Exclude things hard to model statefully or less common for basic UI desc
        "_lv_", "lv_debug_", "lv_mem_", "lv_log_", "lv_ll_", "lv_timer_",
        "lv_anim_", "lv_event_", "lv_group_", "lv_indev_",
        "lv_obj_get_", # Getters are mostly irrelevant for state building
        "lv_style_get_",
        "lv_refr_", # Refresh/rendering related
        "lv_theme_", # Themes add complexity, handle later if needed
        "lv_fs_", # Filesystem
        "lv_draw_", # Drawing internals
    ],
    "enums": ["_LV_"],
    "structs": ["_lv_", "lv_obj", "lv_style", "lv_group", "lv_timer", "lv_event"], # Exclude internal/complex structs we replace/ignore
    "typedefs": ["_lv_"],
    "macros": ["_LV_"],
}

POINTER_ID_PREFIX='@'

# --- Helper Functions ---
# (matches_filters, get_type_name, is_pointer_type, get_pointee_type_info)
# (find_api_definition, get_base_type_info, get_c_type_str)
# ... (Keep these helpers as they were in the previous corrected version) ...
def matches_filters(name: str, includes: List[str], excludes: List[str]) -> bool:
    if not name: return False
    if name in excludes: return False
    if name in includes: return True
    for prefix in excludes:
        if name.startswith(prefix): return False
    for prefix in includes:
        if name.startswith(prefix): return True
    return False

def get_type_name(type_info: Dict[str, Any]) -> Optional[str]:
    if not isinstance(type_info, dict): return None
    json_type = type_info.get("json_type")
    if json_type == "pointer": return get_type_name(type_info.get("type"))
    elif json_type == "array": return get_type_name(type_info.get("type"))
    elif json_type in ["ret_type", "typedef", "arg", "field"]: return get_type_name(type_info.get("type"))
    else: return type_info.get("name")

def is_pointer_type(type_info: Dict[str, Any]) -> bool:
    return isinstance(type_info, dict) and type_info.get("json_type") == "pointer"

def get_pointee_type_info(type_info: Dict[str, Any]) -> Optional[Dict[str, Any]]:
     if is_pointer_type(type_info):
         return type_info.get("type")
     return None

def find_api_definition(api_data: Dict[str, Any], name: str) -> Optional[Dict[str, Any]]:
    if not name: return None
    for category in ["enums", "structs", "unions", "typedefs", "function_pointers"]:
        for item in api_data.get(category, []):
            if item.get("name") == name:
                return item
    # Also check functions, as constructors might be functions returning values
    for item in api_data.get("functions", []):
         if item.get("name") == name:
              # Return a simplified dict indicating it's a function relevant for type resolution
              return {"name": name, "json_type": "function"}
    return None

def get_base_type_info(type_info: Dict[str, Any], api_data: Dict[str, Any]) -> Optional[Dict[str, Any]]:
    if not isinstance(type_info, dict): return None
    current_type_info = type_info
    visited_typedefs = set()
    max_depth = 10 # Prevent infinite loops in case of weird recursion
    depth = 0

    while depth < max_depth:
        depth += 1
        json_type = current_type_info.get("json_type")
        name = current_type_info.get("name")

        if name == "lv_obj_t": return {"name": "lv_obj_t", "json_type": "lvgl_type", "_is_emulated_pointer": True}
        if name == "lv_style_t": return {"name": "lv_style_t", "json_type": "lvgl_type", "_is_emulated_struct": True}

        if json_type == "pointer" or json_type == "array":
            current_type_info = current_type_info.get("type")
        elif json_type in ["ret_type", "arg", "field"]:
             current_type_info = current_type_info.get("type")
        elif json_type == "typedef":
            if name in visited_typedefs: return None # Cycle
            visited_typedefs.add(name)
            typedef_def = next((t for t in api_data.get("typedefs", []) if t.get("name") == name), None)
            if not typedef_def:
                 # Try resolving the type directly if typedef definition is missing
                 current_type_info = current_type_info.get("type")
                 if not isinstance(current_type_info, dict): return None # End of chain
                 continue # Restart loop with the underlying type
            else:
                 current_type_info = typedef_def.get("type") # Follow the typedef definition
        elif json_type in ["enum", "struct", "union", "lvgl_type", "stdlib_type", "primitive_type", "function_pointer"]:
             if json_type == "lvgl_type" and name:
                 # Try finding the actual definition (struct, enum, etc.)
                 base_def = find_api_definition(api_data, name) # find_api checks structs, enums, typedefs
                 if base_def and base_def.get("json_type") != "function": # Exclude simple function name matches here
                     if base_def.get("name") == "lv_obj_t": return {"name": "lv_obj_t", "json_type": "lvgl_type", "_is_emulated_pointer": True}
                     if base_def.get("name") == "lv_style_t": return {"name": "lv_style_t", "json_type": "lvgl_type", "_is_emulated_struct": True}
                     # If base_def is a typedef, follow it one more step
                     if base_def.get("json_type") == "typedef":
                         current_type_info = base_def.get("type")
                         continue
                     return base_def # Found the actual non-typedef definition
                 else:
                      # If not found, maybe it's a typedef to a primitive/stdlib?
                      # Return the lvgl_type itself, marshaller will handle known ones.
                      return current_type_info
             else:
                 return current_type_info # Found a non-lvgl base type or a primitive
        else:
             print(f"Warning: Cannot resolve base type for: {current_type_info}", file=sys.stderr)
             return None

        if not isinstance(current_type_info, dict): return None # Reached end of chain

    print(f"Warning: Max recursion depth reached resolving type: {type_info}", file=sys.stderr)
    return None


def get_c_type_str(type_info: Dict[str, Any], api_data: Dict[str, Any], is_arg_or_field=False) -> str:
    # Add api_data dependency for base type resolution
    if not isinstance(type_info, dict): return "/* unknown */"

    if type_info.get("_is_emulated_pointer"): return type_info.get("name", "cJSON *")
    if type_info.get("_is_emulated_struct"): return type_info.get("name", "struct _emul_struct")

    json_type = type_info.get("json_type")
    name = type_info.get("name")
    quals = type_info.get("quals", [])

    base_type_info = get_base_type_info(type_info, api_data) # Pass api_data
    if base_type_info:
        if base_type_info.get("name") == "lv_obj_t":
             if is_pointer_type(type_info): return "lv_obj_t"
             else: return "/* ERROR: Cannot pass lv_obj_t by value */"
        if base_type_info.get("name") == "lv_style_t":
             if is_pointer_type(type_info): return "lv_style_t *"
             else: return "lv_style_t"

    qual_prefix = " ".join(q for q in quals if q != 'volatile') + (" " if quals else "")
    volatile_qual = " volatile" if 'volatile' in quals else ""

    if json_type == "pointer":
        pointee_type = type_info.get("type")
        if isinstance(pointee_type, dict) and pointee_type.get("json_type") == "function_pointer":
             fp_name = pointee_type.get("name", "fptr")
             return f"{qual_prefix}{fp_name}{volatile_qual} *"
        else:
             pointee_base_info = get_base_type_info(pointee_type, api_data) # Pass api_data
             if pointee_base_info:
                  if pointee_base_info.get("name") == "lv_obj_t": return f"{qual_prefix}lv_obj_t{volatile_qual} *"
                  if pointee_base_info.get("name") == "lv_style_t": return f"{qual_prefix}lv_style_t{volatile_qual} *"

             pointee_str = get_c_type_str(pointee_type, api_data) # Pass api_data
             return f"{pointee_str}{volatile_qual} *{qual_prefix.strip()}"

    elif json_type == "array":
        base_type_str = get_c_type_str(type_info.get("type"), api_data) # Pass api_data
        return f"{qual_prefix}{base_type_str}{volatile_qual}"

    elif json_type in ["ret_type", "arg", "field", "typedef"]:
        return get_c_type_str(type_info.get("type"), api_data) # Pass api_data

    elif json_type == "enum":
        enum_name = type_info.get("name")
        return f"{qual_prefix}{enum_name or f'enum anon_enum_{hash(str(type_info))}'}{volatile_qual}"

    elif json_type == "struct":
         struct_name = type_info.get("name")
         if struct_name == "lv_obj_t": return "/* emulated lv_obj_t */"
         elif struct_name == "lv_style_t": return "lv_style_t"
         elif struct_name: return f"{qual_prefix}{struct_name}{volatile_qual}"
         else:
             # Handle anonymous structs potentially defined via typedef
             if is_arg_or_field: # If used directly, need 'struct { ... }'
                # Generating full anon struct definition here is complex,
                # assume named structs or typedefs are used for fields/args.
                return f"{qual_prefix}struct /* anonymous */{volatile_qual}"
             else: # Likely part of a typedef, just return the kind
                 return f"{qual_prefix}struct{volatile_qual}"


    elif json_type == "union":
        union_name = type_info.get("name")
        return f"{qual_prefix}{union_name or '/* anonymous_union */'}{volatile_qual}"

    elif json_type == "lvgl_type":
        if name == "lv_obj_t": return "lv_obj_t"
        if name == "lv_style_t": return "lv_style_t"
        return f"{qual_prefix}{name}{volatile_qual}"

    elif json_type in ["stdlib_type", "primitive_type"]:
        return f"{qual_prefix}{name}{volatile_qual}"

    elif json_type == "function_pointer":
        return f"{qual_prefix}{name}{volatile_qual}"

    elif json_type == "special_type" and name == "ellipsis":
        return "..."

    else:
        return f"/* unknown_type: {json_type} name: {name} */"


# --- C Code Generation Templates ---
# (C_HEADER_TEMPLATE is unchanged)
C_HEADER_TEMPLATE = """\
#ifndef EMUL_LVGL_H
#define EMUL_LVGL_H

#ifdef __cplusplus
extern "C" {{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t
#include "cJSON.h" // Include cJSON

// --- Core Emulation Type Definitions ---
typedef cJSON* lv_obj_t; // *** Map lv_obj_t to cJSON node ***

// Define lv_style_t as an opaque struct for type safety in C
// Its actual JSON representation is managed internally.
typedef struct _emul_lv_style_t {{ uint8_t _dummy; /* Needs at least one member */ }} lv_style_t;

// --- Forward Declarations (for other structs if needed) ---
{forward_declarations}

// --- Emulation Control ---
/** @brief Initializes the LVGL emulator. Call once at the beginning. */
void emul_lvgl_init(const char* output_json_path);

/** @brief Deinitializes the LVGL emulator and writes the final JSON state. */
void emul_lvgl_deinit(void);

/** @brief Registers a pointer (like a font or image) with a symbolic name for JSON output. */
void emul_lvgl_register_pointer(const void *ptr, const char *name);

// --- LVGL Type Definitions (Excluding lv_obj_t, lv_style_t) ---
{typedef_defs}

// --- LVGL Enum Definitions ---
{enum_defs}

// --- LVGL Struct Definitions (Excluding lv_obj_t, lv_style_t) ---
{struct_defs}

// --- LVGL Union Definitions ---
{union_defs}

// --- LVGL Macro Definitions ---
{macro_defs}

// --- LVGL Function Prototypes (Using emulated types) ---
{function_prototypes}

#ifdef __cplusplus
}} /*extern "C"*/
#endif

#endif /* EMUL_LVGL_H */
"""

# (C_SOURCE_TEMPLATE needs escaping fixed and additions)
C_SOURCE_TEMPLATE = """\
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

typedef struct {{ // Escaped {{
    const void *ptr;
    char name[128]; // Full name including prefix, e.g., "@lv_font_montserrat_14"
}} PointerMapEntry; // Escaped }}

typedef struct {{ // Escaped {{
    const void *style_ptr; // Address of the user's lv_style_t struct
    cJSON *json_node;      // The cJSON object representing this style
}} StyleMapEntry; // Escaped }}

// Global JSON state
static cJSON *g_root_json = NULL; // Root object: {{ "roots": [], "styles": {{}}, "resources": {{}} }}
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
static cJSON* get_style_json_node(const lv_style_t *style_ptr) {{ // Escaped {{
    if (!style_ptr) return NULL;
    for (size_t i = 0; i < g_style_map_count; ++i) {{ // Escaped {{
        if (g_style_map[i].style_ptr == style_ptr) {{ // Escaped {{
            return g_style_map[i].json_node;
        }} // Escaped }}
    }} // Escaped }}
    // fprintf(stderr, "EMUL_LVGL Warning: Style object %p not initialized with lv_style_init()?\\n", (void*)style_ptr);
    return NULL;
}} // Escaped }}

// --- Pointer Management ---

// Generates the string ID for a pointer (e.g., "@lv_font_montserrat_14" or "@ptr_0x...")
static const char* get_pointer_id(const void *ptr) {{ // Escaped {{
    if (ptr == NULL) {{ // Escaped {{
        return NULL; // Return NULL for null C pointers
    }} // Escaped }}
    for (size_t i = 0; i < g_pointer_map_count; ++i) {{ // Escaped {{
        if (g_pointer_map[i].ptr == ptr) {{ // Escaped {{
            return g_pointer_map[i].name;
        }} // Escaped }}
    }} // Escaped }}
    static char generated_id_buffer[MAX_POINTER_MAP_ENTRIES][64];
    static int buffer_idx = 0;
    int current_buf_idx = buffer_idx;
    buffer_idx = (buffer_idx + 1) % MAX_POINTER_MAP_ENTRIES;
    snprintf(generated_id_buffer[current_buf_idx], sizeof(generated_id_buffer[0]),
             "%sptr_%p", POINTER_ID_PREFIX, ptr);
    return generated_id_buffer[current_buf_idx];
}} // Escaped }}

void emul_lvgl_register_pointer(const void *ptr, const char *name) {{ // Escaped {{
    if (!ptr || !name) return;
    if (g_pointer_map_count >= MAX_POINTER_MAP_ENTRIES) {{ // Escaped {{
        fprintf(stderr, "EMUL_LVGL Warning: Pointer map full. Cannot register %s (%p).\\n", name, ptr);
        return;
    }} // Escaped }}

    char full_name[128];
    snprintf(full_name, sizeof(full_name), "%s%s", POINTER_ID_PREFIX, name);

    for (size_t i = 0; i < g_pointer_map_count; ++i) {{ // Escaped {{
        if (g_pointer_map[i].ptr == ptr) {{ // Escaped {{
            strncpy(g_pointer_map[i].name, full_name, sizeof(g_pointer_map[i].name) - 1);
            g_pointer_map[i].name[sizeof(g_pointer_map[i].name) - 1] = '\\0';
             if (g_resources_object && !cJSON_HasObjectItem(g_resources_object, full_name)) {{ // Escaped {{
                 cJSON_AddItemToObject(g_resources_object, full_name, cJSON_CreateObject());
             }} // Escaped }}
            return;
        }} // Escaped }}
    }} // Escaped }}

    g_pointer_map[g_pointer_map_count].ptr = ptr;
    strncpy(g_pointer_map[g_pointer_map_count].name, full_name, sizeof(g_pointer_map[0].name) - 1);
    g_pointer_map[g_pointer_map_count].name[sizeof(g_pointer_map[0].name) - 1] = '\\0';
    g_pointer_map_count++;

    if (g_resources_object) {{ // Escaped {{
         cJSON_AddItemToObject(g_resources_object, full_name, cJSON_CreateObject());
    }} // Escaped }}
}} // Escaped }}


// --- cJSON Marshalling Helpers ---

// Basic marshallers create NEW cJSON objects
static cJSON* marshal_int(int val) {{ return cJSON_CreateNumber(val); }}
static cJSON* marshal_uint(unsigned int val) {{ return cJSON_CreateNumber(val); }}
static cJSON* marshal_long(long val) {{ return cJSON_CreateNumber(val); }}
static cJSON* marshal_ulong(unsigned long val) {{ return cJSON_CreateNumber(val); }}
static cJSON* marshal_int64(int64_t val) {{ return cJSON_CreateNumber((double)val); }}
static cJSON* marshal_uint64(uint64_t val) {{ return cJSON_CreateNumber((double)val); }}
static cJSON* marshal_float(float val) {{ return cJSON_CreateNumber(val); }}
static cJSON* marshal_double(double val) {{ return cJSON_CreateNumber(val); }}
static cJSON* marshal_bool(bool val) {{ return cJSON_CreateBool(val); }}

static cJSON* marshal_string(const char *str) {{ // Escaped {{
    if (!str) return cJSON_CreateNull();
    return cJSON_CreateString(str);
}} // Escaped }}

// Marshals a C pointer to its registered ID ("@name" or "@ptr_...") or null
static cJSON* marshal_c_pointer(const void *ptr) {{ // Escaped {{
    const char* id_str = get_pointer_id(ptr);
    if (!id_str) return cJSON_CreateNull();
    return cJSON_CreateString(id_str);
}} // Escaped }}

// Marshals lv_color_t BY VALUE. Only used if not a constructor result.
static cJSON* marshal_lv_color_t_value(lv_color_t color) {{ // Escaped {{
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
}} // Escaped }}


// Helper to marshal arguments for style properties [value, state]
static cJSON* marshal_style_property_args(cJSON *value_json, int32_t state) {{ // Escaped {{
    // value_json is assumed to be correctly marshalled (either primitive or call representation)
    if (!value_json) value_json = cJSON_CreateNull();

    cJSON *args_array = cJSON_CreateArray();
    if (!args_array) {{ // Escaped {{
        // If value_json was dynamically created for this call, it should be deleted here.
        // However, if it's a shared call representation, deleting it is wrong.
        // Let's assume value_json lifetime is managed by the caller / call representation.
        // cJSON_Delete(value_json); // <<< BE CAREFUL HERE
        return NULL;
    }} // Escaped }}
    cJSON_AddItemToArray(args_array, value_json); // Add value (cJSON manages ownership if creation succeeded)
    cJSON_AddItemToArray(args_array, cJSON_CreateNumber(state));
    return args_array;
}} // Escaped }}


// --- Emulation Control Implementation ---

void emul_lvgl_init(const char* output_json_path) {{ // Escaped {{
    if (g_root_json) {{ fprintf(stderr, "EMUL_LVGL Warning: Already initialized.\\n"); return; }} // Escaped {{ }}

    g_root_json = cJSON_CreateObject();
    if (!g_root_json) {{ fprintf(stderr, "EMUL_LVGL Error: Failed to create root JSON object.\\n"); exit(1); }} // Escaped {{ }}

    g_root_objects_array = cJSON_AddArrayToObject(g_root_json, "roots");
    g_styles_object = cJSON_AddObjectToObject(g_root_json, "styles");
    g_resources_object = cJSON_AddObjectToObject(g_root_json, "resources");

    if (!g_root_objects_array || !g_styles_object || !g_resources_object) {{ // Escaped {{
         fprintf(stderr, "EMUL_LVGL Error: Failed to create root JSON structure.\\n");
         cJSON_Delete(g_root_json); g_root_json = NULL; exit(1);
    }} // Escaped }}

    g_json_output_file = fopen(output_json_path, "w");
    if (!g_json_output_file) {{ // Escaped {{
        perror("EMUL_LVGL Error: Cannot open output JSON file");
        cJSON_Delete(g_root_json); g_root_json = NULL; exit(1);
    }} // Escaped }}

    g_pointer_map_count = 0;
    g_style_map_count = 0;

    cJSON *meta = cJSON_CreateObject();
    if (meta) {{ // Escaped {{
        cJSON_AddStringToObject(meta, "generator", "emul_lvgl");
        cJSON_AddNumberToObject(meta, "timestamp", (double)time(NULL));
        cJSON_AddItemToObject(g_root_json, "metadata", meta);
    }} // Escaped }}
}} // Escaped }}

void emul_lvgl_deinit(void) {{ // Escaped {{
    if (!g_root_json || !g_json_output_file) {{ // Escaped {{
         fprintf(stderr, "EMUL_LVGL Warning: Not initialized or already deinitialized.\\n");
        return;
    }} // Escaped }}

    char *json_string = cJSON_PrintBuffered(g_root_json, 4096, 1);
    if (json_string) {{ // Escaped {{
        fprintf(g_json_output_file, "%s\\n", json_string);
        cJSON_free(json_string);
    }} else {{ // Escaped {{
        fprintf(stderr, "EMUL_LVGL Error: Failed to serialize JSON to string. Trying unformatted.\\n");
        json_string = cJSON_PrintUnformatted(g_root_json);
         if (json_string) {{ // Escaped {{
             fprintf(g_json_output_file, "%s\\n", json_string);
             cJSON_free(json_string);
         }} else {{ // Escaped {{
             fprintf(stderr, "EMUL_LVGL Error: Failed to serialize JSON unformatted either.\\n");
         }} // Escaped }}
    }} // Escaped }}

    fclose(g_json_output_file); g_json_output_file = NULL;
    cJSON_Delete(g_root_json); g_root_json = NULL;
    g_root_objects_array = NULL; g_styles_object = NULL; g_resources_object = NULL;
    g_pointer_map_count = 0; g_style_map_count = 0;
}} // Escaped }}

// --- LVGL Function Implementations ---
{function_implementations} // <<< UNESCAPED PLACEHOLDER

"""


# --- Main Generator Class (EmulLvglGenerator) ---

class EmulLvglGenerator:
    def __init__(self, api_file_path: str, includes: Dict[str, List[str]], excludes: Dict[str, List[str]], output_dir: str, cjson_include_path: str):
        self.api_file_path = api_file_path
        self.includes = includes # Store effective includes
        self.excludes = excludes # Store effective excludes
        self.output_dir = output_dir
        self.cjson_include_path = cjson_include_path
        self.api_data = self._load_api()
        self.filtered_api: Dict[str, Dict[str, Any]] = {}
        self.needed_types: Set[str] = set()

        # Identify potential "Value Constructor" functions
        # This is a heuristic, might need refinement or configuration
        self.constructor_functions: Dict[str, str] = {} # Map func_name -> C return type string
        self._identify_constructors()

    def _identify_constructors(self):
        """Identify functions likely used as value constructors based on name/return type."""
        # Heuristic: functions starting with lv_color_, lv_font_, lv_img_src_ returning non-void
        prefixes = ["lv_color_", "lv_font_", "lv_img_src_"]
        for func_def in self.api_data.get("functions", []):
            name = func_def.get("name")
            ret_type_info = func_def.get("type")
            ret_type_name = get_type_name(ret_type_info.get("type")) if ret_type_info else None

            if name and ret_type_name and ret_type_name != "void":
                if any(name.startswith(p) for p in prefixes):
                    # Get the C type string for the return value
                    c_ret_type = get_c_type_str(ret_type_info, self.api_data) # Pass api_data
                    if "/*" not in c_ret_type: # Ignore unknown types
                         self.constructor_functions[name] = c_ret_type
                         # print(f"Identified constructor: {name} -> {c_ret_type}")


    def _load_api(self) -> Dict[str, Any]:
        """Loads the LVGL API JSON file."""
        try:
            with open(self.api_file_path, 'r', encoding='utf-8') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"Error: API JSON file not found at '{self.api_file_path}'", file=sys.stderr)
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"Error: Failed to parse API JSON file: {e}", file=sys.stderr)
            sys.exit(1)

    def _initial_filter(self):
        """Performs the first pass filtering based on include/exclude lists."""
        print("Performing initial filtering...")
        # Make sure constructor functions needed for nesting are included if their callers are
        # (Dependency resolution should handle this if constructors are used as args)
        categories = ["functions", "enums", "structs", "unions", "typedefs", "macros", "variables"]
        for category in categories:
            self.filtered_api[category] = {}
            inc_list = self.includes.get(category, [])
            exc_list = self.excludes.get(category, [])
            if category == "structs":
                 exc_list = list(set(exc_list + ["lv_obj_t", "_lv_obj_t", "lv_style_t", "_lv_style_t"]))

            for item in self.api_data.get(category, []):
                name = item.get("name")
                if name and matches_filters(name, inc_list, exc_list):
                    self.filtered_api[category][name] = item

    def _find_dependencies_recursive(self, type_info: Dict[str, Any]):
        # (Unchanged from previous version)
        if not isinstance(type_info, dict): return
        base_type = get_base_type_info(type_info, self.api_data)
        if not base_type: return
        if base_type.get("_is_emulated_pointer") or base_type.get("_is_emulated_struct"): return

        json_type = base_type.get("json_type")
        name = base_type.get("name")

        if type_info.get("json_type") in ["pointer", "array", "ret_type", "arg", "field", "typedef"]:
             if type_info.get("json_type") == "typedef" and name and name not in self.needed_types:
                  typedef_def = find_api_definition(self.api_data, name)
                  if typedef_def:
                      self.needed_types.add(name)
                      self._find_dependencies_recursive(typedef_def.get("type"))
             elif type_info.get("json_type") != "typedef":
                 self._find_dependencies_recursive(type_info.get("type"))
             return

        if json_type in ["struct", "union", "enum", "function_pointer", "lvgl_type"]:
             if name and name not in self.needed_types:
                 actual_def = find_api_definition(self.api_data, name)
                 if actual_def and actual_def.get("json_type") != "function":
                     self.needed_types.add(name)
                     if actual_def.get("json_type") in ["struct", "union"]:
                         for field in actual_def.get("fields", []):
                             self._find_dependencies_recursive(field.get("type"))
                     elif actual_def.get("json_type") == "function_pointer":
                          self._find_dependencies_recursive(actual_def.get("type"))
                          for arg in actual_def.get("args", []):
                               self._find_dependencies_recursive(arg.get("type"))

    def _resolve_dependencies(self):
        # (Unchanged from previous version)
        print("Resolving dependencies...")
        original_needed = set(self.needed_types)
        active_constructors = {name for name in self.constructor_functions
                              if name in self.filtered_api.get("functions", {})}

        # Scan included functions for dependencies
        for func_name, item in self.filtered_api.get("functions", {}).items():
             self._find_dependencies_recursive(item.get("type")) # Return type
             for arg in item.get("args", []):
                 self._find_dependencies_recursive(arg.get("type"))
                 # If an arg type matches a constructor return type, ensure constructor is included
                 arg_type_str = get_c_type_str(arg.get("type"), self.api_data)
                 for cons_name, cons_ret_type in self.constructor_functions.items():
                     if arg_type_str == cons_ret_type and cons_name not in self.filtered_api.get("functions",{}):
                          cons_def = find_api_definition(self.api_data, cons_name)
                          if cons_def and cons_def.get("json_type") == "function":
                               inc_list = self.includes.get("functions", [])
                               exc_list = self.excludes.get("functions", [])
                               if matches_filters(cons_name, inc_list, exc_list):
                                    print(f"    Auto-including constructor dependency: {cons_name}")
                                    self.filtered_api.setdefault("functions", {})[cons_name] = cons_def
                                    # Recursively find dependencies of the newly added constructor
                                    self._find_dependencies_recursive(cons_def.get("type"))
                                    for carg in cons_def.get("args", []):
                                        self._find_dependencies_recursive(carg.get("type"))


        # Scan included structs/unions for dependencies
        for category in ["structs", "unions"]:
            for name, item in self.filtered_api.get(category, {}).items():
                if item.get("json_type") in ["struct", "union"]:
                    for field in item.get("fields", []):
                        self._find_dependencies_recursive(field.get("type"))

        # Add newly found needed types back to filtered_api
        added_count = 0
        current_needed = set(self.needed_types)
        newly_added = current_needed - original_needed

        added_in_pass = set()
        while added_in_pass: # Keep iterating until no new types are added
            added_in_pass = set()
            _debug("neWL", newly_added)
            for type_name in newly_added:
                if type_name in ["lv_obj_t", "lv_style_t"]: continue
                _debug("DEP:", type_name)
                definition = find_api_definition(self.api_data, type_name)
                if definition and definition.get("json_type") != "function":
                    category = definition.get("json_type") + "s"
                    if category == "function_pointers": category = "typedefs"

                    if category in self.filtered_api and type_name not in self.filtered_api[category]:
                        exc_list = self.excludes.get(category, [])
                        if type_name not in exc_list:
                           self.filtered_api[category][type_name] = definition
                           added_count += 1
                           added_in_pass.add(type_name)
                           # Scan dependencies of the newly added type
                           self._find_dependencies_recursive(definition)

            original_needed.update(added_in_pass) # Add to processed set
            current_needed = set(self.needed_types)
            newly_added = current_needed - original_needed # Find types added in *this* pass


        print(f"  Found {len(self.needed_types)} total needed C types (excluding emulated).")
        print(f"  Added {added_count} missing C dependencies.")


    def _generate_forward_declarations(self) -> str:
        # (Unchanged from previous version)
        decls = []
        struct_names = sorted([name for name in self.filtered_api.get("structs", {}).keys()
                               if name not in ["lv_obj_t", "lv_style_t"]])
        union_names = sorted(self.filtered_api.get("unions", {}).keys())

        for name in struct_names:
             item = self.filtered_api["structs"][name]
             if name.startswith("_"):
                  decls.append(f"typedef struct {name} {name[1:]};")
             else:
                  decls.append(f"typedef struct {name} {name};")
        for name in union_names:
            decls.append(f"typedef union {name} {name};")

        return "\n".join(decls)


    def _generate_typedefs(self) -> str:
        # (Unchanged from previous version)
        defs = []
        names_done = set(["lv_obj_t", "lv_style_t"])
        typedef_items = sorted(self.filtered_api.get("typedefs", {}).values(), key=lambda x: x.get("name", ""))

        for item in typedef_items:
            name = item.get("name")
            if not name or name in names_done: continue
            type_info = item.get("type")
            c_type_str = get_c_type_str(type_info, self.api_data) # Pass api_data

            is_func_ptr = False
            base_type_info_check = type_info
            # Simple check for function pointer type
            if base_type_info_check.get("json_type") == "pointer" and \
               base_type_info_check.get("type",{}).get("json_type") == "function_pointer":
                is_func_ptr = True
            elif base_type_info_check.get("json_type") == "function_pointer": # Direct function pointer typedef
                is_func_ptr = True


            if is_func_ptr:
                 fp_def = find_api_definition(self.api_data, name)
                 if not fp_def:
                     fp_def = next((fp for fp in self.api_data.get("function_pointers", []) if fp.get("name") == name), None)

                 if fp_def:
                     ret_type_str = get_c_type_str(fp_def.get("type"), self.api_data) # Pass api_data
                     args_list = []
                     args_info = fp_def.get("args", [])
                     if not args_info or (len(args_info) == 1 and get_type_name(args_info[0].get("type")) == "void"):
                         args_list.append("void")
                     else:
                         for i, arg in enumerate(args_info):
                              arg_type_str = get_c_type_str(arg.get("type"), self.api_data) # Pass api_data
                              args_list.append(f"{arg_type_str}")
                     args_str = ", ".join(args_list)
                     defs.append(f"typedef {ret_type_str} (*{name})({args_str});")
                     names_done.add(name)
                 else:
                     print(f"Warning: Could not find definition for function pointer typedef: {name}", file=sys.stderr)
                     defs.append(f"// typedef ??? (*{name})(???); // Definition not found")

            elif c_type_str and c_type_str != name and "/*" not in c_type_str:
                 base_type = get_base_type_info(type_info, self.api_data) # Pass api_data
                 if base_type and (base_type.get("_is_emulated_pointer") or base_type.get("_is_emulated_struct")):
                       names_done.add(name)
                       continue

                 if base_type and base_type.get("json_type") in ["struct", "union"] and not base_type.get("name"):
                     # Generate anon struct/union def only if it's not defined elsewhere
                     anon_def_str = self._generate_struct_union_definition(base_type, is_typedef=True)
                     if anon_def_str and "// Emulated" not in anon_def_str :
                         defs.append(f"typedef {anon_def_str} {name};")
                     elif anon_def_str : # It's an emulated type, skip typedef
                          pass
                     else: # Failed to generate definition
                          defs.append(f"// typedef /* Error generating anon def */ {name};")

                 else:
                     defs.append(f"typedef {c_type_str} {name};")
                 names_done.add(name)

        return "\n".join(d for d in defs if d)


    def _generate_enums(self) -> str:
        # (Unchanged from previous version)
        defs = []
        enum_items = sorted(self.filtered_api.get("enums", {}).values(), key=lambda x: x.get("name") or "")
        for item in enum_items:
            name = item.get("name")
            internal_name = item.get("name")
            definition = f"typedef enum {{\n"
            members = item.get("members", [])
            for member in members:
                m_name = member.get("name")
                m_value = member.get("value", "").replace("0x", "0x")
                if m_name:
                    definition += f"  {m_name}"
                    if m_value: definition += f" = {m_value}"
                    definition += ",\n"
            definition += f"}} {name or internal_name or f'anon_enum_{hash(str(item))}'};"
            defs.append(definition)
        return "\n\n".join(defs)

    def _generate_struct_union_definition(self, item: Dict[str, Any], is_typedef: bool = False) -> str:
        # (Unchanged from previous version)
        name = item.get("name")
        kind = item.get("json_type")
        if name in ["lv_obj_t", "_lv_obj_t", "lv_style_t", "_lv_style_t"]: return f"// Emulated {kind}: {name}"

        struct_union_tag = name or f"anon_{kind}_{hash(str(item))}"
        definition = f"{kind} {struct_union_tag} {{\n"
        fields = item.get("fields", [])
        if not fields and kind == "struct": definition += "  uint8_t _emul_placeholder;\n"

        for field in fields:
            f_name = field.get("name")
            f_type_info = field.get("type")
            f_bitsize = field.get("bitsize")
            if f_name and f_type_info:
                f_type_str = get_c_type_str(f_type_info, self.api_data, is_arg_or_field=True) # Pass api_data
                array_dims = ""
                current_type = f_type_info
                while isinstance(current_type, dict) and current_type.get("json_type") == "array":
                    array_dims += f"[{current_type.get('dim', '')}]"
                    current_type = current_type.get("type")
                definition += f"  {f_type_str} {f_name}{array_dims}"
                if f_bitsize: definition += f" : {f_bitsize}"
                definition += ";\n"
        definition += "}"
        return definition


    def _generate_structs_unions(self, kind="struct") -> str:
        # (Unchanged from previous version)
         defs = []
         category = kind + "s"
         items = sorted(self.filtered_api.get(category, {}).values(), key=lambda x: x.get("name") or "")
         defined_tags = set() # Prevent duplicate struct/union definitions

         for item in items:
             name = item.get("name")
             if name in ["lv_obj_t", "_lv_obj_t", "lv_style_t", "_lv_style_t"]: continue

             is_fwd_decl_placeholder = False # Simplification: assume all listed structs/unions should be defined
             definition_tag = name # Use typedef name as the tag to define

             if (item.get("fields") or not is_fwd_decl_placeholder) and definition_tag not in defined_tags:
                  if name:
                       definition = self._generate_struct_union_definition(item)
                       defs.append(definition + ";")
                       defined_tags.add(definition_tag)
                  else: # Should be handled by typedef generation for anonymous
                      pass

         return "\n\n".join(defs)


    def _generate_macros(self) -> str:
        # (Unchanged from previous version)
        defs = []
        macro_items = sorted(self.filtered_api.get("macros", {}).values(), key=lambda x: x.get("name", ""))
        for item in macro_items:
            name = item.get("name")
            params = item.get("params")
            initializer = item.get("initializer")
            if not name: continue
            definition = f"#define {name}"
            if params is not None: definition += f"({','.join(params)})"
            if initializer is not None: definition += f" {initializer}"
            defs.append(definition)
        return "\n".join(defs)

    def _generate_function_prototypes(self) -> str:
        # (Unchanged from previous version - uses updated get_c_type_str)
        protos = []
        func_items = sorted(self.filtered_api.get("functions", {}).values(), key=lambda x: x.get("name", ""))
        for item in func_items:
            name = item.get("name")
            ret_type_info = item.get("type")
            args_info = item.get("args", [])
            if not name: continue

            ret_type_str = get_c_type_str(ret_type_info, self.api_data) # Pass api_data
            args_list = []

            if not args_info or (len(args_info) == 1 and get_type_name(args_info[0].get("type")) == "void"):
                args_list.append("void")
            else:
                for i, arg in enumerate(args_info):
                    arg_name = arg.get("name") or f"arg{i}"
                    arg_type_info = arg.get("type")
                    arg_type_str = get_c_type_str(arg_type_info, self.api_data) # Pass api_data

                    array_dims = ""
                    current_type = arg_type_info
                    while isinstance(current_type, dict) and current_type.get("json_type") == "array":
                         array_dims += f"[{current_type.get('dim', '')}]"
                         current_type = current_type.get("type")

                    if arg_type_str == "...": args_list.append("...")
                    elif arg_name and arg_type_str and "/*" not in arg_type_str:
                         args_list.append(f"{arg_type_str} {arg_name}{array_dims}")
                    elif arg_type_str and "/*" not in arg_type_str:
                         args_list.append(arg_type_str)

            args_str = ", ".join(args_list)
            protos.append(f"{ret_type_str} {name}({args_str});")
        return "\n".join(protos)


    def _get_marshal_info(self, type_info: Dict[str, Any]) -> Tuple[Optional[str], bool]:
         """
         Determines marshalling approach.
         Returns: (marshal_func_name_or_None, is_constructor_result)
         is_constructor_result is True if the value should be treated as a pre-marshalled cJSON* call representation.
         """
         c_type_str = get_c_type_str(type_info, self.api_data) # Get C type string

         # Check if this C type is the return type of a known constructor function
         for func_name, constructor_ret_type in self.constructor_functions.items():
             # Need to compare canonical C types (e.g., handle "const lv_color_t" vs "lv_color_t")
             # Simple comparison for now, might need normalization
             if c_type_str == constructor_ret_type:
                 # Check if the *specific function* returning this is included
                 if func_name in self.filtered_api.get("functions", {}):
                     return (None, True) # Indicates it's a constructor result

         # If not a constructor result, find the direct marshaller function
         base_type_info = get_base_type_info(type_info, self.api_data)
         if not base_type_info: name = get_type_name(type_info) # Try direct name if resolution failed
         else: name = base_type_info.get("name")

         # --- Find appropriate marshal_xxx function based on type name ---
         # Pointers first
         if is_pointer_type(type_info):
             pointee_type = get_pointee_type_info(type_info)
             pointee_base_info = get_base_type_info(pointee_type, self.api_data) if pointee_type else None
             if pointee_base_info and pointee_base_info.get("name") == "char":
                 return ("marshal_string", False)
             else:
                 # Includes lv_obj_t* (ignored), lv_style_t*, lv_font_t*, other struct*, void* etc.
                 return ("marshal_c_pointer", False) # Use specific name for C pointers

         # Specific known types by value
         if name == "lv_color_t": return ("marshal_lv_color_t_value", False) # Marshal actual value

         # Primitives and standard types
         if name in ["int", "signed int", "int32_t", "int16_t", "int8_t", "lv_coord_t", "lv_style_int_t"]: return ("marshal_int", False)
         if name in ["unsigned int", "uint32_t", "uint16_t", "uint8_t", "lv_opa_t", "lv_res_t"]: return ("marshal_uint", False)
         if name in ["long", "signed long", "long int"]: return ("marshal_long", False)
         if name in ["unsigned long", "unsigned long int", "size_t"]: return ("marshal_ulong", False)
         if name == "int64_t": return ("marshal_int64", False)
         if name == "uint64_t": return ("marshal_uint64", False)
         if name == "float": return ("marshal_float", False)
         if name == "double": return ("marshal_double", False)
         if name == "bool": return ("marshal_bool", False)
         if name in ["char", "signed char", "unsigned char"]: return ("marshal_int", False) # char as number

         # Enums (check if name corresponds to a known enum typedef)
         enum_def = find_api_definition(self.api_data, name)
         if enum_def and enum_def.get("json_type") == "enum": return ("marshal_int", False)

         # Fallback for unhandled types (e.g., structs by value)
         print(f"Warning: No direct marshaller found for type name '{name}' (C type '{c_type_str}'). Treating as unsupported.", file=sys.stderr)
         return (None, False)


    def _generate_function_implementations(self) -> str:
        """Generates C function implementations using cJSON and handling nested calls."""
        impls = []
        func_items = sorted(self.filtered_api.get("functions", {}).values(), key=lambda x: x.get("name", ""))

        for item in func_items:
            name = item.get("name")
            ret_type_info = item.get("type")
            args_info = item.get("args", [])
            c_ret_type_str = get_c_type_str(ret_type_info, self.api_data) # Pass api_data
            is_constructor = name in self.constructor_functions

            if not name: continue

            # --- Function Signature ---
            args_list_sig = []
            args_list_call = []
            is_varargs = False
            if not args_info or (len(args_info) == 1 and get_type_name(args_info[0].get("type")) == "void"):
                args_list_sig.append("void")
            else:
                for i, arg in enumerate(args_info):
                    arg_name = arg.get("name") or f"arg{i}"
                    arg_type_info = arg.get("type")
                    arg_type_str = get_c_type_str(arg_type_info, self.api_data) # Pass api_data

                    array_dims = "" # TODO: Array handling for nested calls?
                    if "/* unknown" in arg_type_str or "/* ERROR" in arg_type_str:
                         print(f"Warning: Skipping argument '{arg_name}' in {name} due to unresolvable type '{arg_type_str}'")
                         continue # Skip arg if type is bad

                    if arg_type_str == "...":
                        args_list_sig.append("...")
                        is_varargs = True
                    else:
                        args_list_sig.append(f"{arg_type_str} {arg_name}{array_dims}")
                        args_list_call.append(arg_name)

            args_sig_str = ", ".join(args_list_sig)
            func_sig = f"{c_ret_type_str} {name}({args_sig_str})"
            impls.append(func_sig + " {{ // Escaped {{")

            # --- Function Body ---
            body = []

            # --- Marshal Arguments Recursively ---
            # Only needed if this function modifies state or is a constructor itself
            if not is_varargs: # Varargs marshalling is too complex for now
                body.append(f"  // Marshal arguments for {name}")
                body.append(f"  cJSON* marshalled_args = cJSON_CreateArray();")
                body.append(f"  if (!marshalled_args) {{ fprintf(stderr, \"EMUL_LVGL Error: Failed to create args array for {name}\\n\"); {'return 0;' if c_ret_type_str != 'void' else ''} }} // Escaped {{ }}") # Handle return for non-void

                for i, arg_name in enumerate(args_list_call):
                    arg_info = args_info[i]
                    arg_type_info = arg_info.get("type")
                    marshal_func, is_constructor_result = self._get_marshal_info(arg_type_info)

                    body.append(f"  // Arg {i}: {arg_name} (Type: {get_c_type_str(arg_type_info, self.api_data)}) -> Marshal: {marshal_func}, IsConstructor: {is_constructor_result}") # Debug comment

                    if is_constructor_result:
                        # Argument is the result of another constructor call (cJSON* cast to C type)
                        # Cast it back to cJSON* and add directly. DO NOT DELETE IT.
                        body.append(f"    cJSON* arg{i}_json = (cJSON*){arg_name};")
                        # Add a clone if ownership is needed by array, or add directly if lifetime is okay?
                        # Let's assume the call representation is persistent enough for the caller's scope.
                        # If the call representation was immediately added to another object, AddItemToArray is fine.
                        # If the constructor just returned it, it might be cleaned up too early.
                        # Safest bet: constructor adds its result to a temporary holding pool? Complex.
                        # Pragmatic bet: Add directly, assume it lives long enough.
                        body.append(f"    if (arg{i}_json) {{ cJSON_AddItemToArray(marshalled_args, arg{i}_json); }} else {{ cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }}") # Escaped {{ }}
                    elif marshal_func:
                        # Use the direct marshaller which creates a *new* cJSON object
                        # Handle potential void* cast needed for generic marshaller (future idea)
                        body.append(f"    cJSON* arg{i}_json = {marshal_func}({arg_name});")
                        body.append(f"    if (arg{i}_json) {{ cJSON_AddItemToArray(marshalled_args, arg{i}_json); }} else {{ cJSON_AddItemToArray(marshalled_args, cJSON_CreateNull()); }}") # Escaped {{ }}
                    else:
                        # Cannot marshal this argument
                        body.append(f"    cJSON_AddItemToArray(marshalled_args, cJSON_CreateString(\"<unsupported_arg>\"));")
                        body.append(f"    fprintf(stderr, \"EMUL_LVGL Warning: Unsupported argument type for '{arg_name}' in {name}\\n\");")

                # body now contains code to populate marshalled_args array
            else: # Varargs case
                 body.append("  // Varargs marshalling not implemented for state building.")
                 body.append("  cJSON* marshalled_args = cJSON_CreateArray(); // Create empty array")
                 # Potentially add known args before ellipsis here if needed


            # --- Specific Function Logic ---

            if is_constructor:
                body.append(f"  // Create call representation for constructor {name}")
                body.append(f"  cJSON* call_obj = cJSON_CreateObject();")
                body.append(f"  if (!call_obj) {{ cJSON_Delete(marshalled_args); return ({c_ret_type_str})NULL; }} // Escaped {{ }}") # Return null casted
                body.append(f"  cJSON_AddStringToObject(call_obj, \"emul_call\", \"{name}\");")
                body.append(f"  cJSON_AddItemToObject(call_obj, \"args\", marshalled_args); // takes ownership of marshalled_args")
                body.append(f"  // Return the cJSON call representation cast to the C return type")
                # Ensure return type is not void before casting
                if c_ret_type_str != "void":
                     body.append(f"  return ({c_ret_type_str})call_obj;")
                else: # Should not happen for constructors usually
                     body.append(f"  cJSON_Delete(call_obj); // Delete if void return")
                     body.append(f"  return;")


            elif name.endswith("_create") and "lv_obj_t" in c_ret_type_str:
                 # ... (Object creation logic remains similar, parenting needs marshalled_args[0]?) ...
                 widget_type = name.replace("lv_", "").replace("_create", "")
                 body.append(f"  // Create widget '{widget_type}'")
                 body.append(f"  lv_obj_t node = cJSON_CreateObject();")
                 body.append(f"  if (!node) {{ cJSON_Delete(marshalled_args); return NULL; }}") # Escaped {{ }}
                 body.append(f"  cJSON_AddStringToObject(node, \"type\", \"{widget_type}\");")
                 body.append(f"  char id_str[64]; snprintf(id_str, sizeof(id_str), \"{POINTER_ID_PREFIX}obj_%p\", (void*)node);")
                 body.append(f"  cJSON_AddStringToObject(node, \"id\", id_str);")
                 # Parenting - parent is the first C argument, which is lv_obj_t (cJSON*)
                 if args_list_call:
                     parent_arg_name = args_list_call[0]
                     body.append(f"  lv_obj_t parent_node = {parent_arg_name};")
                     body.append(f"  if (parent_node) {{ // Escaped {{")
                     body.append(f"      cJSON* children_array = cJSON_GetObjectItem(parent_node, \"children\");")
                     body.append(f"      if (!children_array) {{ children_array = cJSON_AddArrayToObject(parent_node, \"children\"); }} // Escaped {{ }}")
                     body.append(f"      if (children_array) {{ cJSON_AddItemToArray(children_array, cJSON_Duplicate(node, true)); }} // Add a deep copy to parent ")
                     # Original node might be added to roots or returned, duplicate prevents double-free if parent is deleted.
                     # If AddItemToArray fails, node is not deleted by cJSON, we might need to.
                     body.append(f"      else {{ fprintf(stderr, \"EMUL_LVGL Error: Failed to add child array in {name}\\n\"); /* Don't delete node yet */ }} // Escaped {{ }}")
                     body.append(f"  }} else {{ // Escaped {{")
                     body.append(f"      if (g_root_objects_array) {{ cJSON_AddItemToArray(g_root_objects_array, cJSON_Duplicate(node, true)); }} // Add deep copy to roots")
                     body.append(f"      else {{ fprintf(stderr, \"EMUL_LVGL Error: Root array missing in {name}\\n\"); /* Don't delete node yet */ }}") # Escaped {{ }}
                     body.append(f"  }} // Escaped }}")
                 else: # No parent argument? Add to roots by default
                      body.append(f"  if (g_root_objects_array) {{ cJSON_AddItemToArray(g_root_objects_array, cJSON_Duplicate(node, true)); }}") # Escaped {{ }}
                      body.append(f"  else {{ fprintf(stderr, \"EMUL_LVGL Error: Root array missing in {name}\\n\"); }}") # Escaped {{ }}

                 body.append(f"  cJSON_Delete(marshalled_args); // Args usually not needed for create itself")
                 body.append(f"  return node;") # Return the original cJSON* cast to lv_obj_t

            elif name == "lv_style_init":
                 # ... (Style init logic mostly same, uses C pointer arg directly) ...
                 style_ptr_arg = args_list_call[0]
                 body.append(f"  // Initialize style object for {style_ptr_arg}")
                 body.append(f"  if (!{style_ptr_arg}) {{ cJSON_Delete(marshalled_args); return; }} // Escaped {{ }}")
                 body.append(f"  if (get_style_json_node({style_ptr_arg})) {{ /* Reset existing? */ cJSON_Delete(marshalled_args); return; }} // Escaped {{ }}")
                 body.append(f"  cJSON *style_node = cJSON_CreateObject();")
                 body.append(f"  if (!style_node) {{ cJSON_Delete(marshalled_args); return; }} // Escaped {{ }}")
                 body.append(f"  cJSON_AddStringToObject(style_node, \"type\", \"style\");")
                 body.append(f"  const char* style_id = get_pointer_id({style_ptr_arg});")
                 body.append(f"  if (!style_id) {{ cJSON_Delete(style_node); cJSON_Delete(marshalled_args); return; }} // Escaped {{ }}")
                 body.append(f"  cJSON_AddStringToObject(style_node, \"id\", style_id);")
                 body.append(f"  if (g_style_map_count < MAX_STYLE_MAP_ENTRIES) {{ // Escaped {{")
                 body.append(f"      g_style_map[g_style_map_count].style_ptr = {style_ptr_arg};")
                 body.append(f"      g_style_map[g_style_map_count].json_node = style_node;")
                 body.append(f"      g_style_map_count++;")
                 body.append(f"  }} else {{ // Escaped {{")
                 body.append(f"      fprintf(stderr, \"EMUL_LVGL Warning: Style map full!\\n\");")
                 body.append(f"      cJSON_Delete(style_node); cJSON_Delete(marshalled_args); return;")
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  const char* base_id = style_id + strlen(POINTER_ID_PREFIX);")
                 body.append(f"  emul_lvgl_register_pointer({style_ptr_arg}, base_id);")
                 body.append(f"  if (g_styles_object) {{ // Escaped {{")
                 body.append(f"      cJSON_AddItemToObject(g_styles_object, style_id, style_node); // Transfer ownership")
                 body.append(f"  }} else {{ // Escaped {{")
                 body.append(f"      fprintf(stderr, \"EMUL_LVGL Error: Global styles object missing! Style node will leak.\\n\");")
                 # Don't delete style_node here as ownership transfer failed
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  cJSON_Delete(marshalled_args);") # Args not used for init itself

            elif name == "lv_style_reset":
                # ... (Style reset logic same) ...
                 style_ptr_arg = args_list_call[0]
                 body.append(f"  // Reset style object {style_ptr_arg}")
                 body.append(f"  cJSON* style_node = get_style_json_node({style_ptr_arg});")
                 body.append(f"  if (style_node) {{ // Escaped {{")
                 body.append(f"     cJSON *child = style_node->child, *next_child = NULL;")
                 body.append(f"     while (child) {{ // Escaped {{")
                 body.append(f"         next_child = child->next;")
                 body.append(f"         if (child->string && strcmp(child->string, \"type\") != 0 && strcmp(child->string, \"id\") != 0) {{ // Escaped {{")
                 body.append(f"             cJSON_DeleteItemFromObjectCaseSensitive(style_node, child->string);")
                 body.append(f"         }} // Escaped }}")
                 body.append(f"         child = next_child;")
                 body.append(f"     }} // Escaped }}")
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  cJSON_Delete(marshalled_args);") # Args not used

            elif name.startswith("lv_style_set_"):
                 # ... (Style set logic similar, uses Nth item from marshalled_args) ...
                 prop_name = name.replace("lv_style_set_", "")
                 target_cjson_expr = f"get_style_json_node({args_list_call[0]})" # Get style node
                 body.append(f"  // Set style property '{prop_name}'")
                 body.append(f"  cJSON* target_node = {target_cjson_expr};")
                 body.append(f"  if (target_node) {{ // Escaped {{")
                 body.append(f"      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)")
                 body.append(f"      if (value_json) {{ // Escaped {{")
                 body.append(f"          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1); // Detach before adding")
                 body.append(f"          if (cJSON_HasObjectItem(target_node, \"{prop_name}\")) {{ // Escaped {{")
                 body.append(f"              cJSON_ReplaceItemInObjectCaseSensitive(target_node, \"{prop_name}\", detached_value);")
                 body.append(f"          }} else {{ // Escaped {{")
                 body.append(f"              cJSON_AddItemToObject(target_node, \"{prop_name}\", detached_value);")
                 body.append(f"          }} // Escaped }}")
                 body.append(f"      }} // Escaped }}")
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  cJSON_Delete(marshalled_args); // Clean up array and remaining items")

            elif name.startswith("lv_obj_set_style_local_"):
                 # ... (Local style set logic similar, uses marshalled_args items) ...
                 prop_name = name.replace("lv_obj_set_", "")
                 target_cjson_expr = args_list_call[0] # First arg is lv_obj_t (cJSON*)
                 body.append(f"  // Set local style property '{prop_name}'")
                 body.append(f"  lv_obj_t target_node = {target_cjson_expr};")
                 body.append(f"  if (target_node) {{ // Escaped {{")
                 body.append(f"      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)")
                 body.append(f"      cJSON* state_json = cJSON_GetArrayItem(marshalled_args, 2); // State is 3rd arg (index 2)")
                 body.append(f"      if (value_json && state_json) {{ // Escaped {{")
                 body.append(f"          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);")
                 body.append(f"          cJSON* detached_state = cJSON_DetachItemFromArray(marshalled_args, 1); // Index shifts after detach")
                 body.append(f"          cJSON* prop_array = cJSON_CreateArray();")
                 body.append(f"          if (prop_array) {{ // Escaped {{")
                 body.append(f"              cJSON_AddItemToArray(prop_array, detached_value);")
                 body.append(f"              cJSON_AddItemToArray(prop_array, detached_state);")
                 body.append(f"              if (cJSON_HasObjectItem(target_node, \"{prop_name}\")) {{ // Escaped {{")
                 body.append(f"                  cJSON_ReplaceItemInObjectCaseSensitive(target_node, \"{prop_name}\", prop_array);")
                 body.append(f"              }} else {{ // Escaped {{")
                 body.append(f"                  cJSON_AddItemToObject(target_node, \"{prop_name}\", prop_array);")
                 body.append(f"              }} // Escaped }}")
                 body.append(f"          }} else {{ // Escaped {{")
                 body.append(f"              cJSON_Delete(detached_value); cJSON_Delete(detached_state); // Clean up if array fails")
                 body.append(f"          }} // Escaped }}")
                 body.append(f"      }} // Escaped }}")
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  cJSON_Delete(marshalled_args);")

            elif name == "lv_obj_add_style":
                 # ... (Add style logic similar, uses marshalled_args[1] for style ID) ...
                 target_cjson_expr = args_list_call[0]
                 body.append(f"  // Add style reference")
                 body.append(f"  lv_obj_t target_node = {target_cjson_expr};")
                 body.append(f"  if (target_node) {{ // Escaped {{")
                 body.append(f"      cJSON* style_id_json = cJSON_GetArrayItem(marshalled_args, 1); // Style is 2nd arg (index 1)")
                 body.append(f"      if (style_id_json && cJSON_IsString(style_id_json)) {{ // Escaped {{")
                 body.append(f"          cJSON* styles_array = cJSON_GetObjectItem(target_node, \"styles\");")
                 body.append(f"          if (!styles_array) {{ styles_array = cJSON_AddArrayToObject(target_node, \"styles\"); }} // Escaped {{ }}")
                 body.append(f"          if (styles_array) {{ // Escaped {{")
                 # Avoid adding duplicates?
                 body.append(f"              bool found = false;")
                 body.append(f"              cJSON* existing_style; ")
                 body.append(f"              cJSON_ArrayForEach(existing_style, styles_array) {{ // Escaped {{")
                 body.append(f"                  if (cJSON_IsString(existing_style) && strcmp(existing_style->valuestring, style_id_json->valuestring) == 0) {{ found = true; break; }} // Escaped {{ }}")
                 body.append(f"              }} // Escaped }}")
                 body.append(f"              if (!found) {{ cJSON_AddItemToArray(styles_array, cJSON_Duplicate(style_id_json, false)); }} // Add shallow copy of string") # Escaped {{ }}
                 body.append(f"          }} // Escaped }}")
                 body.append(f"      }} // Escaped }}")
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  cJSON_Delete(marshalled_args);")

            elif name.startswith(("lv_obj_set_", "lv_label_set_", "lv_btn_set_", "lv_img_set_")):
                 # ... (Common setter logic, uses marshalled_args[1]) ...
                 match = re.match(r"lv_\w+_set_([a-zA-Z0-9_]+)", name)
                 prop_name = match.group(1) if match else name.split("set_")[-1]
                 target_cjson_expr = args_list_call[0] # First arg is lv_obj_t
                 body.append(f"  // Set property '{prop_name}'")
                 body.append(f"  lv_obj_t target_node = {target_cjson_expr};")
                 body.append(f"  if (target_node) {{ // Escaped {{")
                 body.append(f"      cJSON* value_json = cJSON_GetArrayItem(marshalled_args, 1); // Value is 2nd arg (index 1)")
                 body.append(f"      if (value_json) {{ // Escaped {{")
                 body.append(f"          cJSON* detached_value = cJSON_DetachItemFromArray(marshalled_args, 1);")
                 body.append(f"          if (cJSON_HasObjectItem(target_node, \"{prop_name}\")) {{ // Escaped {{")
                 body.append(f"              cJSON_ReplaceItemInObjectCaseSensitive(target_node, \"{prop_name}\", detached_value);")
                 body.append(f"          }} else {{ // Escaped {{")
                 body.append(f"              cJSON_AddItemToObject(target_node, \"{prop_name}\", detached_value);")
                 body.append(f"          }} // Escaped }}")
                 body.append(f"      }} // Escaped }}")
                 body.append(f"  }} // Escaped }}")
                 body.append(f"  cJSON_Delete(marshalled_args);")

            else:
                 # Default / Other non-constructor, non-state-modifying functions
                 body.append(f"  // Function '{name}' - Not a constructor or direct state modifier.")
                 body.append(f"  cJSON_Delete(marshalled_args); // Clean up marshalled args")

            # --- Return Value (Handle dummy returns) ---
            if not is_constructor and c_ret_type_str != "void":
                body.append(f"  // Default return for {name}")
                if "lv_obj_t" in c_ret_type_str: body.append("  return NULL;")
                elif is_pointer_type(ret_type_info): body.append("  return NULL;") # Includes lv_style_t* etc
                elif "bool" in c_ret_type_str: body.append("  return false;")
                elif "lv_result_t" in c_ret_type_str or "lv_res_t" in c_ret_type_str: body.append("  return 1; // LV_RESULT_OK")
                elif "lv_color_t" in c_ret_type_str: body.append("  lv_color_t dummy_color = {{0}}; return dummy_color;")
                else: body.append("  return 0;") # Int, enum, etc.


            impls.append("\n".join(body))
            impls.append("}} // Escaped }}") # End function body

        return "\n\n".join(impls)


    def generate_files(self):
        """Generates the .h, .c, and config files."""
        print("Generating C files...")
        os.makedirs(self.output_dir, exist_ok=True)

        # --- 1. Generate Config File ---
        config_data = {
            "api_json_file": self.api_file_path,
            "output_dir": self.output_dir,
            "cjson_include_path": self.cjson_include_path,
            "includes": self.includes,
            "excludes": self.excludes,
            "constructor_functions": list(self.constructor_functions.keys()) # Save identified constructors
        }
        config_path = os.path.join(self.output_dir, "gen_wrapper_cfg.json")
        try:
            with open(config_path, 'w', encoding='utf-8') as f:
                json.dump(config_data, f, indent=4)
            print(f"  Generated config: {config_path}")
        except Exception as e:
            print(f"Error writing config file {config_path}: {e}", file=sys.stderr)


        # --- 2. Generate C Files ---
        forward_decls = self._generate_forward_declarations()
        typedef_defs = self._generate_typedefs()
        enum_defs = self._generate_enums()
        struct_defs = self._generate_structs_unions(kind="struct")
        union_defs = self._generate_structs_unions(kind="union")
        macro_defs = self._generate_macros()
        function_protos = self._generate_function_prototypes()
        function_impls = self._generate_function_implementations()

        # Generate Header File
        header_path = os.path.join(self.output_dir, "emul_lvgl.h")
        header_content = C_HEADER_TEMPLATE.format(
            forward_declarations=forward_decls,
            typedef_defs=typedef_defs,
            enum_defs=enum_defs,
            struct_defs=struct_defs,
            union_defs=union_defs,
            macro_defs=macro_defs,
            function_prototypes=function_protos,
        )
        with open(header_path, 'w', encoding='utf-8') as f:
            f.write(header_content)
        print(f"  Generated header: {header_path}")

        # Generate Source File
        source_path = os.path.join(self.output_dir, "emul_lvgl.c")
        source_content = C_SOURCE_TEMPLATE.format( # Removed extra timestamp include here
            function_implementations=function_impls
        )
        with open(source_path, 'w', encoding='utf-8') as f:
            f.write(source_content)
        print(f"  Generated source: {source_path}")

    def run(self):
        """Runs the full generation process."""
        self._initial_filter()
        self._resolve_dependencies()
        self.generate_files() # Generates C files and config file
        print("Done.")

# --- Command Line Argument Parsing ---
def parse_list_arg(arg: Optional[str]) -> List[str]:
    """Parses comma-separated string into a list."""
    return [item.strip() for item in arg.split(',')] if arg else []

def main():
    parser = argparse.ArgumentParser(description="Generate LVGL Emulation C Library (State Builder) from API JSON.")
    # (Arguments same as before)
    parser.add_argument("api_json_file", help="Path to the LVGL API JSON definition file.")
    parser.add_argument("-o", "--output-dir", default="emul_lvgl_state_output",
                        help="Directory to output generated emul_lvgl.h/c and config files (default: emul_lvgl_state_output).")
    parser.add_argument("--cjson-include", default="cjson", help="Subdirectory/path for cJSON headers relative to include paths (e.g., 'cjson' or 'vendor/cjson/include')")
    parser.add_argument("--include-funcs", help="Comma-separated list of function prefixes/names to include.")
    parser.add_argument("--include-enums", help="Comma-separated list of enum prefixes/names to include.")
    parser.add_argument("--include-structs", help="Comma-separated list of struct prefixes/names to include (excluding lv_obj_t, lv_style_t).")
    parser.add_argument("--include-typedefs", help="Comma-separated list of typedef prefixes/names to include (excluding lv_obj_t, lv_style_t).")
    parser.add_argument("--include-macros", help="Comma-separated list of macro prefixes/names to include.")
    parser.add_argument("--exclude-funcs", help="Comma-separated list of function prefixes/names to exclude.")
    parser.add_argument("--exclude-enums", help="Comma-separated list of enum prefixes/names to exclude.")
    parser.add_argument("--exclude-structs", help="Comma-separated list of struct prefixes/names to exclude.")
    parser.add_argument("--exclude-typedefs", help="Comma-separated list of typedef prefixes/names to exclude.")
    parser.add_argument("--exclude-macros", help="Comma-separated list of macro prefixes/names to exclude.")

    args = parser.parse_args()

    # --- Process Includes/Excludes ---
    # Start with defaults
    includes = {k: list(v) for k, v in DEFAULT_INCLUDES.items()} # Deep copy
    excludes = {k: list(v) for k, v in DEFAULT_EXCLUDES.items()} # Deep copy

    # Apply command-line overrides/additions
    if args.include_funcs: includes["functions"] = list(set(includes["functions"] + parse_list_arg(args.include_funcs))) # Use set for potential deduplication if needed
    if args.include_enums: includes["enums"] = list(set(includes["enums"] + parse_list_arg(args.include_enums)))
    if args.include_structs: includes["structs"] = list(set(includes["structs"] + parse_list_arg(args.include_structs)))
    if args.include_typedefs: includes["typedefs"] = list(set(includes["typedefs"] + parse_list_arg(args.include_typedefs)))
    if args.include_macros: includes["macros"] = list(set(includes["macros"] + parse_list_arg(args.include_macros)))

    if args.exclude_funcs: excludes["functions"] = list(set(excludes["functions"] + parse_list_arg(args.exclude_funcs)))
    if args.exclude_enums: excludes["enums"] = list(set(excludes["enums"] + parse_list_arg(args.exclude_enums)))
    if args.exclude_structs: excludes["structs"] = list(set(excludes["structs"] + parse_list_arg(args.exclude_structs)))
    if args.exclude_typedefs: excludes["typedefs"] = list(set(excludes["typedefs"] + parse_list_arg(args.exclude_typedefs)))
    if args.exclude_macros: excludes["macros"] = list(set(excludes["macros"] + parse_list_arg(args.exclude_macros)))


    generator = EmulLvglGenerator(
        api_file_path=args.api_json_file,
        includes=includes, # Pass the final processed lists
        excludes=excludes, # Pass the final processed lists
        output_dir=args.output_dir,
        cjson_include_path=args.cjson_include
    )

    generator.run()

# --- Entry Point ---
if __name__ == "__main__":
    main()
