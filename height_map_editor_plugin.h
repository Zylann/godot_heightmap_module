#ifndef HEIGHT_MAP_EDITOR_PLUGIN_H
#define HEIGHT_MAP_EDITOR_PLUGIN_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "height_map.h"
#include "height_map_brush.h"
#include "height_map_brush_panel.h"

class HeightMapEditorPlugin : public EditorPlugin {
	GDCLASS(HeightMapEditorPlugin, EditorPlugin)
public:
	HeightMapEditorPlugin(EditorNode *p_editor);
	~HeightMapEditorPlugin();

	virtual bool forward_spatial_gui_input(Camera *p_camera, const Ref<InputEvent> &p_event);
	virtual String get_name() const { return "HeightMap"; }
	bool has_main_screen() const { return false; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

protected:
	static void _bind_methods();

private:
	void _mode_selected(HeightMapBrush::Mode mode);
	void _brush_param_changed(Variant value, HeightMapBrushEditor::Param param);
	void _on_texture_index_selected(int index);
	void _height_map_exited_scene();
	void _menu_item_selected(int id);
	void _import_file_selected(String p_path);

	void _import_raw_file_selected(String path);
	void _import_raw_file();

	void paint(Camera &camera, Vector2 screen_pos, int override_mode = -1);

private:
	enum MenuItems {
		MENU_IMPORT_RAW = 0
	};

	EditorNode *_editor;
	HeightMap *_height_map;
	HeightMapEditorPanel *_panel;
	HBoxContainer *_toolbar;
	HeightMapBrush _brush;

	FileDialog *_import_dialog;
	String _import_file_path;
	ConfirmationDialog *_import_confirmation_dialog;
	AcceptDialog *_accept_dialog;

	bool _mouse_pressed;
};

class HeightMapPreviewGenerator : public EditorResourcePreviewGenerator {
	GDCLASS(HeightMapPreviewGenerator, EditorResourcePreviewGenerator)
public:
	bool handles(const String &p_type) const;
	Ref<Texture> generate(const Ref<Resource> &p_from);
};

#endif // HEIGHT_MAP_EDITOR_PLUGIN_H
