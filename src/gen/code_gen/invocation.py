# code_gen/invocation.py
import logging
from collections import defaultdict
from . import unmarshal # Needs unmarshal functions for generation
from type_utils import get_c_type_str, get_signature, WIDGET_CREATE_SIGNATURE

logger = logging.getLogger(__name__)

# Store generated invoke function names by signature category to avoid duplicates
# Key: signature tuple, Value: C function name
generated_invoke_fns = {}

def generate_invoke_signatures(filtered_functions):
    """Groups functions by their call signature tuple."""
    signatures = defaultdict(list)
    for func in filtered_functions:
        try:
            sig = get_signature(func) # Uses the updated get_signature
            signatures[sig].append(func['name'])
        except Exception as e:
            logger.error(f"Could not get signature for {func.get('name', 'UNKNOWN')}: {e}")
    logger.info(f"Found {len(signatures)} unique function signatures.")
    # Log the special create signature if found
    if WIDGET_CREATE_SIGNATURE in signatures:
        logger.info(f"-> Detected {len(signatures[WIDGET_CREATE_SIGNATURE])} functions matching WIDGET_CREATE signature.")
    return signatures

def _generate_widget_create_invoker():
    """Generates the specific C invoker for lv_widget_create(parent) functions."""
    sig_c_name = "invoke_widget_create"
    c_code = f"// Specific Invoker for functions like lv_widget_create(lv_obj_t *parent)\n"
    c_code += f"// Signature: expects target_obj_ptr = parent, dest = lv_obj_t**, args_array = NULL\n"
    c_code += f"static bool {sig_c_name}(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {{\n"
    c_code += f"    if (!func_ptr) {{ LOG_ERR(\"Invoke Error: func_ptr is NULL for {sig_c_name}\"); return false; }}\n"
    c_code += f"    if (!dest) {{ LOG_ERR(\"Invoke Error: dest is NULL for {sig_c_name} (needed for result)\"); return false; }}\n"

    # Although args_array should be NULL, add a warning if it's not.
    c_code += f"    if (args_array != NULL && cJSON_GetArraySize(args_array) > 0) {{\n"
    c_code += f"       LOG_WARN_JSON(args_array, \"Invoke Warning: {sig_c_name} expected 0 JSON args, got %d. Ignoring JSON args.\", cJSON_GetArraySize(args_array));\n"
    c_code += f"    }}\n\n"

    # Cast arguments and function pointer
    c_code += f"    lv_obj_t* parent = (lv_obj_t*)target_obj_ptr;\n"
    # Allow NULL parent (for screen objects, although renderer usually passes active screen)
    # c_code += f"    if (!parent) {{ LOG_ERR("Invoke Error: parent (target_obj_ptr) is NULL for {sig_c_name}"); return false; }}\n"

    c_code += f"    // Define the specific function pointer type\n"
    c_code += f"    typedef lv_obj_t* (*specific_lv_create_func_type)(lv_obj_t*);\n"
    c_code += f"    specific_lv_create_func_type target_func = (specific_lv_create_func_type)func_ptr;\n\n"

    # Call the actual LVGL create function
    c_code += f"    // Call the target LVGL create function\n"
    c_code += f"    lv_obj_t* result = target_func(parent);\n\n"

    # Store the result
    c_code += f"    // Store result widget pointer into *dest\n"
    c_code += f"    *(lv_obj_t**)dest = result;\n\n"

    # Check if result is NULL (optional, LVGL create might return NULL on failure)
    c_code += f"    if (!result) {{\n"
    c_code += f"        // The name of the actual function isn't known here easily, maybe log func_ptr address?\n"
    c_code += f"        LOG_WARN(\"Invoke Warning: {sig_c_name} (func ptr %p) returned NULL.\", func_ptr);\n"
    c_code += f"        // Return true because the invoker itself succeeded, even if LVGL func failed.\n"
    c_code += f"        // The renderer should check the returned object pointer.\n"
    c_code += f"    }}\n\n"

    c_code += f"    return true;\n"
    c_code += f"}}\n\n"
    return sig_c_name, c_code


