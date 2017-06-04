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
	void on_mode_selected(int mode);
	void paint(Camera &camera, Vector2 screen_pos, int override_mode=-1);

private:
	EditorNode *_editor;
	HeightMap * _height_map;
	HeightMapBrushPanel *_brush_panel;
	HBoxContainer *_toolbar;
	HeightMapBrush _brush;

	bool _mouse_pressed;
};

#endif // HEIGHT_MAP_EDITOR_PLUGIN_H
