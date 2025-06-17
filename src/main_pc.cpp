#define SDL_MAIN_HANDLED // Prevent SDL from defining main
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // For usleep, close
#include <stdbool.h>
#include <string.h>
#include <errno.h>
// #include <fcntl.h>     // No longer needed for stdin
// #include <sys/select.h>// No longer needed for stdin
// #include <sys/time.h>  // No longer needed for stdin

#include <sys/stat.h>  // For stat()
#include <time.h>      // For time_t

#include <signal.h>
#include <stdio.h>

#include "SDL2/SDL.h" // Directly include SDL
#include "lvgl.h"

// Assuming lv_drivers are used internally by these functions now
// If not, you might need specific includes from lv_drivers/sdl
// #include "lv_drivers/sdl/sdl.h"
#include "lvgl_json_renderer.h" // Your UI builder header

// Define resolution if not coming from lv_conf.h or elsewhere
#ifndef SDL_HOR_RES
#define SDL_HOR_RES 480
#endif
#ifndef SDL_VER_RES
#define SDL_VER_RES 480
#endif

// Keep original buffer size define, though buffer itself is removed
#define INPUT_BUFFER_SIZE (30 * 1024)

#define FILE_POLL_INTERVAL_MS 500 // Check file every 500 milliseconds

// Use the provided logging macros
#define LOG(s, ...) printf(s __VA_OPT__(,) __VA_ARGS__)
#ifndef LOG_INFO
#define LOG_INFO(s, ...) do { printf("[INFO] "); printf(s __VA_OPT__(,) __VA_ARGS__); printf("\n"); } while(0)
#endif
#define LOG_ERROR(s, ...) do { printf("[ERROR] "); printf(s __VA_OPT__(,) __VA_ARGS__); printf("\n"); } while(0)
#ifndef LOG_WARN
#define LOG_WARN(s, ...) do { printf("[WARN] "); printf(s __VA_OPT__(,) __VA_ARGS__); printf("\n"); } while(0)
#endif
#define LOG_USER(s, ...) do { printf("[USER] "); printf(s __VA_OPT__(,) __VA_ARGS__); printf("\n"); } while(0)
#define LOG_TRACE(s, ...) do { printf("[TRACE] "); printf(s __VA_OPT__(,) __VA_ARGS__); printf("\n"); } while(0)

// --- Global state for file monitoring ---
static char *monitored_filepath = NULL;
static time_t last_mod_time = 0; // Store last modification time
static uint32_t last_file_check_time = 0; // Track time for polling interval


volatile sig_atomic_t bRunning = true; // Keep the signal handler flag

void signal_handler(int interrupt) {
  printf("captured interrupt %d\r\n", interrupt);
  if (interrupt == SIGINT) {
    bRunning = false;
  }
}


