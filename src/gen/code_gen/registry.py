# code_gen/registry.py
import logging
from type_utils import get_c_type_str, lvgl_type_to_widget_name

logger = logging.getLogger(__name__)

# Simple registry config
MAX_REGISTRY_SIZE = 100 # For static array implementation

def generate_registry(use_hash_map=False):
    """Generates the C code for the named pointer registry."""
    c_code = "// --- Pointer Registry ---\n\n"
    c_code += "#include <string.h>\n"
    c_code += "#include <stdlib.h>\n\n" # For malloc/free if needed

    if use_hash_map:
        # Basic hash map implementation needed here (or use external C lib)
        c_code += "// Basic Hash Map Registry (Placeholder - requires implementation)\n"
        c_code += "#define HASH_MAP_SIZE 256\n"
        c_code += "typedef struct registry_entry {\n"
        c_code += "    char *name;\n"
        c_code += "    char *type_name; // Added for type safety\n" # ADDED LINE
        c_code += "    void *ptr;\n"
        c_code += "    struct registry_entry *next;\n"
        c_code += "} registry_entry_t;\n\n"
        c_code += "static registry_entry_t* g_registry_map[HASH_MAP_SIZE] = {0};\n\n"
        c_code += "static unsigned int hash(const char *str) {\n"
        c_code += "    unsigned long hash = 5381;\n"
        c_code += "    int c;\n"
        c_code += "    while ((c = *str++)) hash = ((hash << 5) + hash) + c; /* djb2 */\n"
        c_code += "    return hash % HASH_MAP_SIZE;\n"
        c_code += "}\n\n"
        c_code += "void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr) {\n"
        c_code += "    if (!name || !type_name || !ptr) return;\n" # MODIFIED CONDITION
        c_code += "    unsigned int index = hash(name);\n"
        c_code += "    // Check if name already exists (update or handle error?)\n"
        c_code += "    registry_entry_t *entry = g_registry_map[index];\n"
        c_code += "    while(entry) {\n"
        c_code += "        if(strcmp(entry->name, name) == 0) {\n"
        c_code += "             LOG_WARN(\"Registry Warning: Name '%s' already registered. Updating pointer and type.\", name);\n"
        c_code += "             LV_FREE(entry->type_name); // Free old type_name\n" # ADDED LINE
        c_code += "             entry->type_name = lv_strdup(type_name); // Update type_name\n" # ADDED LINE
        c_code += "             if (!entry->type_name) { LOG_ERR(\"Registry Error: Failed to duplicate type_name for update\"); /* What to do? Original ptr is kept */ return; }\n" # ADDED LINE
        c_code += "             entry->ptr = ptr; // Update existing entry\n"
        c_code += "             return;\n"
        c_code += "        }\n"
        c_code += "        entry = entry->next;\n"
        c_code += "    }\n"
        c_code += "    // Add new entry\n"
        c_code += "    registry_entry_t *new_entry = (registry_entry_t *)LV_MALLOC(sizeof(registry_entry_t));\n"
        c_code += "    if (!new_entry) { LOG_ERR(\"Registry Error: Failed to allocate memory\"); return; }\n"
        c_code += "    new_entry->name = lv_strdup(name);\n"
        c_code += "    if (!new_entry->name) { LV_FREE(new_entry); LOG_ERR(\"Registry Error: Failed to duplicate name\"); return; }\n"
        c_code += "    new_entry->type_name = lv_strdup(type_name);\n" # ADDED LINE
        c_code += "    if (!new_entry->type_name) { LV_FREE(new_entry->name); LV_FREE(new_entry); LOG_ERR(\"Registry Error: Failed to duplicate type_name\"); return; }\n" # ADDED LINE
        c_code += "    new_entry->ptr = ptr;\n"
        c_code += "    new_entry->next = g_registry_map[index];\n"
        c_code += "    g_registry_map[index] = new_entry;\n"
        c_code += "     LOG_INFO(\"Registered pointer '%s' with type '%s'\", name, type_name);\n" # MODIFIED LOG
        c_code += "}\n\n"

        c_code += "void* lvgl_json_get_registered_ptr(const char *name, const char *expected_type_name) {\n" # MODIFIED SIGNATURE
        c_code += "    if (!name) return NULL;\n"
        c_code += "    unsigned int index = hash(name);\n"
        c_code += "    registry_entry_t *entry = g_registry_map[index];\n"
        c_code += "    while (entry != NULL) {\n"
        c_code += "        if (strcmp(entry->name, name) == 0) {\n"
        c_code += "            // Type check\n"
        c_code += "            if (expected_type_name == NULL || entry->type_name == NULL) { // Wildcard or error in registration\n"
        c_code += "                 if(expected_type_name != NULL && entry->type_name == NULL) LOG_WARN(\"Registry: Entry '%s' has no type_name.\", name);\n"
        c_code += "                 return entry->ptr; // No type check possible or requested\n"
        c_code += "            }\n"
        c_code += "            // Smart type comparison: if expected is 'type*', compare with 'type'\n"
        c_code += "            size_t expected_len = strlen(expected_type_name);\n"
        c_code += "            bool types_match = false;\n"
        c_code += "            if (expected_len > 0 && expected_type_name[expected_len - 1] == '*') {\n"
        c_code += "                 // Expected a pointer type like 'lv_style_t *', compare base 'lv_style_t'\n"
        c_code += "                 // Need to be careful with 'const char **' vs 'const char *'\n"
        c_code += "                 // Simple check: compare entry->type_name with expected_type_name minus the last '*'\n"
        c_code += "                 // This assumes stored type_name is always the base type (e.g., \"lv_style_t\")\n"
        c_code += "                 char base_expected_type[256]; // Assume type names are not excessively long\n"
        c_code += "                 if (expected_len -1 < sizeof(base_expected_type)) {\n"
        c_code += "                    strncpy(base_expected_type, expected_type_name, expected_len -1);\n"
        c_code += "                    base_expected_type[expected_len -1] = '\\0';\n"
        c_code += "                    // Trim trailing space if it was 'lv_style_t *'\n"
        c_code += "                    if(expected_len > 1 && base_expected_type[expected_len - 2] == ' ') base_expected_type[expected_len - 2] = '\\0';\n"
        c_code += "                    types_match = (strcmp(entry->type_name, base_expected_type) == 0);\n"
        c_code += "                 }\n"
        c_code += "            } else {\n"
        c_code += "                 // Expected a non-pointer type or already a base type, direct compare\n"
        c_code += "                 types_match = (strcmp(entry->type_name, expected_type_name) == 0);\n"
        c_code += "            }\n\n"
        c_code += "            if (types_match) {\n"
        c_code += "                 return entry->ptr;\n"
        c_code += "            } else {\n"
        c_code += "                 LOG_WARN(\"Registry: Found entry '%s', but type mismatch. Expected compatible with '%s', got '%s'.\", name, expected_type_name, entry->type_name);\n"
        c_code += "                 return NULL; // Type mismatch for the found name\n"
        c_code += "            }\n"
        c_code += "        }\n"
        c_code += "        entry = entry->next;\n"
        c_code += "    }\n"
        c_code += "    return NULL;\n"
        c_code += "}\n\n"

        c_code += "void lvgl_json_registry_clear() {\n"
        c_code += "    for(int i = 0; i < HASH_MAP_SIZE; ++i) {\n"
        c_code += "        registry_entry_t *entry = g_registry_map[i];\n"
        c_code += "        while(entry) {\n"
        c_code += "             registry_entry_t *next = entry->next;\n"
        c_code += "             LV_FREE(entry->type_name);\n"
        c_code += "             LV_FREE(entry->name);\n"
        c_code += "             LV_FREE(entry);\n"
        c_code += "             entry = next;\n"
        c_code += "        }\n"
        c_code += "        g_registry_map[i] = NULL;\n"
        c_code += "    }\n"
        c_code += "     LOG_INFO(\"Pointer registry cleared.\");\n"
        c_code += "}\n\n"

    else:
        # Simple static array registry
        c_code += "// Simple Static Array Registry\n"
        c_code += f"#define MAX_REGISTERED_PTRS {MAX_REGISTRY_SIZE}\n"
        c_code += "typedef struct {\n"
        c_code += "    const char *name;\n"
        c_code += "    void *ptr;\n"
        c_code += "} registry_entry_t;\n\n"
        c_code += "static registry_entry_t g_registry[MAX_REGISTERED_PTRS];\n"
        c_code += "static int g_registry_count = 0;\n\n"

        c_code += "void lvgl_json_register_ptr(const char *name, void *ptr) {\n"
        c_code += "    if (!name || !ptr) return;\n"
        c_code += "    // Check if name already exists (update)\n"
        c_code += "    for (int i = 0; i < g_registry_count; ++i) {\n"
        c_code += "        if (g_registry[i].name && strcmp(g_registry[i].name, name) == 0) {\n"
        c_code += "            LOG_WARN(\"Registry Warning: Name '%s' already registered. Updating pointer.\", name);\n"
        c_code += "            g_registry[i].ptr = ptr;\n"
        c_code += "            return;\n"
        c_code += "        }\n"
        c_code += "    }\n"
        c_code += "    // Add new entry if space available\n"
        c_code += f"    if (g_registry_count < MAX_REGISTERED_PTRS) {{\n"
        # We store the const char* directly from JSON "id" property - assumes lifetime is sufficient.
        # If names are dynamically generated, they need copying.
        c_code += "        g_registry[g_registry_count].name = name; // WARNING: Assumes 'name' lifetime!\n"
        c_code += "        g_registry[g_registry_count].ptr = ptr;\n"
        c_code += "        g_registry_count++;\n"
        c_code += "         LOG_INFO(\"Registered pointer '%s'\", name);\n"
        c_code += "    } else {\n"
        c_code += "        LOG_ERR(\"Registry Error: Maximum number of registered pointers (%d) exceeded.\", MAX_REGISTERED_PTRS);\n"
        c_code += "    }\n"
        c_code += "}\n\n"

        c_code += "void* lvgl_json_get_registered_ptr(const char *name) {\n"
        c_code += "    if (!name) return NULL;\n"
        c_code += "    for (int i = 0; i < g_registry_count; ++i) {\n"
        c_code += "        if (g_registry[i].name && strcmp(g_registry[i].name, name) == 0) {\n"
        c_code += "            return g_registry[i].ptr;\n"
        c_code += "        }\n"
        c_code += "    }\n"
        c_code += "    return NULL;\n"
        c_code += "}\n\n"

        c_code += "void lvgl_json_registry_clear() {\n"
        c_code += "    // For static registry, just reset the count. Pointers themselves are managed elsewhere.\n"
        c_code += "    // We might need to NULL out entries if name pointers are checked.\n"
        c_code += "    for(int i = 0; i < g_registry_count; ++i) { g_registry[i].name = NULL; g_registry[i].ptr = NULL; }\n"
        c_code += "    g_registry_count = 0;\n"
        c_code += "     LOG_INFO(\"Pointer registry cleared.\");\n"
        c_code += "}\n\n"

    return c_code

