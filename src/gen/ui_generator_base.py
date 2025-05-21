import abc
import collections.abc
import json
import logging
from typing import Any, Dict, List, Optional, Tuple, Union

logger = logging.getLogger(__name__)

_SENTINEL = object()

# Copied from src/gen/c_transpiler/transpiler.py
# Helper for JSON parsing that preserves order and allows duplicates
class CJSONObject(collections.abc.Mapping):
    # pylint: disable=too-many-ancestors
    def __init__(self, *args, **kwargs):
        if len(args) > 1:
            raise TypeError(f"expected at most 1 arguments, got {len(args)}")
        if args:
            if isinstance(args[0], list):
                self._kvs = list(args[0])
            elif isinstance(args[0], dict):
                self._kvs = list(args[0].items())
            else:
                raise TypeError(
                    f"expected list or dict, got {type(args[0])}"
                )
        else:
            self._kvs = []
        if kwargs:
            self._kvs.extend(kwargs.items())

        self._rebuild_dict()

    def _rebuild_dict(self):
        self._d = {}
        # Build a dictionary for faster lookups, prioritizing later values for duplicates
        for k, v in self._kvs:
            self._d[k] = v

    def __setitem__(self, key, value):
        if not isinstance(key, str):
            raise TypeError(f"expected str, got {type(key)}")

        # Remove existing entries with the same key
        self._kvs = [(k, v) for k, v in self._kvs if k != key]
        # Add the new key-value pair
        self._kvs.append((key, value))
        self._rebuild_dict()


    def __getitem__(self, key):
        return self._d[key]

    def __len__(self):
        return len(self._kvs)

    def __iter__(self):
        return iter(self._d)

    def __str__(self):
        return f"CJSONObject({self._kvs})"

    def __repr__(self):
        return f"CJSONObject({self._kvs})"

    def items(self):
        return self._kvs.__iter__() # Iterate over the original list of pairs

    def keys(self):
        return [k for k, v in self._kvs]

    def values(self):
        return [v for k, v in self._kvs]

    def get(self, key, default=None):
        return self._d.get(key, default)

    def pop(self, key, default=_SENTINEL):
        if default is _SENTINEL:
            if key not in self._d:
                raise KeyError(key)
            val = self._d.pop(key)
        else:
            val = self._d.pop(key, default)

        # Rebuild _kvs to reflect the removal
        self._kvs = [(k, v) for k, v in self._kvs if k != key]
        return val


def cjson_object_hook(ordered_pairs: List[Tuple[str, Any]]) -> CJSONObject:
    """
    Hook for json.load or json.loads to use CJSONObject.
    Ensures that the order of keys in JSON objects is preserved and allows duplicate keys.
    """
    return CJSONObject(ordered_pairs)


