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
    sig_c_name = "invoke_widget_create"
    c_code = f"// Specific Invoker for functions like lv_widget_create(lv_obj_t *parent)\n"
    c_code += f"// Signature: expects target_obj_ptr_render_mode = parent, result_dest_render_mode = lv_obj_t**, args_array = NULL\n"
    c_code += f"static bool {sig_c_name}(const invoke_table_entry_t *entry, void *target_obj_ptr_render_mode, const char *target_obj_c_name_transpile, void *result_dest_render_mode, char **result_c_name_transpile, cJSON *args_array) {{\n"
    c_code += f"    if (g_current_operation_mode == RENDER_MODE) {{\n"
    c_code += f"        if (!entry || !entry->func_ptr) {{ LOG_ERR(\"Invoke Error: NULL entry or func_ptr for {sig_c_name}\"); return false; }}\n"
    c_code += f"        if (!result_dest_render_mode) {{ LOG_ERR(\"Invoke Error: result_dest_render_mode is NULL for {sig_c_name} (needed for result)\"); return false; }}\n"
    c_code += f"        if (args_array != NULL && cJSON_GetArraySize(args_array) > 0) {{\n"
    c_code += f"           LOG_WARN_JSON(args_array, \"Invoke Warning: {sig_c_name} expected 0 JSON args, got %d for func '%s'. Ignoring JSON args.\", cJSON_GetArraySize(args_array), entry->name);\n"
    c_code += f"        }}\n\n"
    c_code += f"        lv_obj_t* parent = (lv_obj_t*)target_obj_ptr_render_mode;\n"
    c_code += f"        typedef lv_obj_t* (*specific_lv_create_func_type)(lv_obj_t*);\n"
    c_code += f"        specific_lv_create_func_type target_func = (specific_lv_create_func_type)entry->func_ptr;\n"
    c_code += f"        lv_obj_t* result = target_func(parent);\n"
    c_code += f"        *(lv_obj_t**)result_dest_render_mode = result;\n"
    c_code += f"        if (!result) {{\n"
    c_code += f"            LOG_WARN(\"Invoke Warning: Create function '%s' returned NULL.\", entry->name);\n"
    c_code += f"        }}\n"
    c_code += f"    }} else {{ // TRANSPILE_MODE\n"
    c_code += f"        // This invoker should ideally not be called for widget creation in transpile mode,\n"
    c_code += f"        // as render_json_node_internal handles it directly by generating lv_..._create(...) calls.\n"
    c_code += f"        // If it IS called, it means a JSON like {{ \"invoke\": \"lv_obj_create\", \"args\": [] }} was processed by apply_setters_and_attributes.\n"
    c_code += f"        // This is unusual for create functions which are normally part of node's 'type'.\n"
    c_code += f"        LOG_WARN(\"invoke_widget_create called in TRANSPILE_MODE for %s. Creation is usually handled by render_json_node_internal. This implies an 'invoke' attribute was used for a create function.\", entry->name);\n"
    c_code += f"        // We need to generate a C variable for the result.\n"
    c_code += f"        if (result_c_name_transpile && strcmp(entry->ret_type, \"void\") != 0) {{\n"
    c_code += "             g_transpile_var_counter++;\n"
    c_code += f"            *result_c_name_transpile = transpile_generate_c_var_name(NULL, entry->ret_type, g_transpile_var_counter);\n"
    c_code += f"            if (!*result_c_name_transpile) {{ LOG_ERR(\"TRANSPILE_MODE: Failed to generate result C name for %s\", entry->name); return false; }}\n"
    c_code += f"            // No extern for H file here as it's assumed to be a local var in the create_ui function.\n"
    c_code += f"            // The parent for creation in this invoked scenario is target_obj_c_name_transpile (if it's the parent obj var name)\n"
    c_code += f"            // or more generically, it should be explicitly passed if this pattern is to be supported.\n"
    c_code += f"            const char* parent_c_var = target_obj_c_name_transpile ? target_obj_c_name_transpile : \"NULL\"; // Assuming target_obj_c_name_transpile is the parent C var.\n"
    c_code += f"            transpile_write_line(g_output_c_file, \"    %s *%s = %s(%s);\", entry->ret_type, *result_c_name_transpile, entry->name, parent_c_var);\n"
    c_code += f"        }} else if (result_c_name_transpile) {{\n"
    c_code += f"            *result_c_name_transpile = NULL;\n"
    c_code += f"        }}\n"
    c_code += f"        // No actual call, just C code generation. Assume success unless name generation failed.\n"
    c_code += f"    }}\n"
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

    c_code = f"// Generic Invoker for signature category: {signature_category}\n"
    c_code += f"// Handles {len(function_list)} functions like '{representative_func['name']}'\n"
    c_code += f"// WARNING: Uses simplified signature for casting func_ptr in RENDER_MODE, relies on compatible calling conventions.\n"
    c_code += f"static bool {sig_c_name}(const invoke_table_entry_t *entry, void *target_obj_ptr_render_mode, const char *target_obj_c_name_transpile, void *result_dest_render_mode, char **result_c_name_transpile, cJSON *args_array) {{\n"
    c_code += f"    if (!entry || !entry->func_ptr || !entry->ret_type) {{ LOG_ERR(\"Invoke Error: Invalid entry passed to {sig_c_name} (for func '%s')\", entry ? entry->name : \"NULL_ENTRY\"); return false; }}\n"
    c_code += f"    if (g_current_operation_mode == RENDER_MODE) {{\n"
    # RENDER_MODE: Existing logic, using target_obj_ptr_render_mode and result_dest_render_mode
    arg_buffers = []
    for i in range(num_c_args):
        buffer_type = _get_buffer_type_for_sig_component(sig_arg_comps[i])
        if buffer_type != "void":
           c_code += f"        {buffer_type} arg_buf{i};\n"
           arg_buffers.append(f"arg_buf{i}")
    result_buffer_type = "void"
    result_buffer_name = None
    if sig_ret_comp != 'void':
        result_buffer_type = _get_buffer_type_for_sig_component(sig_ret_comp)
        result_buffer_name = "result_buf"
        c_code += f"        {result_buffer_type} {result_buffer_name};\n"
    c_code += "\n"
    num_json_args_expected = num_c_args
    first_arg_is_target = False
    if num_c_args > 0 and (sig_arg_comps[0] == 'POINTER' or sig_arg_comps[0].endswith('*')):
        first_arg_is_target = True
        num_json_args_expected = num_c_args - 1
        c_code += f"        if (!entry->arg_types[0]) {{ LOG_ERR(\"Invoke Error: Missing type string for target arg 0 of '%s'\", entry->name); return false; }}\n"
        c_code += f"        arg_buf0 = (void*)target_obj_ptr_render_mode;\n"
    c_code += f"        if (!cJSON_IsArray(args_array)) {{\n"
    c_code += f"           if ({num_json_args_expected} == 0 && args_array == NULL) {{ /* Okay */ }}\n"
    c_code += f"           else {{ LOG_ERR_JSON(args_array, \"Invoke Error: args_array is not a valid array for {sig_c_name} (func '%s')\", entry->name); return false; }}\n"
    c_code += f"        }}\n"
    c_code += f"        int arg_count = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);\n"
    c_code += f"        if (arg_count != {num_json_args_expected}) {{ LOG_ERR_JSON(args_array, \"Invoke Error: Expected {num_json_args_expected} JSON args for func '%s', got %d for {sig_c_name}\", entry->name, arg_count); return false; }}\n\n"
    for i in range(num_json_args_expected):
        c_arg_index = i + (1 if first_arg_is_target else 0)
        c_code += f"        const char* specific_type_str{c_arg_index} = entry->arg_types[{c_arg_index}];\n"
        c_code += f"        if (!specific_type_str{c_arg_index}) {{ LOG_ERR(\"Invoke Error: Internal setup error - missing type for arg {c_arg_index} of '%s'\", entry->name); return false; }}\n"
        buffer_ptr = f"&arg_buf{c_arg_index}"
        c_code += f"        cJSON *json_arg{i} = cJSON_GetArrayItem(args_array, {i});\n"
        c_code += f"        if (!json_arg{i}) {{ LOG_ERR(\"Invoke Error: Failed to get JSON arg {i} for func '%s' ({sig_c_name})\", entry->name); return false; }}\n"
        unmarshal_call = f"unmarshal_value(json_arg{i}, specific_type_str{c_arg_index}, (void*){buffer_ptr}, target_obj_ptr_render_mode)"
        c_code += f"        if (!({unmarshal_call})) {{\n"
        c_code += f"            LOG_ERR_JSON(json_arg{i}, \"Invoke Error: Failed to unmarshal JSON arg {i} as type '%s' for func '%s' ({sig_c_name})\", specific_type_str{c_arg_index}, entry->name);\n"
        c_code += f"            return false;\n"
        c_code += f"        }}\n"
    c_code += "\n"
    simplified_ret_type = _get_buffer_type_for_sig_component(sig_ret_comp)
    simplified_arg_types_render = []
    for i_render in range(num_c_args): # Use a different loop variable
        simplified_arg_types_render.append(_get_buffer_type_for_sig_component(sig_arg_comps[i_render]))
    simplified_args_str = ", ".join(simplified_arg_types_render) if simplified_arg_types_render else "void"
    c_code += f"        typedef {simplified_ret_type} (*invoker_func_type)({simplified_args_str});\n"
    c_code += f"        invoker_func_type target_func = (invoker_func_type)entry->func_ptr;\n"
    call_args_str = ", ".join(arg_buffers)
    if sig_ret_comp == 'void':
        c_code += f"        target_func({call_args_str});\n"
    else:
        c_code += f"        {result_buffer_name} = target_func({call_args_str});\n"
    c_code += "\n"
    if sig_ret_comp != 'void':
        c_code += "        if (result_dest_render_mode) {\n"
        c_code += f"            const char* specific_ret_type_str = entry->ret_type ? entry->ret_type : \"void\";\n"
        c_code += f"            if (strchr(specific_ret_type_str, '*')) {{ *(void**)result_dest_render_mode = (void*){result_buffer_name}; }}\n"
        c_code += f"            else if (strcmp(specific_ret_type_str, \"float\") == 0 || strcmp(specific_ret_type_str, \"double\") == 0) {{ *({result_buffer_type} *)result_dest_render_mode = ({result_buffer_type}){result_buffer_name}; }}\n"
        c_code += f"            else if (strcmp(specific_ret_type_str, \"void\") != 0) {{ *({result_buffer_type} *)result_dest_render_mode = ({result_buffer_type}){result_buffer_name}; }}\n"
        c_code += "        }\n"
    c_code += f"    }} else {{ // TRANSPILE_MODE\n"
    c_code += f"        char* c_code_args[{MAX_ARGS_SUPPORTED}];\n"
    c_code += f"        int c_code_arg_count = 0;\n"
    c_code += f"        bool transpile_arg_ok = true;\n"
    c_code += f"        int num_json_args_expected_transpile = {num_c_args} - ({1 if first_arg_is_target else 0});\n"
    c_code += f"        int current_arg_count_transpile = (args_array == NULL) ? 0 : cJSON_GetArraySize(args_array);\n"
    c_code += f"        if (current_arg_count_transpile != num_json_args_expected_transpile) {{ LOG_ERR_JSON(args_array, \"TRANSPILE Error: Expected %d JSON args for func '%s', got %d\", num_json_args_expected_transpile, entry->name, current_arg_count_transpile); return false; }}\n\n"

    c_code += f"        for (int i = 0; i < num_json_args_expected_transpile; ++i) {{\n"
    c_code += f"            cJSON *json_arg = cJSON_GetArrayItem(args_array, i);\n"
    c_code += f"            int c_arg_idx = i + ({1 if first_arg_is_target else 0});\n"
    c_code += f"            const char* arg_type_str = entry->arg_types[c_arg_idx];\n"
    c_code += f"            if (!arg_type_str) {{ LOG_ERR(\"TRANSPILE Error: Missing type for arg %d of '%s'\", c_arg_idx, entry->name); transpile_arg_ok = false; break; }}\n"
    c_code += f"            // Use target_obj_ptr_render_mode for context, as it might hold a temporary object for @-pointer resolution even in transpile mode planning.\n"
    c_code += f"            if (!unmarshal_value_to_c_string(json_arg, arg_type_str, &c_code_args[c_code_arg_count++], target_obj_ptr_render_mode)) {{\n"
    c_code += f"                LOG_ERR_JSON(json_arg, \"TRANSPILE Error: Failed to unmarshal JSON arg %d to C string for func '%s' (type '%s')\", i, entry->name, arg_type_str);\n"
    c_code += f"                transpile_arg_ok = false; break;\n"
    c_code += f"            }}\n"
    c_code += f"        }}\n\n"
    c_code += f"        if (!transpile_arg_ok) {{\n"
    c_code += f"            for (int k = 0; k < c_code_arg_count; ++k) lv_free(c_code_args[k]);\n"
    c_code += f"            return false;\n"
    c_code += f"        }}\n\n"
    c_code += f"        char call_str[1024]; call_str[0] = '\\0';\n"
    c_code += f"        char all_args_c_str[512]; all_args_c_str[0] = '\\0';\n"
    c_code += f"        int current_c_code_arg = 0;\n"

    # Construct argument list string
    c_code += f"        for (int k = 0; k < c_code_arg_count; ++k) {{\n"
    c_code += f"            strcat(all_args_c_str, c_code_args[k]);\n"
    c_code += f"            if (k < c_code_arg_count - 1) strcat(all_args_c_str, \", \");\n"
    c_code += f"        }}\n\n"

    c_code += f"        const char* effective_target_c_name = target_obj_c_name_transpile;\n"
    c_code += f"        // If first arg is target, it's the object itself. If not, target_obj_c_name_transpile might be NULL or irrelevant for static calls.\n"
    c_code += f"        bool is_method_call = {str(first_arg_is_target).lower()};\n"
    # Handle return value assignment
    c_code += f"        if (strcmp(entry->ret_type, \"void\") != 0 && result_c_name_transpile != NULL) {{\n"
    c_code += "             g_transpile_var_counter++;\n"
    c_code += f"            *result_c_name_transpile = transpile_generate_c_var_name(NULL, entry->ret_type, g_transpile_var_counter);\n"
    c_code += f"            if (!*result_c_name_transpile) {{ LOG_ERR(\"TRANSPILE_MODE: Failed to generate result C name for %s\", entry->name); transpile_arg_ok = false; }}\n"
    c_code += f"            else {{\n"
    c_code += f"                // transpile_write_line(g_output_h_file, \"extern %s %s; // Result of %s\", entry->ret_type, *result_c_name_transpile, entry->name); // Optional H declaration\n"
    c_code += f"                if (is_method_call) {{\n"
    c_code += f"                    snprintf(call_str, sizeof(call_str), \"    %s %s = %s(%s%s%s);\", entry->ret_type, *result_c_name_transpile, entry->name, effective_target_c_name ? effective_target_c_name : \"NULL_TARGET_ERR\", (c_code_arg_count > 0 ? \", \" : \"\"), all_args_c_str);\n"
    c_code += f"                }} else {{\n"
    c_code += f"                    snprintf(call_str, sizeof(call_str), \"    %s %s = %s(%s);\", entry->ret_type, *result_c_name_transpile, entry->name, all_args_c_str);\n"
    c_code += f"                }}\n"
    c_code += f"            }}\n"
    c_code += f"        }} else {{\n" // No return value or result_c_name_transpile is NULL
    c_code += f"            if (is_method_call) {{\n"
    c_code += f"                snprintf(call_str, sizeof(call_str), \"    %s(%s%s%s);\", entry->name, effective_target_c_name ? effective_target_c_name : \"NULL_TARGET_ERR\", (c_code_arg_count > 0 ? \", \" : \"\"), all_args_c_str);\n"
    c_code += f"            }} else {{\n"
    c_code += f"                snprintf(call_str, sizeof(call_str), \"    %s(%s);\", entry->name, all_args_c_str);\n"
    c_code += f"            }}\n"
    c_code += f"        }}\n\n"
    c_code += f"        if (transpile_arg_ok) {{ transpile_write_line(g_output_c_file, call_str); }}\n\n"
    c_code += f"        for (int k = 0; k < c_code_arg_count; ++k) lv_free(c_code_args[k]);\n"
    c_code += f"        if (!transpile_arg_ok && result_c_name_transpile && *result_c_name_transpile) {{ lv_free(*result_c_name_transpile); *result_c_name_transpile = NULL; }}\n"
    c_code += f"        return transpile_arg_ok;\n"
    c_code += f"    }}\n" # End RENDER_MODE / TRANSPILE_MODE if/else
    c_code += f"    return true;\n" # Default return for RENDER_MODE if no errors
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
    c_code += "static bool unmarshal_value(cJSON *json_value, const char *expected_c_type, void *dest, void *implicit_parent);\n"
    c_code += "// Forward declaration for the C string unmarshaler (to be implemented in unmarshal.py's generated code)\n"
    c_code += "static bool unmarshal_value_to_c_string(cJSON *json_value, const char *expected_c_type, char **c_string_dest, void *implicit_parent_for_context);\n"
    c_code += "// Forward declarations for transpilation helpers (defined in C_SOURCE_TEMPLATE of generator.py)\n"
    c_code += "typedef enum { RENDER_MODE, TRANSPILE_MODE } operation_mode_t;\n"
    c_code += "extern operation_mode_t g_current_operation_mode;\n"
    c_code += "extern FILE *g_output_c_file;\n"
    c_code += "extern FILE *g_output_h_file;\n"
    c_code += "extern int g_transpile_var_counter;\n"
    c_code += "static char* transpile_generate_c_var_name(const char* json_id, const char* type, int count);\n"
    c_code += "static void transpile_write_line(FILE* file, const char *line_fmt, ...);\n\n"

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
    # Updated invoke_fn_t signature
    c_code += "typedef bool (*invoke_fn_t)(\n"
    c_code += "    const struct invoke_table_entry_s *entry,\n"
    c_code += "    void *target_obj_ptr_render_mode,\n"
    c_code += "    const char *target_obj_c_name_transpile,\n"
    c_code += "    void *result_dest_render_mode,\n"
    c_code += "    char **result_c_name_transpile,\n"
    c_code += "    cJSON *json_args_array\n"
    c_code += ");\n\n"

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