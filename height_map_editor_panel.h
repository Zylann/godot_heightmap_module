#ifndef HEIGHT_MAP_BRUSH_PANEL_H
#define HEIGHT_MAP_BRUSH_PANEL_H

#include <scene/gui/label.h>
#include <scene/gui/slider.h>
#include <scene/gui/spin_box.h>
#include <scene/gui/split_container.h>
#include <scene/gui/color_picker.h>

class HeightMapBrushEditor : public Control {
	GDCLASS(HeightMapBrushEditor, Control)
public:

	static const char *SIGNAL_PARAM_CHANGED;

	HeightMapBrushEditor();
	~HeightMapBrushEditor();

	enum Param {
		BRUSH_SIZE = 0,
		BRUSH_OPACITY,
		BRUSH_COLOR,
		BRUSH_HEIGHT
	};

	void init_params(int size, float opacity, float height, Color color);

protected:
	static void _bind_methods();

private:
	void on_param_changed(Variant value, int param);

private:
	Label *_size_label;
	Slider *_size_slider;

	Label *_opacity_label;
	Slider *_opacity_slider;

	SpinBox *_height_edit;
	ColorPickerButton *_color_picker;
};

class HeightMapEditorPanel : public Control {
	GDCLASS(HeightMapEditorPanel, Control)
public:

	static const char *SIGNAL_TEXTURE_INDEX_SELECTED;

	HeightMapEditorPanel();
	~HeightMapEditorPanel();

	HeightMapBrushEditor &get_brush_editor() const { return *_brush_editor; }

private:
	void _on_texture_index_selected(int i);

	static void _bind_methods();

private:
	HeightMapBrushEditor *_brush_editor;
};


VARIANT_ENUM_CAST(HeightMapBrushEditor::Param)


#endif // HEIGHT_MAP_BRUSH_PANEL_H
