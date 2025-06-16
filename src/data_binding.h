// data_binding.h
#ifndef DATA_BINDING_H__
#define DATA_BINDING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h" // Add this line to include lvgl.h
#include "machine/machine_interface.h" // Add this line to include machine_interface.h

// Configurable number of UI elements that can listen to a single data point or action.
#define CNC_UI_MAX_LISTENERS 10

// --- Part 1: UI Actions (Events from UI to CNC) ---

typedef enum {
    // Machine Control
    ACTION_HOME_ALL,
    ACTION_HOME_AXIS_X,
    ACTION_HOME_AXIS_Y,
    ACTION_HOME_AXIS_Z,
    ACTION_HOME_AXIS_A,
    ACTION_CONNECT,
    ACTION_DISCONNECT,
    ACTION_EMERGENCY_STOP,
    ACTION_RESET_ALARM,
    // Job Execution
    ACTION_JOB_START,
    ACTION_JOB_PAUSE,
    ACTION_JOB_RESUME,
    ACTION_JOB_STOP,
    ACTION_MACRO_RUN,
    // Manual Movement (Jogging)
    ACTION_JOG_CONTINUOUS_START_POS,
    ACTION_JOG_CONTINUOUS_START_NEG, // Corrected typo from issue
    ACTION_JOG_CONTINUOUS_STOP,
    ACTION_JOG_STEP,
    ACTION_JOG_SET_AXIS,
    ACTION_JOG_SET_AXIS_X,
    ACTION_JOG_SET_AXIS_Y,
    ACTION_JOG_SET_AXIS_Z,
    ACTION_JOG_SET_AXIS_A,
    ACTION_JOG_STORE_AXIS,
    ACTION_JOG_RESTORE_AXIS,
    ACTION_JOG_SET_STEP,
    // Coordinate Systems & Offsets
    ACTION_WCS_SET,
    ACTION_WCS_NEXT,
    ACTION_WCS_ZERO_AXIS_X,
    ACTION_WCS_ZERO_AXIS_Y,
    ACTION_WCS_ZERO_AXIS_Z,
    ACTION_WCS_ZERO_AXIS_A,
    ACTION_WCS_ZERO_ALL,
    // Spindle & Feedrate
    ACTION_SPINDLE_ON,
    ACTION_SPINDLE_OFF,
    ACTION_SPINDLE_SET_SPEED,
    ACTION_FEED_OVERRIDE_SET,
    // Files & Probing
    ACTION_FILES_LIST,
    ACTION_MACROS_LIST,
    ACTION_PROBE_MODE,
    ACTION_PROBE_START, // Added from issue description
    // Dialog/Modal Responses
    ACTION_MODAL_OK,
    ACTION_MODAL_CANCEL,
    ACTION_MODAL_CHOICE,
    ACTION_MODAL_INPUT_INT,
    ACTION_MODAL_INPUT_FLOAT,
    ACTION_MODAL_INPUT_STR,
    _ACTION_COUNT // Sentinel value for array sizing
} data_action_t;


// --- Part 2: UI Display (Data from CNC to UI) ---

typedef enum {
    // Core Machine State
    DATA_MACHINE_STATUS_TEXT,
    DATA_IS_CONNECTED,
    DATA_IS_HOMED_X,
    DATA_IS_HOMED_Y,
    DATA_IS_HOMED_Z,
    DATA_IS_HOMED_ALL,
    // Position Readouts (DRO)
    DATA_POS_MACHINE_X,
    DATA_POS_MACHINE_Y,
    DATA_POS_MACHINE_Z,
    DATA_POS_WORK_X,
    DATA_POS_WORK_Y,
    DATA_POS_WORK_Z,
    DATA_POS_DISTANCE_TO_GO_X,
    DATA_POS_DISTANCE_TO_GO_Y,
    DATA_POS_DISTANCE_TO_GO_Z,
    // Feed, Speed & Tool
    DATA_FEED_CURRENT,
    DATA_FEED_REQUESTED,
    DATA_FEED_OVERRIDE_PCT,
    DATA_SPINDLE_SPEED_CURRENT,
    DATA_SPINDLE_SPEED_REQUESTED,
    DATA_CURRENT_TOOL,
    // Jogging & WCS State
    DATA_JOG_CURRENT_AXIS,
    DATA_JOG_CURRENT_AXIS_TEXT,
    DATA_JOG_CURRENT_STEP,
    DATA_WCS_CURRENT_TEXT,
    DATA_WCS_CURRENT,
    // Job & File Information
    DATA_JOB_FILENAME,
    DATA_JOB_PROGRESS,
    DATA_JOB_ELAPSED_TIME,
    DATA_JOB_REMAINING_TIME,
    DATA_FILE_TEXT_LIST,
    DATA_MACRO_LIST,
    // Sensors & Diagnostics
    DATA_ENDSTOP_STATE_X,
    DATA_ENDSTOP_STATE_Y,
    DATA_ENDSTOP_STATE_Z,
    DATA_ENDSTOP_STATE_A,
    DATA_PROBE_1,
    DATA_PROBE_2,
    // Modal Dialogs
    DATA_MODAL_DIALOG,
    _DATA_COUNT // Sentinel value for array sizing
} data_binding_value_t;

