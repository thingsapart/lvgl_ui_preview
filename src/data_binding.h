#ifndef DATA_BINDING_TEMPLATE_H__
#define DATA_BINDING_TEMPLATE_H__

/****************************************************************************
 *
 *               Generic Data-Binding and Action Model for LVGL
 *
 ****************************************************************************
 *
 * ## Decoupling UI from Application Logic ##
 *
 * In many embedded systems, UI code (e.g., LVGL event handlers) becomes
 * tightly coupled with the application's business logic. A button's click
 * handler might directly call a motor control function, and a timer might
 * directly read a sensor and format a string for a label. This creates
 * several problems:
 *
 * - **Maintainability:** Intertwining UI and logic make it difficult
 *   to change either the UI or the logic without breaking the other.
 * - **Testability:** Testing application logic becomes difficul without a
 *   running UI, and vice-versa.
 * - **Reusability:** UI components are not easily reusable in different contexts.
 * - **Collaboration:** UI designers (who may use tools like SquareLine Studio)
 *   cannot work effectively if they need deep knowledge of the C business logic.
 *
 * This header provides a template for a "ViewModel" or "Presenter" pattern
 * to solve these issues. It creates an abstraction layer that acts as a
 * mediator between the UI and the application logic.
 *
 * - **Actions (UI -> Logic):** The UI does not call logic functions directly.
 *   Instead, it triggers a generic "action" (e.g., `ACTION_SAVE_SETTINGS`).
 *   The binding layer maps this action to the correct application function.
 * 
 *   *BUT* this is currently just done by being able to fetch pre-canned
 *   event-handlers that implement the desired action for multiple types
 *   of widgets. This was done to ease integration into UI-code, it would
 *   usually only involve a 1-line code addition.
 *
 * - **Data Bindings (Logic -> UI):** The application logic does not directly
 *   manipulate UI widgets. Instead, it updates its own state and then
 *   "notifies" the binding layer that a piece of data has changed (e.g.,
 *   `DATA_CURRENT_USER_NAME`). The binding layer is responsible for finding all
 *   UI widgets that are "listening" for this data and updating them accordingly.
 *
 * Note that some work is still involved in creating the action event handlers
 * and display functions, especially supporting multiple types of display widgets,
 * or multiple types of events (like click, press, release or value-changed).
 * So the majority of the work is in point 5. below.
 * 
 * ## Concerete implementation guide ##
 *
 * 1.  **Define Actions & Data Points:**
 *     - In your new header, populate the `app_action_t` enum with all the
 *       actions your UI can trigger (e.g., `ACTION_LOGIN`, `ACTION_START_TIMER`).
 *     - Populate the `app_data_t` enum with all the data your UI needs to
 *       display (e.g., `DATA_SYSTEM_VOLTAGE`, `DATA_LOGGED_IN_USER`).
 *
 * 2.  **Define the Main Context:**
 *     - Rename the `app_context_t` struct to something specific for your
 *       application (e.g., `cnc_viewmodel_t`, `thermostat_controller_t`).
 *     - The `void* user_data` member should point to your main application
 *       state struct, giving the action handlers access to your logic.
 *
 * 3.  **Create an Implementation File (`.c`):**
 *     - In a new `.c` file (e.g., `my_app_bindings.c`), you will implement
 *       the logic that connects the enums to your application.
 *
 * 4.  **Implement Action Handlers:**
 *     - Create a set of static `lv_event_cb_t` functions, one for each action.
 *       These functions will get the `app_context_t` from the event's
 *       `user_data`, cast its `user_data` member to your application's state
 *       struct, and call the appropriate logic function.
 *     - In an initialization function for your context, assign these function
 *       pointers to the `context.registry.action_handlers` array.
 *
 * 5.  **Implement Data Notifiers:**
 *     - Create functions in your `.c` file that your application logic can call
 *       when its state changes (e.g., `my_app_notify_voltage_changed(float v)`).
 *     - These "notifier" functions will iterate through the registered listeners
 *       in `context.registry.display_registry` for the corresponding data
 *       point and update their LVGL widgets (e.g., using `lv_label_set_text_fmt`).
 *
 * 6.  **Connect in UI Code:**
 *     - **To trigger an action:** When creating a button, get its handler via
 *       `action_registry_get_handler(context, ACTION_SAVE_SETTINGS)` and
 *       assign it using `lv_obj_add_event_cb()`.
 *     - **To display data:** For example, when creating a label, register it using
 *       `data_binding_register_widget(context, DATA_SYSTEM_VOLTAGE, my_label, "%.2f V")`.
 *       Other widgets can be supported by checking the 
 *
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Configurable number of UI elements that can listen to a single data point.
#define UI_MAX_LISTENERS 10

// --- Part 1: UI Actions (Events from UI to Application Logic) ---

/**
 * @brief Defines all possible actions the UI can trigger.
 *
 * TODO: Populate this enum with actions specific to your application.
 *       For example: ACTION_LOGIN, ACTION_LOGOUT, ACTION_SAVE_SETTINGS, ...
 */
