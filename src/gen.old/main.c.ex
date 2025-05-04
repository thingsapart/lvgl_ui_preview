/**
 * @file main.c
 * @brief Example using ui_builder to load and watch a JSON UI definition.
 *
 * This example demonstrates:
 * 1. Basic LVGL initialization (replace with your platform's drivers).
 * 2. Registering external resources (fonts).
 * 3. Loading the initial UI from a JSON file using ui_builder.
 * 4. Using inotify (Linux) to watch the JSON file for changes.
 * 5. Reloading the UI when the file is modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> // For usleep, read
#include <fcntl.h>  // For open
#include <errno.h>
#include <string.h>
#include <sys/inotify.h> // For file watching (Linux specific)
#include <sys/stat.h>   // For stat

#include "lvgl.h"
// Include your display and input driver headers here
// #include "lv_drivers/display/fbdev.h"
// #include "lv_drivers/indev/evdev.h"
#include "ui_builder.h" // Include the generated builder header

#define JSON_BUFFER_SIZE (1024 * 100) // Adjust as needed (e.g., 100 KB)
#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN    (1024 * (INOTIFY_EVENT_SIZE + 16))

// --- LVGL HAL Initialization (Replace with your platform) ---

static void hal_init(void) {
    // Initialize LVGL
    lv_init();

    // --- Display Driver Setup (Example: Framebuffer) ---
    // fbdev_init();
    // static lv_display_t * disp;
    // static lv_draw_buf_t draw_buf;
    // static lv_color_t buf1[LV_HOR_RES_MAX * 10]; // Adjust buffer size
    // lv_draw_buf_init(&draw_buf, buf1, NULL, LV_HOR_RES_MAX * 10);
    // disp = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
    // lv_display_set_flush_cb(disp, fbdev_flush);
    // lv_display_set_draw_buf(disp, &draw_buf);
    // lv_display_set_default(disp);
    printf("HAL Info: Display driver needs to be configured for your platform.\n");
    // --- Dummy display setup for compilation ---
    static lv_display_t * disp;
    static lv_draw_buf_t draw_buf;
    #define MY_DISP_HOR_RES 480
    #define MY_DISP_VER_RES 320
    static lv_color_t buf1[MY_DISP_HOR_RES * 10];
    lv_draw_buf_init(&draw_buf, buf1, NULL, MY_DISP_HOR_RES * 10);
    disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_draw_buf(disp, &draw_buf);
    lv_display_set_flush_cb(disp, NULL); // No actual flushing
    lv_display_set_default(disp);


    // --- Input Device Setup (Example: evdev) ---
    // evdev_init();
    // static lv_indev_t * indev_touchpad;
    // indev_touchpad = lv_indev_create();
    // lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    // lv_indev_set_read_cb(indev_touchpad, evdev_read);
    // lv_indev_set_driver_data(indev_touchpad, "/dev/input/eventX"); // Adjust device path
    printf("HAL Info: Input device driver needs to be configured for your platform.\n");

}

// --- File Reading ---
char* read_file_to_string(const char *filename) {
    FILE *fp = fopen(filename, "rb"); // Read binary to handle potential encoding issues
    if (!fp) {
        perror("Error opening file");
        fprintf(stderr, "   Filename: %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET); /* same as rewind(f); */

    if (fsize < 0) {
        perror("Error getting file size");
        fclose(fp);
        return NULL;
    }

    char *string = (char*)malloc(fsize + 1);
    if (!string) {
        perror("Error allocating memory for file content");
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(string, 1, fsize, fp);
    fclose(fp);

    if (read_size != fsize) {
        fprintf(stderr, "Error reading file entirely (read %zu, expected %ld)\n", read_size, fsize);
        free(string);
        return NULL;
    }

    string[fsize] = 0; // Null-terminate the string
    return string;
}

// --- Main Application Logic ---
volatile bool file_changed = true; // Start with true to load initially
const char *json_filepath = NULL;
time_t last_mod_time = 0;

