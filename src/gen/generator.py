# generator.py
import argparse
import json
import logging
import os
from pathlib import Path # Ensure Path is imported

import api_parser
import type_utils
from pathlib import Path # Ensure Path is imported
from code_gen import invocation, unmarshal, registry, renderer # Assuming these are in code_gen subpackage

# Basic Logging Setup
logging.basicConfig(level=logging.INFO, format='%(levelname)s: [%(filename)s:%(lineno)d] %(message)s')
logger = logging.getLogger(__name__)

DEFAULT_MACRO_NAMES_TO_EXPORT = [
    "LV_SIZE_CONTENT",
    "LV_COORD_MAX",
    "LV_COORD_MIN",
    "LV_GRID_CONTENT",    # Often LV_COORD_MAX or a value derived from it
    "LV_FLEX_CONTENT",    # Similar to LV_GRID_CONTENT
    "LV_CHART_POINT_NONE", # Often LV_COORD_MIN
    "LV_RADIUS_CIRCLE",   # E.g., 0x7FFF or similar large value
    "LV_IMG_ZOOM_NONE",   # Typically 256
    "LV_BTNMATRIX_BTN_ID_NONE", # Typically 0xFFFF
    "LV_ANIM_REPEAT_INFINITE", # Typically 0xFFFF or 0xFFFFFFFF
    "LV_ANIM_PLAYTIME_INFINITE", # Typically 0xFFFFFFFF
    "LV_OPA_TRANSP", "LV_OPA_COVER", # Test filtering (likely enums)
    "LV_ALIGN_DEFAULT", "LV_ALIGN_CENTER", "LV_ALIGN_OUT_TOP_LEFT", # Test filtering (likely enums)
    "LV_STATE_DEFAULT", "LV_STATE_FOCUSED", "LV_STATE_ANY", # Test filtering (likely enums)
    "LV_PART_MAIN", "LV_PART_ANY", # Test filtering (likely enums)
    "LV_GRID_TEMPLATE_LAST", "LV_GRID_FR_1", "LV_GRID_FR_2", "LV_GRID_FR_3", "LV_GRID_FR_4",
    "LV_GRID_FR_5", "LV_GRID_FR_10"
]

# --- C File Templates ---

