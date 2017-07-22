#ifndef HEIGHT_MAP_BRUSH_PANEL_H
#define HEIGHT_MAP_BRUSH_PANEL_H

#include <scene/gui/label.h>
#include <scene/gui/slider.h>
#include <scene/gui/spin_box.h>

class HeightMapBrushPanel : public Control {
	GDCLASS(HeightMapBrushPanel, Control)
public:
	HeightMapBrushPanel();
	~HeightMapBrushPanel();

	static const char *PARAM_CHANGED;
	static const char *SIGNAL_FILE_IMPORTED;

	enum Params {
		BRUSH_SIZE = 0,
		BRUSH_OPACITY,
		BRUSH_HEIGHT
	};

	void init_params(int size, float opacity, float height);

protected:
	static void _bind_methods();

private:
	void on_param_changed(Variant value, int param);
	void _import_file_selected(String p_path);

private:
	Label *_size_label;
	Slider *_size_slider;

	Label *_opacity_label;
	Slider *_opacity_slider;

	SpinBox *_height_edit;
};

#endif // HEIGHT_MAP_BRUSH_PANEL_H
