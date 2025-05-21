# generator.py
import argparse
import json
import logging
import os
from pathlib import Path # Ensure Path is imported

import api_parser
import type_utils # type_utils might be used by api_parser or other parts indirectly
from pathlib import Path # Ensure Path is imported

from unified_processor.dynamic_generator import DynamicUIGenerator
from unified_processor.static_transpiler import StaticUICodeTranspiler

# Basic Logging Setup
logging.basicConfig(level=logging.INFO, format='%(levelname)s: [%(filename)s:%(lineno)d] %(message)s')
logger = logging.getLogger(__name__)

# DEFAULT_MACRO_NAMES_TO_EXPORT has been moved to DynamicUIGenerator

# These defaults are still used by argparse
C_TRANSPILE_OUTPUT_DIR_DEFAULT = "output_c_transpiled"
C_TRANSPILE_UI_SPEC_DEFAULT = "ui.json" # Default name for the UI spec file

def main():
    parser = argparse.ArgumentParser(description="LVGL JSON UI Generator")
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

    output_path = Path(args.output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    if args.mode == "c_transpile":
        if args.ui_spec is None:
            args.ui_spec = C_TRANSPILE_UI_SPEC_DEFAULT
            logger.info(f"--ui-spec not provided for c_transpile mode, defaulting to {args.ui_spec}")
        if not Path(args.ui_spec).exists():
            logger.error(f"UI specification file '{args.ui_spec}' not found (required for 'c_transpile' mode).")
            return 1
        # Adjust output directory for C transpiler mode if default is used ("output" which is default for preview)
        if args.output_dir == "output": 
            args.output_dir = C_TRANSPILE_OUTPUT_DIR_DEFAULT
            output_path = Path(args.output_dir) # Re-assign output_path
            output_path.mkdir(parents=True, exist_ok=True) # Ensure it's created
            logger.info(f"Output directory for c_transpile mode set to: {args.output_dir}")
    
    logger.info(f"Parsing API definition from: {args.api_json}")
    string_values_override = {}
    if args.string_values_json:
        try:
            with open(args.string_values_json, 'r') as f_sv:
                string_values_override = json.load(f_sv)
            logger.info(f"Loaded {len(string_values_override)} string-value overrides from {args.string_values_json}")
        except Exception as e:
            logger.error(f"Failed to load string_values.json from {args.string_values_json}: {e}. Proceeding without overrides.")
            string_values_override = {}

    api_info = api_parser.parse_api(args.api_json, string_values_override=string_values_override)
    if not api_info:
        logger.critical("Failed to parse API information. Exiting.")
        return 1

    if args.mode == "preview":
        logger.info("Initializing DynamicUIGenerator for 'preview' mode...")
        # Note: lv_def_json_path and str_vals_json_path are part of BaseUIProcessor.
        # Assuming args.api_json is the lv_def.json equivalent.
        # args.string_values_json is for str_vals.json.
        dynamic_gen = DynamicUIGenerator(
            api_info=api_info,
            lv_def_json_path=args.api_json, 
            str_vals_json_path=args.string_values_json
        )
        # The `macro_names_list_override` argument is not currently in `generate_interpreter_c_files`
        # It will be handled internally by DynamicUIGenerator if needed, or passed later.
        dynamic_gen.generate_interpreter_c_files(
            output_dir_path=str(output_path), 
            debug_mode=args.debug,
            macro_names_list_override=args.macro_names_list.split(',') if args.macro_names_list else None
        )
        logger.info(f"'preview' mode C files would be generated in {output_path}")


    elif args.mode == "c_transpile":
        logger.info("Initializing StaticUICodeTranspiler for 'c_transpile' mode...")
        static_transpiler = StaticUICodeTranspiler(
            api_info=api_info,
            lv_def_json_path=args.api_json,
            str_vals_json_path=args.string_values_json
        )
        
        logger.info(f"Loading UI spec for transpilation: {args.ui_spec}")
        ui_spec_root_node = static_transpiler.load_ui_spec(args.ui_spec)
        if not ui_spec_root_node:
            logger.error(f"Failed to load UI spec from {args.ui_spec}. Exiting.")
            return 1
        
        ui_spec_filename_base = Path(args.ui_spec).stem.replace('-', '_').replace('.', '_')
        c_func_name = f"create_ui_from_{ui_spec_filename_base}"

        static_transpiler.generate_transpiled_c_files(
            output_dir_path=str(output_path),
            ui_spec_root_node=ui_spec_root_node,
            c_function_name=c_func_name
        )
        logger.info(f"'c_transpile' mode C files for {c_func_name} would be generated in {output_path}")

    else:
        logger.error(f"Unknown mode: {args.mode}")
        return 1

    logger.info("Generation complete.")
    return 0

if __name__ == "__main__":
    # Set higher log level for libraries during generation itself
    logging.getLogger("api_parser").setLevel(logging.WARNING)
    logging.getLogger("type_utils").setLevel(logging.WARNING)
    # Add others if needed
    exit(main())