def _generate_generic_invoke_fn(signature_category, sig_c_name, repr_func_name, api_info):
    """Generates the C code for a generic invoke_fn_t implementation."""
    # Find the representative function to get specific C types
    func_repr = next((f for f in api_info['functions'] if f['name'] == repr_func_name), None)
    if not func_repr:
        logger.error(f"Could not find representative function '{repr_func_name}' for signature {signature_category}")
        return "" # Should not happen

    # Extract specific C types for return value and arguments
    ret_c_type, ret_ptr_lvl, _ = func_repr['_resolved_ret_type']
    specific_ret_c_type_str = get_c_type_str(ret_c_type, ret_ptr_lvl)
    is_void_return = (ret_c_type == 'void' and ret_ptr_lvl == 0)

    specific_arg_c_type_strs = []
    specific_arg_c_types_full = [] # Keep tuple (base, ptr_lvl, is_array)
    if func_repr['_resolved_arg_types']:
        for i, (arg_c_type, arg_ptr_lvl, arg_is_array) in enumerate(func_repr['_resolved_arg_types']):
             specific_arg_c_type_strs.append(get_c_type_str(arg_c_type, arg_ptr_lvl))
             specific_arg_c_types_full.append((arg_c_type, arg_ptr_lvl, arg_is_array))

    num_c_args = len(specific_arg_c_type_strs) # Number of C arguments for the representative func

    c_code = f"// Generic Invoker for signature category: {signature_category}\n"
    c_code += f"// Representative LVGL func: {repr_func_name} ({specific_ret_c_type_str}({', '.join(specific_arg_c_type_strs)}))\n"
    c_code += f"static bool {sig_c_name}(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr) {{\n"
    c_code += f"    if (!func_ptr) {{ LOG_ERR(\"Invoke Error: func_ptr is NULL for {sig_c_name}\"); return false; }}\n"

    # Declare C argument variables
    for i in range(num_c_args):
        c_code += f"    {specific_arg_c_type_strs[i]} arg{i};\n"
    if not is_void_return:
        c_code += f"    {specific_ret_c_type_str} result;\n"

    c_code += "\n"

    # Define the specific C function pointer type
    args_for_typedef = ", ".join(specific_arg_c_type_strs) if specific_arg_c_type_strs else "void"
    c_code += f"    // Define specific function pointer type based on representative function\n"
    c_code += f"    typedef {specific_ret_c_type_str} (*specific_lv_func_type)({args_for_typedef});\n"
    c_code += f"    specific_lv_func_type target_func = (specific_lv_func_type)func_ptr;\n\n"

    # --- Argument Unmarshaling Logic ---
    num_json_args_expected = num_c_args
    first_arg_is_target = False

    # Check if the function expects the target object as its *first* C argument (heuristic: ptr to lv_..._t)
    if num_c_args > 0 and specific_arg_c_types_full:
         first_c_arg_type, first_c_arg_ptr, _ = specific_arg_c_types_full[0]
         # Assume first arg is target if it's a pointer to an LVGL type (struct/widget)
         if first_c_arg_ptr == 1 and first_c_arg_type.startswith("lv_") and first_c_arg_type.endswith("_t"):
             first_arg_is_target = True
             num_json_args_expected = num_c_args - 1 # JSON provides args *after* the target
             # Assign target_obj_ptr to the first C argument variable
             c_code += f"    // Assign target_obj_ptr to first C argument (arg0)\n"
             c_code += f"    arg0 = ({specific_arg_c_type_strs[0]})target_obj_ptr;\n"
             c_code += f"    // It's okay if target_obj_ptr is NULL for some functions (e.g. lv_style_set_...)\n"
             c_code += f"    // Add validation if target MUST be non-NULL for this signature group if needed.\n"
             # c_code += f"    if (!arg0) {{ LOG_ERR("Invoke Error: target_obj_ptr (first arg) is NULL for {sig_c_name}"); return false; }}\n"

    # Check JSON array validity and size
    c_code += f"    // Expecting {num_json_args_expected} arguments from JSON array\n"
    c_code += f"    if (!cJSON_IsArray(args_array)) {{\n"
    # Allow 0 expected JSON args if array is NULL (e.g., void function with only target object)
    c_code += f"       if ({num_json_args_expected} == 0 && args_array == NULL) {{ /* Okay */ }}\n"
    c_code += f"       else {{ LOG_ERR_JSON(args_array, \"Invoke Error: args_array is not a valid array for {sig_c_name}\"); return false; }}\n"
    c_code += f"    }}\n"
    c_code += f"    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);\n"
    c_code += f"    if (arg_count != {num_json_args_expected}) {{ LOG_ERR_JSON(args_array, \"Invoke Error: Expected {num_json_args_expected} JSON args, got %d for {sig_c_name}\", arg_count); return false; }}\n\n"

    # Unmarshal arguments from JSON array
    c_code += "    // Unmarshal arguments from JSON\n"
    for i in range(num_json_args_expected):
        c_arg_index = i + (1 if first_arg_is_target else 0) # Index into specific_arg_c_type_strs/argN vars
        c_code += f"    cJSON *json_arg{i} = cJSON_GetArrayItem(args_array, {i});\n"
        c_code += f"    if (!json_arg{i}) {{ LOG_ERR(\"Invoke Error: Failed to get JSON arg {i} for {sig_c_name}\"); return false; }}\n"

        base_arg_type, ptr_lvl, is_array = specific_arg_c_types_full[c_arg_index]
        specific_type_str = specific_arg_c_type_strs[c_arg_index]
        # Use the main unmarshal_value dispatcher
        unmarshal_call = f"unmarshal_value(json_arg{i}, \"{specific_type_str}\", &arg{c_arg_index})"
        c_code += f"    // Unmarshal JSON arg {i} into C arg {c_arg_index} (type {specific_type_str})\n"
        c_code += f"    if (!({unmarshal_call})) {{\n"
        c_code += f"        LOG_ERR_JSON(json_arg{i}, \"Invoke Error: Failed to unmarshal JSON arg {i} (expected C type {specific_type_str}) for {sig_c_name}\");\n"
        c_code += f"        return false;\n"
        c_code += f"    }}\n"
    c_code += "\n"

    # --- Function Call ---
    c_code += "    // Call the target LVGL function\n"
    call_args_str = ", ".join([f"arg{i}" for i in range(num_c_args)]) # Pass all declared C args
    if is_void_return:
        c_code += f"    target_func({call_args_str});\n"
    else:
        c_code += f"    result = target_func({call_args_str});\n"

    c_code += "\n"

    # --- Store Result ---
    if not is_void_return:
        c_code += "    // Store result if dest is provided\n"
        c_code += "    if (dest) {\n"
        c_code += f"        *(({specific_ret_c_type_str} *)dest) = result;\n"
        c_code += "    }\n"

    c_code += "\n    return true;\n"
    c_code += "}\n\n"
    return c_code