C_HEADER_TEMPLATE = """
#ifndef LVGL_JSON_RENDERER_H
#define LVGL_JSON_RENDERER_H

#ifdef __cplusplus
extern "C" {{
#endif

#include <lvgl.h>
#include <cjson/cJSON.h> // Needed for cJSON types and free()
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // For logging fprintf
#include <stdlib.h> // For logging malloc/free (if used in json_node_to_string)

// --- Logging Macros (Provide basic implementation or allow override) ---
// Helper to stringify JSON node (defined in .c file)
// NOTE: Caller must free the returned string using cJSON_free()
char* json_node_to_string(cJSON *node); // Exposed for potential external use? Maybe keep static.

#ifndef LOG_ERR
#define LOG_ERR(fmt, ...) fprintf(stderr, "ERROR: [%s:%d] " fmt "\\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef LOG_ERR_JSON
#define LOG_ERR_JSON(node, fmt, ...) do {{ \\
        char* _json_str = json_node_to_string(node); \\
        fprintf(stderr, "ERROR: [%s:%d] " fmt " [Near JSON: %s]\\n", __FILE__, __LINE__, ##__VA_ARGS__, _json_str ? _json_str : " N/A"); \\
        if (_json_str) cJSON_free(_json_str); \\
    }} while(0)
#endif
#ifndef LOG_WARN
#define LOG_WARN(fmt, ...) fprintf(stderr, "WARN: [%s:%d] " fmt "\\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef LOG_WARN_JSON
#define LOG_WARN_JSON(node, fmt, ...) do {{ \\
        char* _json_str = json_node_to_string(node); \\
        fprintf(stderr, "WARN: [%s:%d] " fmt " [Near JSON: %s]\\n", __FILE__, __LINE__, ##__VA_ARGS__, _json_str ? _json_str : " N/A"); \\
        if (_json_str) cJSON_free(_json_str); \\
    }} while(0)
#endif
#ifndef LOG_INFO
#define LOG_INFO(fmt, ...) printf("INFO: [%s:%d] " fmt "\\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef LOG_DEBUG
#ifdef LVGL_JSON_RENDERER_DEBUG
#define LOG_DEBUG(fmt, ...) printf("DEBUG: [%s:%d] " fmt "\\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) (void)0
#endif
#endif

#define LV_MALLOC lv_malloc
#define LV_FREE lv_free
#define LV_GRID_FR_1 LV_GRID_FR(1)
#define LV_GRID_FR_2 LV_GRID_FR(2)
#define LV_GRID_FR_3 LV_GRID_FR(3)
#define LV_GRID_FR_4 LV_GRID_FR(4)
#define LV_GRID_FR_5 LV_GRID_FR(5)
#define LV_GRID_FR_10 LV_GRID_FR(10)

// Forward declare if not in lvgl.h for some reason, or ensure lvgl.h is included first.
// These might be part of LVGL's standard headers, but good to be aware of.
extern lv_obj_t *lv_screen_active(void);


// --- Public API ---

/**
 * @brief Adds a custom string-to-integer mapping for enum unmarshaling.
 * Allows overriding or extending generated enum values at runtime.
 * The 'name' string must persist for the lifetime of its use or be a literal.
 *
 * @param name The string representation of the enum member.
 * @param value The integer value of the enum member.
 * @return true if added successfully, false if the user enum table is full.
 */
bool lvgl_json_add_user_enum_mapping(const char *name, int value);

/**
 * @brief Clears all runtime user-added enum mappings.
 */
void lvgl_json_clear_user_enum_mappings(void);

/**
 * @brief Renders a UI described by a cJSON object tree.
 *
 * Parses the JSON definition and creates corresponding LVGL objects and applies properties.
 * Assumes cJSON library is linked. Requires LVGL to be initialized beforehand.
 *
 * @param root_json The root cJSON object (must be an array of objects or a single object).
 * @param implicit_root_parent The LVGL parent object for all top-level elements defined in the JSON.
 *                             If NULL, lv_screen_active() will be used.
 * @return true if rendering was successful, false otherwise. Errors are logged.
 */
bool lvgl_json_render_ui(cJSON *root_json, lv_obj_t *implicit_root_parent);


char *lvgl_json_register_str(const char *name);
void lvgl_json_register_str_clear();

void lvgl_json_register_clear();

/**
 * @brief Registers a pointer with a given name. Used for referencing objects/styles by ID ('@name').
 *
 * @param name The name to register the pointer under (should not include '@').
 * @param ptr_type_name The C type name of the pointer being registered (e.g., \"lv_style_t\", \"lv_obj_t\").
 * @param ptr The pointer to register.
 */
void lvgl_json_register_ptr(const char *name, const char *ptr_type_name, void *ptr);

/**
 * @brief Retrieves a previously registered pointer by name.
 *
 * @param name The name of the pointer to retrieve (should not include '@').
 * @param expected_ptr_type_name The expected C type name of the pointer (e.g., \"lv_style_t *\", \"lv_font_t *\").\n"
 *                                 If a pointer type (ending with '*'), the base type will be checked against the registered base type.
 *                                 Can be NULL to skip type checking (not recommended).
 * @return The registered pointer, or NULL if not found.
 */
void* lvgl_json_get_registered_ptr(const char *name, const char *expected_ptr_type_name);

/**
 * @brief Clears all entries from the pointer registry.
 * Behavior depends on registry implementation (e.g., frees memory in hash map).
 */
void lvgl_json_registry_clear();

/**
 * @brief Generates a JSON string of predefined macro values.
 *
 * This function checks a list of known macro names (configured at generation time).
 * For each name, if it's defined as a C macro AND it's not found in the
 * generated enum table (i.e., it's a true macro, not an enum member),
 * its name and value are added to the JSON string.
 *
 * The primary use case is to obtain values for constants like LV_SIZE_CONTENT,
 * LV_COORD_MAX, etc., which are macros, not enums.
 * The output JSON string can be copied and used to create a "string_values.json"
 * file for tools that might need these mappings.
 *
 * @return A dynamically allocated JSON string. The caller MUST free this string
 *         using `cJSON_free()`. Returns NULL or an error JSON string on failure.
 */
char* lvgl_json_generate_values_json(void);

// --- Custom Managed Object Creator Prototypes ---
{custom_creator_prototypes}

#ifdef __cplusplus
}} /*extern "C"*/
#endif

#endif /* LVGL_JSON_RENDERER_H */
"""

