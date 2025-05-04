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
        c_code += "void lvgl_json_register_ptr(const char *name, void *ptr) {\n"
        c_code += "    if (!name || !ptr) return;\n"
        c_code += "    unsigned int index = hash(name);\n"
        c_code += "    // Check if name already exists (update or handle error?)\n"
        c_code += "    registry_entry_t *entry = g_registry_map[index];\n"
        c_code += "    while(entry) {\n"
        c_code += "        if(strcmp(entry->name, name) == 0) {\n"
        c_code += "             LOG_WARN(\"Registry Warning: Name '%s' already registered. Updating pointer.\", name);\n"
        c_code += "             entry->ptr = ptr; // Update existing entry\n"
        c_code += "             return;\n"
        c_code += "        }\n"
        c_code += "        entry = entry->next;\n"
        c_code += "    }\n"
        c_code += "    // Add new entry\n"
        c_code += "    registry_entry_t *new_entry = (registry_entry_t *)LV_MALLOC(sizeof(registry_entry_t));\n" # Use LV_MALLOC
        c_code += "    if (!new_entry) { LOG_ERR(\"Registry Error: Failed to allocate memory\"); return; }\n"
        c_code += "    new_entry->name = lv_strdup(name);\n" # Use lv_strdup if available or normal strdup
        c_code += "    if (!new_entry->name) { LV_FREE(new_entry); LOG_ERR(\"Registry Error: Failed to duplicate name\"); return; }\n"
        c_code += "    new_entry->ptr = ptr;\n"
        c_code += "    new_entry->next = g_registry_map[index];\n"
        c_code += "    g_registry_map[index] = new_entry;\n"
        c_code += "     LOG_INFO(\"Registered pointer '%s'\", name);\n"
        c_code += "}\n\n"

        c_code += "void* lvgl_json_get_registered_ptr(const char *name) {\n"
        c_code += "    if (!name) return NULL;\n"
        c_code += "    unsigned int index = hash(name);\n"
        c_code += "    registry_entry_t *entry = g_registry_map[index];\n"
        c_code += "    while (entry != NULL) {\n"
        c_code += "        if (strcmp(entry->name, name) == 0) {\n"
        c_code += "            return entry->ptr;\n"
        c_code += "        }\n"
        c_code += "        entry = entry->next;\n"
        c_code += "    }\n"
        c_code += "    return NULL;\n"
        c_code += "}\n\n"

        # Need a cleanup function to free allocated names and entries if the registry is dynamic
        c_code += "void lvgl_json_registry_clear() {\n"
        c_code += "    for(int i = 0; i < HASH_MAP_SIZE; ++i) {\n"
        c_code += "        registry_entry_t *entry = g_registry_map[i];\n"
        c_code += "        while(entry) {\n"
        c_code += "             registry_entry_t *next = entry->next;\n"
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
    c_code += "extern void lvgl_json_register_ptr(const char *name, void *ptr);\n"
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
        c_code += f"    lvgl_json_register_ptr(name, (void*)new_obj);\n"
        c_code += f"    return new_obj;\n"
        c_code += f"}}\n\n"

    return c_code