def generate_invocation_helpers(signatures, api_info):
    """Generates all the C invoke_fn_t helper functions."""
    global generated_invoke_fns
    generated_invoke_fns.clear() # Reset for idempotency

    c_code = ""
    c_code += "// Forward declaration for the main unmarshaler\n"
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest);\n\n"

    c_code += "// --- Invocation Helper Functions ---\n"

    signature_map = {} # Map func_name -> invoke_fn_name

    # Sort signatures for consistent output order (optional)
    sorted_signatures = sorted(signatures.keys())

    # Generate the special create invoker if needed
    widget_create_invoker_name = None
    if WIDGET_CREATE_SIGNATURE in signatures:
        widget_create_invoker_name, create_invoker_code = _generate_widget_create_invoker()
        c_code += create_invoker_code
        generated_invoke_fns[WIDGET_CREATE_SIGNATURE] = widget_create_invoker_name
        # Map all functions with this signature to the special invoker
        for func_name in signatures[WIDGET_CREATE_SIGNATURE]:
            signature_map[func_name] = widget_create_invoker_name
        logger.info(f"Generated specific invoker '{widget_create_invoker_name}' for WIDGET_CREATE functions.")


    # Generate generic invokers for all other signatures
    for signature_category in sorted_signatures:
        # Skip the special create signature if we already generated it
        if signature_category == WIDGET_CREATE_SIGNATURE:
            continue

        # Get list of functions and pick one as representative for C types
        func_name_list = signatures[signature_category]
        if not func_name_list: continue # Should not happen
        representative_func_name = func_name_list[0]

        # Create a C-safe name for the signature function
        sig_c_name_parts = [str(s).replace(' ', '_').replace('*', 'p').replace('[', '_').replace(']', '_') for s in signature_category]
        sig_c_name = f"invoke_{'_'.join(sig_c_name_parts)}"
        # Ensure C name validity (e.g., starts with letter/underscore, max length?)
        sig_c_name = ''.join(c if c.isalnum() else '_' for c in sig_c_name) # Basic sanitization
        if not sig_c_name[0].isalpha() and sig_c_name[0] != '_':
             sig_c_name = "_" + sig_c_name

        # Store the mapping for the table generation
        generated_invoke_fns[signature_category] = sig_c_name
        for func_name in func_name_list:
            # Only map if not already mapped (e.g., create funcs mapped above)
            if func_name not in signature_map:
                 signature_map[func_name] = sig_c_name

        # Generate the C code for this invoker
        c_code += _generate_generic_invoke_fn(signature_category, sig_c_name, representative_func_name, api_info)

    return c_code, signature_map

