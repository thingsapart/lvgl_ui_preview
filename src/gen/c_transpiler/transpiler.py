# gen/c_transpiler/transpiler.py
import logging
import json
import re
# import copy # Not strictly needed if StaticCTranspiler handles its own copies
from pathlib import Path # For robust path operations
from typing import Any, Dict, Union

# Corrected imports to use the new structure
from ..ui_generator_base import cjson_object_hook, CJSONObject 
from .static_transpiler import StaticCTranspiler
# Removed unused import: from generator import C_COMMON_DEFINES
# Removed unused import: import type_utils (StaticCTranspiler handles its own)

logger = logging.getLogger(__name__)

# Old CJSONObject and cjson_object_hook are removed as they are imported from ui_generator_base
# Old CTranspiler class is removed as its logic is now in StaticCTranspiler


def generate_c_transpiled_ui(api_info: Dict[str, Any], 
                             ui_spec_path: str, 
                             output_dir: str, 
                             output_filename_base: str = "ui_transpiled"):
    """
    Generates C code from a JSON UI specification using StaticCTranspiler.
    Writes the output to .c and .h files.
    """
    logger.info(f"Starting C transpilation for UI spec: {ui_spec_path}")
    logger.info(f"Output directory: {output_dir}, Filename base: {output_filename_base}")

    ui_spec_data: Union[Dict, CJSONObject, list, None] = None
    try:
        with open(ui_spec_path, 'r', encoding='utf-8') as f:
            # Use cjson_object_hook from ui_generator_base
            ui_spec_data = json.load(f, object_pairs_hook=cjson_object_hook)
    except FileNotFoundError:
        logger.error(f"UI specification file '{ui_spec_path}' not found.")
        return
    except json.JSONDecodeError as e:
        logger.error(f"Error decoding JSON from UI spec file '{ui_spec_path}': {e}")
        return
    except Exception as e: 
        logger.error(f"An unexpected error occurred while loading UI spec from {ui_spec_path}: {e}")
        return

    if not ui_spec_data:
        logger.error("No UI specification data loaded or data is empty. Aborting.")
        return
    
    if not isinstance(ui_spec_data, (list, CJSONObject)): # Check against imported CJSONObject
        logger.error(f"Loaded UI spec data is not a list or CJSONObject (type: {type(ui_spec_data)}). Cannot process.")
        return

    # Instantiate the new StaticCTranspiler
    transpiler = StaticCTranspiler(api_info, ui_spec_data)
    
    # Define the C parameter name for the parent LVGL object in the generated function
    parent_param_c_name = "screen_parent" 
    
    # Call process_ui to populate the C code lists within the transpiler
    # This method is from BaseUIGenerator and drives the processing.
    transpiler.process_ui(
        initial_parent_ref=parent_param_c_name, # For StaticCTranspiler, this is the C var name
        initial_context_data=CJSONObject([]), # Start with an empty CJSONObject context
        initial_named_path_prefix=None 
    )

    # Sanitize filename base for use in C function names and header guards
    safe_filename_base = re.sub(r'[^a-zA-Z0-9_]', '_', output_filename_base)
    c_function_name = f"create_ui_{safe_filename_base}"

    # Get the C code and Header code from StaticCTranspiler
    # build_c_function_content in StaticCTranspiler returns a tuple (c_code_str, h_code_str)
    c_code_str, h_code_str = transpiler.build_c_function_content(
        function_name=c_function_name, 
        parent_param_name=parent_param_c_name 
    )

    # Ensure output directory exists using pathlib
    Path(output_dir).mkdir(parents=True, exist_ok=True)

    c_file_path = Path(output_dir) / f"{output_filename_base}.c"
    h_file_path = Path(output_dir) / f"{output_filename_base}.h"

    try:
        with open(c_file_path, 'w', encoding='utf-8') as f:
            f.write(c_code_str)
        logger.info(f"Successfully wrote C code to: {c_file_path}")

        with open(h_file_path, 'w', encoding='utf-8') as f:
            f.write(h_code_str)
        logger.info(f"Successfully wrote H header to: {h_file_path}")
    except IOError as e:
        logger.error(f"Error writing C/H files: {e}")
    except Exception as e: 
        logger.error(f"An unexpected error occurred while writing output files: {e}")


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG) 
    
    # Create a dummy api_info.json (replace with actual path or loading mechanism)
    dummy_api_info_path = "dummy_api_info_transpiler.json" # Use a different name to avoid conflict
    dummy_api_data = {
        "lv_obj_create": {"args": ["lv_obj_t*"], "return": "lv_obj_t*"},
        "lv_label_create": {"args": ["lv_obj_t*"], "return": "lv_obj_t*"},
        "lv_button_create": {"args": ["lv_obj_t*"], "return": "lv_obj_t*"},
        "lv_style_init": {"args": ["lv_style_t*"]},
        "lv_obj_set_width": {"args": ["lv_obj_t*", "lv_coord_t"]},
        "lv_obj_set_height": {"args": ["lv_obj_t*", "lv_coord_t"]},
        "lv_label_set_text": {"args": ["lv_obj_t*", "const char*"]},
        "lv_obj_add_style": {"args": ["lv_obj_t*", "lv_style_t*", "lv_style_selector_t"]},
        "lv_style_set_bg_color": {"args": ["lv_style_t*", "lv_color_t"]},
        "lv_obj_set_grid_dsc_array": {"args": ["lv_obj_t*", "const lv_coord_t[]", "const lv_coord_t[]"]},
        # Add other functions as needed by StaticCTranspiler's LVGL_CONSTANTS_MAP or format_value logic
    }
    with open(dummy_api_info_path, 'w') as f:
        json.dump(dummy_api_data, f, indent=2)

    dummy_ui_spec_path = "dummy_ui_spec_transpiler.json" 
    dummy_ui_data = { # Using CJSONObject structure for testing if loaded directly
        "type": "obj",
        "id": "@screen1",
        "children": [
            {
                "type": "style",
                "id": "@main_style",
                "bg_color": "#FF0000"
            },
            {
                "type": "label",
                "id": "@lbl1",
                "text": "Hello Transpiled World!",
                "add_style": ["@main_style", "PART.MAIN"] 
            },
            {
                "type": "button",
                "id": "@btn1",
                "width": 120,
                "height": {"call": "MY_CUSTOM_HEIGHT_MACRO", "args": [50, 2]}, 
                "children": [
                    {"type": "label", "text": "Click Static"}
                ]
            },
            {
                "type": "grid",
                "id": "@my_grid",
                "width": 200, "height": 150,
                "cols": [100, "LV_GRID_FR(1)"], 
                "rows": [50, "LV_GRID_FR(1)"],
                "children": [
                    {"type": "label", "id": "@grid_child1", "text": "G1"},
                    {"type": "label", "id": "@grid_child2", "text": "G2"}
                ]
            }
        ]
    }
    with open(dummy_ui_spec_path, 'w') as f:
        json.dump(dummy_ui_data, f, indent=2) # Save as standard JSON

    output_directory = Path("./gen_output_transpiler")
    
    api_info_loaded = {}
    try:
        with open(dummy_api_info_path, 'r') as f:
            api_info_loaded = json.load(f)
    except Exception as e:
        logger.error(f"Failed to load dummy_api_info for test: {e}")

    logger.info("--- Running Static C Transpiler Test ---")
    if api_info_loaded: 
        generate_c_transpiled_ui(api_info_loaded, dummy_ui_spec_path, str(output_directory), "test_static_ui")
        logger.info("--- Static C Transpiler Test Finished ---")
        logger.info(f"Generated files are in {output_directory.resolve()}")
    else:
        logger.error("Skipping Static C Transpiler Test due to missing api_info.")

    # Clean up dummy files (optional)
    # import os
    # os.remove(dummy_api_info_path)
    # os.remove(dummy_ui_spec_path)
    logger.info(f"Note: Dummy files {dummy_api_info_path} and {dummy_ui_spec_path} created for testing.")