// Function to load/reload the UI
void load_or_reload_ui(lv_obj_t* screen) {
    if (!json_filepath) return;

    printf("Attempting to load UI from: %s\n", json_filepath);

    // Check modification time to avoid unnecessary reloads if watching fails
    struct stat file_stat;
    if (stat(json_filepath, &file_stat) == 0) {
        if (!file_changed && file_stat.st_mtime <= last_mod_time) {
           // printf("File unchanged, skipping reload.\n");
           return;
        }
        last_mod_time = file_stat.st_mtime;
    } else {
        perror("Error stating file");
        // Continue anyway, maybe file appears later
    }


    char *json_buffer = read_file_to_string(json_filepath);
    if (!json_buffer) {
        fprintf(stderr, "Failed to read JSON file for UI build.\n");
        file_changed = false; // Reset flag after failed attempt
        return;
    }

    printf("Building UI...\n");
    // Before building, ensure all external resources are registered
    // Example: Registering built-in fonts
    ui_builder_register_external_resource("lv_font_montserrat_14", &lv_font_montserrat_14);
    ui_builder_register_external_resource("lv_font_montserrat_20", &lv_font_montserrat_20);
    ui_builder_register_external_resource("lv_font_montserrat_24", &lv_font_montserrat_24);
    // Add registrations for any other fonts or images used in your JSON

    // Build the UI onto the active screen (or a specific container)
    bool build_ok = ui_builder_build_ui(json_buffer, screen);

    free(json_buffer); // Free the buffer after parsing

    if (build_ok) {
        printf("UI build successful.\n");
    } else {
        fprintf(stderr, "UI build failed.\n");
    }
    file_changed = false; // Reset flag after processing
}

// --- File Watching (Linux inotify example) ---
static int init_file_watch(const char *filepath) {
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        perror("inotify_init1");
        return -1;
    }

    int wd = inotify_add_watch(fd, filepath, IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF);
    if (wd < 0) {
        perror("inotify_add_watch");
        fprintf(stderr, "   Watching file: %s\n", filepath);
        close(fd);
        return -1;
    }
    printf("Started watching file: %s\n", filepath);
    return fd; // Return the inotify file descriptor
}

static void handle_file_events(int fd) {
    char buffer[INOTIFY_BUF_LEN];
    ssize_t len = read(fd, buffer, INOTIFY_BUF_LEN);

    if (len < 0) {
        if (errno != EAGAIN) { // EAGAIN is expected for non-blocking read
            perror("read inotify fd");
        }
        return; // No events ready or error
    }
     if (len == 0) {
         return; // Should not happen with blocking read, but possible?
     }


    // Process events
    int i = 0;
    while (i < len) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
         // printf("inotify event: mask=0x%x\n", event->mask); // Debug
        if ((event->mask & IN_MODIFY) || (event->mask & IN_CLOSE_WRITE)) {
            printf("File '%s' modified or closed after write.\n", json_filepath);
            file_changed = true;
        }
        if ((event->mask & IN_DELETE_SELF) || (event->mask & IN_MOVE_SELF)) {
             printf("Watched file '%s' deleted or moved. Stopping watch.\n", json_filepath);
             // Need to handle re-adding the watch if the file reappears
             // For simplicity, we just stop watching here. Re-run the app.
             inotify_rm_watch(fd, event->wd); // wd might be invalid now, but try
             close(fd); // Close the main fd
             // Or set fd = -1 and try re-initializing in the main loop
             file_changed = true; // Trigger a reload attempt (might fail if file gone)
             return; // Exit this handler
        }
        i += INOTIFY_EVENT_SIZE + event->len;
    }
}


// --- Main ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_json_file>\n", argv[0]);
        return 1;
    }
    json_filepath = argv[1];

    // 1. Initialize LVGL and HAL
    hal_init();

    // 2. Initialize File Watcher (Linux specific)
    int notify_fd = init_file_watch(json_filepath);
    if (notify_fd < 0) {
        fprintf(stderr, "Warning: Could not initialize file watching. UI will only load once.\n");
    }

    // 3. Main Loop
    lv_obj_t * scr = lv_screen_active(); // Get default screen

    while (1) {
        // Check for file changes if watching is active
        if (notify_fd >= 0) {
            handle_file_events(notify_fd);
        }

        // Reload UI if needed
        if (file_changed) {
           load_or_reload_ui(scr);
        }

        // LVGL task handler
        lv_timer_handler();
        usleep(5000); // Sleep for 5ms
    }

    // Cleanup (won't be reached in this example)
    if (notify_fd >= 0) {
        close(notify_fd);
    }
    ui_builder_clear_external_resources(); // Clear registered resources

    return 0;
}