C_SOURCE_TEMPLATE = """
#include "lvgl_json_renderer.h"
#include <string.h> // For strcmp, strchr, strncpy, strlen etc.
#include <stdio.h>  // For snprintf, logging
#include <stdlib.h> // For strtoul, strtol

// LVGL functions used internally (ensure they are linked)
// extern lv_obj_t * lv_screen_active(void); // Declared in lvgl.h
// extern void *lv_malloc(size_t size); // Declared in lv_mem.h / lv_conf.h
// extern void lv_free(void *ptr);     // Declared in lv_mem.h / lv_conf.h
// extern char *lv_strdup(const char *str); // Declared in lv_mem.h / lv_conf.h


// --- Logging Helper ---

// Convert cJSON node to a compact string for logging (caller must free result)
char* json_node_to_string(cJSON *node) {{
    if (!node) {{
        // Use standard malloc/strdup if lv_strdup isn't guaranteed
        char *null_str = (char*)malloc(5); // "NULL" + '\\0'
        if (null_str) strcpy(null_str, "NULL");
        return null_str;
    }}
    // PrintUnformatted is more compact for logs
    char *str = cJSON_PrintUnformatted(node);
    if (!str) {{
        char *err_str = (char*)malloc(40); // "{{\"error\":\"Failed to print JSON\"}}" + '\\0'
        if (err_str) strcpy(err_str, "{{\\"error\\":\\"Failed to print JSON\\"}}");
        return err_str;
    }}
    // Optional: Truncate very long strings if needed for logs
    // const int max_len = 120;
    // if (str && strlen(str) > max_len) {{ str[max_len-3] = '.'; str[max_len-2] = '.'; str[max_len-1] = '.'; str[max_len] = '\\0'; }}
    return str; // cJSON_Print... allocates, caller must use cJSON_free
}}

// --- Configuration ---
// Add any compile-time configuration here if needed

// --- Global Context ---

static cJSON* g_current_render_context = NULL;

static void set_current_context(cJSON* new_context) {{
    // LOG_DEBUG("Setting context from %p to %p", (void*)g_current_render_context, (void*)new_context);
    g_current_render_context = new_context;
}}

static cJSON* get_current_context(void) {{
    return g_current_render_context;
}}

// --- Invocation Table ---
{invocation_table_def}

// --- Forward declaration ---
static const invoke_table_entry_t* find_invoke_entry(const char *name);
static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent);

// --- Pointer Registry Implementation ---
{registry_code}

// --- Enum Unmarshaling ---
{enum_unmarshal_code}

// --- Macro Values JSON Exporter ---
{macro_values_exporter_code}

// --- Primitive Unmarshalers ---
{primitive_unmarshal_code}

// --- Coordinate (lv_coord_t) Unmarshaler ---
{coord_unmarshal_code}

// --- Custom Unmarshalers (#color, @ptr) ---
{custom_unmarshal_code}

// --- Invocation Helper Functions ---
{invocation_helpers_code}

// --- Invocation Table ---
{invocation_table_code}

// --- Function Lookup Implementation ---
{find_function_code}

// --- Main Value Unmarshaler Implementation ---
{main_unmarshaler_code}

// --- Custom Managed Object Creators ---
{custom_creators_code}

// --- JSON UI Renderer Implementation ---
{renderer_code}

"""

C_TRANSPILE_OUTPUT_DIR_DEFAULT = "output_c_transpiled"
C_TRANSPILE_UI_SPEC_DEFAULT = "ui.json" # Default name for the UI spec file


