# type_utils.py
import logging

logger = logging.getLogger(__name__)

# Mapping from C types to cJSON check functions
CJSON_TYPE_CHECKS = {
    "int": "cJSON_IsNumber",
    "int8_t": "cJSON_IsNumber",
    "uint8_t": "cJSON_IsNumber",
    "int16_t": "cJSON_IsNumber",
    "uint16_t": "cJSON_IsNumber",
    "int32_t": "cJSON_IsNumber",
    "uint32_t": "cJSON_IsNumber",
    "int64_t": "cJSON_IsNumber",
    "uint64_t": "cJSON_IsNumber",
    "size_t": "cJSON_IsNumber",
    "lv_coord_t": "cJSON_IsNumber", # Assuming lv_coord_t is int based
    "lv_opa_t": "cJSON_IsNumber",   # Assuming lv_opa_t is int based
    "float": "cJSON_IsNumber",
    "double": "cJSON_IsNumber",
    "bool": "cJSON_IsBool",
    "_Bool": "cJSON_IsBool",
    "char": "cJSON_IsString", # Single char might be passed as number? Assume string.
}
# Default check if type not found
DEFAULT_CJSON_CHECK = "cJSON_IsObject" # Or maybe check IsString or IsNumber as common cases


PRIMITIVE_TYPES = {
    "void", "char", "signed char", "unsigned char",
    "short", "signed short", "unsigned short",
    "int", "signed int", "unsigned int",
    "long", "signed long", "unsigned long",
    "long long", "signed long long", "unsigned long long",
    "float", "double", "long double",
    "bool", "_Bool",
    "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t",
    "int64_t", "uint64_t", "size_t", "ssize_t", "intptr_t", "uintptr_t",
    "lv_color_t", # Treat color as primitive for unmarshaling? Yes, '#' prefix
    "lv_opa_t", # Usually uint8_t
    "lv_coord_t", # Usually int32_t
}

# Special constant signature tuple for standard widget create functions
WIDGET_CREATE_SIGNATURE = ('WIDGET_CREATE', 'lv_obj_t*', 'lv_obj_t*') # Internal category, Return Type, Arg1 Type

def get_c_type_str(base_type, pointer_level):
    """Generates C type string like 'int', 'char *', 'const lv_obj_t **'"""
    if not base_type:
        base_type = "void" # Fallback

    # Handle const qualifier if present in base_type (though API parser might handle this)
    # Qualifiers often appear in the 'quals' list in the JSON struct. Need to integrate.

    stars = "*" * pointer_level
    if stars:
        return f"{base_type}{' ' if base_type[-1] != '*' else ''}{stars}"
    else:
        return base_type

def get_unmarshal_signature_type(c_type, pointer_level, is_array):
    """Gets a simplified type name for signature generation, treating pointers/arrays."""
    if pointer_level > 0 or is_array:
        # Collapse all pointers/arrays to a generic 'ptr' or specific known types
        if c_type == "char" and pointer_level == 1:
            return "const char *" # Treat char* special for strings
        elif c_type.startswith("lv_") and c_type.endswith("_t") and pointer_level == 1:
            return f"{c_type} *" # Distinguish lv_obj_t*, lv_style_t* etc.
        else:
            # Treat other pointer types generically for signature matching
            # We might need more specific ones like 'lv_font_t *' later if needed
             return f"POINTER" # Generic pointer signature component
    elif c_type in ["float", "double", "long double"]:
        return "FLOAT" # Treat all float types similarly for unmarshaling
    elif c_type == "bool" or c_type == "_Bool":
        return "BOOL"
    elif c_type == "lv_color_t":
        return "lv_color_t" # Keep specific for custom unmarshaler
    elif c_type == "void":
        return "void"
    elif c_type in PRIMITIVE_TYPES or c_type.endswith("_t"): # Include lvgl typedefs that might be ints/enums
        # Treat most number-like things (int, enum, lv_coord_t) as 'INT' for signature
        return "INT" # Group integers, enums, opa, coord etc.
    else:
        # Likely a struct passed by value (rare in LVGL api) or complex type
        logger.warning(f"Unhandled type for signature: {c_type}")
        return "UNKNOWN"


def get_signature(func_data):
    """
    Generates a tuple signature for grouping invoke functions.
    Returns a special signature for standard lv_widget_create(parent) functions.
    """
    func_name = func_data.get('name', '')
    ret_type, ret_ptr, _ = func_data['_resolved_ret_type']
    args_types = func_data['_resolved_arg_types']

    # --- Check for lv_widget_create(parent) pattern ---
    # Name: lv_<something>_create
    # Return: lv_obj_t* (or compatible pointer like lv_label_t*)
    # Args: Exactly one arg: lv_obj_t*
    is_create_func = (
        func_name.startswith("lv_") and
        func_name.endswith("_create") and
        ret_ptr == 1 and ret_type.startswith("lv_") and ret_type.endswith("_t") and # Returns lv_..._t*
        len(args_types) == 1 and
        args_types[0][0] == 'lv_obj_t' and args_types[0][1] == 1 # Arg0 is lv_obj_t*
    )

    if is_create_func:
        # logger.debug(f"Detected widget create function: {func_name}")
        return WIDGET_CREATE_SIGNATURE # Return the special constant signature

    # --- Default signature generation ---
    sig_ret = get_unmarshal_signature_type(ret_type, ret_ptr, False)
    sig_args = []
    for arg_type, arg_ptr, arg_is_array in args_types:
        sig_args.append(get_unmarshal_signature_type(arg_type, arg_ptr, arg_is_array))

    # logger.debug(f"Generated signature for {func_name}: {(sig_ret, *sig_args)}")
    return (sig_ret, *sig_args)


def c_type_to_cjson_check(c_type_str):
    """Suggests a cJSON type check function based on C type."""
    # Very basic mapping, needs improvement
    if "char *" in c_type_str or c_type_str.startswith('@') or c_type_str.startswith('#'): # Heuristic for strings/custom
        return "cJSON_IsString"
    if "*" in c_type_str: # Pointers might be strings ('@..') or numbers (address)
        return None # No single check, needs custom logic
    if c_type_str == "bool":
        return "cJSON_IsBool"
    if c_type_str in ["float", "double"] or "int" in c_type_str or "size_t" in c_type_str or c_type_str == "lv_opa_t" or c_type_str == "lv_coord_t":
        return "cJSON_IsNumber"
    if c_type_str.startswith("LV_"): # Enum Value Name
        return "cJSON_IsString" # Assuming enums passed as strings
    if c_type_str == "lv_color_t":
        return "cJSON_IsString" # Assuming '#rrggbb' format

    # Fallback for lv_ types that are likely enums or structs
    if c_type_str.startswith("lv_"):
        # Enums are often strings, structs are complex/pointers
        return None # Requires specific unmarshaler

    logger.debug(f"No specific cJSON check found for C type: {c_type_str}, using default.")
    return DEFAULT_CJSON_CHECK

def is_lvgl_struct_ptr(c_type, pointer_level):
    """Checks if type is lv_..._t *"""
    return pointer_level == 1 and c_type.startswith("lv_") and c_type.endswith("_t") and c_type != "lv_color_t"

def is_primitive(c_type):
    """Checks if it's a basic C primitive type name."""
    return c_type in PRIMITIVE_TYPES

def lvgl_type_to_widget_name(lv_type):
    """Extracts 'label' from 'lv_label_t', 'obj' from 'lv_obj_t'"""
    if lv_type.startswith("lv_") and lv_type.endswith("_t"):
        return lv_type[3:-2]
    return None