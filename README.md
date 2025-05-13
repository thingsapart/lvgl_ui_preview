# lvgl_ui_preview

Define and preview LVGL UI layouts in YAML/JSON.

Mostly vibe-coded because this was supposed to be a quick thing... but now it's somehow parsing the LVGL JSON API definition and auto-generating a ~30k loc C library from the API definition automatically... 🤷 

Half-rudimentary but somewhat useful. A couple things to note:
* C-pointers used in API calls need to be "registered" to make them known to the YAML,
* "component"s (aka sub-views) can be defined and reused,
* context values (accessible via $name),
* supports declaring `lv_style_t` styles (they are also automatically registered) as well as per-object local style,
* pretty good support for many lvgl functions, as well as nested function calls in "setter" arguments,
* property setters and methods are "resolved" from a shorthand (several prefixes are tried),
* macro-defined values (like `LV_SIZE_CONTENT`) need to be passed as an additional json file to the generator,
* support for named/registered widgets/pointers (to access parts of the UI from C code),
* "with-blocks" to apply properties to sub-components,
* "unescaping" special characters in values ("$name$" => "$name", "!name!" => "!name", "@name@" => "@name", "100%%" => "100%" "#name#" => "#name")

## Registry: C-pointers used in API calls need to be "registered" to make them known to the YAML

