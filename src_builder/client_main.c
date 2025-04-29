// client_main.c
#include <stdio.h>
#include <stdlib.h>
#include "emul_lvgl.h"

// Font pointers remain the same
lv_font_t lv_font_montserrat_14 = (const lv_font_t)0xABC14;
lv_font_t lv_font_montserrat_18 = (const lv_font_t)0xABC18;
lv_font_t lv_font_montserrat_24 = (const lv_font_t)0xABC24;

// lv_obj_center needs to take lv_obj_t *
void lv_obj_center(lv_obj_t * obj) {
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
}

void lv_test_screen(lv_obj_t *screen) {
    lv_obj_t * btn = lv_button_create(screen);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30); // Pass btn directly
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x007AFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Emulated Btn");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label); // Pass label directly

    lv_obj_t * info_label = lv_label_create(screen);
    lv_label_set_text_fmt(info_label, "This UI is Emulated (%s)", "v1.01");
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_text_font(info_label, lv_font_default(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * info_label2 = lv_label_create(screen);
    lv_label_set_text(info_label2, "... and more");
    lv_obj_align(info_label2, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(info_label2, lv_font_default(), LV_PART_MAIN | LV_STATE_DEFAULT);
}

#include "../src/ui/gen_views/feed_rate_view.h"

int main() {
    printf("--- Running LVGL Emulation Client (Using lv_obj_t *) ---\n");
    emul_lvgl_init();
    emul_lvgl_register_named_pointer(&lv_font_montserrat_14, "montserrat_14");
    emul_lvgl_register_named_pointer(&lv_font_montserrat_18, "montserrat_18");
    emul_lvgl_register_named_pointer(&lv_font_montserrat_24, "montserrat_24");

    printf("Creating emulated UI...\n");
    // ** Use lv_obj_t * for all object variables **
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    feed_rate_view_t *fv = feed_rate_view_create(screen);

    printf("Generating JSON...\n");
    // ** Pass pointer to screen object **
    char* json_output = emul_lvgl_render_to_json();

    if (json_output) {
        // ... (print and write JSON output as before) ...
        printf("\n--- Generated JSON Output ---\n%s\n-----------------------------\n\n", json_output);
        FILE *fp = fopen("ui_layout.json", "w");
        if (fp) { fprintf(fp, "%s", json_output); fclose(fp); printf("JSON written to ui_layout.json\n"); }
        else { perror("Failed to write ui_layout.json"); }
        free(json_output);
    } else {
        printf("Failed to generate JSON!\n");
    }

    emul_lvgl_deinit();
    printf("--- Emulation Client Finished ---\n");
    return 0;
}