class BaseUIGenerator(abc.ABC):
    def __init__(self, api_info: Dict[str, Any], ui_spec_data: CJSONObject): # Changed: ui_spec_data is CJSONObject
        self.api_info = api_info
        self.ui_spec_data = ui_spec_data # This is the root CJSONObject (or list of them)
        self.component_definitions: Dict[str, CJSONObject] = {} # Store CJSONObjects

    @abc.abstractmethod
    def create_entity(self, py_node_data: CJSONObject, type_str: str, 
                      parent_entity_info: Optional[Dict[str, Any]], 
                      named_path_prefix: Optional[str]) -> Dict[str, Any]:
        """
        Creates a UI entity.
        Args:
            py_node_data: The CJSONObject for the current node.
            type_str: The resolved type string of the node.
            parent_entity_info: Entity info dict of the parent, or None for root.
            named_path_prefix: Current named path prefix.
        Returns:
            A dictionary containing information about the created entity (e.g., its C variable name, type).
            This dictionary is the 'entity_info' or 'entity_ref'.
        """
        pass

    @abc.abstractmethod
    def call_setter(self, target_entity_info: Dict[str, Any], setter_fn_name: str, 
                    py_value_args: List[Any], py_current_context: CJSONObject) -> None:
        """
        Applies a property/setter to a target entity.
        Args:
            target_entity_info: Entity info dict of the target.
            setter_fn_name: Resolved C name of the setter function.
            py_value_args: List of Python values for the arguments (after formatting if needed by specific generator).
            py_current_context: The current Python CJSONObject context.
        """
        pass

    @abc.abstractmethod
    def format_value(self, py_json_value: Any, expected_c_type: str, 
                     context_entity_info: Optional[Dict[str, Any]], 
                     py_current_context: CJSONObject) -> Any:
        """
        Converts a Python/JSON value to a generator-specific representation.
        For StaticCTranspiler, this might be a C literal string.
        For DynamicRuntimeRenderer, this might be a C variable name string for a cJSON object,
        or it might pre-process certain values.
        Args:
            py_json_value: The Python value from the CJSONObject.
            expected_c_type: Hint for the expected C type (e.g. "lv_coord_t", "const char*").
            context_entity_info: Entity info of the object this value is for (e.g. for LV_PCT).
            py_current_context: The current Python CJSONObject context.
        Returns:
            A representation of the value suitable for the generator.
        """
        pass

    @abc.abstractmethod
    def handle_children_attribute(self, py_children_data: List[CJSONObject], 
                                  parent_entity_info: Dict[str, Any], 
                                  parent_named_path_prefix: Optional[str], 
                                  py_current_context: CJSONObject) -> None:
        """
        Processes the "children" attribute of a node.
        This method will typically iterate py_children_data and call self.process_node for each child.
        Args:
            py_children_data: List of CJSONObjects representing child nodes.
            parent_entity_info: Entity info of the parent for these children.
            parent_named_path_prefix: The named path prefix for these children.
            py_current_context: The current Python CJSONObject context for the children.
        """
        pass

    @abc.abstractmethod
    def handle_named_attribute(self, py_named_value: str, target_entity_info: Dict[str, Any], 
                               path_prefix: Optional[str], type_for_registry_str: str) -> Optional[str]:
        """
        Handles the "named" attribute for an entity.
        Args:
            py_named_value: The string value of the "named" attribute.
            target_entity_info: Entity info of the target entity being named.
            path_prefix: Current path prefix.
            type_for_registry_str: The C type string for registry (e.g., "lv_button_t").
        Returns:
            The new effective path prefix if changed by the naming, otherwise None or original.
        """
        pass

    @abc.abstractmethod
    def handle_with_attribute(self, py_with_node: CJSONObject, current_entity_info: Dict[str, Any], 
                              path_prefix: Optional[str], py_current_context: CJSONObject) -> None:
        """
        Handles the "with" attribute.
        The implementation will resolve `py_with_node.obj` and then call `self.apply_attributes`
        for the `py_with_node.do` block, targeting the resolved object.
        Args:
            py_with_node: The CJSONObject representing the value of the "with" attribute.
            current_entity_info: Entity info of the entity this "with" is attached to.
            path_prefix: Current path prefix.
            py_current_context: The current Python CJSONObject context.
        """
        pass

    @abc.abstractmethod
    def handle_grid_layout(self, target_entity_info: Dict[str, Any], 
                           py_cols_data: Optional[List[Any]], py_rows_data: Optional[List[Any]], 
                           py_current_context: CJSONObject) -> None:
        """
        Handles grid layout properties "cols" and "rows".
        Args:
            target_entity_info: Entity info of the grid container.
            py_cols_data: Python list for column descriptors.
            py_rows_data: Python list for row descriptors.
            py_current_context: The current Python CJSONObject context.
        """
        pass

    @abc.abstractmethod
    def register_component_definition(self, component_id: str, component_root_data: CJSONObject) -> None: # component_root_data is CJSONObject
        """Stores component definitions. `component_id` does not include '@'."""
        pass

    @abc.abstractmethod
    def get_component_definition(self, component_id: str) -> Optional[CJSONObject]: # component_id does not include '@'
        """Retrieves component definitions."""
        pass

    @abc.abstractmethod
    def get_entity_reference_for_id(self, id_str: str) -> Optional[Dict[str, Any]]: # id_str includes '@'
        """
        Retrieves a registered entity's info. Called with '@id'.
        """
        pass

    @abc.abstractmethod
    def get_type_information(self, entity_info: Dict[str, Any]) -> Dict[str, Any]:
        """
        Gets type information about an entity, possibly augmenting the provided entity_info.
        The input `entity_info` is the dict returned by `create_entity`.
        This method might be used to add more detailed type info if needed by the generator.
        Returns an (potentially augmented) entity_info dictionary.
        """
        pass
    
    @abc.abstractmethod
    def register_entity_internally(self, id_str: str, entity_info: Dict[str, Any], path_for_registration: str) -> None:
        """
        Internally registers an entity with its ID and info for later lookup by this generator.
        Args:
            id_str: The ID string (e.g., "@my_button", "my_label_in_named").
            entity_info: The entity information dictionary.
            path_for_registration: The fully resolved path for this registration.
        """
        pass

    # --- Concrete methods ---
    def get_node_id(self, py_node_data: CJSONObject) -> Optional[str]:
        """Extracts a node's ID. Returns ID including '@' if present."""
        node_id = py_node_data.get("id")
        if isinstance(node_id, str) and node_id.startswith("@"):
            return node_id
        elif isinstance(node_id, str): # ID without '@' is not considered a registerable ID by default
            logger.info(f"Node has an 'id': \"{node_id}\" that does not start with '@'. It will not be used for registration unless handled by 'named'.")
        return None

    def get_node_type(self, py_node_data: CJSONObject) -> str:
        """Extracts a node's type, defaulting to "obj"."""
        return py_node_data.get("type", "obj")
        
    def get_node_context_override(self, py_node_data: CJSONObject) -> Optional[CJSONObject]:
        """Gets local context overrides from a node, expects a CJSONObject."""
        context_override = py_node_data.get("context")
        if isinstance(context_override, CJSONObject):
            return context_override
        elif context_override is not None:
            logger.warning(f"Node 'context' property is not a CJSONObject (type: {type(context_override)}). Ignoring.")
        return None

    def process_ui(self, 
                   initial_parent_entity_info: Optional[Dict[str, Any]], 
                   initial_py_context_data: Optional[CJSONObject] = None, 
                   initial_named_path_prefix: Optional[str] = None) -> None:
        py_context = initial_py_context_data if initial_py_context_data is not None else CJSONObject([]) # Ensure it's a CJSONObject
        
        if isinstance(self.ui_spec_data, list):
            for node_data in self.ui_spec_data: # node_data here is CJSONObject
                self.process_node(node_data, initial_parent_entity_info, py_context, initial_named_path_prefix)
        elif isinstance(self.ui_spec_data, CJSONObject): 
            self.process_node(self.ui_spec_data, initial_parent_entity_info, py_context, initial_named_path_prefix)
        else:
            logger.warning(f"ui_spec_data is not a list or CJSONObject, but {type(self.ui_spec_data)}. Skipping processing.")

    def process_node(self, py_node_data: CJSONObject, 
                     parent_entity_info: Optional[Dict[str, Any]], 
                     py_current_context: CJSONObject, 
                     named_path_prefix: Optional[str]) -> Optional[Dict[str, Any]]:
        
        node_type_str = self.get_node_type(py_node_data)
        node_id_str = self.get_node_id(py_node_data) # Includes '@' if valid ID
        
        # Handle local context overrides
        local_context_override = self.get_node_context_override(py_node_data)
        if local_context_override is not None:
            # Create a new context by overlaying local_context_override on py_current_context
            # This assumes CJSONObject can be created from a list of pairs, and updated.
            # A more robust merge might be needed if order from both is critical beyond simple override.
            # For now, simple overlay: local keys override parent context keys.
            # py_current_context might be a CJSONObject([])
            
            # Create a new CJSONObject for the new context
            # Start with parent context, then update with local overrides
            # This requires CJSONObject to support dict-like initialization or an update method
            
            # Simplistic merge: convert to dicts, merge, then back to CJSONObject if necessary
            # This loses ordering if py_current_context had duplicates not overridden.
            # A better CJSONObject.update or merge method would be ideal.
            # For now:
            temp_context_dict = {}
            for k,v in py_current_context.items(): temp_context_dict[k] = v
            for k,v in local_context_override.items(): temp_context_dict[k] = v # Override
            py_current_context = CJSONObject(list(temp_context_dict.items()))


        if node_type_str == "component":
            if node_id_str and node_id_str.startswith("@"):
                # Component ID does not include '@' for registration map key
                self.register_component_definition(node_id_str[1:], py_node_data) 
            else:
                logger.warning("Component node encountered without a valid '@id'. Skipping registration.")
            return None 

        if node_type_str == "use-view":
            return self.process_use_view_node(py_node_data, parent_entity_info, py_current_context, named_path_prefix)

        if node_type_str == "context":
            return self.process_context_node(py_node_data, parent_entity_info, py_current_context, named_path_prefix)

        # Regular node processing
        effective_path_for_node_creation = named_path_prefix
        # If node has an ID, it forms part of the path for its own creation (if relevant to generator)
        # and is the base for its children's paths if "named" is not used.
        if node_id_str: 
            id_segment = node_id_str[1:] # Remove '@'
            if named_path_prefix:
                effective_path_for_node_creation = f"{named_path_prefix}.{id_segment}"
            else:
                effective_path_for_node_creation = id_segment
        
        created_entity_info = self.create_entity(py_node_data, node_type_str, parent_entity_info, effective_path_for_node_creation)

        if created_entity_info is not None:
            # Allow generator to augment type information if needed
            final_entity_info = self.get_type_information(created_entity_info)

            if node_id_str and node_id_str.startswith("@"):
                # Path for internal registration is the one used at creation
                self.register_entity_internally(node_id_str, final_entity_info, effective_path_for_node_creation)

            # Apply attributes
            # Path prefix for children/named attributes within this node is its own effective path.
            self.apply_attributes(
                py_attributes_obj=py_node_data, # Attributes are from the node itself
                target_entity_info=final_entity_info,
                parent_for_children_entity_info=final_entity_info, 
                path_prefix_for_named_and_children=effective_path_for_node_creation,
                default_type_for_registry_if_named=node_type_str, 
                py_current_context=py_current_context
            )
            return final_entity_info
        return None

    def process_use_view_node(self, py_node_data: CJSONObject, 
                              parent_entity_info: Optional[Dict[str, Any]], 
                              py_current_context: CJSONObject, 
                              named_path_prefix: Optional[str]) -> Optional[Dict[str, Any]]:
        view_id_attr = py_node_data.get("viewId") 
        if not view_id_attr or not isinstance(view_id_attr, str): # Should not start with '@'
            logger.error("'use-view' node is missing 'viewId' string attribute. Skipping.")
            return None

        component_root_data = self.get_component_definition(view_id_attr)
        if component_root_data is None:
            logger.error(f"Component definition for '{view_id_attr}' not found. Skipping 'use-view'.")
            return None

        use_view_instance_id_str = self.get_node_id(py_node_data) # ID of the use-view node itself, e.g. "@my_button_instance"
        instance_path_segment = named_path_prefix
        
        if use_view_instance_id_str: # If the use-view node has an ID like "@my_instance"
            segment = use_view_instance_id_str[1:] # remove '@'
            if instance_path_segment:
                instance_path_segment = f"{instance_path_segment}.{segment}"
            else:
                instance_path_segment = segment
        else: # No ID on use-view, use component name to make path segment unique
            if instance_path_segment:
                instance_path_segment = f"{instance_path_segment}.{view_id_attr}_instance"
            else:
                instance_path_segment = f"{view_id_attr}_instance"
        
        # The component's root node data might be the whole CJSONObject stored, or under a "definition" key.
        # Assuming it's the whole CJSONObject for now.
        instantiated_component_root_info = self.process_node(
            component_root_data, # This is a CJSONObject
            parent_entity_info,  # Parented to the use-view's parent
            py_current_context,  # Context is from the use-view node's scope
            instance_path_segment # Path for entities *within* this instance
        )

        if instantiated_component_root_info:
            # If the use-view node itself had an ID, register the instantiated root with this ID
            if use_view_instance_id_str:
                 self.register_entity_internally(use_view_instance_id_str, instantiated_component_root_info, instance_path_segment)

            do_block_data = py_node_data.get("do")
            if do_block_data and isinstance(do_block_data, CJSONObject):
                # Attributes in "do" apply to the instantiated component's root.
                # Path prefix for "named" items inside "do" is the instance's path.
                self.apply_attributes(
                    py_attributes_obj=do_block_data,
                    target_entity_info=instantiated_component_root_info,
                    parent_for_children_entity_info=None, 
                    path_prefix_for_named_and_children=instance_path_segment, 
                    default_type_for_registry_if_named=self.get_node_type(component_root_data), 
                    py_current_context=py_current_context
                )
        return instantiated_component_root_info


    def process_context_node(self, py_node_data: CJSONObject, 
                             parent_entity_info: Optional[Dict[str, Any]], 
                             py_current_context: CJSONObject, 
                             named_path_prefix: Optional[str]) -> Optional[Dict[str, Any]]:
        values_data = py_node_data.get("values") # Should be CJSONObject
        for_data = py_node_data.get("for")       # Should be CJSONObject

        if not isinstance(values_data, CJSONObject) or not isinstance(for_data, CJSONObject):
            logger.error("'context' node is missing 'values' or 'for' CJSONObjects. Skipping.")
            return None
        
        # Create new context for the 'for' block
        new_context_for_block = CJSONObject([]) # Start fresh or clone?
        # For now, overlay: parent context + values_data
        # A more robust merge might be needed.
        temp_context_dict = {}
        for k,v in py_current_context.items(): temp_context_dict[k] = v
        
        # Resolve values in `values_data` using the `py_current_context` before adding them
        for key, value_spec in values_data.items():
            # Pass parent_entity_info as context_entity_info for resolving values like LV_PCT
            resolved_value = self.format_value(value_spec, "any", parent_entity_info, py_current_context)
            temp_context_dict[key] = resolved_value
        
        new_context_for_block = CJSONObject(list(temp_context_dict.items()))
        
        # Process the 'for' block with the new context.
        # The parent and named_path_prefix are inherited directly.
        # A context node itself typically doesn't create a new visual entity to be returned.
        # It processes its content ("for" block) within a new context.
        # The result of process_node on "for_data" will be returned if needed, but often it's None or not used.
        return self.process_node(for_data, parent_entity_info, new_context_for_block, named_path_prefix)


    def apply_attributes(self, py_attributes_obj: CJSONObject, 
                         target_entity_info: Dict[str, Any], 
                         parent_for_children_entity_info: Optional[Dict[str, Any]],
                         path_prefix_for_named_and_children: Optional[str],
                         default_type_for_registry_if_named: str,
                         py_current_context: CJSONObject) -> None:

        effective_path_for_children = path_prefix_for_named_and_children

        for attr_name, attr_value_py in py_attributes_obj.items(): # attr_value_py is Python data
            if attr_name in ("type", "id", "context"): # Structural, already handled
                continue

            # Grid layout handling (cols/rows)
            # Check if target_entity_info indicates it's a grid container (e.g. its original_json_type was "grid")
            is_grid_container = target_entity_info.get("original_json_type") == "grid" # Example check
            if attr_name == "cols" and is_grid_container:
                rows_data_py = py_attributes_obj.get("rows") # py_attributes_obj is CJSONObject
                self.handle_grid_layout(target_entity_info, attr_value_py, rows_data_py, py_current_context)
                # Avoid processing "rows" again if "cols" was present and handled both
                # CJSONObject iteration order is preserved, so if "cols" is first, this is fine.
                # If "rows" is first, handle_grid_layout would be called again for "cols" if not careful.
                # A robust solution might involve collecting both and calling once.
                # For now, rely on order or specific check in handle_grid_layout.
                # Let's assume handle_grid_layout is smart enough or we ensure it's called once.
                # To be safe, if "cols" is processed, and "rows" also exists, skip "rows" later.
                # This is tricky. A better way is for the caller of apply_attributes to handle grid specially for "grid" type.
                # For now: if "cols" is present, it handles both. If only "rows", it handles "rows".
                # This means if "rows" appears first, it's processed. If "cols" appears later, it's processed again.
                # This part of the logic might need refinement for "grid" type nodes.
                # A common pattern is to extract grid_col, grid_row in process_node for "grid" type
                # and call handle_grid_layout there, then skip here.
                # For now, this simplified logic might call handle_grid_layout twice if both present.
                continue
            if attr_name == "rows" and is_grid_container and not py_attributes_obj.get("cols"):
                 # Only handle "rows" if "cols" was not present (to avoid double processing by above)
                 self.handle_grid_layout(target_entity_info, None, attr_value_py, py_current_context)
                 continue

            if attr_name == "named":
                if isinstance(attr_value_py, str):
                    # type_for_registry_str is derived from target_entity_info, e.g. "lv_button_t"
                    # This requires target_entity_info to contain a C-like type string.
                    # Let's assume get_type_information provides this.
                    full_type_info = self.get_type_information(target_entity_info)
                    c_type_str = full_type_info.get('c_type_str', default_type_for_registry_if_named).replace("*","").strip()

                    new_path_segment = self.handle_named_attribute(
                        attr_value_py, target_entity_info,
                        path_prefix_for_named_and_children,
                        c_type_str 
                    )
                    if new_path_segment: 
                        effective_path_for_children = new_path_segment
                else:
                    logger.warning(f"'named' attribute value is not a string: {attr_value_py}. Skipping.")
                continue

            if attr_name == "children":
                if isinstance(attr_value_py, list) and parent_for_children_entity_info is not None:
                    self.handle_children_attribute(
                        attr_value_py, parent_for_children_entity_info,
                        effective_path_for_children, # Use the potentially updated path
                        py_current_context
                    )
                elif parent_for_children_entity_info is None:
                    logger.warning(f"'children' attribute found but no parent_for_children_entity_info for {target_entity_info.get('c_var_name', 'unknown_target')}. Skipping children.")
                else:
                    logger.warning(f"'children' attribute value is not a list: {attr_value_py}. Skipping.")
                continue

            if attr_name == "with":
                if isinstance(attr_value_py, CJSONObject):
                    self.handle_with_attribute(attr_value_py, target_entity_info, effective_path_for_children, py_current_context)
                else:
                    logger.warning(f"'with' attribute value is not a CJSONObject: {attr_value_py}. Skipping.")
                continue
            
            # --- Regular setter ---
            # 1. Determine setter function name (e.g., from api_info, heuristics)
            # This logic might be complex and specific to the generator (e.g. C name mangling)
            # For BaseUIGenerator, we'll assume a simple heuristic or that api_info provides it.
            # Example: if target_entity_info['lvgl_type'] is 'button', prop 'width', then 'lv_button_set_width' or 'lv_obj_set_width'.
            # This needs a helper method, possibly abstract or part of concrete class.
            # For now, simplified:
            setter_fn_name = f"set_{attr_name}" # Placeholder - concrete class must resolve this properly
            
            # 2. Ensure py_value_args is a list
            py_value_args_list = attr_value_py if isinstance(attr_value_py, list) else [attr_value_py]
            
            # 3. (Optional) Format arguments if needed (e.g. resolve context vars for each arg)
            # The `format_value` method is designed for individual values. If args themselves are complex,
            # they might need pre-processing here or `call_setter` needs to be smarter.
            # For now, assume `py_value_args_list` contains values that `call_setter`'s `format_value` can handle.
            
            # 4. Call the abstract setter
            self.call_setter(target_entity_info, setter_fn_name, py_value_args_list, py_current_context)
