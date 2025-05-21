import json
import abc
import logging
from collections import OrderedDict # For potential CJSONObject-like behavior

# Assuming api_parser is in the 'gen' directory and accessible
# from .. import api_parser # If api_parser.py is one level up
# For now, we assume api_info is passed in, so direct import might not be needed here.
# from .type_utils import TypeResolver # Example, if needed directly

logger = logging.getLogger(__name__)

class BaseUIProcessor(abc.ABC):
    def __init__(self, api_info, lv_def_json_path, str_vals_json_path):
        self.api_info = api_info
        self.lv_def = {}
        self.str_vals = {}

        try:
            with open(lv_def_json_path, 'r') as f:
                self.lv_def = json.load(f)
        except FileNotFoundError:
            logger.error(f"LVGL definitions file not found: {lv_def_json_path}")
            # Depending on strictness, could raise error or continue with empty
        except json.JSONDecodeError:
            logger.error(f"Failed to parse LVGL definitions JSON: {lv_def_json_path}")

        try:
            with open(str_vals_json_path, 'r') as f:
                self.str_vals = json.load(f)
        except FileNotFoundError:
            logger.warning(f"String values file not found: {str_vals_json_path}. Proceeding without it.")
        except json.JSONDecodeError:
            logger.error(f"Failed to parse string values JSON: {str_vals_json_path}")

        self.component_definitions = {}
        self.current_context = {} # Stores context variables, e.g., {"primary_color": "0xFF0000"}
        self.current_path_prefix = "" # e.g., "main_screen.user_panel."

    # --- Abstract Core Processing Methods (to be implemented by subclasses) ---

    @abc.abstractmethod
    def _handle_widget_creation(self, json_node, parent_ref, context_at_creation):
        """
        Handles the creation of a widget based on json_node.
        Returns a tuple: (entity_ref, entity_lvgl_type_str, entity_original_json_type)
        """
        pass

    @abc.abstractmethod
    def _handle_style_creation(self, json_node, context_at_creation):
        """
        Handles the creation of a style object.
        Returns a reference to the created style entity.
        """
        pass

    @abc.abstractmethod
    def _apply_property(self, entity_ref, entity_lvgl_type_str, prop_name, prop_value_json, current_context):
        """
        Applies a single property to an entity.
        """
        pass

    @abc.abstractmethod
    def _resolve_value(self, json_value_node, expected_type_hint, current_context, target_entity_ref_for_pct=None):
        """
        Resolves a JSON value node to a concrete value or representation suitable for the target language.
        expected_type_hint can guide resolution (e.g., "color", "font", "lv_coord_t").
        target_entity_ref_for_pct is provided if the value might be LV_PCT and needs the target entity.
        """
        pass

    @abc.abstractmethod
    def _register_entity_by_id(self, entity_ref, id_str, entity_type_str_for_registry, lvgl_type_str):
        """
        Registers an entity with a given ID.
        entity_type_str_for_registry could be "widget", "style", "component_instance".
        """
        pass

    @abc.abstractmethod
    def _register_entity_by_named_attr(self, entity_ref, named_str, entity_type_str_for_registry, lvgl_type_str, current_path_prefix):
        """
        Registers an entity with a 'named' attribute, potentially prefixed by current_path_prefix.
        """
        pass

    @abc.abstractmethod
    def _get_entity_ref_from_id(self, id_str, expected_type_hint=None):
        """
        Retrieves an entity reference by its ID.
        expected_type_hint can help verify the type of the retrieved entity.
        """
        pass

    @abc.abstractmethod
    def _perform_do_block_on_entity(self, entity_ref, entity_lvgl_type_str, entity_original_json_type, do_block_json, context_for_do_block, path_prefix_for_do_block_attrs, type_for_named_in_do_block):
        """
        Executes a 'do' block on a given entity. This involves applying properties or actions
        defined within the 'do' block to the entity.
        """
        pass

    # --- Concrete JSON Interpretation Logic ---

    def load_ui_spec(self, ui_spec_json_path):
        """Loads the main UI JSON file."""
        try:
            with open(ui_spec_json_path, 'r') as f:
                # Consider object_pairs_hook=OrderedDict if order matters significantly
                # For now, standard load is fine.
                ui_spec = json.load(f)
                return ui_spec
        except FileNotFoundError:
            logger.error(f"UI specification file not found: {ui_spec_json_path}")
            return None
        except json.JSONDecodeError as e:
            logger.error(f"Failed to parse UI specification JSON: {ui_spec_json_path} - {e}")
            return None

    def process_ui_spec(self, root_json_node, initial_parent_ref=None, initial_context=None, initial_path_prefix=""):
        """
        Main entry point for processing a loaded UI spec.
        Iterates if root_json_node is a list, or processes directly if it's an object.
        """
        if initial_context is None:
            initial_context = {}
        
        self.current_context = initial_context.copy() # Start with a fresh copy for this processing run
        self.current_path_prefix = initial_path_prefix

        if isinstance(root_json_node, list):
            results = []
            for node in root_json_node:
                result = self.process_node(node, initial_parent_ref, self.current_context.copy(), self.current_path_prefix)
                if result:
                    results.append(result)
            return results
        elif isinstance(root_json_node, dict):
            return self.process_node(root_json_node, initial_parent_ref, self.current_context.copy(), self.current_path_prefix)
        else:
            logger.error(f"Root of UI specification must be an object or a list, got {type(root_json_node)}")
            return None

    def process_node(self, json_node, parent_ref, current_context, current_path_prefix):
        """
        Recursive workhorse for processing a single JSON node in the UI spec.
        """
        if not isinstance(json_node, dict):
            logger.warning(f"Expected a JSON object for node, got {type(json_node)}. Skipping.")
            return None

        node_type = json_node.get("type", "obj") # Default to "obj" if type is not specified
        node_id = json_node.get("id")
        
        # Handle local context updates. This context applies to the current node and its children.
        effective_context = current_context.copy()
        if "context" in json_node:
            local_context_updates = json_node.get("context")
            if isinstance(local_context_updates, dict):
                effective_context.update(local_context_updates)
            else:
                logger.warning(f"Node 'context' property expected a dictionary, got {type(local_context_updates)} for ID '{node_id}'. Ignoring.")

        # Update path prefix if this node has an ID that should contribute to it
        path_for_node_and_children = current_path_prefix
        if node_id and not node_id.startswith("@"): # IDs starting with @ are component defs, not instance paths
             path_for_node_and_children += node_id + "."

        entity_ref = None
        entity_lvgl_type = None
        entity_original_json_type = node_type

        if node_type == "component":
            self._handle_component_definition(json_node)
            return None # Component definition doesn't create an immediate entity
        
        elif node_type == "use-view":
            return self._handle_use_view_instantiation(json_node, parent_ref, effective_context, current_path_prefix)

        elif node_type == "context": # A wrapper type to provide context to its "for" item
            return self._handle_context_block(json_node, parent_ref, current_path_prefix, effective_context)
            
        elif node_type == "style":
            entity_ref = self._handle_style_creation(json_node, effective_context)
            entity_lvgl_type = "lv_style_t" # Assuming styles are generally lv_style_t
            # Styles might not have an "original_json_type" in the same way widgets do,
            # but we pass node_type for consistency.
        else: # Presumed widget type (e.g., "button", "obj", "grid", etc.)
            try:
                created_entity_info = self._handle_widget_creation(json_node, parent_ref, effective_context)
                if created_entity_info:
                    entity_ref, entity_lvgl_type, entity_original_json_type = created_entity_info
                else:
                    logger.warning(f"Widget creation failed or returned None for node type '{node_type}' with ID '{node_id}'.")
                    return None
            except Exception as e:
                logger.error(f"Error during widget creation for type '{node_type}' with ID '{node_id}': {e}", exc_info=True)
                return None

        if entity_ref is None:
            # If style or widget creation didn't yield an entity, we can't proceed with it.
            # _handle_style_creation or _handle_widget_creation should log issues.
            return None

        # Register entity if "id" is present (common for widgets and styles)
        if node_id:
            # Use a generic type for registration unless more specific is needed.
            # For widgets, entity_original_json_type (like "button") is good.
            # For styles, "style" is appropriate.
            registry_type_str = entity_original_json_type if node_type != "style" else "style"
            if node_id.startswith("@"):
                logger.warning(f"IDs starting with '@' are reserved for component definitions. Found on '{node_id}' of type '{node_type}'. It will not be registered as an instance ID.")
            else:
                self._register_entity_by_id(entity_ref, node_id, registry_type_str, entity_lvgl_type)

        # Apply other attributes (properties, children, named, with)
        # Pass path_for_node_and_children for 'named' attributes within this entity and for its children's path prefix
        if entity_ref: # Ensure entity was successfully created
            self._apply_attributes(entity_ref, entity_lvgl_type, entity_original_json_type, json_node, effective_context, path_for_node_and_children)
        
        return entity_ref


    def _handle_component_definition(self, json_node):
        component_id = json_node.get("id")
        component_root = json_node.get("root")
        if not component_id or not component_root:
            logger.error("Component definition requires 'id' and 'root' properties.")
            return

        if not component_id.startswith("@"):
            logger.error(f"Component definition ID '{component_id}' must start with '@'.")
            return
            
        clean_id = component_id[1:] # Remove '@'
        if clean_id in self.component_definitions:
            logger.warning(f"Redefining component '{clean_id}'. Previous definition will be overwritten.")
        
        self.component_definitions[clean_id] = component_root
        logger.info(f"Component '{clean_id}' defined.")


    def _apply_attributes(self, entity_ref, entity_lvgl_type_str, entity_original_json_type,
                          attributes_json_node, current_context, path_prefix_for_named_and_children):
        """
        Applies attributes from attributes_json_node to the entity_ref.
        path_prefix_for_named_and_children is the prefix for any 'named' attributes
        defined directly in this node and also serves as the base for children's paths.
        """
        for key, value in attributes_json_node.items():
            if key in ("type", "id", "context"): # Handled by process_node or specific handlers
                continue

            if key == "named":
                if isinstance(value, str):
                    # entity_original_json_type is like "button", "style", etc.
                    self._register_entity_by_named_attr(entity_ref, value, entity_original_json_type, entity_lvgl_type_str, path_prefix_for_named_and_children)
                else:
                    logger.warning(f"'named' attribute expects a string value, got {type(value)}. Skipping.")
                continue

            if key == "children":
                if isinstance(value, list):
                    self._handle_children(value, entity_ref, current_context, path_prefix_for_named_and_children)
                else:
                    logger.warning(f"'children' attribute expects a list, got {type(value)}. Skipping.")
                continue
            
            if key == "with":
                if isinstance(value, dict):
                    self._handle_with_block(value, entity_ref, entity_lvgl_type_str, current_context, path_prefix_for_named_and_children)
                else:
                    logger.warning(f"'with' attribute expects a dictionary (block), got {type(value)}. Skipping.")
                continue

            # If not a special key, assume it's a property to be applied
            try:
                self._apply_property(entity_ref, entity_lvgl_type_str, key, value, current_context)
            except Exception as e:
                logger.error(f"Error applying property '{key}' to entity of type '{entity_lvgl_type_str}': {e}", exc_info=True)


    def _handle_children(self, children_array_json, parent_entity_ref, current_context, path_prefix_for_children):
        """
        Processes an array of child nodes.
        path_prefix_for_children is the prefix that children will inherit and potentially add to.
        """
        if not isinstance(children_array_json, list):
            logger.error(f"Expected a list for children, got {type(children_array_json)}")
            return

        for child_node in children_array_json:
            self.process_node(child_node, parent_entity_ref, current_context.copy(), path_prefix_for_children)
            # Each child gets a copy of the context and the same path prefix from the parent.
            # If a child has an ID, process_node will append it to create its own path_for_node_and_children.

    def _handle_use_view_instantiation(self, use_view_json_node, parent_ref, current_context, current_path_prefix):
        component_id_ref = use_view_json_node.get("component")
        if not component_id_ref or not component_id_ref.startswith("@"):
            logger.error(f"'use-view' must have a 'component' property starting with '@', got: {component_id_ref}")
            return None

        clean_component_id = component_id_ref[1:]
        component_root_template = self.component_definitions.get(clean_component_id)
        if not component_root_template:
            logger.error(f"Component definition '@{clean_component_id}' not found for 'use-view'.")
            return None

        # Context for the component instance:
        # 1. Start with the context inherited by the 'use-view' node itself.
        # 2. Override/extend with any 'context' property directly on the 'use-view' node.
        # Note: 'use_view_json_node.get("context")' was already handled to form 'current_context'
        # passed into this function. So 'current_context' is the correct one to use here.
        context_for_component_instance = current_context.copy()
        
        # Path prefix for the component instance:
        # If 'use-view' has an 'id', it forms the base path for elements defined *within* the component.
        instance_id = use_view_json_node.get("id")
        path_prefix_for_component_instance = current_path_prefix
        if instance_id:
            if instance_id.startswith("@"):
                 logger.warning(f"ID for 'use-view' ('{instance_id}') should not start with '@'. This prefix is for component definitions.")
            else:
                path_prefix_for_component_instance += instance_id + "."
        
        # Deepcopy the template to prevent modification of the stored component definition
        component_instance_json = json.loads(json.dumps(component_root_template)) # Simple deepcopy

        # Process the root of the component instance
        # The parent_ref for the component's root is the same parent_ref as the 'use-view' node.
        instantiated_component_root_ref = self.process_node(
            component_instance_json, 
            parent_ref, 
            context_for_component_instance, 
            path_prefix_for_component_instance
        )

        if not instantiated_component_root_ref:
            logger.error(f"Failed to instantiate root of component '@{clean_component_id}' for 'use-view' (ID: {instance_id}).")
            return None
            
        # If the 'use-view' node has an 'id', register the instantiated component root with this ID.
        # This makes the entire component instance identifiable.
        if instance_id and not instance_id.startswith("@"):
            # Assuming entity_lvgl_type and original_json_type can be retrieved or are known for component roots
            # This part might need refinement based on what _handle_widget_creation returns for the component's root.
            # For now, let's assume the component's root processing already handled its own registration if it had an ID.
            # What we need here is to alias the 'use-view' instance_id to the component's root entity.
            # This requires the `_register_entity_by_id` to be flexible or to have a dedicated method.
            # For now, we'll call it with generic types. The subclass needs to handle this.
            # We need the LVGL type of the component's root. This should come from its processing.
            # This is tricky because process_node returns the ref, not its type here.
            # Let's assume for now the component root itself will handle its main ID registration.
            # The 'id' on 'use-view' is more like an alias or an instance name.
            # We might need a specific method in subclasses: _alias_entity_id(existing_ref, new_id_alias)
            # Or _register_entity_by_id can handle if id_str is already known for existing_ref.
             logger.info(f"Instance '{instance_id}' of component '@{clean_component_id}' created. Its root ref is {instantiated_component_root_ref}.")
             # We might need to store this mapping of instance_id to instantiated_component_root_ref if 'do' blocks
             # or other mechanisms need to refer to the 'use-view' instance by its own ID.
             # For now, _register_entity_by_id is called by process_node if the root of the component has an ID.
             # If the use-view node itself has an ID, that ID should refer to the component's root element.
             # The LVGL type of the component root is needed. This is a gap.
             # Let's assume the component's root is always 'lv_obj_t' for now for registration purposes.
             # This is a placeholder, actual type should be determined from the component's root node.
             # This also assumes that process_node for the component's root returns its LVGL type.
             # This part needs careful implementation in derived classes.
             # A quick fix: _get_lvgl_type_for_ref(instantiated_component_root_ref) from subclass.
             # For now, we will rely on the component's root JSON having an "id" if it needs to be directly referenced.
             # The "id" on use-view is more for the "do" block targeting.

        # Handle "do" block on the 'use-view' node, targeting the instantiated component's root
        do_block = use_view_json_node.get("do")
        if do_block and instantiated_component_root_ref:
            if isinstance(do_block, dict):
                # The 'do' block on 'use-view' operates on the component's root.
                # We need the LVGL type and original JSON type of that root.
                # This information is not directly returned by process_node.
                # This is a structural challenge. Subclasses will need a way to get this.
                # For now, we'll pass None and let subclasses figure it out or fetch it.
                # A better way would be for process_node to return (ref, lvgl_type, original_json_type)
                # Or, the component root's properties are already known if it was just created.
                # Let's assume `_perform_do_block_on_entity` can fetch type info from `instantiated_component_root_ref`
                self._perform_do_block_on_entity(
                    instantiated_component_root_ref,
                    None, # Placeholder for LVGL type of component root
                    None, # Placeholder for original JSON type of component root
                    do_block,
                    context_for_component_instance, # Context for resolving values within the 'do' block
                    path_prefix_for_component_instance, # Path prefix for 'named' attributes within the 'do' block
                    "obj" # Default type for 'named' in 'do' block, might need to be smarter
                )
            else:
                logger.warning(f"'do' block on 'use-view' (ID: {instance_id}) must be a dictionary. Got {type(do_block)}")
        
        return instantiated_component_root_ref # Return the reference to the root of the instantiated component.

    def _handle_context_block(self, context_wrapper_json_node, parent_ref, current_path_prefix, inherited_context):
        """
        Handles a 'context' wrapper node.
        Merges 'values' into current context and processes the 'for' node.
        """
        context_values_to_set = context_wrapper_json_node.get("values")
        node_to_process_under_context = context_wrapper_json_node.get("for")

        if not isinstance(context_values_to_set, dict):
            logger.error("'values' in context block must be a dictionary.")
            return None
        if not isinstance(node_to_process_under_context, dict): # Assuming 'for' contains a single node definition
            logger.error("'for' in context block must be a dictionary (a single node).")
            return None

        new_context_for_child = inherited_context.copy()
        new_context_for_child.update(context_values_to_set)

        # The context block itself doesn't create an entity.
        # It influences the context for the node defined in its "for" property.
        # The parent_ref and current_path_prefix are passed through from the context block's parent.
        return self.process_node(node_to_process_under_context, parent_ref, new_context_for_child, current_path_prefix)


    def _handle_with_block(self, with_block_json, current_entity_ref, current_entity_lvgl_type, current_context, path_prefix_for_with_block):
        """
        Handles a 'with' block, resolving a target object and performing a 'do' block on it.
        """
        target_obj_specifier = with_block_json.get("obj")
        do_block_for_target = with_block_json.get("do")

        if not target_obj_specifier:
            logger.error("'with' block requires an 'obj' specifier.")
            return
        if not isinstance(do_block_for_target, dict):
            logger.error("'with' block requires a 'do' dictionary.")
            return
        
        target_entity_ref = None
        target_entity_lvgl_type = None # Need to determine this for _perform_do_block_on_entity
        target_entity_original_json_type = None # Same here

        if isinstance(target_obj_specifier, str):
            # Assume it's an ID string
            target_entity_ref = self._get_entity_ref_from_id(target_obj_specifier)
            if not target_entity_ref:
                logger.error(f"In 'with' block, could not find entity by ID: '{target_obj_specifier}'")
                return
            # How to get type information for this resolved entity? Subclass responsibility.
            # For now, pass None, _perform_do_block_on_entity must be robust or fetch it.

        elif isinstance(target_obj_specifier, dict):
            # Assume it's an inline value definition that needs resolution, e.g., a color or a font
            # This usage of 'with' is less common for 'obj', more for applying properties.
            # Let's assume for now 'obj' in 'with' always refers to an existing entity by ID.
            # If it were to resolve a value, what would 'do' apply to?
            # This implies 'obj' should primarily be for entity lookups.
            # If `_resolve_value` can return an entity reference (e.g. for a font or color *object*),
            # then this could be valid. For now, focus on ID-based lookup.
            logger.warning(f"'with' block 'obj' specifier as a dictionary is not fully supported for entity lookup yet. Assuming ID string. Got: {target_obj_specifier}")
            return
        else:
            logger.error(f"'with' block 'obj' specifier must be an ID string. Got {type(target_obj_specifier)}")
            return

        if target_entity_ref:
            # The 'do' block operates on the resolved target_entity_ref.
            # The context for the 'do' block is the current_context of the 'with' block itself.
            # Path prefix for 'named' attributes inside this 'do' block should probably be unique
            # or based on the 'with' block's context, not the target object's original path.
            # Using path_prefix_for_with_block which is inherited by the 'with' block.
            self._perform_do_block_on_entity(
                target_entity_ref,
                None, # Placeholder for LVGL type of target entity
                None, # Placeholder for original JSON type of target entity
                do_block_for_target,
                current_context,
                path_prefix_for_with_block, # Path prefix for 'named' in this do block
                "obj" # Default type for 'named' within this 'do' block
            )
        else:
            logger.error(f"Target object for 'with' block not resolved: {target_obj_specifier}")

# Example usage (conceptual, would be in a main script)
# if __name__ == '__main__':
#     # This is illustrative. Subclasses would be instantiated and used.
#     # Mock api_info, paths
#     mock_api_info = {"functions": [], "enums": [], "enum_members": {}} # Simplified
#     lv_def_path = "path/to/lv_def.json"
#     str_vals_path = "path/to/str_vals.json"
#     ui_spec_path = "path/to/ui_spec.json"

#     # class MyConcreteProcessor(BaseUIProcessor):
#     #     # ... implementations of abstract methods ...
#     #     pass
#     # processor = MyConcreteProcessor(mock_api_info, lv_def_path, str_vals_path)
#     # ui_spec_data = processor.load_ui_spec(ui_spec_path)
#     # if ui_spec_data:
#     #     processed_elements = processor.process_ui_spec(ui_spec_data)
#     #     # ... do something with processed_elements
pass
