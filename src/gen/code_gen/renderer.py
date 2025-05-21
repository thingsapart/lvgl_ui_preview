# code_gen/renderer.py
import logging
import json
from typing import Any, Dict

# Assuming ui_generator_base and runtime_renderer are in sibling directories or packages
from ..ui_generator_base import cjson_object_hook, CJSONObject
from .runtime_renderer import DynamicRuntimeRenderer

logger = logging.getLogger(__name__)

# Placeholder for loading api_info. In a real scenario, this would parse
# an XML/JSON/header file that describes the LVGL API.
def load_api_info(api_spec_path: str = "api_info.json") -> Dict[str, Any]:
    """Loads API information from a JSON file."""
    try:
        with open(api_spec_path, 'r', encoding='utf-8') as f:
            api_info = json.load(f)
        logger.info(f"Successfully loaded API info from {api_spec_path}")
        return api_info
    except FileNotFoundError:
        logger.warning(f"API info file {api_spec_path} not found. Using empty API info.")
        return {} # Return empty dict if not found, allows basic operation
    except json.JSONDecodeError:
        logger.error(f"Error decoding JSON from API info file {api_spec_path}. Using empty API info.")
        return {}
    except Exception as e:
        logger.error(f"An unexpected error occurred while loading API info from {api_spec_path}: {e}. Using empty API info.")
        return {}

def generate_renderer(custom_creators_map: Dict[str, str], 
                      ui_spec_json_path: str = "lv_def.json",
                      api_spec_json_path: str = "api_info.json") -> str:
    """
    Generates the C code for parsing the JSON UI and rendering it dynamically at runtime.
    This function now uses DynamicRuntimeRenderer.
    """
    logger.info(f"Starting C code generation for runtime renderer with UI spec: {ui_spec_json_path}")

    # 1. Load API information
    api_info = load_api_info(api_spec_json_path)
    if not api_info:
        logger.warning("API info is empty. Code generation might be incomplete or incorrect.")

    # 2. Load UI specification data using cjson_object_hook
    ui_spec_data: Any = None
    try:
        with open(ui_spec_json_path, 'r', encoding='utf-8') as f:
            ui_spec_data = json.load(f, object_pairs_hook=cjson_object_hook)
        logger.info(f"Successfully loaded UI spec data from {ui_spec_json_path}")
    except FileNotFoundError:
        logger.error(f"UI specification file '{ui_spec_json_path}' not found. Cannot generate renderer.")
        return "// Error: UI specification file not found."
    except json.JSONDecodeError as e:
        logger.error(f"Error decoding JSON from UI spec file '{ui_spec_json_path}': {e}. Cannot generate renderer.")
        return f"// Error: JSON decode error in UI spec file: {e}"
    except Exception as e:
        logger.error(f"An unexpected error occurred while loading UI spec from {ui_spec_json_path}: {e}")
        return f"// Error: Unexpected error loading UI spec: {e}"

    if not isinstance(ui_spec_data, (list, CJSONObject)):
        logger.error(f"Loaded UI spec data is not a list or CJSONObject (type: {type(ui_spec_data)}). Cannot process.")
        return "// Error: UI spec data is not in the expected format (list or object at root)."

    # 3. Instantiate DynamicRuntimeRenderer
    # DynamicRuntimeRenderer will use the Python representation of ui_spec_data (CJSONObject)
    # and generate C code that processes this data via cJSON at runtime.
    renderer = DynamicRuntimeRenderer(api_info, ui_spec_data, custom_creators_map)

    # 4. Call the main processing method of the renderer.
    # This populates the C code lists within the DynamicRuntimeRenderer instance.
    # The parameters passed to process_ui here are for the Python-side processing,
    # which in turn generates C code. The C function parameters (like root_json_c_param_name)
    # are handled internally by DynamicRuntimeRenderer when it builds the C function shells.
    
    # The initial_parent_lvgl_c_var for the top-level process_ui call should be the
    # C parameter name that will be used in the generated lvgl_json_render_ui C function.
    # Similar for initial_cjson_context_var.
    # The Python ui_spec_data is already set in the renderer.
    # The Python context (initial_py_context_data) can be an empty CJSONObject or None.
    
    # These are the C parameter names for the main generated function `lvgl_json_render_ui`
    c_root_json_param = "root_json_c_var" 
    c_implicit_parent_param = "implicit_root_parent_c_var"
    c_initial_context_param = "initial_context_c_var"

    # The DynamicRuntimeRenderer's process_ui method will be called by its own
    # build_c_file_content method, which sets up the main C function structure.
    # The Python-side process_ui needs to be called to prepare the internal C code snippets
    # that will be inserted into that structure.
    # The initial call to process_ui should refer to the C parameters of the
    # main C function `lvgl_json_render_ui`.

    # The `process_ui` in `BaseUIGenerator` is designed for the Python-side traversal.
    # `DynamicRuntimeRenderer` will need a way to translate this Python traversal
    # into C code that performs a *runtime* traversal of the cJSON data.
    # The current `DynamicRuntimeRenderer` methods (create_entity, call_setter, etc.)
    # are designed to generate C code for specific operations *within* the runtime C functions.
    # The main loop/logic of `render_json_node_runtime` and `apply_setters_and_attributes_runtime`
    # needs to be constructed by `build_c_file_content`.

    # The call to `renderer.process_ui` here drives the Python-side processing,
    # which in turn calls the C-generating methods of DynamicRuntimeRenderer.
    # These C-generating methods add lines to `renderer.c_code`.
    # The initial call to `process_ui` sets the stage for the root of the UI.
    # The `initial_parent_ref` for the very first call to `process_ui` would be
    # the main parent parameter of the `lvgl_json_render_ui` C function.
    
    # The `DynamicRuntimeRenderer.process_ui` will use `self.ui_spec_data` (the Python CJSONObject)
    # and generate C code based on it. The C code will, at runtime, take `root_json_c_param_name`
    # (a cJSON*) as its input.
    # The `initial_parent_ref` in `process_ui` for the Python side corresponds to the
    # `parent_lvgl_c_var` argument in `create_entity` (which is a C variable name string).
    # So, the first call should use the C parameter name.

    initial_context_py = CJSONObject() # An empty context for the start if none specific
    
    # This call processes the Python CJSONObject `ui_spec_data` and generates C code snippets
    # into `renderer.c_code` sections.
    renderer.process_ui(
        initial_parent_ref=c_implicit_parent_param, # This is the C variable name string
        initial_context_data=initial_context_py,    # This is the CJSONObject for context
        initial_named_path_prefix=None
    )
    # Note: DynamicRuntimeRenderer's methods will need to internally manage/access 
    # the C variable name for the context (e.g., 'initial_context_c_var') if needed,
    # as BaseUIGenerator.process_ui doesn't pass it explicitly with this signature.
    # This might require storing it on self in DynamicRuntimeRenderer or adapting methods.

    # 5. Call a method on the renderer instance to get the fully assembled C code string.
    # The `build_c_file_content` method in `DynamicRuntimeRenderer` is responsible for
    # creating the overall C file structure, including the main C functions
    # `lvgl_json_render_ui`, `render_json_node_runtime`, `apply_setters_and_attributes_runtime`,
    # and inserting the C code snippets generated by the `process_ui` call above.
    final_c_code = renderer.build_c_file_content() # build_c_file_content already knows the standard names

    logger.info("Successfully generated C code for runtime renderer.")
    return final_c_code

