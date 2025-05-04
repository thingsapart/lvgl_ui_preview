#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "generated_builder/ui_builder.h" // Adjust path

// --- Pointer Registry Setup ---

// Example registry entries (replace with your actual fonts/images)
static const lv_font_t my_font_roboto_16; // Assume this is initialized elsewhere
static const lv_img_dsc_t my_icon_settings; // Assume this is initialized

typedef struct {
    const char* name;
    const void* ptr;
} RegistryEntry;

RegistryEntry app_registry[] = {
    {"font_roboto_16", &my_font_roboto_16},
    {"icon_settings", &my_icon_settings},
    // Add all other named resources your UI might use
    {NULL, NULL} // Terminator
};

// The lookup callback function required by ui_builder
const void* registry_lookup(const char *name, void *user_data) {
    // user_data could be used to pass a more complex registry context if needed
    RegistryEntry *registry = (RegistryEntry*)app_registry; // Using global directly here
    for (int i = 0; registry[i].name != NULL; i++) {
        if (strcmp(name, registry[i].name) == 0) {
            return registry[i].ptr;
        }
    }
    printf("Registry lookup failed for name: %s\n", name);
    return NULL;
}

// --- Main Application ---

int main() {
    // --- LVGL Initialization (Your standard init) ---
    lv_init();
    // ... initialize display driver, input driver ...
    // lv_disp_t * disp = lv_disp_drv_register(&disp_drv);
    // ... etc ...


    // --- Assume you have a JSON string from emul_lvgl ---
    // (Load from file, receive over network, etc.)
    const char* json_from_emulator =
        "{\n"
        "  \"objects\": {\n"
        "    \"obj_1\": {\n"
        "      \"type\": \"lv_obj_create\",\n"
        "      \"parent\": null,\n"
        "      \"properties\": {\"width\": 300, \"height\": 200, \"align\": 0 },\n" // Assume align 0 is LV_ALIGN_DEFAULT
        "      \"styles\": {}\n"
        "    },\n"
        "    \"obj_2\": {\n"
        "      \"type\": \"lv_btn_create\",\n"
        "      \"parent\": \"obj_1\",\n"
        "      \"properties\": {\"width\": 120, \"height\": 40, \"pos_x\": 20, \"pos_y\": 30},\n"
        "      \"styles\": {\"part_0\": [\"style_1\"] }\n"
        "    },\n"
        "    \"obj_3\": {\n"
        "      \"type\": \"lv_label_create\",\n"
        "      \"parent\": \"obj_2\",\n"
        "      \"properties\": {\"text\": \"Built!\", \"text_font\": \"font_roboto_16\", \"align\": 9 },\n" // Assume 9 is LV_ALIGN_CENTER
        "      \"styles\": {}\n"
        "    }\n"
        "  },\n"
        "  \"styles\": {\n"
        "     \"style_1\": {\n"
        "       \"properties\": { \"bg_color\": \"#FF0000\", \"radius\": 5 }\n"
        "     }\n"
        "  }\n"
        "}";


    // --- Build the UI ---
    lv_obj_t* screen = lv_scr_act(); // Get the active screen as the parent
    printf("Building UI from JSON...\n");
    bool success = ui_builder_load_json(screen, json_from_emulator, registry_lookup, NULL); // Pass NULL for user_data

    if (success) {
        printf("UI Built Successfully!\n");
        // The UI elements should now be children of 'screen'
    } else {
        printf("UI Building Failed!\n");
        // Handle error appropriately
    }

    // --- LVGL Main Loop ---
    while (1) {
        lv_timer_handler();
        // ... your delay ...
    }

    // NOTE: Styles created by ui_builder_load_json using malloc() are leaked
    // in this example. You need a strategy to free them when the UI is destroyed.

    return 0;
}