// --- Helper Function: Load UI from File ---
// (Copied from previous file-watching example, adapted logging)
bool load_and_build_ui(const char *filepath) {
    LOG_INFO("STRING VALUES: %s", lvgl_json_generate_values_json());

    LOG_INFO("Attempting to load UI from: %s", filepath);

    FILE *fp = fopen(filepath, "rb"); // Open in binary read mode
    if (!fp) {
        // Log warning but don't necessarily show error on screen unless it's initial load failure
        LOG_WARN("Failed to open file '%s': %s", filepath, strerror(errno));
        return false;
    }

    // --- Get file size ---
    if (fseek(fp, 0, SEEK_END) != 0) {
         LOG_ERROR("fseek SEEK_END failed for '%s': %s", filepath, strerror(errno));
         fclose(fp);
         return false;
    }
    long file_size = ftell(fp);
     if (file_size < 0) {
         LOG_ERROR("ftell failed for '%s': %s", filepath, strerror(errno));
         fclose(fp);
         return false;
    }
     if (fseek(fp, 0, SEEK_SET) != 0) { // Rewind to beginning
         LOG_ERROR("fseek SEEK_SET failed for '%s': %s", filepath, strerror(errno));
         fclose(fp);
         return false;
    }

    if (file_size == 0) {
        LOG_WARN("File '%s' is empty. Clearing screen.", filepath);
        fclose(fp);
        // Treat empty file as "show nothing". build_ui_from_json should handle null/empty string gracefully
        // or we clear explicitly here.
        lv_obj_t * scr = lv_screen_active();
        if (scr) {
            lv_obj_clean(scr);
            lv_obj_t* lbl = lv_label_create(scr);
            lv_label_set_text(lbl, "Error:\nFailed processing\nempty UI file.");
            lv_obj_center(lbl);
        }
        return false; // Indicate failure
    }

    // --- Allocate buffer and read file ---
    // Use malloc for file content buffer, size based on file
    char *file_content = (char *)malloc(file_size + 1); // +1 for null terminator
    if (!file_content) {
        LOG_ERROR("Failed to allocate %ld bytes for file content from '%s'", file_size + 1, filepath);
        fclose(fp);
        return false;
    }

    size_t bytes_read = fread(file_content, 1, file_size, fp);
    fclose(fp); // Close file as soon as read is done

    if (bytes_read != (size_t)file_size) {
        LOG_ERROR("Failed to read entire file '%s' (%zu bytes read, %ld expected)", filepath, bytes_read, file_size);
        free(file_content);
        return false;
    }

    file_content[file_size] = '\0'; // Null-terminate the content for cJSON

    // --- Build UI ---
    // build_ui_from_json will clear the screen before building
    cJSON *root = cJSON_Parse(file_content);
    if (!root) {
        LOG_ERROR("Failed to parse JSON");
        return false;
    }
    free(file_content); // Free the buffer

    lvgl_json_register_str_clear();

    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    bool success = lvgl_json_render_ui(root, scr);

    if (!success) {
        LOG_ERROR("Failed to build UI from JSON content of '%s'.", filepath);
         // build_ui_from_json might have cleared screen, show error
         // lv_obj_t * scr = lv_screen_active();
         if (scr) {
            // Check if screen is already empty from build_ui_from_json failure
            if(lv_obj_get_child_count(scr) == 0) {
                lv_obj_t* lbl = lv_label_create(scr);
                lv_label_set_text(lbl, "Error:\nFailed to parse\nUI file content.");
                lv_obj_center(lbl);
            }
         }
        return false;
    } else {
        LOG_INFO("UI rebuilt successfully from '%s'.", filepath);
        return true;
    }
}


// --- SDL/LVGL Forward Declarations (assuming these exist from lv_drivers/sdl or similar) ---
// These replace the direct calls used in the file-watching example
extern lv_display_t * lv_sdl_window_create(int width, int height);
extern lv_indev_t * lv_sdl_mouse_create(void);
extern lv_indev_t * lv_sdl_mousewheel_create(void);
extern lv_indev_t * lv_sdl_keyboard_create(void);
extern void lv_sdl_window_set_zoom(lv_display_t * display, int zoom);
extern void lv_log_print_g_cb(const char * buf); // Example log function

LV_FONT_DECLARE(font_kode_14);
LV_FONT_DECLARE(font_kode_20);
LV_FONT_DECLARE(font_kode_24);
LV_FONT_DECLARE(font_kode_30);
LV_FONT_DECLARE(font_kode_36);
LV_FONT_DECLARE(lcd_7_segment_24);
LV_FONT_DECLARE(lcd_7_segment_18);
LV_FONT_DECLARE(lcd_7_segment_14);

#define LV_GRID_FR_1 LV_GRID_FR(1)
#define LV_BORDER_SIDE_TOP_BOTTOM (LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM)

#include "ui_transpiled.h"

