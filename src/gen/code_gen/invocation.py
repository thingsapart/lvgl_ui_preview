# code_gen/invocation.py
import logging
from collections import defaultdict
from . import unmarshal # Needs unmarshal functions for generation
from type_utils import get_c_type_str, get_signature, WIDGET_CREATE_SIGNATURE

logger = logging.getLogger(__name__)

# Max args for table entry type storage
MAX_ARGS_SUPPORTED = 8

# Store generated invoke function names by signature category to avoid duplicates
# Key: signature tuple, Value: C function name
generated_invoke_fns = {}

def generate_invoke_signatures(filtered_functions):
    """Groups functions by their call signature tuple."""
    signatures = defaultdict(list)
    for func in filtered_functions:
        try:
            sig = get_signature(func) # Uses the updated get_signature
            # Store the whole function dict for later use if needed (e.g., representative)
            signatures[sig].append(func)
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
    # Updated signature: takes entry pointer
    c_code = f"// Specific Invoker for functions like lv_widget_create(lv_obj_t *parent)\n"
    c_code += f"// Signature: expects target_obj_ptr = parent, dest = lv_obj_t**, args_array = NULL\n"
    c_code += f"static bool {sig_c_name}(const invoke_table_entry_t *entry, void *target_obj_ptr, void *dest, cJSON *args_array) {{\n"
    c_code += f"    if (!entry || !entry->func_ptr) {{ LOG_ERR(\"Invoke Error: NULL entry or func_ptr for {sig_c_name}\"); return false; }}\n"
    c_code += f"    if (!dest) {{ LOG_ERR(\"Invoke Error: dest is NULL for {sig_c_name} (needed for result)\"); return false; }}\n"

    c_code += f"    // Although args_array should be NULL, add a warning if it's not.\n"
    c_code += f"    if (args_array != NULL && cJSON_GetArraySize(args_array) > 0) {{\n"
    c_code += f"       LOG_WARN_JSON(args_array, \"Invoke Warning: {sig_c_name} expected 0 JSON args, got %d for func '%s'. Ignoring JSON args.\", cJSON_GetArraySize(args_array), entry->name);\n"
    c_code += f"    }}\n\n"

    # Cast arguments and function pointer
    c_code += f"    lv_obj_t* parent = (lv_obj_t*)target_obj_ptr;\n"

    c_code += f"    // Define the specific function pointer type (always lv_obj_t*(lv_obj_t*) for this invoker)\n"
    c_code += f"    typedef lv_obj_t* (*specific_lv_create_func_type)(lv_obj_t*);\n"
    c_code += f"    specific_lv_create_func_type target_func = (specific_lv_create_func_type)entry->func_ptr;\n\n"

    # Call the actual LVGL create function
    c_code += f"    // Call the target LVGL create function\n"
    c_code += f"    lv_obj_t* result = target_func(parent);\n\n"

    # --- BEGIN TRACE WIDGET CREATE INVOKER ---
    c_code += f"    // Tracing the widget creation call\n"
    c_code += f"    char L_parent_trace_name[64]; snprintf(L_parent_trace_name, sizeof(L_parent_trace_name), \"trace_obj_%p\", (void*)parent);\n"
    c_code += f"    char L_result_trace_name[64]; snprintf(L_result_trace_name, sizeof(L_result_trace_name), \"trace_obj_%p\", (void*)result);\n"
    c_code += f"    const char* L_ret_type_str = entry->ret_type ? entry->ret_type : \"lv_obj_t*\"; // Default for create invokers\n"
    c_code += f"    printf(\"%s%s *%s = %s(%s); /* Invoked by {sig_c_name}. Actual parent: %p, created_obj: %p */\\n\", get_c_trace_indent_str(), L_ret_type_str, L_result_trace_name, entry->name, L_parent_trace_name, (void*)parent, (void*)result);\n"
    c_code += f"    // --- END TRACE WIDGET CREATE INVOKER ---\n\n"

    # Store the result
    c_code += f"    // Store result widget pointer into *dest\n"
    c_code += f"    *(lv_obj_t**)dest = result;\n\n"

    # Check if result is NULL (optional, LVGL create might return NULL on failure)
    c_code += f"    if (!result) {{\n"
    c_code += f"        LOG_WARN(\"Invoke Warning: Create function '%s' returned NULL.\", entry->name);\n"
    c_code += f"        // Return true because the invoker itself succeeded.\n"
    c_code += f"    }}\n\n"

    c_code += f"    return true;\n"
    c_code += f"}}\n\n"
    return sig_c_name, c_code


def _get_buffer_type_for_sig_component(sig_comp):
    """ Determine appropriate C stack buffer type for a simplified signature component """
    if sig_comp == 'INT' or sig_comp == 'BOOL' or sig_comp == 'lv_color_t':
        # Use a type guaranteed to hold the largest integer/enum/color type
        return "int64_t" # Sufficiently large for most integer/enum types
    elif sig_comp == 'FLOAT':
        return "double" # Use double to hold float or double
    elif sig_comp == 'POINTER' or sig_comp == 'const char *' or sig_comp.endswith('*'):
        return "void*" # Pointers are stored as void*
    elif sig_comp == 'void':
         return "void" # Should not be used for args normally
    else:
        logger.warning(f"Cannot determine buffer type for signature component: {sig_comp}. Using void*.")
        return "void*"


