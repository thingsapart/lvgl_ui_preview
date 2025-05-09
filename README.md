# lvgl_ui_preview

Define and preview LVGL UI layouts in YAML/JSON.

Mostly vibe-coded because this was supposed to be a quick thing... but now it's somehow parsing the LVGL JSON API definition and auto-generating a ~30k loc C library from the API definition automatically... 🤷 

Half-rudimentary but somewhat useful. A couple things to note:
* C-pointers used in API calls need to be "registered" to make them known to the YAML,
* "component"s (aka sub-views) can be defined and reused (todo: "context" or template values),
* supports declaring `lv_style_t` styles (they are also automatically registered) as well as per-object local style,
* pretty good support for many lvgl functions, as well as nested function calls in "setter" arguments,
* property setters and methods are "resolved" from a shorthand (several prefixes are tried),
* macro-defined values (like `LV_SIZE_CONTENT`) need to be passed as an additional json file to the generator.

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
            ovr: Jog Ovr```

# Convert your YAML to JSON

* `yq -o=json eval ui.yaml > ui.json`

# Formerly

A simple way to preview LVGL UIs quickly to play with layouts/widgets/UI designs (see lvgl_gen_emu branch for a library to turn lvgl C calls into JSON and back to LVGL calls).
