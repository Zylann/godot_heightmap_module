#ifndef HEIGHT_MAP_EDITOR_PLUGIN_H
#define HEIGHT_MAP_EDITOR_PLUGIN_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "height_map.h"
#include "height_map_brush_panel.h"


class HeightMapEditorPlugin : public EditorPlugin {
	GDCLASS(HeightMapEditorPlugin, EditorPlugin)
public:
	HeightMapEditorPlugin(EditorNode *p_editor);
	~HeightMapEditorPlugin();

	virtual bool forward_spatial_input_event(Camera *p_camera, const Ref<InputEvent> &p_event);
	virtual String get_name() const { return "HeightMap"; }
	bool has_main_screen() const { return false; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

private:
	EditorNode *_editor;
	HeightMap * _height_map;
	HeightMapBrushPanel *_brush_panel;
};

#endif // HEIGHT_MAP_EDITOR_PLUGIN_H
