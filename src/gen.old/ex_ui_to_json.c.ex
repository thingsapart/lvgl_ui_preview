#include <stdio.h>
#include <stdlib.h>
#include "generated_emul/emul_lvgl.h" // Adjust path

// Assume some dummy font/image pointers exist
const lv_font_t my_font;
const lv_img_dsc_t my_icon;

int main() {
    // Initialize the emulator
    emul_lvgl_init();

    // Register named pointers that your UI code uses
    emul_lvgl_register_named_pointer(&my_font, "font_roboto_16");
    emul_lvgl_register_named_pointer(&my_icon, "icon_settings");

    // --- Start using the emulated LVGL API ---

    // Create a screen (parent = NULL)
    emul_lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 320, 240);

    // Create a button on the screen
    emul_lv_obj_t* btn = lv_btn_create(screen);
    lv_obj_set_pos(btn, 50, 50);
    lv_obj_set_size(btn, 100, 40);

    // Create a label on the button
    emul_lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me");
    lv_obj_center(label); // NOTE: lv_obj_center might not be whitelisted by default!

    // Create a style
    emul_lv_style_t* style_btn_red; // Need a way to get the pointer back
    // FIXME: lv_style_init doesn't easily return the emulator pointer.
    // Workaround: Let's assume we know the ID or add a create function.
    // For now, we'll manually create a style entry for demo
    // A better approach would be an `emul_lv_style_create` function.

    // ---- SIMULATING STYLE CREATION (manual cJSON for demo) ----
    emul_internal_style_t* manual_style = malloc(sizeof(emul_internal_style_t));
    snprintf(manual_style->id, sizeof(manual_style->id), "style_%ld", ++g_style_counter); // Accessing internal counter directly - bad practice, just for demo
    cJSON* style_node = cJSON_CreateObject();
    cJSON_AddItemToObject(g_styles, manual_style->id, style_node);
    cJSON_AddObjectToObject(style_node, "properties");
    style_btn_red = (emul_lv_style_t*)manual_style;
    // ---- END SIMULATED STYLE CREATION ----


    // Set style properties (assuming style_btn_red points to a valid emul style)
    lv_style_set_bg_color(style_btn_red, lv_color_hex(0xFF0000)); // lv_color_hex needs emulation or replacement
    lv_style_set_radius(style_btn_red, 5);

    // Apply the style to the button
    lv_obj_add_style(btn, style_btn_red, LV_PART_MAIN); // LV_PART_MAIN needs definition

    // Set a property using a registered pointer
    lv_label_set_text_font(label, &my_font); // Assumes lv_label_set_text_font is generated

    // --- Rendering ---

    // Get the JSON representation
    char *json_output = emul_lvgl_render_to_json();

    if (json_output) {
        printf("--- Generated JSON ---\n%s\n----------------------\n", json_output);
        free(json_output); // IMPORTANT: Free the string returned by cJSON_Print
    } else {
        printf("Failed to generate JSON.\n");
    }

    // Cleanup
    emul_lvgl_deinit();

    return 0;
}
