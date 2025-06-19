The JSON API-SPEC provides a comprehensive, high-level translation of the LVGL API. It transforms the flat C-style function list from the source specification into a hierarchical, object-oriented structure that is easier for tools to parse.

### Summary of the Format

This is a machine-readable JSON format that provides a structured, high-level representation of the LVGL C API. Its primary purpose is to serve as an intermediate specification for tooling, such as UI generators, linters, or documentation builders. It translates the flat, procedural nature of the LVGL C API into a hierarchical, object-oriented model that is easier to parse and reason about.

The core of the format is the transformation of the C API into distinct, organized sections:

1.  **Widget Hierarchy:** The format defines all LVGL widgets in an object-oriented hierarchy (`obj` -> `label`, `obj` -> `button`, etc.). It explicitly lists each widget's creation function, available methods, and—most importantly—a set of high-level **properties** that are inferred from C setter functions (e.g., the C function `lv_label_set_text` is represented as a `text` property on the `label` widget).
2.  **Non-Widget Objects:** Other key LVGL types, like `lv_style_t`, are described as `objects` with their own set of properties derived from their respective setter functions (e.g., `lv_style_set_bg_color` becomes a `bg_color` property on the `style` object).
3.  **Value and Constant Mapping:** The format provides exhaustive lookup maps for all `enums` (e.g., `lv_bar_mode_t`) and global `constants` (e.g., `LV_ALIGN_CENTER`), making these values easily accessible to tools without needing to parse C header files.
4.  **Raw Function Reference:** For comprehensive tooling that may need access to the entire API, a complete, flat list of all C `functions` and their signatures is included as a global reference.

In essence, this format acts as a definitive "database" of the LVGL API, abstracting away the complexities of C source parsing and presenting the library's features in a clean, structured, and predictable way for automated tools.

---

### Formalized Pseudo-Code Description

The following pseudo-code describes the structure of the API-SPEC definition.

#### Root Element

The API specification is a single root object containing all categorized definitions.

```
API_SPEC_Definition ::= ApiSpecObject
```

#### Top-Level Elements

These are the primary properties of the root `ApiSpecObject`.

```
ApiSpecObject {
  constants: Map<string, string>               // Maps constant names to their values.
  enums: Map<string, EnumDefinition>           // Maps enum type names to their definitions.
  functions: Map<string, FunctionDefinition>   // Maps all C function names to their signatures.
  widgets: Map<string, WidgetDefinition>       // Maps widget type names to their definitions.
  objects: Map<string, ObjectDefinition>       // Maps non-widget object types to their definitions.
}
```

#### Element Definitions

**1. WidgetDefinition**
Describes a single LVGL widget, its place in the hierarchy, and its associated properties and methods.

```
WidgetDefinition {
  inherits: string | null                        // The name of the parent widget (e.g., "obj"), or null for the base object.
  create: string                                 // Optional. The C function used to create an instance (e.g., "lv_label_create").
  properties: Map<string, PropertyDefinition>    // A map of high-level properties inferred from C setter functions.
  methods: Map<string, FunctionDefinition>       // A map of C functions that operate on this widget type.
}
```

**2. ObjectDefinition**
Describes a non-widget type (like `style`) primarily as a collection of properties.

```
ObjectDefinition ::= Map<string, PropertyDefinition>
```

**3. EnumDefinition**
Defines the members of a single enumerated type.

```
EnumDefinition ::= Map<string, string>      // Maps an enum member name (e.g., "LV_BAR_MODE_NORMAL") to its string-represented value.
```

**4. PropertyDefinition**
Defines a high-level property that was inferred from a C function. This links a simple name (e.g., `text`) to its underlying implementation.

```
PropertyDefinition {
  setter: string                             // The name of the C function used to set this property (e.g., "lv_label_set_text").
  type: string                               // A simplified, high-level type for the property (e.g., "string", "int", "color").
}
```

**5. FunctionDefinition**
Provides the signature of a single C function.

```
FunctionDefinition {
  return_type: string                        // The C return type of the function (e.g., "void", "lv_obj_t*").
  args: List<string>                         // An ordered list of the function's C argument types (e.g., ["lv_obj_t*", "const char*"]).
}
```
---


Key Sections:
*   **`widgets`**: Details the full widget hierarchy, defining each widget's parent, creation function, methods, and a list of high-level `properties` inferred from C setter functions (e.g., `lv_label_set_text` becomes a `text` property).
*   **`objects`**: Describes non-widget types like `style` and their associated properties.
*   **`enums` / `constants`**: Provide exhaustive maps of all enumerated values and global constants.
*   **`functions`**: Contains a complete, flat reference of all raw C function signatures for tooling that requires full API access.

---

JSON schema:

```
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "LVGL Translated API Definition",
  "description": "A structured, high-level representation of the LVGL API, derived from its source code specification.",
  "type": "object",
  "properties": {
    "constants": {
      "type": "object",
      "description": "A map of global constants and parameter-less macros. Keys are the constant names, values are their string-represented values or expressions.",
      "additionalProperties": {
        "type": "string"
      }
    },
    "enums": {
      "type": "object",
      "description": "A map of all enumerated types. The key is the enum type name (e.g., 'lv_result_t').",
      "additionalProperties": {
        "type": "object",
        "description": "An individual enum, mapping member names to their string-represented values.",
        "additionalProperties": {
          "type": "string"
        }
      }
    },
    "functions": {
      "type": "object",
      "description": "A flat map of all functions in the API. This serves as a global reference.",
      "additionalProperties": {
        "$ref": "#/$defs/functionDefinition"
      }
    },
    "widgets": {
      "type": "object",
      "description": "The hierarchy of all widgets, their properties, and methods.",
      "additionalProperties": {
        "$ref": "#/$defs/widgetDefinition"
      }
    },
    "objects": {
      "type": "object",
      "description": "Non-widget objects like styles, which are primarily containers for properties.",
      "additionalProperties": {
        "type": "object",
        "description": "An individual object type, mapping property names to their definitions.",
        "additionalProperties": {
          "$ref": "#/$defs/propertyDefinition"
        }
      }
    }
  },
  "required": [
    "constants",
    "enums",
    "functions",
    "widgets",
    "objects"
  ],
  "$defs": {
    "functionDefinition": {
      "type": "object",
      "properties": {
        "return_type": {
          "type": "string",
          "description": "The C return type of the function."
        },
        "args": {
          "type": "array",
          "description": "An ordered list of the function's C argument types.",
          "items": {
            "type": "string"
          }
        }
      },
      "required": [
        "return_type",
        "args"
      ]
    },
    "propertyDefinition": {
      "type": "object",
      "properties": {
        "setter": {
          "type": "string",
          "description": "The name of the C function used to set this property."
        },
        "type": {
          "type": "string",
          "description": "A simplified, high-level type for the property (e.g., 'string', 'int', 'color', 'enum')."
        }
      },
      "required": [
        "setter",
        "type"
      ]
    },
    "widgetDefinition": {
      "type": "object",
      "properties": {
        "inherits": {
          "type": [
            "string",
            "null"
          ],
          "description": "The name of the parent widget, or null for the base object."
        },
        "create": {
          "type": "string",
          "description": "The name of the C function used to create this widget instance."
        },
        "properties": {
          "type": "object",
          "description": "A map of properties available for this widget.",
          "additionalProperties": {
            "$ref": "#/$defs/propertyDefinition"
          }
        },
        "methods": {
          "type": "object",
          "description": "A map of methods specific to this widget.",
          "additionalProperties": {
            "$ref": "#/$defs/functionDefinition"
          }
        }
      },
      "required": [
        "inherits",
        "properties",
        "methods"
      ]
    }
  }
}
```

---

```
Short Example:

    {
  "constants": {
    "LV_OPA_COVER": "0x0FF",
    "LV_ALIGN_CENTER": "0x09",
    "LV_RADIUS_CIRCLE": "0x7FFF"
  },
  "enums": {
    "lv_result_t": {
      "LV_RESULT_INVALID": "0x0",
      "LV_RESULT_OK": "0x1"
    },
    "lv_bar_mode_t": {
      "LV_BAR_MODE_NORMAL": "0x0",
      "LV_BAR_MODE_SYMMETRICAL": "0x1",
      "LV_BAR_MODE_RANGE": "0x2"
    }
  },
  "functions": {
    "lv_obj_set_width": {
      "return_type": "void",
      "args": [
        "lv_obj_t*",
        "lv_coord_t"
      ]
    },
    "lv_label_create": {
      "return_type": "lv_obj_t*",
      "args": [
        "lv_obj_t*"
      ]
    },
    "lv_label_set_text": {
      "return_type": "void",
      "args": [
        "lv_obj_t*",
        "const char*"
      ]
    },
    "lv_style_init": {
      "return_type": "void",
      "args": [
        "lv_style_t*"
      ]
    },
    "lv_style_set_bg_color": {
      "return_type": "void",
      "args": [
        "lv_style_t*",
        "lv_color_t"
      ]
    }
  },
  "widgets": {
    "obj": {
      "inherits": null,
      "properties": {
        "width": {
          "setter": "lv_obj_set_width",
          "type": "size"
        },
        "height": {
          "setter": "lv_obj_set_height",
          "type": "size"
        }
      },
      "methods": {
        "lv_obj_set_width": {
          "return_type": "void",
          "args": [
            "lv_obj_t*",
            "lv_coord_t"
          ]
        },
        "lv_obj_set_height": {
          "return_type": "void",
          "args": [
            "lv_obj_t*",
            "lv_coord_t"
          ]
        }
      }
    },
    "label": {
      "inherits": "obj",
      "create": "lv_label_create",
      "properties": {
        "text": {
          "setter": "lv_label_set_text",
          "type": "string"
        }
      },
      "methods": {
        "lv_label_create": {
          "return_type": "lv_obj_t*",
          "args": [
            "lv_obj_t*"
          ]
        },
        "lv_label_set_text": {
          "return_type": "void",
          "args": [
            "lv_obj_t*",
            "const char*"
          ]
        }
      }
    },
    "button": {
      "inherits": "obj",
      "create": "lv_button_create",
      "properties": {},
      "methods": {
        "lv_button_create": {
          "return_type": "lv_obj_t*",
          "args": [
            "lv_obj_t*"
          ]
        }
      }
    }
  },
  "objects": {
    "style": {
      "bg_color": {
        "setter": "lv_style_set_bg_color",
        "type": "color"
      },
      "radius": {
        "setter": "lv_style_set_radius",
        "type": "int"
      }
    }
  }
}
```