def _generate_generic_invoke_fn(signature_category, sig_c_name, function_list, api_info):
    """Generates the C code for a generic invoke_fn_t implementation."""
    # function_list contains the list of function dicts sharing this signature category.
    # We only need one representative function *in Python* to determine buffer sizes.
    if not function_list:
         logger.error(f"Cannot generate invoker {sig_c_name}: function list is empty.")
         return ""
    representative_func = function_list[0] # Used only for buffer sizing check

    # Simplified signature components (used for buffer types)
    sig_ret_comp = signature_category[0]
    sig_arg_comps = signature_category[1:]
    num_c_args = len(sig_arg_comps) # Number of args based on *simplified* signature

    # Check if the number of args in the representative function matches the simplified one
    # This is a sanity check
    if len(representative_func['_resolved_arg_types']) != num_c_args:
         logger.warning(f"Mismatch between simplified arg count ({num_c_args}) and representative func '{representative_func['name']}' ({len(representative_func['_resolved_arg_types'])} args) for invoker {sig_c_name}.")
         # Proceed cautiously, num_c_args from simplified signature dictates the loop below

    # Updated C function signature: takes entry pointer
    c_code = f"// Generic Invoker for signature category: {signature_category}\n"
    c_code += f"// Handles {len(function_list)} functions like '{representative_func['name']}'\n"
    c_code += f"static bool {sig_c_name}(const invoke_table_entry_t *entry, void *target_obj_ptr, void *dest, cJSON *args_array) {{\n"
    c_code += f"    if (!entry || !entry->func_ptr || !entry->ret_type || !entry->arg_types) {{ LOG_ERR(\"Invoke Error: Invalid entry passed to {sig_c_name} (for func '%s')\", entry ? entry->name : \"NULL_ENTRY\"); return false; }}\n" # Improved error log

    # Declare stack buffers for arguments based on simplified signature
    c_code += "    // Declare stack buffers for arguments (sized based on signature category)\n"
    for i in range(num_c_args):
        buffer_type = _get_buffer_type_for_sig_component(sig_arg_comps[i])
        if buffer_type != "void":
           c_code += f"    {buffer_type} arg_buf{i};\n"

    # Declare buffer for result if needed
    result_buffer_type = "void"
    if sig_ret_comp != 'void':
        result_buffer_type = _get_buffer_type_for_sig_component(sig_ret_comp)
        c_code += f"    {result_buffer_type} result_buf;\n"

    c_code += "\n"

    # --- Argument Unmarshaling Logic ---
    num_json_args_expected = num_c_args
    first_arg_is_target = False

    # Check if the *first argument based on simplified signature* is the target object
    # This relies on the simplified signature accurately reflecting the target object pattern.
    if num_c_args > 0 and (sig_arg_comps[0] == 'POINTER' or sig_arg_comps[0].endswith('*')):
        first_arg_is_target = True
        num_json_args_expected = num_c_args - 1
        c_code += f"    // Assign target_obj_ptr to first C argument buffer (arg_buf0)\n"
        c_code += f"    // Expected specific type for target is entry->arg_types[0]\n"
        c_code += f"    if (!entry->arg_types[0]) {{ LOG_ERR(\"Invoke Error: Missing type string for target arg 0 of '%s'\", entry->name); return false; }}\n"
        c_code += f"    arg_buf0 = (void*)target_obj_ptr; // Store as void* in buffer\n"
        # Add check if target MUST be non-NULL? Requires analyzing all functions in the group.
        # c_code += f"    if (!arg_buf0 && /* check if non-null required */ ) {{ ... return false; }} \n"


    # Check JSON array validity and size
    c_code += f"    // Expecting {num_json_args_expected} arguments from JSON array for function '{representative_func}'\n"
    c_code += f"    if (!cJSON_IsArray(args_array)) {{\n"
    c_code += f"       if ({num_json_args_expected} == 0 && args_array == NULL) {{ /* Okay */ }}\n"
    c_code += f"       else {{ LOG_ERR_JSON(args_array, \"Invoke Error: args_array is not a valid array for {sig_c_name} (func '%s')\", entry->name); return false; }}\n"
    c_code += f"    }}\n"
    c_code += f"    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);\n"
    c_code += f"    if (arg_count != {num_json_args_expected}) {{ LOG_ERR_JSON(args_array, \"Invoke Error: Expected {num_json_args_expected} JSON args for func '%s', got %d for {sig_c_name}\", entry->name, arg_count); return false; }}\n\n"

    # Unmarshal arguments from JSON array into stack buffers
    c_code += "    // Unmarshal arguments from JSON into stack buffers using specific types from entry\n"
    for i in range(num_json_args_expected):
        c_arg_index = i + (1 if first_arg_is_target else 0) # Index into arg_bufN and entry->arg_types

        # C code accesses entry->arg_types at runtime
        c_code += f"    const char* specific_type_str{c_arg_index} = entry->arg_types[{c_arg_index}];\n"
        c_code += f"    if (!specific_type_str{c_arg_index}) {{\n"
        c_code += f"        LOG_ERR(\"Invoke Error: Internal setup error - missing type for arg {c_arg_index} of '%s'\", entry->name);\n"
        c_code += f"        return false;\n"
        c_code += f"    }}\n"

        buffer_ptr = f"&arg_buf{c_arg_index}" # Address of the stack buffer

        c_code += f"    cJSON *json_arg{i} = cJSON_GetArrayItem(args_array, {i});\n"
        c_code += f"    if (!json_arg{i}) {{ LOG_ERR(\"Invoke Error: Failed to get JSON arg {i} for func '%s' ({sig_c_name})\", entry->name); return false; }}\n"

        # Generate the call to unmarshal_value using the specific type string retrieved *at runtime*
        unmarshal_call = f"unmarshal_value(json_arg{i}, specific_type_str{c_arg_index}, (void*){buffer_ptr}, target_obj_ptr)"
        c_code += f"    // Unmarshal JSON arg {i} into C arg buffer {c_arg_index} (using specific type found in entry->arg_types[{c_arg_index}])\n"
        c_code += f"    if (!({unmarshal_call})) {{\n"
        # Log the specific type string used during the failed unmarshal attempt
        c_code += f"        LOG_ERR_JSON(json_arg{i}, \"Invoke Error: Failed to unmarshal JSON arg {i} as type '%s' for func '%s' ({sig_c_name})\", specific_type_str{c_arg_index}, entry->name);\n"
        c_code += f"        return false;\n"
        c_code += f"    }}\n"
    c_code += "\n"

    # --- Function Call ---
    # Cast the generic func_ptr to the SPECIFIC function type using info from entry
    c_code += "    // Cast function pointer to specific type from table entry\n"
    c_code += "    // Generate the C typedef string dynamically based on entry->ret_type and entry->arg_types\n"
    # This part needs careful C string construction within the generated C code.
    c_code += "    char func_typedef_str[512];\n" # Buffer for typedef string
    c_code += "    char func_args_str[256];\n" # Buffer for args part of typedef
    c_code += "    func_args_str[0] = '\\0';\n"
    c_code += f"   if ({num_c_args} == 0) {{\n"
    c_code += "        strcpy(func_args_str, \"void\");\n"
    c_code += "    } else {\n"
    c_code += "        for (int i = 0; i < " + str(num_c_args) + "; ++i) {\n" # Use Python's num_c_args here
    c_code += "            if (!entry->arg_types[i]) { strcat(func_args_str, \"void /*ERROR*/\"); }\n" # Error case
    c_code += "            else { strcat(func_args_str, entry->arg_types[i]); }\n"
    c_code += "            if (i < " + str(num_c_args - 1) + ") { strcat(func_args_str, \", \"); }\n"
    c_code += "        }\n"
    c_code += "    }\n"
    # Ensure ret_type exists
    c_code += "    const char* specific_ret_type = entry->ret_type ? entry->ret_type : \"void\";\n"
    c_code += "    snprintf(func_typedef_str, sizeof(func_typedef_str), \"%s (*)(%s)\", specific_ret_type, func_args_str);\n"
    c_code += "    // This ^^^ string building is just for potential debugging/logging.\n"
    c_code += "    // The actual cast uses the specific types directly:\n"

    # Generate the specific typedef and cast
    specific_ret_type_c = "entry->ret_type ? entry->ret_type : \"void\"" # C expression
    specific_arg_types_list_c = []
    if num_c_args > 0:
        for i in range(num_c_args):
            specific_arg_types_list_c.append(f"entry->arg_types[{i}] ? entry->arg_types[{i}] : \"void /*ERROR*/\"")
    specific_arg_types_str_c = ", ".join(specific_arg_types_list_c) if specific_arg_types_list_c else "void"

    # C code generation: typedef needs actual types, not strings. This approach is flawed.
    # We cannot dynamically create a C typedef from strings at runtime easily.
    # The C code must CAST the generic function pointer directly.

    # Revert: We *must* cast the generic void* func_ptr to the specific type *known at C compile time*.
    # This means the invoker *cannot* be fully generic based on the entry struct alone
    # without using libffi or similar.

    # --- Backtrack: Simpler approach that might work for common cases ---
    # Assume the simplified signature grouping is good enough for the function pointer *cast*.
    # This is the original flawed assumption, but let's try making the *call* work.

    c_code = f"// Generic Invoker for signature category: {signature_category}\n"
    c_code += f"// Handles {len(function_list)} functions like '{representative_func['name']}'\n"
    c_code += f"// WARNING: Uses simplified signature for casting func_ptr, relies on compatible calling conventions.\n"
    c_code += f"static bool {sig_c_name}(const invoke_table_entry_t *entry, void *target_obj_ptr, void *dest, cJSON *args_array) {{\n"
    c_code += f"    if (!entry || !entry->func_ptr || !entry->ret_type) {{ LOG_ERR(\"Invoke Error: Invalid entry passed to {sig_c_name} (for func '%s')\", entry ? entry->name : \"NULL_ENTRY\"); return false; }}\n"

    # Declare stack buffers based on simplified signature
    c_code += "    // Declare stack buffers for arguments (sized based on signature category)\n"
    arg_buffers = [] # Store names of buffer variables
    for i in range(num_c_args):
        buffer_type = _get_buffer_type_for_sig_component(sig_arg_comps[i])
        if buffer_type != "void":
           c_code += f"    {buffer_type} arg_buf{i};\n"
           arg_buffers.append(f"arg_buf{i}")

    result_buffer_type = "void"
    result_buffer_name = None
    if sig_ret_comp != 'void':
        result_buffer_type = _get_buffer_type_for_sig_component(sig_ret_comp)
        result_buffer_name = "result_buf"
        c_code += f"    {result_buffer_type} {result_buffer_name};\n"
    c_code += "\n"

    # --- Unmarshal using specific types from entry ---
    num_json_args_expected = num_c_args
    first_arg_is_target = False
    if num_c_args > 0 and (sig_arg_comps[0] == 'POINTER' or sig_arg_comps[0].endswith('*')):
        first_arg_is_target = True
        num_json_args_expected = num_c_args - 1
        c_code += f"    if (!entry->arg_types[0]) {{ LOG_ERR(\"Invoke Error: Missing type string for target arg 0 of '%s'\", entry->name); return false; }}\n"
        c_code += f"    arg_buf0 = (void*)target_obj_ptr;\n"

    # Check JSON array validity and size (same as before)
    c_code += f"    // Expecting {num_json_args_expected} arguments from JSON array for function '{representative_func}'\n"
    c_code += f"    if (!cJSON_IsArray(args_array)) {{\n"
    c_code += f"       if ({num_json_args_expected} == 0 && args_array == NULL) {{ /* Okay */ }}\n"
    c_code += f"       else {{ LOG_ERR_JSON(args_array, \"Invoke Error: args_array is not a valid array for {sig_c_name} (func '%s')\", entry->name); return false; }}\n"
    c_code += f"    }}\n"
    c_code += f"    int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);\n"
    c_code += f"    if (arg_count != {num_json_args_expected}) {{ LOG_ERR_JSON(args_array, \"Invoke Error: Expected {num_json_args_expected} JSON args for func '%s', got %d for {sig_c_name}\", entry->name, arg_count); return false; }}\n\n"

    # Unmarshal into buffers using specific types from entry
    c_code += "    // Unmarshal arguments from JSON into stack buffers using specific types from entry\n"
    for i in range(num_json_args_expected):
        c_arg_index = i + (1 if first_arg_is_target else 0)
        c_code += f"    const char* specific_type_str{c_arg_index} = entry->arg_types[{c_arg_index}];\n"
        c_code += f"    if (!specific_type_str{c_arg_index}) {{ LOG_ERR(\"Invoke Error: Internal setup error - missing type for arg {c_arg_index} of '%s'\", entry->name); return false; }}\n"
        buffer_ptr = f"&arg_buf{c_arg_index}"
        c_code += f"    cJSON *json_arg{i} = cJSON_GetArrayItem(args_array, {i});\n"
        c_code += f"    if (!json_arg{i}) {{ LOG_ERR(\"Invoke Error: Failed to get JSON arg {i} for func '%s' ({sig_c_name})\", entry->name); return false; }}\n"
        unmarshal_call = f"unmarshal_value(json_arg{i}, specific_type_str{c_arg_index}, (void*){buffer_ptr}, target_obj_ptr)"
        c_code += f"    // Unmarshal JSON arg {i} into C arg buffer {c_arg_index} (using specific type 'specific_type_str{c_arg_index}')\n"
        c_code += f"    if (!({unmarshal_call})) {{\n"
        c_code += f"        LOG_ERR_JSON(json_arg{i}, \"Invoke Error: Failed to unmarshal JSON arg {i} as type '%s' for func '%s' ({sig_c_name})\", specific_type_str{c_arg_index}, entry->name);\n"
        c_code += f"        return false;\n"
        c_code += f"    }}\n"
    c_code += "\n"

    # --- Function Call ---
    # Cast func_ptr based on the SIMPLIFIED signature category. This is the weak point.
    c_code += "    // Cast function pointer based on simplified signature category\n"
    simplified_ret_type = _get_buffer_type_for_sig_component(sig_ret_comp)
    simplified_arg_types = []
    for i in range(num_c_args):
        simplified_arg_types.append(_get_buffer_type_for_sig_component(sig_arg_comps[i]))
    simplified_args_str = ", ".join(simplified_arg_types) if simplified_arg_types else "void"

    c_code += f"    typedef {simplified_ret_type} (*invoker_func_type)({simplified_args_str});\n"
    c_code += f"    invoker_func_type target_func = (invoker_func_type)entry->func_ptr;\n\n"

    # --- BEGIN TRACE GENERIC INVOKER CALL ---
    c_code += "    // Prepare arguments for trace printf\\n"
    c_code += "    char L_target_trace_name[64] = \\\"NULL_TARGET\\\";\\n"
    c_code += "    if (target_obj_ptr && first_arg_is_target) { snprintf(L_target_trace_name, sizeof(L_target_trace_name), \\\"trace_obj_%p\\\", target_obj_ptr); }\\n"
    c_code += "    else if (target_obj_ptr) { snprintf(L_target_trace_name, sizeof(L_target_trace_name), \\\"target_obj_%p_NON_TRACEABLE\\\", target_obj_ptr); } // Target is not the first arg, or not ptr for trace obj format \\n"

    c_code += "    char L_args_for_trace[512]; L_args_for_trace[0] = '\\\\0'; int L_args_len = 0;\\n"
    c_code += "    char L_current_arg_str[128];\\n"

    for i in range(num_c_args):
        c_type_str_expr = f"entry->arg_types[{i}]" # C expression to get the type string
        sig_comp = sig_arg_comps[i]
        buffer_var_name = f"arg_buf{i}"

        c_code += f"    // Formatting arg {i} ({sig_comp} from {buffer_var_name}) using C type {c_type_str_expr}\\n"
        c_code += f"    memset(L_current_arg_str, 0, sizeof(L_current_arg_str));\\n"
        c_code += f"    const char* c_type_str_arg{i} = {c_type_str_expr};\\n"
        c_code += f"    if (!c_type_str_arg{i}) {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"<ERR_NO_TYPESTR_ARG{i}>\\\"); }}\\n"

        if i == 0 and first_arg_is_target:
            c_code += f"    strncpy(L_current_arg_str, L_target_trace_name, sizeof(L_current_arg_str)-1); // Use pre-formatted target trace name\\n"
        else:
            c_code += f"    const char* c_type_str_arg{i} = {c_type_str_expr};\\n"
            c_code += f"    if (!c_type_str_arg{i}) {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"<ERR_NO_TYPESTR_ARG{i}>\\\"); }}\\n"
            if sig_comp == 'const char *':
                c_code += f"    else {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"\\\\\\\"%s\\\\\\\"\\\", ({buffer_var_name}) ? (char*){buffer_var_name} : \\\"NULL_STR\\\"); }}\\n"
            elif sig_comp == 'POINTER' or sig_comp.endswith('*'): # Generic pointers
                c_code += f"    else {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"trace_obj_%p\\\", (void*){buffer_var_name}); }}\\n"
            elif sig_comp == 'BOOL':
            # Buffer type for BOOL is int64_t, value is 0 or 1 (or non-zero)
                c_code += f"    else {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"%s\\\", {buffer_var_name} ? \\\"true\\\" : \\\"false\\\"); }}\\n"
            else: # INT, lv_color_t, Enums assumed to be in int64_t buffer
                c_code += f"    else {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"%lld /* %s */\\\", (long long int){buffer_var_name}, c_type_str_arg{i}); }}\\n" # Use %lld for int64_t

        # Append to L_args_for_trace
        # The first actual argument to the function call trace is L_target_trace_name IF it's the object.
        # Subsequent args in L_args_for_trace are from the JSON.
        # This logic needs to be careful about which arg we are formatting for L_args_for_trace

        # Revised logic for L_args_for_trace construction:
        # It should contain ALL C arguments passed to the function, including target_obj_ptr if it's the first C arg.
        # So, the loop for L_args_for_trace should build a string of ALL args.
        # The printf then decides whether to print L_target_trace_name or the first part of L_args_for_trace.

        # Let's simplify: L_args_for_trace will be a string of all C arguments passed to target_func.
        # The printf will then just print entry->name(L_args_for_trace).
        if i == 0: # First argument overall
             c_code += f"    L_args_len += snprintf(L_args_for_trace + L_args_len, sizeof(L_args_for_trace) - L_args_len, \\\"%s\\\", L_current_arg_str);\\n"
        else: # Subsequent arguments
             c_code += f"    L_args_len += snprintf(L_args_for_trace + L_args_len, sizeof(L_args_for_trace) - L_args_len, \\\", %s\\\", L_current_arg_str);\\n"

    if num_c_args == 0 : # Handle functions with no arguments
        c_code += "    strncpy(L_args_for_trace, \"void\", sizeof(L_args_for_trace)-1);\\n"


    # Determine the first part of the call string for trace
    # If first_arg_is_target, L_target_trace_name should be used as the first argument.
    # L_args_for_trace should then contain the rest of the arguments.
    # This part of the prompt was complex and might need a different approach for constructing the printf.
    # For now, let's assume L_args_for_trace contains all arguments correctly formatted.

    c_code += f"    // Trace the call to the C invoker function itself and the actual LVGL function call it's about to make.\n"
    c_code += f"    printf(\"%s// Calling C invoker: {sig_c_name} for LVGL func '%s' on target '%s'\\n\", get_c_trace_indent_str(), entry->name, L_target_trace_name);\n"
    c_code += f"    printf(\"%s%s(%s); /* About to be invoked by {sig_c_name} */\\n\", get_c_trace_indent_str(), entry->name, L_args_for_trace);\\n"
    c_code += "    // --- END TRACE GENERIC INVOKER CALL ---\\n\n"


    c_code += "    // Call the target LVGL function using values from stack buffers\n"
    call_args_str = ", ".join(arg_buffers)

    if sig_ret_comp == 'void':
        c_code += f"    target_func({call_args_str});\n"
    else:
        c_code += f"    {result_buffer_name} = target_func({call_args_str});\n"
    c_code += "\n"

    # --- Store Result & Trace Result ---
    if sig_ret_comp != 'void':
        c_code += "    // Store result (from result_buf) if dest is provided and Trace Result\\n"
        c_code += "    if (dest) {\n"
        # Copy result buffer to destination. Size mismatch IS possible here.
        # Use specific type string from entry for casting the destination pointer.
        c_code += f"        const char* specific_ret_type_str = entry->ret_type ? entry->ret_type : \"void\";\n"
        c_code += f"        // Copy result from buffer to dest (casting dest based on specific return type '{sig_ret_comp}')\n"
        c_code += f"        // WARNING: Assumes calling convention compatibility & sufficient space at dest!\n"
        # Simple assignment cast (assuming primitive/pointer returns)
        c_code += f"        if (strchr(specific_ret_type_str, '*')) {{ // Pointer type\n"
        c_code += f"             *(void**)dest = (void*){result_buffer_name};\n"
        c_code += f"             printf(\"%s// Result for %s: trace_obj_%p\\n\", get_c_trace_indent_str(), entry->name, (void*){result_buffer_name});\n"
        c_code += f"        }} else if (strcmp(specific_ret_type_str, \"bool\") == 0) {{ // Bool type\n"
        c_code += f"             *(_Bool *)dest = (_Bool){result_buffer_name};\n" # Use _Bool for C99 bool
        c_code += f"             printf(\"%s// Result for %s: %s\\n\", get_c_trace_indent_str(), entry->name, {result_buffer_name} ? \"true\" : \"false\");\n"
        c_code += f"        }} else if (strcmp(specific_ret_type_str, \"float\") == 0 || strcmp(specific_ret_type_str, \"double\") == 0) {{ // Float/Double\n"
        c_code += f"             *({result_buffer_type} *)dest = ({result_buffer_type}){result_buffer_name};\n" # Assumes result_buffer_type is double
        c_code += f"             printf(\"%s// Result for %s: %g\\n\", get_c_trace_indent_str(), entry->name, (double){result_buffer_name});\n"
        c_code += f"        }} else if (strcmp(specific_ret_type_str, \"void\") != 0) {{ // Integer/Enum/Color?\n"
        c_code += f"             *({result_buffer_type} *)dest = ({result_buffer_type}){result_buffer_name};\n" # Assumes result_buffer_type is int64_t
        c_code += f"             printf(\"%s// Result for %s: %lld /* %s */\\n\", get_c_trace_indent_str(), entry->name, (long long int){result_buffer_name}, specific_ret_type_str);\n"
        c_code += f"        }}\n"
    c_code += "    } else {\n"
    c_code += "        // If dest is NULL but function is not void, still trace the call (done above) but not result storage or result trace.\n"
    c_code += "    }\n"

    c_code += "\n    return true;\n"
            c_code += f"    else {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"%s\\\", {buffer_var_name} ? \\\"true\\\" : \\\"false\\\"); }}\\n"
        else: # INT, lv_color_t, Enums assumed to be in int64_t buffer
            c_code += f"    else {{ snprintf(L_current_arg_str, sizeof(L_current_arg_str)-1, \\\"%lld /* %s */\\\", (long long int){buffer_var_name}, c_type_str_arg{i}); }}\\n" # Use %lld for int64_t

        # Append to L_args_for_trace
        # The first actual argument to the function call trace is L_target_trace_name IF it's the object.
        # Subsequent args in L_args_for_trace are from the JSON.
        # This logic needs to be careful about which arg we are formatting for L_args_for_trace

        # Revised logic for L_args_for_trace construction:
        # It should contain ALL C arguments passed to the function, including target_obj_ptr if it's the first C arg.
        # So, the loop for L_args_for_trace should build a string of ALL args.
        # The printf then decides whether to print L_target_trace_name or the first part of L_args_for_trace.

        # Let's simplify: L_args_for_trace will be a string of all C arguments passed to target_func.
        # The printf will then just print entry->name(L_args_for_trace).
        if i == 0: # First argument overall
             c_code += f"    L_args_len += snprintf(L_args_for_trace + L_args_len, sizeof(L_args_for_trace) - L_args_len, \\\"%s\\\", L_current_arg_str);\\n"
        else: # Subsequent arguments
             c_code += f"    L_args_len += snprintf(L_args_for_trace + L_args_len, sizeof(L_args_for_trace) - L_args_len, \\\", %s\\\", L_current_arg_str);\\n"

    if num_c_args == 0 : # Handle functions with no arguments
        c_code += "    strncpy(L_args_for_trace, \"void\", sizeof(L_args_for_trace)-1);\\n"


    # Determine the first part of the call string for trace
    # If first_arg_is_target, L_target_trace_name should be used as the first argument.
    # L_args_for_trace should then contain the rest of the arguments.
    # This part of the prompt was complex and might need a different approach for constructing the printf.
    # For now, let's assume L_args_for_trace contains all arguments correctly formatted.

    c_code += f"    printf(\"%s%s(%s); /* Invoked by {sig_c_name} */\\n\", get_c_trace_indent_str(), entry->name, L_args_for_trace);\\n"
    c_code += "    // --- END TRACE GENERIC INVOKER CALL ---\\n\n"


    c_code += "    // Call the target LVGL function using values from stack buffers\n"
    call_args_str = ", ".join(arg_buffers)

    if sig_ret_comp == 'void':
        c_code += f"    target_func({call_args_str});\n"
    else:
        c_code += f"    {result_buffer_name} = target_func({call_args_str});\n"
    c_code += "\n"

    # --- Store Result & Trace Result ---
    if sig_ret_comp != 'void':
        c_code += "    // Store result (from result_buf) if dest is provided and Trace Result\\n"
        c_code += "    if (dest) {\n"
        # Copy result buffer to destination. Size mismatch IS possible here.
        # Use specific type string from entry for casting the destination pointer.
        c_code += f"        const char* specific_ret_type_str = entry->ret_type ? entry->ret_type : \"void\";\n"
        c_code += f"        // Copy result from buffer to dest (casting dest based on specific return type '{sig_ret_comp}')\n"
        c_code += f"        // WARNING: Assumes calling convention compatibility & sufficient space at dest!\n"
        # Simple assignment cast (assuming primitive/pointer returns)
        c_code += f"        if (strchr(specific_ret_type_str, '*')) {{ // Pointer type\n"
        c_code += f"             *(void**)dest = (void*){result_buffer_name};\n"
        c_code += f"             printf(\"%s// Result for %s: trace_obj_%p\\n\", get_c_trace_indent_str(), entry->name, (void*){result_buffer_name});\n"
        c_code += f"        }} else if (strcmp(specific_ret_type_str, \"bool\") == 0) {{ // Bool type\n"
        c_code += f"             *(_Bool *)dest = (_Bool){result_buffer_name};\n" # Use _Bool for C99 bool
        c_code += f"             printf(\"%s// Result for %s: %s\\n\", get_c_trace_indent_str(), entry->name, {result_buffer_name} ? \"true\" : \"false\");\n"
        c_code += f"        }} else if (strcmp(specific_ret_type_str, \"float\") == 0 || strcmp(specific_ret_type_str, \"double\") == 0) {{ // Float/Double\n"
        c_code += f"             *({result_buffer_type} *)dest = ({result_buffer_type}){result_buffer_name};\n" # Assumes result_buffer_type is double
        c_code += f"             printf(\"%s// Result for %s: %g\\n\", get_c_trace_indent_str(), entry->name, (double){result_buffer_name});\n"
        c_code += f"        }} else if (strcmp(specific_ret_type_str, \"void\") != 0) {{ // Integer/Enum/Color?\n"
        c_code += f"             *({result_buffer_type} *)dest = ({result_buffer_type}){result_buffer_name};\n" # Assumes result_buffer_type is int64_t
        c_code += f"             printf(\"%s// Result for %s: %lld /* %s */\\n\", get_c_trace_indent_str(), entry->name, (long long int){result_buffer_name}, specific_ret_type_str);\n"
        c_code += f"        }}\n"
        c_code += "    } else {\n"
        c_code += "        // If dest is NULL but function is not void, still trace the call (done above) but not result storage or result trace.\n"
        c_code += "    }\n"

    c_code += "\n    return true;\n"
    c_code += "}\n\n"
    return c_code