int main_transpiled(int argc, char *argv[]) {
    // --- Initial UI Load ---
    create_ui_ui_transpiled(lv_screen_active());

    // --- LVGL Main Loop ---
    LOG_USER("Starting LVGL main loop...");
    uint32_t lastTick = SDL_GetTicks(); // Initialize last tick time
    bRunning = true; // Ensure flag is set before loop

    while (bRunning) {
        uint32_t current_time_ms = SDL_GetTicks(); // Get current time once per loop

        // --- Handle LVGL Tasks (using target code's style) ---
        // Use SDL_Delay for general yielding/pacing
        SDL_Delay(10);

        // Update LVGL tick (keep from target code)
        uint32_t current = SDL_GetTicks(); // Fetch potentially updated ticks
        lv_tick_inc(current - lastTick);
        lastTick = current;

        // Handle LVGL timers and drawing (keep from target code)
        lv_timer_handler();
    }

    // --- Cleanup ---
    LOG_USER("Exiting...");
    // Add explicit cleanup if necessary (e.g., lv_display_destroy, SDL_Quit)
    // Depending on lv_drivers behavior, some cleanup might happen automatically.
    // Consider adding:
    // lv_display_delete(lvDisplay); // Or similar function if provided by driver
    // SDL_Quit();

    return 0;
}

int main_render(int argc, char *argv[]) {
    // --- Initial UI Load ---
    LOG_USER("Monitoring file: %s", monitored_filepath);
    struct stat initial_stat;
    bool initial_load_success = false;
    if (stat(monitored_filepath, &initial_stat) == 0) {
        if (load_and_build_ui(monitored_filepath)) {
             last_mod_time = initial_stat.st_mtime; // Store initial mod time on success
             initial_load_success = true;
        }
        // Error message handled within load_and_build_ui if it failed
    } else {
        // File doesn't exist or error stating it initially
        LOG_WARN("Initial stat failed for '%s': %s. Waiting for file creation.", monitored_filepath, strerror(errno));
        // last_mod_time remains 0, so the first successful stat will trigger a load
    }

    if (!initial_load_success) {
        // Show a waiting message if initial load didn't happen or failed
        lv_obj_t* lbl = lv_label_create(lv_screen_active());
        if (errno == ENOENT) {
             lv_label_set_text_fmt(lbl, "Waiting for UI file:\n%s", monitored_filepath);
        } else {
             lv_label_set_text(lbl, "Error loading initial UI file.\nCheck logs.");
        }
        lv_obj_center(lbl);
    }
    
    // --- LVGL Main Loop ---
    LOG_USER("Starting LVGL main loop...");
    uint32_t lastTick = SDL_GetTicks(); // Initialize last tick time
    bRunning = true; // Ensure flag is set before loop

    while (bRunning) {
        uint32_t current_time_ms = SDL_GetTicks(); // Get current time once per loop

        // --- Check File for Modifications (Polling) ---
        if (current_time_ms - last_file_check_time >= FILE_POLL_INTERVAL_MS) {
            last_file_check_time = current_time_ms; // Update check time
            struct stat current_stat;
            if (stat(monitored_filepath, &current_stat) == 0) {
                // File exists, check modification time
                if (current_stat.st_mtime != last_mod_time) {
                    LOG_INFO("Detected file change (mtime: %ld -> %ld). Reloading...",
                              (long)last_mod_time, (long)current_stat.st_mtime);

                    // Attempt to reload UI, update last_mod_time only on success
                    if (load_and_build_ui(monitored_filepath)) {
                        last_mod_time = current_stat.st_mtime;
                    } else {
                        // Reload failed, keep old mod time to retry on next change
                        // Error message handled within load_and_build_ui
                    }
                }
            } else {
                // Stat failed - file might be deleted or inaccessible
                if (errno == ENOENT) {
                    if (last_mod_time != 0) { // Log only if it existed before
                        LOG_WARN("Monitored file '%s' seems to have been deleted.", monitored_filepath);
                        // Keep the last valid UI displayed
                        last_mod_time = 0; // Reset time so it reloads if recreated
                    }
                } else {
                     // Other error (e.g., permissions)
                     LOG_WARN("stat failed for '%s': %s", monitored_filepath, strerror(errno));
                }
            }
        }


        // --- Handle LVGL Tasks (using target code's style) ---
        // Use SDL_Delay for general yielding/pacing
        SDL_Delay(10);

        // Update LVGL tick (keep from target code)
        uint32_t current = SDL_GetTicks(); // Fetch potentially updated ticks
        lv_tick_inc(current - lastTick);
        lastTick = current;

        // Handle LVGL timers and drawing (keep from target code)
        lv_timer_handler();
    }

    // --- Cleanup ---
    LOG_USER("Exiting...");
    // Add explicit cleanup if necessary (e.g., lv_display_destroy, SDL_Quit)
    // Depending on lv_drivers behavior, some cleanup might happen automatically.
    // Consider adding:
    // lv_display_delete(lvDisplay); // Or similar function if provided by driver
    // SDL_Quit();

    return 0;
}

