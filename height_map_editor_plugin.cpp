#include <core/os/input.h>
#include <scene/3d/camera.h>
#include <scene/scene_string_names.h>

#include "height_map_editor_plugin.h"

inline Ref<Texture> get_icon(String name) {
	return EditorNode::get_singleton()->get_gui_base()->get_icon(name, "EditorIcons");
}

HeightMapEditorPlugin::HeightMapEditorPlugin(EditorNode *p_editor) {
	_editor = p_editor;
	_mouse_pressed = false;
	_height_map = NULL;

	_brush.set_radius(5);

	_brush_panel = memnew(HeightMapBrushPanel);
	_brush_panel->connect(HeightMapBrushPanel::PARAM_CHANGED, this, "_on_brush_param_changed");
	_brush_panel->init_params(
			_brush.get_radius(),
			_brush.get_opacity(),
			_brush.get_flatten_height());
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
	//mode_icons[HeightMapBrush::MODE_TEXTURE] = get_icon("ImmediateGeometry");

	String mode_tooltip[HeightMapBrush::MODE_COUNT];
	mode_tooltip[HeightMapBrush::MODE_ADD] = TTR("Add");
	mode_tooltip[HeightMapBrush::MODE_SUBTRACT] = TTR("Subtract");
	mode_tooltip[HeightMapBrush::MODE_SMOOTH] = TTR("Smooth");
	mode_tooltip[HeightMapBrush::MODE_FLATTEN] = TTR("Flatten");
	//mode_tooltip[HeightMapBrush::MODE_TEXTURE] = TTR("Texture paint");
	mode_tooltip[HeightMapBrush::MODE_COLOR] = TTR("Color paint");

	_toolbar->add_child(memnew(VSeparator));

	Ref<ButtonGroup> mode_group_ref = Ref<ButtonGroup>(memnew(ButtonGroup));

	for (int mode = 0; mode < HeightMapBrush::MODE_COUNT; ++mode) {
		// TODO Add localized tooltips
		// TODO Fix ButtonArray look so that we can use icons that have a visible pressed state!
		ToolButton *button = memnew(ToolButton);
		button->set_icon(mode_icons[mode]);
		button->set_text(mode_tooltip[mode]);
		button->set_tooltip(mode_tooltip[mode]);
		button->set_toggle_mode(true);
		button->set_button_group(mode_group_ref);

		if(mode == _brush.get_mode()) {
			button->set_pressed(true);
		}

		button->connect("pressed", this, "_on_mode_selected", varray(mode));
		_toolbar->add_child(button);
	}

	get_resource_previewer()->add_preview_generator(Ref<EditorResourcePreviewGenerator>(memnew(HeightMapPreviewGenerator())));
}

HeightMapEditorPlugin::~HeightMapEditorPlugin() {
}

bool HeightMapEditorPlugin::forward_spatial_gui_input(Camera *p_camera, const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND_V(_height_map == NULL, false);

	_height_map->_manual_viewer_pos = p_camera->get_global_transform().origin;

	bool captured_event = false;

	Ref<InputEventMouseButton> mb_ref = p_event;
	Ref<InputEventMouseMotion> mm_ref = p_event;

	if (mb_ref.is_valid()) {
		InputEventMouseButton &mb = **mb_ref;

		if (mb.get_button_index() == BUTTON_LEFT || mb.get_button_index() == BUTTON_RIGHT) {
			if (mb.is_pressed() == false)
				_mouse_pressed = false;

			// Need to check modifiers before capturing the event because they are used in navigation schemes
			if (mb.get_control() == false && mb.get_alt() == false && mb.get_button_index() == BUTTON_LEFT) {
				if (mb.is_pressed())
					_mouse_pressed = true;

				captured_event = true;

				if(_mouse_pressed == false) {
					// Just finished painting

					ERR_FAIL_COND_V(_height_map->get_data().is_null(), captured_event);
					HeightMapData *heightmap_data = *_height_map->get_data();

					HeightMapBrush::UndoData ur_data = _brush.pop_undo_redo_data(*heightmap_data);

					Dictionary undo_data;
					undo_data["chunk_positions"] = ur_data.chunk_positions;
					undo_data["data"] = ur_data.undo;
					undo_data["channel"] = ur_data.channel;

					Dictionary redo_data;
					redo_data["chunk_positions"] = ur_data.chunk_positions;
					redo_data["data"] = ur_data.redo;
					redo_data["channel"] = ur_data.channel;

					UndoRedo &ur = *EditorNode::get_singleton()->get_undo_redo();

					String action_name;
					switch(ur_data.channel) {
						case HeightMapData::CHANNEL_COLOR:
							action_name = TTR("Modify HeightMapData Color");
							break;
						case HeightMapData::CHANNEL_HEIGHT:
							action_name = TTR("Modify HeightMapData Height");
							break;
						default:
							action_name = TTR("Modify HeightMapData");
							break;
					}

					ur.create_action(action_name);
					ur.add_do_method(heightmap_data, "_apply_undo", redo_data);
					ur.add_undo_method(heightmap_data, "_apply_undo", undo_data);

					// Small hack here:
					// commit_actions executes the do method, however terrain modifications are heavy ones,
					// so we don't really want to re-run an update in every chunk that was modified during painting.
					// The data is already in its final state, so we just prevent the resource from applying changes here.
					heightmap_data->_disable_apply_undo = true;
					ur.commit_action();
					heightmap_data->_disable_apply_undo = false;
				}
			}
		}

	} else if (mm_ref.is_valid() && _mouse_pressed) {
		InputEventMouseMotion &mm = **mm_ref;
		Input &input = *Input::get_singleton();

		if (input.is_mouse_button_pressed(BUTTON_LEFT)) {
			paint(*p_camera, mm.get_position());
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
		_brush.paint(height_map, hit_pos_in_cells, override_mode);
	}
}

void HeightMapEditorPlugin::edit(Object *p_object) {

	//printf("Edit %i\n", p_object);
	HeightMap *node = p_object ? p_object->cast_to<HeightMap>() : NULL;

	if(_height_map) {
		_height_map->disconnect(SceneStringNames::get_singleton()->tree_exited, this, "_height_map_exited_scene");
	}

	_height_map = node;

	if(_height_map) {
		_height_map->connect(SceneStringNames::get_singleton()->tree_exited, this, "_height_map_exited_scene");
	}
}

void HeightMapEditorPlugin::_height_map_exited_scene() {
	//print_line("HeightMap exited scene");
	edit(NULL);
}

bool HeightMapEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("HeightMap");
}

