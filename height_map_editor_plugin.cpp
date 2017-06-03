#include "height_map_editor_plugin.h"

HeightMapEditorPlugin::HeightMapEditorPlugin(EditorNode *p_editor) {
	_editor = p_editor;

	_brush_panel = memnew(HeightMapBrushPanel);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_BOTTOM, _brush_panel);
	_brush_panel->hide();
}

HeightMapEditorPlugin::~HeightMapEditorPlugin() {
}

bool HeightMapEditorPlugin::forward_spatial_input_event(Camera *p_camera, const Ref<InputEvent> &p_event) {
	return false;
}

void HeightMapEditorPlugin::edit(Object *p_object) {
	_height_map = p_object ? p_object->cast_to<HeightMap>() : NULL;
}

bool HeightMapEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("HeightMap");
}

void HeightMapEditorPlugin::make_visible(bool p_visible) {
	_brush_panel->set_visible(p_visible);
}