def generate_macro_values_exporter_c_code(macro_names_list):
    # This C code will rely on djb2_hash_c, compare_generated_enum_hash,
    # g_generated_enum_table, and G_GENERATED_ENUM_TABLE_SIZE being defined earlier in the C file.

    helper_is_enum_c_code = """
// Helper to check if a name is a known generated enum member
static bool is_name_an_enum(const char *name_to_check) {
    if (!name_to_check) return false;
    // Ensure djb2_hash_c and compare_generated_enum_hash are available (defined earlier as static)
    uint32_t input_hash = djb2_hash_c(name_to_check);

    if (G_GENERATED_ENUM_TABLE_SIZE == 0) return false; // No enums to check against

    const generated_enum_entry_t *entry_ptr = (const generated_enum_entry_t *)bsearch(
        &input_hash,
        g_generated_enum_table,
        G_GENERATED_ENUM_TABLE_SIZE,
        sizeof(generated_enum_entry_t),
        compare_generated_enum_hash);

    if (!entry_ptr) return false; // No matching hash

    // Hash matches, now check name. Collisions are contiguous.
    // Go to the start of the hash collision block
    while (entry_ptr > g_generated_enum_table && (entry_ptr - 1)->hash == input_hash) {
        entry_ptr--;
    }
    // Iterate through all entries with the same hash
    while (entry_ptr < (g_generated_enum_table + G_GENERATED_ENUM_TABLE_SIZE) && entry_ptr->hash == input_hash) {
        if (strcmp(entry_ptr->original_name, name_to_check) == 0) {
            return true; // Exact name match
        }
        entry_ptr++;
    }
    return false; // Hash matched, but no exact name match in collision group
}
"""

    main_func_c_code_parts = [
        helper_is_enum_c_code,
        "\nchar* lvgl_json_generate_values_json(void) {",
        "    cJSON *json_root = cJSON_CreateObject();",
        "    if (!json_root) {",
        "        LOG_ERR(\"Failed to create cJSON root for macro values.\");",
        "        return NULL;",
        "    }\n",
    ]

    for macro_name in macro_names_list:
        # Ensure macro_name is a valid C identifier
        if not all(c.isalnum() or c == '_' for c in macro_name) or not macro_name:
            logger.warning(f"Skipping potentially invalid macro name for C code generation: '{macro_name}'")
            continue
        
        main_func_c_code_parts.append(f"    #ifdef {macro_name}")
        main_func_c_code_parts.append(f"    if (!is_name_an_enum(\"{macro_name}\")) {{")
        # cJSON_AddNumberToObject takes a double.
        # Casting the macro to double should handle most integer types correctly.
        # Parentheses around ({macro_name}) are important for complex macro expressions.
        main_func_c_code_parts.append(f"        cJSON_AddNumberToObject(json_root, \"{macro_name}\", (double)({macro_name}));")
        main_func_c_code_parts.append(f"    }}")
        main_func_c_code_parts.append(f"    #else")
        main_func_c_code_parts.append(f"    // LOG_DEBUG(\"Macro {macro_name} not defined, skipped for JSON output.\");")
        main_func_c_code_parts.append(f"    #endif // {macro_name}")
        main_func_c_code_parts.append("") 

    main_func_c_code_parts.extend([
        "    char *json_string = cJSON_PrintUnformatted(json_root);",
        "    cJSON_Delete(json_root);",
        "",
        "    if (!json_string) {",
        "        LOG_ERR(\"Failed to print cJSON object for macro values.\");",
        "        char *err_str = (char*)LV_MALLOC(30); // Use LV_MALLOC",
        "        if(err_str) snprintf(err_str, 30, \"{{\\\"error\\\":\\\"print_failed\\\"}}\");",
        "        else { /* Malloc failed, cannot report error string */ }",
        "        return err_str; // Caller still uses cJSON_free or lv_free",
        "    }",
        "    return json_string; // Caller must cJSON_free() this (or lv_free if cJSON uses lv_mem).",
        "}"
    ])
    return "\n".join(main_func_c_code_parts)

