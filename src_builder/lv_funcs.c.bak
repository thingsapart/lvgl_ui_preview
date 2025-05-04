#include "lv_funcs.h"

#include "emul_lvgl.h"

lv_scale_section_t *lv_scale_add_section(lv_obj_t *obj) {
    cJSON *target_node = get_obj_json((lv_obj_t *)obj);
    if (target_node) {
        cJSON *props_node = get_or_create_object_property_node(target_node, "lv_scale_sections");
        cJSON * sections_node = cJSON_GetObjectItemCaseSensitive(props_node, "sections");
        if (!sections_node) { 
            sections_node = cJSON_AddArrayToObject(props_node, "sections");
        }
        if (sections_node) {
            cJSON_AddItemToArray(sections_node, cJSON_CreateObject());
            return (lv_scale_section_t *) cJSON_GetArrayItem(sections_node, cJSON_GetArraySize(sections_node) - 1);
        }
    }
    return NULL;
}

void lv_scale_section_set_range(lv_scale_section_t *section, int32_t min, int32_t max) {
    cJSON *target_node = (cJSON *) section;
    if (target_node) {
        cJSON *props_node = get_or_create_object_property_node(target_node, "range");
        if (props_node) {
            cJSON_AddNumberToObject(props_node, "min", (double)min);
            cJSON_AddNumberToObject(props_node, "max", (double)max);
        }
    }
}

void lv_scale_set_section_range(lv_obj_t *scale, lv_scale_section_t *section, int32_t min, int32_t max) {
   lv_scale_section_set_range(section, min, max);
}

void lv_scale_section_set_style(lv_scale_section_t *section, lv_part_t part, lv_style_t *section_part_style) {

}

void lv_scale_set_section_style_main(lv_obj_t *scale, lv_scale_section_t *section, const lv_style_t *style)  {

}

void lv_scale_set_section_style_indicator(lv_obj_t *scale, lv_scale_section_t *section, const lv_style_t *style)  {

}

void lv_scale_set_section_style_items(lv_obj_t *scale, lv_scale_section_t *section, const lv_style_t *style)  {

}