void btn_clicked(lv_event_t *evt) {
    printf("CLICKED!!\n");
}

int main(int argc, char *argv[]) {

    // --- Argument Parsing ---
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_ui_json_file>\n", argv[0]);
        return 1;
    }
    monitored_filepath = argv[1];


    // --- LVGL & SDL Initialization (using target code's style) ---
    lv_init();

    // Workaround for sdl2 `-m32` crash (keep from target code)
    #ifndef WIN32
        setenv("DBUS_FATAL_WARNINGS", "0", 1);
    #endif

    // Register LVGL log callback if enabled (keep from target code)
    #if LV_USE_LOG != 0
        // Ensure LV_USE_LOG is set appropriately in lv_conf.h
        // lv_log_register_print_cb(lv_log_print_g_cb); // Use your actual log function if needed
    #endif

    // Create SDL display and input devices (keep from target code)
    lv_display_t *lvDisplay = lv_sdl_window_create(SDL_HOR_RES, SDL_VER_RES);
    if (!lvDisplay) { // Add basic check
        LOG_ERROR("Failed to create SDL window");
        return 1;
    }
    lv_indev_t *lvMouse = lv_sdl_mouse_create();
    lv_indev_t *lvMouseWheel = lv_sdl_mousewheel_create();
    lv_indev_t *lvKeyboard = lv_sdl_keyboard_create();

    // Optional: set zoom (keep from target code)
    // lv_sdl_window_set_zoom(lvDisplay, 1);

    // Use SDL_GetTicks for lv_tick_inc (keep from target code)
    // lv_tick_set_cb(SDL_GetTicks); // lv_tick_set_cb seems deprecated/removed in v9? Use lv_tick_inc directly.

    // Setup signal handler (keep from target code)
    signal(SIGINT, signal_handler);

    lvgl_json_register_ptr("font_kode_14", "lv_font_t", (void *) &font_kode_14);
    lvgl_json_register_ptr("font_kode_20", "lv_font_t", (void *) &font_kode_20);
    lvgl_json_register_ptr("font_kode_24", "lv_font_t", (void *) &font_kode_24);
    lvgl_json_register_ptr("font_kode_30", "lv_font_t", (void *) &font_kode_30);
    lvgl_json_register_ptr("font_kode_36", "lv_font_t", (void *) &font_kode_36);
    lvgl_json_register_ptr("lcd_7_segment_14", "lv_font_t", (void *) &lcd_7_segment_14);
    lvgl_json_register_ptr("lcd_7_segment_18", "lv_font_t", (void *) &lcd_7_segment_18);
    lvgl_json_register_ptr("lcd_7_segment_24", "lv_font_t", (void *) &lcd_7_segment_24);
    lvgl_json_register_ptr("font_montserrat_24", "lv_font_t", (void *) &lv_font_montserrat_24);
    lvgl_json_register_ptr("font_montserrat_14", "lv_font_t", (void *) &lv_font_montserrat_14);
    lvgl_json_register_ptr("font_montserrat_12", "lv_font_t", (void *) &lv_font_montserrat_12);

    lvgl_json_register_ptr("btn_clicked", "lv_event_cb_t", (void *) &btn_clicked);

//#define TRANSPILE
#ifdef TRANSPILE
    return main_transpiled(argc, argv);
#else
    return main_render(argc, argv);
#endif
}