typedef enum {
    // -- EXAMPLE ACTIONS --
    // ACTION_ITEM_SAVE,
    // ACTION_ITEM_CANCEL,
    // ACTION_JOG_X_PLUS,

    _ACTION_COUNT // Sentinel value for array sizing. Must be last.
} app_action_t;

// --- Part 2: UI Display (Data from Application Logic to UI) ---

/**
 * @brief Defines all data points the UI can display.
 *
 * TODO: Populate this enum with data points specific to your application.
 *       For example: DATA_USER_NAME, DATA_IP_ADDRESS, DATA_MOTOR_RPM, ...
 */
typedef enum {
    // -- EXAMPLE DATA POINTS --
    // DATA_CURRENT_TEMPERATURE,
    // DATA_NETWORK_STATUS,
    // DATA_BATTERY_LEVEL,

    _DATA_COUNT // Sentinel value for array sizing. Must be last.
} app_data_t;

// --- Core Data Structures ---

/**
 * @brief A handle to a UI widget that is "listening" for data changes.
 */
typedef struct {
    lv_obj_t* widget;
    const char *format_str; // Optional format string for simple label updates.
} ui_listener_t;

/**
 * @brief A registry entry holding all listeners for a single data point.
 */
typedef struct {
    ui_listener_t listeners[UI_MAX_LISTENERS];
    size_t count;
} data_binding_list_t;

/**
 * @brief The central registry for all actions and data bindings.
 *
 * This struct is intended to be embedded within a larger application-specific
 * context struct.
 */
typedef struct {
    // Registry for all displayable data points
    data_binding_list_t display_registry[_DATA_COUNT];

    // Array of pre-generated LVGL event callbacks for actions.
    lv_event_cb_t action_handlers[_ACTION_COUNT];
} data_binding_registry_t;

/**
 * @brief The main application context for the data binding model.
 *
 * TODO: Rename this struct to match your application (e.g., `cnc_viewmodel_t`).
 *       This struct should be instantiated once and passed around to all UI
 *       and logic functions that need to interact.
 */
typedef struct app_context_t {
    data_binding_registry_t registry;
    void* user_data; // A pointer to your main application state struct.
                     // This provides the action handlers with access to your logic.
} app_context_t;

extern app_context_t *REGISTRY;

// --- API Functions ---

/**
 * @brief Registers a UI widget to be updated when a specific data point changes.
 *
 * @param context The main application context.
 * @param data_type The data point to listen for (e.g., DATA_CURRENT_TEMPERATURE).
 * @param widget The LVGL widget to be updated.
 * @param format_str Optional printf-style format string for lv_label updates.
 *                   If NULL, the notifier function must handle the update logic.
 * @return true on success, false if the listener list for this data is full.
 */
bool data_binding_register_widget(app_context_t* context, app_data_t data_type, lv_obj_t* widget, const char * format_str);