def generate_invoke_table_def():
    c_code = "// Signature of the function pointers in the table entry\n"
    c_code += "typedef bool (*invoke_fn_t)(void *target_obj_ptr, void *dest, cJSON *args_array, void *func_ptr);\n\n"
    c_code += "// Structure for each entry in the invocation table\n"
    c_code += "typedef struct {\n"
    c_code += "    const char *name;       // LVGL function name (e.g., \"lv_obj_set_width\")\n"
    c_code += "    invoke_fn_t invoke;   // Pointer to the C invocation wrapper function\n"
    c_code += "    void *func_ptr;     // Pointer to the actual LVGL function\n"
    c_code += "} invoke_table_entry_t;\n\n"
    return c_code


def generate_invoke_table(filtered_functions, signature_map):
    """Generates the invoke table definition and data."""

    c_code = "// The global invocation table\n"
    c_code += "static const invoke_table_entry_t g_invoke_table[] = {\n"

    # Sort functions for consistent output order
    filtered_functions.sort(key=lambda f: f['name'])

    count = 0
    for func in filtered_functions:
        name = func['name']
        if name in signature_map:
            invoke_func_name = signature_map[name]
            # Ensure the C invoker function exists (was generated)
            # This check is somewhat redundant if signature_map is built correctly
            # from generated_invoke_fns keys
            sig = get_signature(func) # Recalculate signature to look up in generated_invoke_fns
            if sig not in generated_invoke_fns:
                 logger.error(f"Internal Error: Signature {sig} for function '{name}' has a mapping to '{invoke_func_name}' but no invoker was generated for this signature.")
                 continue

            c_code += f"    {{\"{name}\", &{invoke_func_name}, (void*)&{name}}},\n"
            count += 1
        else:
            logger.warning(f"Function '{name}' is filtered but has no signature mapping. Skipping invoke table entry.")

    c_code += "    {NULL, NULL, NULL} // Sentinel\n"
    c_code += "};\n\n"
    c_code += f"#define INVOKE_TABLE_SIZE {count}\n\n"

    logger.info(f"Generated invoke table with {count} entries.")
    return c_code

def generate_find_function():
    """Generates C code to find a function in the invoke table."""
    c_code = "// --- Function Lookup ---\n\n"
    c_code += "// Finds an entry in the invocation table by function name.\n"
    c_code += "static const invoke_table_entry_t* find_invoke_entry(const char *name) {\n"
    c_code += "    if (!name) return NULL;\n"
    # Simple linear search - could be replaced with hash map or binary search if sorted
    c_code += "    for (int i = 0; g_invoke_table[i].name != NULL; ++i) {\n"
    c_code += "        // Direct string comparison\n"
    c_code += "        if (strcmp(g_invoke_table[i].name, name) == 0) {\n"
    c_code += "            return &g_invoke_table[i];\n"
    c_code += "        }\n"
    c_code += "    }\n"
    # Fallback logic (e.g., for lv_obj_set_prop) is handled by the caller (renderer)
    c_code += "    return NULL;\n"
    c_code += "}\n\n"
    return c_code