// client_main.c
#include <stdio.h>
#include <stdlib.h> // For free()
#include "emul_lvgl.h" // Include the emulation library header

// --- Define placeholder font pointers (match those in emul_lvgl.h example) ---
// Values don't matter, must be unique non-NULL pointers.
const lv_font_t lv_font_montserrat_14 = (const lv_font_t)0xABC14;
const lv_font_t lv_font_montserrat_18 = (const lv_font_t)0xABC18;
// Define lv_font_default() if your code uses it
#define lv_font_default() (&lv_font_montserrat_14) // Example: Map default to 14

// Need lv_obj_center implementation if used above
// Usually: static inline void lv_obj_center(lv_obj_t obj) { lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0); }
// Since this client only includes emul_lvgl.h, let's add it here if needed,
// or better, add lv_obj_center to emul_lvgl.h/c
void lv_obj_center(lv_obj_t obj) {
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
}

int main() {
    printf("--- Running LVGL Emulation Client ---\n");

    emul_lvgl_init(); // Initialize the emulation library

    // Register the fonts you defined with their JSON names
    emul_lvgl_register_font(&lv_font_montserrat_14, "montserrat_14");
    emul_lvgl_register_font(&lv_font_montserrat_18, "montserrat_18");

    // --- Your LVGL UI Creation Code (using the emulated functions) ---
    printf("Creating emulated UI...\n");
    lv_obj_t screen = lv_obj_create(lv_screen_active());

    // Simple Button
    lv_obj_t btn = lv_btn_create(screen);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x007AFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Label on Button
    lv_obj_t label = lv_label_create(btn);
    lv_label_set_text(label, "Emulated Btn");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label); // lv_obj_center is usually a macro/inline for lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0)
                          // We haven't implemented lv_obj_center, so let's use align:
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); // Replaced lv_obj_center

    // Another Label
    lv_obj_t info_label = lv_label_create(screen);
    lv_label_set_text_fmt(info_label, "This UI is Emulated (%s)", "v1.0");
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -20);
     lv_obj_set_style_text_font(info_label, lv_font_default(), LV_PART_MAIN | LV_STATE_DEFAULT);


    // --- Generate JSON ---
    printf("Generating JSON...\n");
    // Note: We get the JSON for the 'screen' object which acts as the parent for top-level widgets
    char* json_output = emul_lvgl_get_json(screen);

    if (json_output) {
        printf("\n--- Generated JSON Output ---\n");
        printf("%s\n", json_output);
        printf("-----------------------------\n\n");

        // In a real scenario, you would send this string to the viewer
        // e.g., write to the file monitored by lvgl_remote_ui
        FILE *fp = fopen("ui_layout.json", "w"); // Overwrite the file
        if (fp) {
            fprintf(fp, "%s", json_output);
            fclose(fp);
            printf("JSON written to ui_layout.json\n");
        } else {
            perror("Failed to write ui_layout.json");
        }


        free(json_output); // IMPORTANT: Free the returned string
    } else {
        printf("Failed to generate JSON!\n");
    }

    emul_lvgl_deinit(); // Cleanup emulation library resources
    printf("--- Emulation Client Finished ---\n");
    return 0;
}


