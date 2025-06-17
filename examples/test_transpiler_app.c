// examples/test_transpiler_app.c
#include <stdio.h>
#include <stdlib.h>
#include <lvgl.h>        // For LVGL types if used by renderer's headers or basic init
#include "cjson/cJSON.h" // For JSON parsing

// Assuming lvgl_json_renderer.h will be in the include path
// or relative to where this test app is compiled from.
// For a typical project structure, it might be "../src/lvgl_json_renderer.h"
// For this subtask, assume it can be found as "lvgl_json_renderer.h"
#include "lvgl_json_renderer.h"

// Dummy LVGL init for the transpiler app, not for actual rendering here
void initialize_lvgl_minimal_for_transpiler() {
    // lv_init(); // Usually not needed if the library only uses type definitions
    // and doesn't call LVGL functions that require full init for transpilation logic.
    // The generated lvgl_json_renderer.c might call lv_color_hex or lv_pct,
    // which are generally safe without full display/driver init.
    // If lvgl_json_get_registered_ptr or enum lookups need LVGL state, then lv_init() is needed.
    // For safety during testing of the full library:
    lv_init();
    LOG_INFO("Minimal LVGL initialized for transpiler app.");
}

char* read_json_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s for reading\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*)malloc(length + 1);
    if (data) {
        size_t read_len = fread(data, 1, length, f);
        if (read_len != (size_t)length) {
            fprintf(stderr, "Error reading file %s\n", filename);
            free(data);
            data = NULL;
        } else {
            data[length] = '\0';
        }
    }
    fclose(f);
    return data;
}

int main(int argc, char** argv) {
    initialize_lvgl_minimal_for_transpiler();

    const char* json_file_path = "examples/test_transpile.json";
    // Allow overriding JSON file path via command line for flexibility
    if (argc > 1) {
        json_file_path = argv[1];
    }

    printf("Reading UI JSON from: %s\n", json_file_path);
    char* json_string = read_json_file(json_file_path);
    if (!json_string) {
        return 1;
    }

    cJSON *root_json = cJSON_Parse(json_string);
    if (!root_json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "JSON parsing error");
        if (error_ptr != NULL) {
            fprintf(stderr, " before: [%s]", error_ptr);
        }
        fprintf(stderr, "\n");
        free(json_string);
        return 1;
    }

    // Define the output base name. Files will be created relative to execution path.
    const char* output_base_name = "examples/transpiled_ui_output";
    printf("Attempting to transpile UI to C files with base: %s ...\n", output_base_name);

    bool success = lvgl_json_transpile_ui(root_json, output_base_name);

    if (success) {
        printf("Transpilation successful! Output files: %s.c, %s.h\n", output_base_name, output_base_name);
    } else {
        printf("Transpilation failed.\n");
    }

    cJSON_Delete(root_json);
    free(json_string);

    return success ? 0 : 1;
}