# Rest of invocation.py (generate_invocation_helpers, generate_invoke_table, generate_find_function)
# remains the same as in the previous correct version, using the updated invoke_fn_t signature
# and populating the table entry fields correctly.


def generate_invocation_helpers(signatures, api_info):
    """Generates all the C invoke_fn_t helper functions."""
    global generated_invoke_fns
    generated_invoke_fns.clear() # Reset for idempotency

    c_code = ""
    c_code += "// Forward declaration for the main unmarshaler\n"
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent);\n\n"
    # Forward declare the table struct type for the invoker signatures
    c_code += "struct invoke_table_entry_s;\n"
    c_code += "typedef struct invoke_table_entry_s invoke_table_entry_t;\n\n"


    c_code += "// --- Invocation Helper Functions ---\n"

    signature_map = {} # Map func_name -> invoke_fn_name

    # Sort signatures for consistent output order (optional)
    # Need to convert dict_keys to list before sorting if using Python 3.7+
    sorted_signatures = sorted(list(signatures.keys()))

    # Generate the special create invoker if needed
    widget_create_invoker_name = None
    if WIDGET_CREATE_SIGNATURE in signatures:
        widget_create_invoker_name, create_invoker_code = _generate_widget_create_invoker()
        c_code += create_invoker_code
        generated_invoke_fns[WIDGET_CREATE_SIGNATURE] = widget_create_invoker_name
        # Map all functions with this signature to the special invoker
        for func_dict in signatures[WIDGET_CREATE_SIGNATURE]:
            signature_map[func_dict['name']] = widget_create_invoker_name
        logger.info(f"Generated specific invoker '{widget_create_invoker_name}' for WIDGET_CREATE functions.")


    # Generate generic invokers for all other signatures
    for signature_category in sorted_signatures:
        # Skip the special create signature if we already generated it
        if signature_category == WIDGET_CREATE_SIGNATURE:
            continue

        # Get list of function dicts
        function_list = signatures[signature_category]
        if not function_list: continue # Should not happen

        # Create a C-safe name for the signature function based on category
        sig_c_name_parts = [str(s).replace(' ', '_').replace('*', 'p').replace('[', '_').replace(']', '_') for s in signature_category]
        sig_c_name = f"invoke_{'_'.join(sig_c_name_parts)}"
        sig_c_name = ''.join(c if c.isalnum() else '_' for c in sig_c_name) # Basic sanitization
        if not sig_c_name[0].isalpha() and sig_c_name[0] != '_':
             sig_c_name = "_" + sig_c_name
        # Add length limit? Avoid collisions. Maybe add hash of signature? For now, keep simple.

        # Check if we already generated an invoker for this exact C name
        # (Can happen if different categories sanitize to same name) - Add suffix if needed.
        original_sig_c_name = sig_c_name
        name_suffix = 0
        while sig_c_name in generated_invoke_fns.values() and name_suffix < 100:
            name_suffix += 1
            sig_c_name = f"{original_sig_c_name}_{name_suffix}"
        if name_suffix >= 100:
            logger.error(f"Could not generate unique C name for invoker category {signature_category}")
            continue

        # Store the mapping for the table generation
        generated_invoke_fns[signature_category] = sig_c_name
        for func_dict in function_list:
            # Only map if not already mapped (e.g., create funcs mapped above)
            if func_dict['name'] not in signature_map:
                 signature_map[func_dict['name']] = sig_c_name

        # Generate the C code for this invoker
        # Pass function_list which contains representative function info needed by generator
        c_code += _generate_generic_invoke_fn(signature_category, sig_c_name, function_list, api_info)

    return c_code, signature_map