# Example usage (optional, for testing)
if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    
    # Create dummy api_info.json for testing
    dummy_api_info = {
        "lv_label_set_text": {"args": ["lv_obj_t*", "const char*"]},
        "lv_obj_set_width": {"args": ["lv_obj_t*", "lv_coord_t"]},
        "lv_obj_set_height": {"args": ["lv_obj_t*", "lv_coord_t"]},
        "lv_obj_create": {"args": ["lv_obj_t*"], "return": "lv_obj_t*"},
        "lv_label_create": {"args": ["lv_obj_t*"], "return": "lv_obj_t*"},
        "lv_button_create": {"args": ["lv_obj_t*"], "return": "lv_obj_t*"},
        # Add more dummy API entries as needed by DynamicRuntimeRenderer's initial C code or processing
    }
    with open("api_info.json", "w") as f:
        json.dump(dummy_api_info, f, indent=2)

    # Create dummy lv_def.json for testing
    dummy_ui_spec = {
        "type": "obj",
        "id": "@screen1",
        "children": [
            {
                "type": "label",
                "id": "@lbl1",
                "text": "Hello World"
            },
            {
                "type": "button",
                "id": "@btn1",
                "width": 100,
                "height": 50,
                "children": [
                    {"type": "label", "text": "Click Me"}
                ]
            }
        ]
    }
    with open("lv_def.json", "w") as f:
        json.dump(dummy_ui_spec, f, indent=2)

    # Dummy custom creators map
    custom_creators = {"style": "my_custom_style_creator"}

    generated_c = generate_renderer(custom_creators, "lv_def.json", "api_info.json")
    print("\n--- Generated C Code: ---")
    print(generated_c)

    # Clean up dummy files
    import os
    # os.remove("api_info.json")
    # os.remove("lv_def.json")
    print("\nNote: Dummy api_info.json and lv_def.json created for testing. You may want to remove them.")
