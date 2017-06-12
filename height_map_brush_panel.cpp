#include "height_map_brush_panel.h"

const char *HeightMapBrushPanel::PARAM_CHANGED = "param_changed";

HeightMapBrushPanel::HeightMapBrushPanel() {

	set_custom_minimum_size(Vector2(35, 112));

	int y = 10;
	const int spacing = 24;

	{
		Label *label = memnew(Label);
		label->set_text(TTR("Brush size"));
		label->set_position(Vector2(10, y));
		add_child(label);

		_size_slider = memnew(HSlider);
		_size_slider->set_position(Vector2(100, y));
		_size_slider->set_min(1);
		_size_slider->set_max(50);
		_size_slider->set_step(1);
		_size_slider->set_size(Vector2(120, 10));
		_size_slider->connect("value_changed", this, "_on_param_changed", varray(BRUSH_SIZE));
		add_child(_size_slider);

		_size_label = memnew(Label);
		_size_label->set_position(Vector2(224, y));
		add_child(_size_label);
	}
	y += spacing;
	{
		Label *label = memnew(Label);
		label->set_text(TTR("Brush opacity"));
		label->set_position(Vector2(10, y));
		add_child(label);

		_opacity_slider = memnew(HSlider);
		_opacity_slider->set_position(Vector2(100, y));
		_opacity_slider->set_min(0);
		_opacity_slider->set_max(1);
		_opacity_slider->set_step(0.1);
		_opacity_slider->set_size(Vector2(120, 10));
		_opacity_slider->connect("value_changed", this, "_on_param_changed", varray(BRUSH_OPACITY));
		add_child(_opacity_slider);

		_opacity_label = memnew(Label);
		_opacity_label->set_position(Vector2(224, y));
		add_child(_opacity_label);
	}
	y += spacing;
	{
		Label *label = memnew(Label);
		label->set_text(TTR("Flatten height"));
		label->set_position(Vector2(10, y));
		add_child(label);

		_height_edit = memnew(SpinBox);
		_height_edit->set_position(Vector2(100, y));
		_height_edit->set_page(0.1);
		_height_edit->set_size(Vector2(80, 10));
		_height_edit->connect("value_changed", this, "_on_param_changed", varray(BRUSH_HEIGHT));
		add_child(_height_edit);
	}
}

HeightMapBrushPanel::~HeightMapBrushPanel() {
}

void HeightMapBrushPanel::init_params(int size, float opacity, float height) {
	_size_slider->set_value(size);
	_opacity_slider->set_as_ratio(opacity);
	_height_edit->set_value(height);
}

void HeightMapBrushPanel::on_param_changed(Variant value, int param) {

	switch(param) {
	case BRUSH_SIZE:
		_size_label->set_text(String::num(value));
		break;

	case BRUSH_OPACITY:
		_opacity_label->set_text(String::num(value));
		break;

	case BRUSH_HEIGHT:
		_height_label->set_text(String::num(value));
		break;

	default:
		break;
	}

	emit_signal(PARAM_CHANGED, value, param);
}

void HeightMapBrushPanel::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_on_param_changed", "value", "param"), &HeightMapBrushPanel::on_param_changed);

	ADD_SIGNAL(MethodInfo(PARAM_CHANGED));
}