def generate_invoke_table_def():
    """Generates the invoke table definition and data."""

    c_code = "// --- Invocation Table ---\n\n"
    c_code += "// Forward declaration of the invoker function signature type\n"
    c_code += "struct invoke_table_entry_s;\n"
    c_code += "typedef bool (*invoke_fn_t)(const struct invoke_table_entry_s *entry, void *target_obj_ptr, void *dest, cJSON *args_array);\n\n"

    c_code += "// Structure for each entry in the invocation table\n"
    c_code += f"typedef struct invoke_table_entry_s {{\n" # Use struct tag here
    c_code += "    const char *name;           // LVGL function name (e.g., \"lv_obj_set_width\")\n"
    c_code += "    invoke_fn_t invoke;       // Pointer to the C invocation wrapper function\n"
    c_code += "    void *func_ptr;         // Pointer to the actual LVGL function\n"
    c_code += "    const char *ret_type;       // Specific C return type string\n"
    c_code += f"    const char *arg_types[{MAX_ARGS_SUPPORTED}]; // Specific C argument type strings\n"
    c_code += f"}} invoke_table_entry_t;\n\n" # Typedef name here
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
            # Ensure the C invoker function exists
            sig = get_signature(func)
            if sig not in generated_invoke_fns:
                 logger.error(f"Internal Error: Signature {sig} for function '{name}' mapped to '{invoke_func_name}' but no invoker function was generated for this signature.")
                 continue

            # Get specific type strings
            ret_c_type, ret_ptr_lvl, _ = func['_resolved_ret_type']
            specific_ret_type_str = get_c_type_str(ret_c_type, ret_ptr_lvl)

            specific_arg_type_strs = ["NULL"] * MAX_ARGS_SUPPORTED # Initialize with NULL
            num_args = len(func['_resolved_arg_types'])
            if num_args > MAX_ARGS_SUPPORTED:
                logger.error(f"Function '{name}' has {num_args} arguments, exceeding MAX_ARGS_SUPPORTED ({MAX_ARGS_SUPPORTED}). Skipping invoke table entry.")
                continue
            for i in range(num_args):
                 arg_c_type, arg_ptr_lvl, _ = func['_resolved_arg_types'][i]
                 specific_arg_type_strs[i] = f"\"{get_c_type_str(arg_c_type, arg_ptr_lvl)}\"" # Store as quoted string literal

            # Format the entry
            c_code += f"    {{\n"
            c_code += f"        .name = \"{name}\",\n"
            c_code += f"        .invoke = &{invoke_func_name},\n"
            c_code += f"        .func_ptr = (void*)&{name},\n"
            c_code += f"        .ret_type = \"{specific_ret_type_str}\",\n"
            c_code += f"        .arg_types = {{ {', '.join(specific_arg_type_strs)} }}\n"
            c_code += f"    }},\n"
            count += 1
        else:
            # This function was filtered but didn't map to any generated invoker
            logger.warning(f"Function '{name}' is filtered but has no invoker mapping. Skipping invoke table entry.")

    c_code += "    {NULL, NULL, NULL, NULL, {NULL}} // Sentinel\n" # Match struct init
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