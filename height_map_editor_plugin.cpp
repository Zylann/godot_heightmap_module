#include <core/os/input.h>
#include <scene/3d/camera.h>
#include <scene/gui/button_array.h>

#include "height_map_editor_plugin.h"

inline Ref<Texture> get_icon(String name) {
	return EditorNode::get_singleton()->get_gui_base()->get_icon(name, "EditorIcons");
}

HeightMapEditorPlugin::HeightMapEditorPlugin(EditorNode *p_editor) {
	_editor = p_editor;
	_mouse_pressed = false;

	_brush.set_radius(5);

	_brush_panel = memnew(HeightMapBrushPanel);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_BOTTOM, _brush_panel);
	_brush_panel->hide();

	_toolbar = memnew(HBoxContainer);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, _toolbar);
	_toolbar->hide();

	// TODO Need proper icons, these are borrowed from existing ones
	Ref<Texture> mode_icons[HeightMapBrush::MODE_COUNT];
	mode_icons[HeightMapBrush::MODE_ADD] = get_icon("AnimSet");
	mode_icons[HeightMapBrush::MODE_SUBTRACT] = get_icon("AnimGet");
	mode_icons[HeightMapBrush::MODE_SMOOTH] = get_icon("SphereShape");
	mode_icons[HeightMapBrush::MODE_FLATTEN] = get_icon("HSize");
	mode_icons[HeightMapBrush::MODE_TEXTURE] = get_icon("ImmediateGeometry");

	String mode_tooltip[HeightMapBrush::MODE_COUNT];
	mode_tooltip[HeightMapBrush::MODE_ADD] = TTR("Add");
	mode_tooltip[HeightMapBrush::MODE_SUBTRACT] = TTR("Subtract");
	mode_tooltip[HeightMapBrush::MODE_SMOOTH] = TTR("Smooth");
	mode_tooltip[HeightMapBrush::MODE_FLATTEN] = TTR("Flatten");
	mode_tooltip[HeightMapBrush::MODE_TEXTURE] = TTR("Texture paint");

	ButtonArray *mode_selector = memnew(ButtonArray);
	for (int mode = 0; mode < HeightMapBrush::MODE_COUNT; ++mode) {
		// TODO Add localized tooltips
		mode_selector->add_icon_button(mode_icons[mode], "", mode_tooltip[mode]);
	}

	mode_selector->connect("button_selected", this, "_on_mode_selected");

	_toolbar->add_child(mode_selector);
}

HeightMapEditorPlugin::~HeightMapEditorPlugin() {
}

bool HeightMapEditorPlugin::forward_spatial_gui_input(Camera *p_camera, const Ref<InputEvent> &p_event) {

	bool captured_event = false;

	Ref<InputEventMouseButton> mb_ref = p_event;
	Ref<InputEventMouseMotion> mm_ref = p_event;

	if (mb_ref.is_valid()) {
		InputEventMouseButton &mb = **mb_ref;

		if (mb.get_button_index() == BUTTON_LEFT || mb.get_button_index() == BUTTON_RIGHT) {
			if (mb.is_pressed() == false)
				_mouse_pressed = false;

			// Need to check modifiers before capturing the event because they are used in navigation schemes
			if (mb.get_control() == false && mb.get_alt() == false) {
				if (mb.is_pressed())
					_mouse_pressed = true;
				captured_event = true;

				// TODO Prepare undo/redo
			}
		}

	} else if (mm_ref.is_valid() && _mouse_pressed) {
		InputEventMouseMotion &mm = **mm_ref;
		Input &input = *Input::get_singleton();

		if (_brush.get_mode() == HeightMapBrush::MODE_ADD && input.is_mouse_button_pressed(BUTTON_RIGHT)) {
			paint(*p_camera, mm.get_pos(), HeightMapBrush::MODE_SUBTRACT);
			captured_event = true;

		} else if (input.is_mouse_button_pressed(BUTTON_LEFT)) {
			paint(*p_camera, mm.get_pos());
			captured_event = true;
		}
	}

	return captured_event;
}

void HeightMapEditorPlugin::paint(Camera &camera, Vector2 screen_pos, int override_mode) {
	ERR_FAIL_COND(_height_map == NULL);

	Vector3 origin = camera.project_ray_origin(screen_pos);
	Vector3 dir = camera.project_ray_normal(screen_pos);

	HeightMap &height_map = *_height_map;

	Point2i hit_pos_in_cells;
	if (height_map.cell_raycast(origin, dir, hit_pos_in_cells)) {
		_brush.paint_world_pos(height_map, hit_pos_in_cells, override_mode);
	}
}

void HeightMapEditorPlugin::edit(Object *p_object) {
	_height_map = p_object ? p_object->cast_to<HeightMap>() : NULL;
}

bool HeightMapEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("HeightMap");
}

void HeightMapEditorPlugin::make_visible(bool p_visible) {
	_brush_panel->set_visible(p_visible);
	_toolbar->set_visible(p_visible);
}

void HeightMapEditorPlugin::on_mode_selected(int mode) {
	ERR_FAIL_COND(mode < 0 || mode >= HeightMapBrush::MODE_COUNT);
	_brush.set_mode((HeightMapBrush::Mode)mode);
}

void HeightMapEditorPlugin::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_on_mode_selected", "mode"), &HeightMapEditorPlugin::on_mode_selected);
}