/**
 * @brief String-based version of data_binding_register_widget.
 *
 * Note: This is less efficient and requires an implementation that maps strings
 * to app_data_t enum values. Recommended for use with UI generators.
 *
 * @param context The main application context.
 * @param data_type_s The data point to listen for, as a C-string (e.g., "CURRENT_TEMPERATURE").
 * @param widget The LVGL widget to be updated.
 * @param format_str Optional format string.
 * @return true on success.
 */
bool data_binding_register_widget_s(app_context_t* context, const char * data_type_s, lv_obj_t* widget, const char * format_str);


/**
 * @brief Retrieves a pre-configured LVGL event handler for a specific action.
 *
 * @param context The main application context.
 * @param action_type The desired action (e.g., ACTION_SAVE).
 * @return A valid lv_event_cb_t to be used with lv_obj_add_event_cb(). Returns NULL if not found.
 */
lv_event_cb_t action_registry_get_handler(app_context_t* context, app_action_t action_type);

/**
 * @brief String-based version of action_registry_get_handler.
 *
 * Note: This is less efficient and requires an implementation that maps strings
 * to app_action_t enum values. Recommended for use with UI generators.
 *
 * @param context The main application context.
 * @param action_type_s The desired action as a C-string (e.g., "SAVE").
 * @return A valid lv_event_cb_t. Returns NULL if not found.
 */
lv_event_cb_t action_registry_get_handler_s(app_context_t* context, const char *action_type_s);


/*
// --- EXAMPLE IMPLEMENTATIONS (to be placed in your .c file) ---

// In your .c file, you would first need a way to map string names to enums
// for the _s versions of the functions.

// static const struct { const char* name; app_data_t val; } DATA_MAP[] = {
//     { "CURRENT_TEMPERATURE", DATA_CURRENT_TEMPERATURE },
//     { "NETWORK_STATUS",      DATA_NETWORK_STATUS },
// };
// static app_data_t data_from_string(const char* s) { ... loop through DATA_MAP ... }


bool data_binding_register_widget(app_context_t* context, app_data_t data_type, lv_obj_t* widget, const char * format_str) {
    if (!context || !widget || data_type >= _DATA_COUNT) {
        return false;
    }

    data_binding_list_t* list = &context->registry.display_registry[data_type];

    if (list->count >= UI_MAX_LISTENERS) {
        // Optional: Log an error that the listener list is full
        return false;
    }

    list->listeners[list->count].widget = widget;
    list->listeners[list->count].format_str = format_str;
    list->count++;

    return true;
}

lv_event_cb_t action_registry_get_handler(app_context_t* context, app_action_t action_type) {
    if (!context || action_type >= _ACTION_COUNT) {
        return NULL;
    }
    // This assumes the action_handlers array has been populated during initialization.
    // The handler itself is what contains the logic. This function just retrieves it.
    return context->registry.action_handlers[action_type];
}

// Example of a notifier function in your application logic
void my_app_update_temperature(app_context_t* context, float new_temp) {
    // 1. Update the actual application state
    // my_app_state_t* state = (my_app_state_t*)context->user_data;
    // state->temperature = new_temp;

    // 2. Notify all registered UI widgets
    data_binding_list_t* list = &context->registry.display_registry[DATA_CURRENT_TEMPERATURE];
    for (size_t i = 0; i < list->count; ++i) {
        ui_listener_t* listener = &list->listeners[i];
        if (listener->widget && listener->format_str) {
            lv_label_set_text_fmt(listener->widget, listener->format_str, new_temp);
        } else if (lv_obj_check_type(listener->widget, &lv_bar_class)) {
            // Handle other widget types if needed
            lv_bar_set_value(listener->widget, (int32_t)new_temp, LV_ANIM_ON);
        }
    }
}
*/


#ifdef __cplusplus
}
#endif

#endif // DATA_BINDING_TEMPLATE_H__