The registry provides a way to link C-host-code and ui-layout definitions and allow passing objects between them. The C host-app can register pointers that can be accessed in the ui-layout definition via special "@" values (for example `text_font: @font_xyz` to set label's font to a previously registered font "font_xyz").

On the other hand, ui-layout blocks can be assigned a "named" attribute to store the current block in the registry (eg `{ "type": "button", "named": "btn1", ...}` would store a lv_button_t in the registry under name "btn1".

1. Call `lvgl_json_register_ptr(name, type_name, ptr)` to register,
2. and `lvgl_json_get_registered_ptr(name, expected_type_name)` to retrieve,
3. reference registered pointers via "@name" attributes in the ui layout,
4. store current obj/block in the registry via `"name": "id-to-store"` in ui layouts.

Somewhat typesafe by specifying a registered and expected types during registration and retrieval.

## Components (aka reusable sub-views)

1. Define a component using a `{ "type": "component", .... }` block,
2. and reference said component using a `{ "type": "use-view", ... } block,
3. use context to pass values into components.

See example ui code. See "feed_rate_scale" and "axis_pos_display" components and their "use-view"s for example, also note the use of "context" properties to pass information into sub-views.

## Styles (lv_style_t)

Supported using `{ "type": "style", "id": "@someid" ... }` bocks. The "id" attribute (whose value must start with "@") must be specified. Styles are memory allocated on the heap and saved to the registry using the specified id (omiting the leading "@").

For example, the following code creates and registers a style named "bar_indicator" and two lvgl widgets (obj and button) using said style:
```
- type: style
  id: '@bar_indicator'
  radius: 4

- type: obj
  add_style: ['@bar_indicator', 0]
- type: button
  add_style: ['@bar_indicator', 0]
```

## Context

Generally, any property value starting with "$" is used to retrieve its value from the current context. If not found, the property is not set. The current context can be assinged along in widget and `use-view` blocks.

For example, this is how context would be passed to a sub-view (it has to be parametrized using "$..." property values:

```
        - type: use-view
          id: '@axis_pos_display'
          context:
            axis: X
            wcs_pos: '11.000'
            abs_pos: '51.000'
            delta_pos: '2.125'
```

## Calling lvgl functions

Many lvgl functions that are synthesized from the LVGL api definition can be called as part of the property assignment, for example one can `lv_pct` when assigning a width attribute as `..., "width": { "call": "lv_pct", args: [50] }, ...`.

an object block with properties "call" and "args" is interpreted as a call.

## With blocks

With blocks allow "working with" the result of an expression (could be referencing a registered pointer or result of a call). This is useful to access or create sub-objects like "lv_dropdown_get_list" to access the "list" sub-object of a dropdown and assign a prop/style:

```
  - type: dropdown
    ...
    with:
      obj: { call: 'lv_dropdown_get_list', args: [] }                                                                 
      do: 
        min_width: 200
```

or adding sub-menus to menus.

## Special string values and unescaping them

There are a couple short-hands for common attribute types and special values:

* "$ctx-var" accesses a context-variable of name "ctx-var",
* "!this is a static string" provides heap-allocated versions of string that "outlast" the json parsing/ui-generation phase as they are sometimes needed by certain lvgl functions like `lv_dropdown_set_options` (when the component does not allocate and copy the value),
* "@id" references a registered pointer like `text_font: @lv_font_montserrat_24` would reference a previously registered variable named "lv_font_montserrat_24" (eg via `lvgl_json_register_ptr("font_montserrat_24", "lv_font_t", (void *) &lv_font_montserrat_24);` in the main application),
* "#aabbcc" is a short-hand for `lv_color_hex(0xaa, 0xbb, 0xcc)`,
* "nnn%" is a short-hand for `lv_pct(nnn)`.

These can be unescaped to retrieve regular strings (for example to set the text of a label to "100%", one would need to use `text: 100%%`):

* "$name$" => "$name"
* "!name!" => "!name"
* "@name@" => "@name"
* "100%%" => "100%"
* "#name#" => "#name"

# Example
![An example layout: CNC status interface](https://github.com/thingsapart/lvgl_ui_preview/blob/main/docs/ui_ex.jpeg?raw=true)

An example layout: CNC status interface:

```
# Styles
- type: style
  id: '@debug'
  outline_width: 1
  outline_color: '#ffeeff'
  outline_opa: 150
  border_width: 1
  border_color: '#ffeeff'
  border_opa: 150
  radius: 0

- type: style
  id: '@container'
  pad_all: 0
  margin_all: 0
  border_width: 0
  pad_row: 3
  pad_column: 5

- type: style
  id: '@bar_indicator'
  radius: 4

- type: style
  id: '@flex_x'
  layout: LV_LAYOUT_FLEX
  flex_flow: LV_FLEX_FLOW_ROW

- type: style
  id: '@flex_y'
  layout: LV_LAYOUT_FLEX
  flex_flow: LV_FLEX_FLOW_COLUMN

# Components/Sub-Views
- type: component
  id: '@axis_pos_display'
  root:
    add_style: ['@flex_y', 0]
    add_style: ['@container', 0]
    size: [100%, LV_SIZE_CONTENT]
    pad_all: 10
    pad_bottom: 20
    children:
      -
        add_style: ['@flex_x', 0]
        add_style: ['@container', 0]
        size: [100%, LV_SIZE_CONTENT]
        children:
          - type: 'label'
            text_font: '@font_kode_24'
            text: $axis
            width: 50
          - type: 'label'
            text_font: '@font_kode_24'
            text: $wcs_pos
            flex_grow: 1
            text_align: LV_TEXT_ALIGN_RIGHT
      -
        add_style: ['@flex_x', 0]
        add_style: ['@container', 0]
        size: [100%, LV_SIZE_CONTENT]
        children:
          - type: 'label'
            text_font: '@font_kode_14'
            text: $abs_pos
            flex_grow: 1
            text_align: LV_TEXT_ALIGN_RIGHT
          - type: 'label'
            text_font: '@font_montserrat_14'
            text: "\uF124"
            width: 14
          - type: 'label'
            text_font: '@font_kode_14'
            text: $delta_pos
            text_align: LV_TEXT_ALIGN_RIGHT
            flex_grow: 1
          - type: 'label'
            text_font: '@font_montserrat_14'
            text: "\uF051"
            width: 14

- type: component
  id: '@feed_rate_scale'
  root:
    size: [100%, 100%]
    add_style: ['@container', 0]
    layout: LV_LAYOUT_FLEX
    flex_flow: LV_FLEX_FLOW_ROW
    height: LV_SIZE_CONTENT
    children:
      # Left Column
      - layout: LV_LAYOUT_FLEX
        add_style: ['@container', 0]
        flex_flow: LV_FLEX_FLOW_COLUMN
        width: 100%
        height: LV_SIZE_CONTENT
        flex_grow: 1
        children:
          - type: label
            text: $label
            height: LV_SIZE_CONTENT
            width: 100%
            text_font: '@font_kode_20'
          - type: grid
            cols: [LV_GRID_CONTENT, LV_GRID_FR_1]
            rows: [LV_GRID_CONTENT]
            add_style: ['@container', 0]
            width: 100%
            height: LV_SIZE_CONTENT
            children:
              - type: label
                text: $letter
                text_font: '@font_kode_24'
                grid_cell: [LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1]
                height: LV_SIZE_CONTENT
                pad_left: 10
              - type: label
                grid_cell: [LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_END, 0, 1]
                text_font: '@font_kode_24'
                height: LV_SIZE_CONTENT
                text: '1000'
                pad_right: 10
          - layout: LV_LAYOUT_FLEX
            flex_flow: LV_FLEX_FLOW_COLUMN
            add_style: ['@container', 0]
            width: 100%
            height: LV_SIZE_CONTENT
            children:
              - type: bar
                width: 100%
                height: 20
                margin_left: 15
                margin_right: 15
                add_style: ['@bar_indicator', LV_PART_MAIN]
                add_style: ['@bar_indicator', LV_PART_INDICATOR]
                value: [65, 0]
                bg_color: '#5dd555'
                bg_opa: 255
              - type: scale
                width: 100%
                height: 20
                margin_left: 15
                margin_right: 15

      # Right column
      - layout: LV_LAYOUT_FLEX
        add_style: ['@container', 0]
        flex_flow: LV_FLEX_FLOW_COLUMN
        width: LV_SIZE_CONTENT
        height: LV_SIZE_CONTENT
        pad_right: 0
        children:
          - type: label
            text: $unit
            text_font: '@font_kode_20'
          - type: label
            text: $ovr
          - type: label
            text: 100%%
          - type: label
            text: 65%%

# Main UI Layout
- layout: LV_LAYOUT_FLEX
  flex_flow: LV_FLEX_FLOW_ROW
  add_style: ['@container', 0]
  size: [100%, 240]
  children:
    # Left main column (axis positions)
    - layout: LV_LAYOUT_FLEX
      flex_flow: LV_FLEX_FLOW_COLUMN
      add_style: ['@container', 0]
      height: 100%
      flex_grow: 60
      border_side: LV_BORDER_SIDE_RIGHT
      border_width: 2
      radius: 0
      border_color: '#ffffff'
      border_opa: 90
      children:
        - type: use-view
          id: '@axis_pos_display'
          context:
            axis: X
            wcs_pos: '11.000'
            abs_pos: '51.000'
            delta_pos: '2.125'
        - type: use-view
          id: '@axis_pos_display'
          context:
            axis: Y
            wcs_pos: '22.000'
            abs_pos: '72.000'
            delta_pos: '-12.125'
        - type: use-view
          id: '@axis_pos_display'
          context:
            axis: Z
            wcs_pos: '1.000'
            abs_pos: '1.000'
            delta_pos: '0.125'

    # Right main column (Feeds & Speeds)
    - layout: LV_LAYOUT_FLEX
      flex_flow: LV_FLEX_FLOW_COLUMN
      add_style: ['@container', 0]
      height: 100%
      flex_grow: 45
      children:
        - type: use-view
          id: '@feed_rate_scale'
          context:
            unit: MM/MIN
            label: FEED
            letter: F
            ovr: Feed Ovr
        - type: use-view
          id: '@feed_rate_scale'
          context:
            unit: /MIN
            label: SPEED
            letter: S
            ovr: Speed Ovr
        # - type: use-view
          id: '@feed_rate_scale'
          context:
            unit: MM/MIN
            label: JOG
            letter: J
            ovr: Jog Ovr
```

# Convert your YAML to JSON

* `yq -o=json eval ui.yaml > ui.json`

# Formerly

A simple way to preview LVGL UIs quickly to play with layouts/widgets/UI designs (see lvgl_gen_emu branch for a library to turn lvgl C calls into JSON and back to LVGL calls).