def find_init_functions(filtered_functions):
    """Finds functions matching the pattern 'lv_type_init(lv_type_t * ptr)'."""
    init_functions = []
    for func in filtered_functions:
        name = func.get('name', '')
        args = func.get('_resolved_arg_types', [])
        ret = func.get('_resolved_ret_type', ('void', 0, False))

        # Check:
        # 1. Name ends with _init
        # 2. Returns void
        # 3. Takes exactly one argument
        # 4. Argument is a pointer (level 1) to an lvgl type (lv_..._t)
        # 5. The type name matches the function prefix (e.g., lv_style_init -> lv_style_t *)
        if name.endswith("_init") and ret[0] == 'void' and ret[1] == 0 and len(args) == 1:
            arg_type, arg_ptr_lvl, _ = args[0]
            if arg_ptr_lvl == 1 and arg_type.startswith("lv_") and arg_type.endswith("_t"):
                 type_prefix = arg_type[:-2] # e.g., lv_style
                 if name.startswith(type_prefix):
                      init_functions.append(func)
                      logger.debug(f"Found init function: {name} for type {arg_type}")

    logger.info(f"Found {len(init_functions)} init functions for custom creators.")
    return init_functions


def generate_custom_creators(init_functions, api_info):
    """Generates wrapper functions for types initialized with _init functions."""
    c_code = "// --- Custom Managed Object Creators ---\n\n"
    # Need LV_MALLOC defined (usually from lv_conf.h or lv_mem.h)
    c_code += "#ifndef LV_MALLOC\n"
    c_code += "#include <stdlib.h>\n"
    c_code += "#define LV_MALLOC malloc\n"
    c_code += "#define LV_FREE free\n"
    c_code += "#endif\n\n"

    # Make registry functions available
    c_code += "extern void lvgl_json_register_ptr(const char *name, const char *type_name, void *ptr);\n"
    # Include original init functions if needed directly
    for func in init_functions:
         c_code += f"extern void {func['name']}({get_c_type_str(*func['_resolved_arg_types'][0][:2])});\n"
    c_code += "\n"


    for func in init_functions:
        init_func_name = func['name']
        # Extract type, e.g., lv_style_t from lv_style_init
        arg_type, arg_ptr_lvl, _ = func['_resolved_arg_types'][0]
        if arg_ptr_lvl != 1 or not arg_type.endswith("_t"): continue # Sanity check

        # Create function name, e.g., lv_style_create_managed
        creator_func_name = f"{arg_type[:-2]}_create_managed"
        c_type_str = arg_type # e.g., "lv_style_t"

        c_code += f"// Creator for {c_type_str} using {init_func_name}\n"
        c_code += f"{c_type_str}* {creator_func_name}(const char *name) {{\n"
        c_code += f"    if (!name) {{\n"
        c_code += f"        LOG_ERR(\"{creator_func_name}: Name cannot be NULL.\");\n"
        c_code += f"        return NULL;\n"
        c_code += f"    }}\n"
        c_code += f"    LOG_INFO(\"Creating managed {c_type_str} with name '%s'\", name);\n"
        # Allocate memory for the struct
        c_code += f"    {c_type_str} *new_obj = ({c_type_str}*)LV_MALLOC(sizeof({c_type_str}));\n"
        c_code += f"    if (!new_obj) {{\n"
        c_code += f"        LOG_ERR(\"{creator_func_name}: Failed to allocate memory for {c_type_str}.\");\n"
        c_code += f"        return NULL;\n"
        c_code += f"    }}\n"
        # Call the init function
        c_code += f"    {init_func_name}(new_obj);\n"
        # Register the allocated pointer
        c_code += f"    lvgl_json_register_ptr(name, \"{c_type_str}\", (void*)new_obj);\n"
        c_code += f"    return new_obj;\n"
        c_code += f"}}\n\n"
# ...

    return c_code