// A struct to hold a reference to a UI widget that needs updating.
// Based on type of widget (eg lv_led, lv_label, ...) the handler may update the widget differently.
typedef struct {
    lv_obj_t* widget;
    const char *format_str; // Format string for the provided value to turn into label text, NULL if not used.
} data_binding_t;

// A struct to hold all listeners for a specific data value.
typedef struct {
    data_binding_t listeners[CNC_UI_MAX_LISTENERS];
    data_binding_value_t value; // Repeated here for introspectability, not strictly necessary as it's encoded in parent array.
    size_t count;
} data_binding_registry_entry_t;

// Forward declare cnc_ui_viewmodel_t
typedef struct cnc_ui_viewmodel_t cnc_ui_viewmodel_t;

typedef struct data_binding_registry_t {
    // Registry for all displayable data points
    data_binding_registry_entry_t display_registry[_DATA_COUNT];

    // Pre-generated LVGL event callbacks for actions.
    lv_event_cb_t action_handlers[_ACTION_COUNT];
} data_binding_registry_t;

/**
 * @brief Registers a UI widget to be updated when a specific data point changes.
 *
 * @param registry The data_binding_registry_t instance.
 * @param data_type The data point to listen for (e.g., DATA_POS_WORK_X).
 * @param widget The LVGL widget (e.g., lv_label_t*) to be updated.
 * @param format_str Optional format string for the value.
 * @return true on success, false if no more listener slots are available.
 */
bool data_binding_register_widget(data_binding_registry_t* registry, data_binding_value_t data_type, lv_obj_t* widget, const char * format_str);

/**
 * @brief Registers a UI widget to be updated when a specific data point changes.
 *
 * @param registry The data_binding_registry_t instance.
 * @param data_type_s The data point to listen for (e.g., "POS_WORK_X") as a C-string.
 * @param widget The LVGL widget (e.g., lv_label_t*) to be updated.
 * @param format_str Optional format string for the value.
 * @return true on success, false if no more listener slots are available.
 */
bool data_binding_register_widget_s(data_binding_registry_t* registry, const char * data_type_s, lv_obj_t* widget, const char * format_str);

/**
 * @brief Retrieves a pre-configured LVGL event handler for a specific CNC action.
 *
 * This function is the core of decoupling. UI code calls this to get a handler
 * to attach to a button, slider, etc.
 *
 * @param registry data_binding_registry_t instance.
 * @param action_type The desired action (e.g., ACTION_HOME_ALL).
 * @return A valid lv_event_cb_t that can be used with lv_obj_add_event_cb().
 */
lv_event_cb_t action_registry_get_handler(data_binding_registry_t* registry, data_action_t action_type);

/**
 * @brief Retrieves a pre-configured LVGL event handler for a specific CNC action.
 *
 * This function is the core of decoupling. UI code calls this to get a handler
 * to attach to a button, slider, etc.
 *
 * @param registry data_binding_registry_t instance.
 * @param action_type_s The desired action (e.g., "HOME_ALL") as string.
 * @return A valid lv_event_cb_t that can be used with lv_obj_add_event_cb().
 */
lv_event_cb_t action_registry_get_handler_s(data_binding_registry_t* registry, const char *action_type_s);

#ifdef __cplusplus
}
#endif

#endif // DATA_BINDING_H__