void HeightMapEditorPlugin::make_visible(bool p_visible) {
	_brush_panel->set_visible(p_visible);
	_toolbar->set_visible(p_visible);
}

void HeightMapEditorPlugin::_mode_selected(int mode) {
	ERR_FAIL_COND(mode < 0 || mode >= HeightMapBrush::MODE_COUNT);
	_brush.set_mode((HeightMapBrush::Mode)mode);
}

void HeightMapEditorPlugin::_brush_param_changed(Variant value, int param) {

	switch (param) {
		case HeightMapBrushPanel::BRUSH_SIZE:
			_brush.set_radius(value);
			break;

		case HeightMapBrushPanel::BRUSH_OPACITY:
			_brush.set_opacity(value);
			break;

		case HeightMapBrushPanel::BRUSH_HEIGHT:
			_brush.set_flatten_height(value);
			break;

		default:
			ERR_PRINT("Unknown parameter");
			break;
	}
}

void HeightMapEditorPlugin::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_on_mode_selected", "mode"), &HeightMapEditorPlugin::_mode_selected);
	ClassDB::bind_method(D_METHOD("_on_brush_param_changed", "value", "param"), &HeightMapEditorPlugin::_brush_param_changed);
	ClassDB::bind_method(D_METHOD("_height_map_exited_scene"), &HeightMapEditorPlugin::_height_map_exited_scene);
}

//------------------------------------------
// Preview generator

bool HeightMapPreviewGenerator::handles(const String &p_type) const {
	return p_type == "HeightMapData";
}

Ref<Texture> HeightMapPreviewGenerator::generate(const Ref<Resource> &p_from) {

	Ref<HeightMapData> data_ref = p_from;
	ERR_FAIL_COND_V(data_ref.is_null(), Ref<Texture>());
	HeightMapData &data = **data_ref;

	if(data.get_resolution() == 0)
		return Ref<Texture>();

	Ref<Image> normals_ref = data.get_image(HeightMapData::CHANNEL_NORMAL);
	ERR_FAIL_COND_V(normals_ref.is_null(), Ref<Texture>());
	Image &normals = **normals_ref;

	int thumbnail_size = EditorSettings::get_singleton()->get("filesystem/file_dialog/thumbnail_size");
	thumbnail_size *= EDSCALE;
	Ref<Image> img_ref;
	img_ref.instance();
	Image &im = **img_ref;

	im.create(thumbnail_size, thumbnail_size, 0, Image::FORMAT_RGBA8);

	im.lock();
	normals.lock();

	Vector3 light_dir = Vector3(-1, -0.5, -1).normalized();

	for(int y = 0; y < im.get_height(); ++y) {
		for(int x = 0; x < im.get_width(); ++x) {

			float fx = static_cast<float>(x) / im.get_width();
			float fy = static_cast<float>(im.get_height() - y - 1) / im.get_height();
			Point2i mpos(fx * normals.get_width(),
						 fy * normals.get_height());

			Vector3 n = HeightMapData::decode_normal(normals.get_pixel(mpos.x, mpos.y));

			float ndot = -n.dot(light_dir);
			float gs = CLAMP(0.5*ndot+0.5, 0.0, 1.0);
			Color col(gs, gs, gs, 1.0);

			im.set_pixel(x, y, col);
		}
	}

	im.unlock();
	normals.unlock();

	Ref<ImageTexture> ptex = Ref<ImageTexture>(memnew(ImageTexture));

	ptex->create_from_image(img_ref, 0);
	return ptex;
}