def main():
    parser = argparse.ArgumentParser(description="LVGL JSON UI Renderer Library Generator")
    parser.add_argument("-a", "--api-json", required=True, help="Path to the LVGL API JSON definition file.")
    parser.add_argument("-o", "--output-dir", default="output", help="Directory to write the generated C library files.")
    parser.add_argument("--debug", action="store_true", help="Enable debug logging macros in generated code.")
    parser.add_argument(
        "--mode",
        choices=["preview", "c_transpile"],
        default="preview",
        help="Generation mode: 'preview' (JSON interpreter library) or 'c_transpile' (direct C code)."
    )
    parser.add_argument(
        "--ui-spec",
        default=None, # Default to None, require if mode is c_transpile
        help=f"Path to the UI specification JSON file (required for 'c_transpile' mode). Defaults to {C_TRANSPILE_UI_SPEC_DEFAULT} if not provided and mode is c_transpile."
    )
    parser.add_argument("-m",
        "--macro-names-list",
        type=str,
        default=None,
        help="Comma-separated list of macro names to include in the generated values JSON string function. Overrides default list."
    )
    parser.add_argument("-s",
        "--string-values-json",
        default=None,
        help="Path to a JSON file containing macro string-to-value mappings. These values will override or extend enums from the API definition for unmarshaling."
    )
    # Add arguments for include/exclude lists here if needed
    args = parser.parse_args()

    if args.mode == "c_transpile":
        if args.ui_spec is None:
            args.ui_spec = C_TRANSPILE_UI_SPEC_DEFAULT
            logger.info(f"--ui-spec not provided for c_transpile mode, defaulting to {args.ui_spec}")
        if not Path(args.ui_spec).exists():
            logger.error(f"UI specification file '{args.ui_spec}' not found (required for 'c_transpile' mode).")
            return 1
        # Adjust output directory for C transpiler mode if default is used
        if args.output_dir == "output": # Default for preview mode
            args.output_dir = C_TRANSPILE_OUTPUT_DIR_DEFAULT
            logger.info(f"Output directory for c_transpile mode set to: {args.output_dir}")

    logger.info(f"Parsing API definition from: {args.api_json}")
    
    output_path = Path(args.output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    # Pass include/exclude lists from args if implemented
    # api_info = api_parser.parse_api(args.api_json) # Old line

    string_values_override = {}
    if args.string_values_json:
        try:
            with open(args.string_values_json, 'r') as f_sv:
                string_values_override = json.load(f_sv)
            logger.info(f"Loaded {len(string_values_override)} string-value overrides from {args.string_values_json}")
        except Exception as e:
            logger.error(f"Failed to load string_values.json from {args.string_values_json}: {e}. Proceeding without overrides.")
            string_values_override = {} # Ensure it's a dict

    api_info = api_parser.parse_api(args.api_json, string_values_override=string_values_override)
    if not api_info:
        logger.critical("Failed to parse API information. Exiting.")
        return 1

    if args.mode == "preview":
        generate_preview_mode(api_info, args, output_path)
    elif args.mode == "c_transpile":
        # Ensure the c_transpiler module can be imported
        try:
            from c_transpiler import transpiler as c_transpiler_module
            c_transpiler_module.generate_c_transpiled_ui(api_info, args.ui_spec, output_path)
        except ImportError as e:
            logger.error(f"Could not import C transpiler module: {e}")
            logger.error("Please ensure 'gen/c_transpiler/transpiler.py' exists and is structured correctly.")
            return 1
        except Exception as e:
            logger.error(f"Error during C transpilation: {e}", exc_info=True)
            return 1
    else:
        logger.error(f"Unknown mode: {args.mode}")
        return 1

    logger.info("Generation complete.")
    return 0

def generate_preview_mode(api_info, args, output_dir):
    """Generates the files for the 'preview' (JSON interpreter) mode."""
    logger.info("Generating files for 'preview' mode...")

    # Original logic from main() for preview mode starts here
    
    # --- Prepare macro names for JSON value exporter ---
    macro_names_for_exporter = DEFAULT_MACRO_NAMES_TO_EXPORT
    if args.macro_names_list:
        macro_names_for_exporter = [name.strip() for name in args.macro_names_list.split(',') if name.strip()]
        logger.info(f"Using custom list of {len(macro_names_for_exporter)} macros for JSON value exporter.")
    else:
        logger.info(f"Using default list of {len(macro_names_for_exporter)} macros for JSON value exporter.")
    
    logger.info("Generating C code for macro values JSON exporter...")
    macro_values_exporter_c = generate_macro_values_exporter_c_code(macro_names_for_exporter)

    # --- Generate Code Sections ---
    logger.info("Generating pointer registry...")
    registry_c = registry.generate_registry()

    logger.info("Generating enum unmarshalers...")
    # Pass the new 'hashed_and_sorted_enum_members' and the old 'enum_members' map
    enum_unmarshal_c = unmarshal.generate_enum_unmarshalers(
        api_info['hashed_and_sorted_enum_members'],
        api_info['enum_members'] # The old map, can be used as a fallback or for type heuristics if needed
    )
    logger.info("Generating primitive unmarshalers...")
    primitive_unmarshal_c = unmarshal.generate_primitive_unmarshalers()

    logger.info("Generating coordinate unmarshaler...") # New step
    coord_unmarshal_c = unmarshal.generate_coord_unmarshaler(api_info)

    logger.info("Generating custom unmarshalers...")
    custom_unmarshal_c = unmarshal.generate_custom_unmarshalers(api_info)

    logger.info("Grouping functions by signature...")
    signatures = invocation.generate_invoke_signatures(api_info['functions'])

    logger.info("Generating invocation helpers...")
    invocation_helpers_c, signature_map = invocation.generate_invocation_helpers(signatures, api_info)

    logger.info("Generating invocation table...")
    invocation_table_def = invocation.generate_invoke_table_def()
    invocation_table_c = invocation.generate_invoke_table(api_info['functions'], signature_map)

    logger.info("Generating function lookup...")
    find_function_c = invocation.generate_find_function()

    logger.info("Generating main unmarshaler...")
    main_unmarshaler_c = unmarshal.generate_main_unmarshaler()

    logger.info("Finding init functions...")
    init_functions = registry.find_init_functions(api_info['functions'])

    logger.info("Generating custom creators...")
    custom_creators_c = registry.generate_custom_creators(init_functions, api_info)

    # Build map { "style": "lv_style_create_managed", ... } for renderer
    custom_creators_map = {}
    custom_creator_prototypes_h = ""
    for func in init_functions:
         arg_type, _, _ = func['_resolved_arg_types'][0]
         widget_name = type_utils.lvgl_type_to_widget_name(arg_type)
         if widget_name:
              creator_func_name = f"{arg_type[:-2]}_create_managed"
              custom_creators_map[widget_name] = creator_func_name
              custom_creator_prototypes_h += f"/** @brief Creates a managed {arg_type} identified by name. Allocates memory. */\n"
              custom_creator_prototypes_h += f"extern {arg_type}* {creator_func_name}(const char *name);\n"


    logger.info("Generating renderer logic...")
    renderer_c = renderer.generate_renderer(custom_creators_map)

    # --- Assemble Files ---
    logger.info("Assembling C source file...")
    c_source_content = C_SOURCE_TEMPLATE.format(
        registry_code=registry_c,
        enum_unmarshal_code=enum_unmarshal_c,
        primitive_unmarshal_code=primitive_unmarshal_c,
        coord_unmarshal_code=coord_unmarshal_c, # Added
        custom_unmarshal_code=custom_unmarshal_c,
        invocation_helpers_code=invocation_helpers_c,
        invocation_table_code=invocation_table_c,
        invocation_table_def=invocation_table_def,
        find_function_code=find_function_c,
        main_unmarshaler_code=main_unmarshaler_c,
        custom_creators_code=custom_creators_c,
        renderer_code=renderer_c,
        macro_values_exporter_code=macro_values_exporter_c,
    )

    logger.info("Assembling C header file...")
    c_header_content = C_HEADER_TEMPLATE.format(
        custom_creator_prototypes=custom_creator_prototypes_h
    )

    # Add debug define if requested
    if args.debug:
        c_header_content = "#define LVGL_JSON_RENDERER_DEBUG 1\n" + c_header_content


    # --- Write Files ---
    header_path = output_dir / "lvgl_json_renderer.h"
    source_path = output_dir / "lvgl_json_renderer.c"

    logger.info(f"Writing header file to: {header_path}")
    with open(header_path, "w") as f:
        f.write(c_header_content)

    logger.info(f"Writing source file to: {source_path}")
    with open(source_path, "w") as f:
        f.write(c_source_content)


if __name__ == "__main__":
    # Set higher log level for libraries during generation itself
    logging.getLogger("api_parser").setLevel(logging.WARNING)
    logging.getLogger("type_utils").setLevel(logging.WARNING)
    # Add others if needed
    exit(main())