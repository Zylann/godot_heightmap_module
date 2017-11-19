#include "height_map_brush_panel.h"
#include "height_map_data.h"

#include "scene/gui/button.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/item_list.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/color_picker.h"

const char *HeightMapBrushEditor::SIGNAL_PARAM_CHANGED = "param_changed";

const char *HeightMapEditorPanel::SIGNAL_TEXTURE_INDEX_SELECTED = "texture_index_selected";


HeightMapBrushEditor::HeightMapBrushEditor() {

	//set_custom_minimum_size(Vector2(35, 112));

	int y0 = 10;
	int y = y0;
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
	y += spacing;
	{
		ColorPickerButton *color_picker = memnew(ColorPickerButton);
		color_picker->set_position(Vector2(10, y));
		color_picker->set_size(Vector2(100, 24));
		color_picker->get_picker()->connect("color_changed", this, "_on_param_changed", varray(BRUSH_COLOR));
		add_child(color_picker);
	}
}

HeightMapBrushEditor::~HeightMapBrushEditor() {
}

void HeightMapBrushEditor::init_params(int size, float opacity, float height) {
	_size_slider->set_value(size);
	_opacity_slider->set_as_ratio(opacity);
	_height_edit->set_value(height);

	_size_label->set_text(String::num(size));
	_opacity_label->set_text(String::num(opacity));
}

void HeightMapBrushEditor::on_param_changed(Variant value, int param) {

	switch (param) {
		case BRUSH_SIZE:
			_size_label->set_text(String::num(value));
			break;

		case BRUSH_OPACITY:
			_opacity_label->set_text(String::num(value));
			break;

		default:
			break;
	}

	emit_signal(SIGNAL_PARAM_CHANGED, value, param);
}

void HeightMapBrushEditor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_on_param_changed", "value", "param"), &HeightMapBrushEditor::on_param_changed);

	ADD_SIGNAL(MethodInfo(SIGNAL_PARAM_CHANGED));
}






HeightMapEditorPanel::HeightMapEditorPanel() {

	// TODO This UI may change in the future, it's WIP

	int m = 4;

	set_custom_minimum_size(Vector2(300, 112));

	// Using a secondary container, because margins don't work when inside a container...
	HSplitContainer *main_container = memnew(HSplitContainer);
	main_container->set_split_offset(100);
	main_container->set_anchors_preset(Control::PRESET_WIDE);
	main_container->set_margin(MARGIN_LEFT, m);
	main_container->set_margin(MARGIN_TOP, m);
	main_container->set_margin(MARGIN_RIGHT, -m);
	main_container->set_margin(MARGIN_BOTTOM, -m);
	add_child(main_container);

	{
		// Splat editor

		int w = 32;
		int bh = 24;
		int bm = 2;

		Control *splat_editor_container = memnew(Control);
		splat_editor_container->set_custom_minimum_size(Vector2(4 * (w + 10), 10));

		ItemList *textures_container = memnew(ItemList);
		textures_container->set_anchor(MARGIN_LEFT, Control::ANCHOR_BEGIN);
		textures_container->set_anchor(MARGIN_RIGHT, Control::ANCHOR_END);
		textures_container->set_anchor(MARGIN_TOP, Control::ANCHOR_BEGIN);
		textures_container->set_anchor(MARGIN_BOTTOM, Control::ANCHOR_END);
		textures_container->set_margin(MARGIN_LEFT, 0);
		textures_container->set_margin(MARGIN_RIGHT, 0);
		textures_container->set_margin(MARGIN_TOP, 0);
		textures_container->set_margin(MARGIN_BOTTOM, -bh-bm);
		textures_container->set_same_column_width(true);
		textures_container->set_max_columns(0);
		textures_container->set_fixed_icon_size(Vector2(w, w));
		textures_container->set_icon_mode(ItemList::ICON_MODE_TOP);

		for(int i = 0; i < 8; ++i) {
			textures_container->add_item(String::num(i));
		}

		textures_container->connect("item_selected", this, "_on_texture_index_selected");

		splat_editor_container->add_child(textures_container);

		HBoxContainer *buttons_container = memnew(HBoxContainer);
		buttons_container->set_anchor(MARGIN_LEFT, Control::ANCHOR_BEGIN);
		buttons_container->set_anchor(MARGIN_RIGHT, Control::ANCHOR_END);
		buttons_container->set_anchor(MARGIN_TOP, Control::ANCHOR_END);
		buttons_container->set_anchor(MARGIN_BOTTOM, Control::ANCHOR_END);
		buttons_container->set_margin(MARGIN_LEFT, 0);
		buttons_container->set_margin(MARGIN_RIGHT, 0);
		buttons_container->set_margin(MARGIN_TOP, -bh);
		buttons_container->set_margin(MARGIN_BOTTOM, 0);

		Button *assign_button = memnew(Button);
		assign_button->set_text("+");
		buttons_container->add_child(assign_button);

		Button *clear_button = memnew(Button);
		clear_button->set_text("-");
		buttons_container->add_child(clear_button);

		Button *replace_button = memnew(Button);
		replace_button->set_text(TTR("Load..."));
		buttons_container->add_child(replace_button);

		splat_editor_container->add_child(buttons_container);

		main_container->add_child(splat_editor_container);
	}

	_brush_editor = memnew(HeightMapBrushEditor);
	main_container->add_child(_brush_editor);
}

HeightMapEditorPanel::~HeightMapEditorPanel() {

}

void HeightMapEditorPanel::_on_texture_index_selected(int i) {
	emit_signal(SIGNAL_TEXTURE_INDEX_SELECTED, i);
}

void HeightMapEditorPanel::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_on_texture_index_selected", "i"), &HeightMapEditorPanel::_on_texture_index_selected);

	ADD_SIGNAL(MethodInfo(SIGNAL_TEXTURE_INDEX_SELECTED));
}



