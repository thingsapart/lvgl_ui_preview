### Summary of the JSON UI-SPEC Format

This is a declarative UI format for the LVGL graphics library, expressed in JSON or YAML. It allows developers to define complex user interfaces, including reusable components and styling, which are then programmatically generated into LVGL C objects.

The core of the format revolves around a few key concepts:

1.  **Widget Tree:** The UI is defined as a hierarchical tree of widget objects, similar to how LVGL itself works. Each object can have properties and children.
2.  **C-Code Interoperability (Registry):** A "registry" acts as a bridge between the declarative UI and the host C application. The C code can register pointers (like fonts, images, or C variables), which can then be referenced by name within the UI definition (e.g., `text_font: @my_font`). Conversely, UI elements can be given a `named` attribute to register them for access from the C code.
3.  **Components & Reusability:** UI sections can be defined as reusable `component` blocks. These components can then be instantiated multiple times using a `use-view` block, promoting a modular design.
4.  **Parameterization (Context):** Components are parameterized using a `context`. When a component is used (`use-view`), values are passed into its context. Inside the component definition, these values are accessed with a `$` prefix (e.g., `text: $title`).
5.  **Styling:** The format supports both the definition of global, reusable `lv_style_t` objects and the application of local style properties directly on a widget.
6.  **Shorthands and Abstractions:** The format simplifies development by providing several shorthands. Property names are automatically resolved to corresponding LVGL setter functions (e.g., `flex_flow` becomes `lv_obj_set_flex_flow`). Special prefixes are used for values like colors (`#rrggbb`), percentages (`50%`), registered pointers (`@name`), and context variables (`$name`).

In essence, the format provides a high-level, human-readable abstraction over the LVGL C API, enabling rapid UI development, theming, and clear separation between UI layout and application logic.

---

### Formalized Pseudo-Code Description

The following pseudo-code describes the structure of the UI definition.

#### Root Element

A UI definition is a list of top-level elements.

```
UI_Definition ::= List<TopLevelElement>
```

#### Top-Level Elements

These are the primary building blocks that can appear at the root of the definition.

```
TopLevelElement ::= Widget | ComponentDefinition | StyleDefinition
```

#### Element Definitions

**1. Widget**
The fundamental UI element. It represents an LVGL object (`lv_obj_t`) and its derivatives (`lv_label_t`, `lv_button_t`, etc.). If `type` is omitted, it defaults to a base object (`lv_obj_t`), useful as a container.

```
Widget {
  type: string                 // Optional. The LVGL widget type (e.g., "label", "button", "bar").
  named: string                // Optional. Registers this widget instance with the given name.
  children: List<Widget>       // Optional. A list of child widgets.
  with: List<WithBlock>        // Optional. Apply properties to a sub-object or expression result.
  ...properties: PropertyMap   // Any number of widget-specific properties.
}
```
*   A special widget type `use-view` is used to instantiate a component (see below).

**2. ComponentDefinition**
Defines a reusable UI template. It is not rendered directly but is referenced by a `use-view` widget.

```
ComponentDefinition {
  type: "component"            // Required.
  id: "@component_name"        // Required. The unique identifier for this component.
  root: Widget                 // Required. The root widget definition for this component.
}
```

**3. StyleDefinition**
Defines a reusable `lv_style_t` object that is automatically registered.

```
StyleDefinition {
  type: "style"                // Required.
  id: "@style_name"            // Required. The unique identifier to register this style.
  ...properties: StylePropertyMap // Any number of style properties (e.g., `radius`, `bg_color`).
}
```

**4. UseView (Component Instantiation)**
A special type of `Widget` that instantiates a defined `Component`.

```
UseView {
  type: "use-view"             // Required.
  id: "@component_name"        // Required. References a defined component.
  context: Map<string, Value>  // Optional. Provides key-value pairs for parameterization.
  ...properties: PropertyMap   // Optional. Overrides for the component's root widget properties.
}
```

---

#### Attribute Groups & Value Types

**PropertyMap**
A map of key-value pairs where keys are shorthand LVGL property names (e.g., `text`, `width`, `align`, `add_style`) and values can be of type `Value`.

```
PropertyMap ::= Map<string, Value>
```
*   **Property Name Resolution:** A key like `prop_name` on a widget of `type_name` is resolved to a C function by trying `lv_type_name_set_prop_name()`, then `lv_obj_set_prop_name()`.

**WithBlock**
Applies a set of properties to a sub-object, such as the list part of a dropdown.

```
WithBlock {
  obj: Value | FunctionCall    // Required. An expression that yields an object to work with.
  do: PropertyMap              // Required. Properties to apply to the object from 'obj'.
}
```

**FunctionCall**
Represents a direct call to an LVGL function.

```
FunctionCall {
  call: string                 // Required. The name of the LVGL function to call.
  args: List<Value>            // Optional. A list of arguments for the function.
}
```

**Value**
Represents the value of a property. It can be a standard JSON/YAML type or one of the special formats.

```
Value ::=
  | string                      // A standard string.
  | number                      // A number.
  | boolean                     // true or false.
  | Array<Value>                // A list of values (e.g., for size `[100, 50]`).
  | FunctionCall                // The result of an LVGL function call.
  | SpecialValueString          // A string with a special prefix indicating its type.
```

**SpecialValueString Formats**

| Prefix | Format         | Description                                        | Example                   |
|--------|----------------|----------------------------------------------------|---------------------------|
| `@`    | `@name`        | **Registry Reference:** A pointer from the C registry. | `text_font: @my_font`     |
| `$`    | `$name`        | **Context Variable:** A value from the current context. | `text: $title`            |
| `!`    | `!string`      | **Static String:** A heap-allocated, persistent string. | `options: !'A\nB\nC'`      |
| `#`    | `#RRGGBB`      | **Color:** A hex color, resolved to `lv_color_hex()`.| `bg_color: #ff0000`       |
| `%`    | `N%` (suffix)  | **Percentage:** A percentage, resolved to `lv_pct(N)`. | `width: 50%`              |

**Value Unescaping**
To use the special characters (`$`, `!`, `@`, `%`, `#`) literally in a string, they must be escaped by doubling them.

| Escaped     | Unescaped Result |
|-------------|------------------|
| `$$name$$`  | `$name$`         |
| `!!name!!`  | `!name!`         |
| `@@name@@`  | `@name@`         |
| `100%%`     | `100%`           |
| `##name##`  | `#name#`         |
