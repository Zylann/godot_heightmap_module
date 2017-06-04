#include "height_map_brush.h"

HeightMapBrush::HeightMapBrush() {
	_opacity = 1;
	_radius = 0;
	_shape_sum = 0;
	_mode = MODE_ADD;
}

void HeightMapBrush::set_mode(Mode mode) {
	_mode = mode;
}

void HeightMapBrush::set_radius(int p_radius) {
	if (p_radius != _radius) {
		ERR_FAIL_COND(p_radius <= 0);
		_radius = p_radius;
		generate_procedural(_radius);
		// TODO Allow to set a texture as base
	}
}

void HeightMapBrush::generate_procedural(int radius) {
	ERR_FAIL_COND(radius <= 0);
	int size = 2 * radius;
	_shape.resize(Point2i(size, size), false);

	_shape_sum = 0;

	for (int y = -radius; y < radius; ++y) {
		for (int x = -radius; x < radius; ++x) {
			float d = Vector2(x, y).distance_to(Vector2(0, 0)) / static_cast<float>(radius);
			float v = 1.f - d * d * d;
			if (v > 1.f)
				v = 1.f;
			if (v < 0.f)
				v = 0.f;
			_shape.set(x+radius, y+radius, v);
			_shape_sum += v;
		}
	}
}

void HeightMapBrush::paint_world_pos(HeightMap &height_map, Point2i cell_pos, int override_mode) {

	float delta = _opacity * 1.f / 60.f;
	Mode mode = _mode;

	if (override_mode != -1) {
		ERR_FAIL_COND(override_mode < 0 || override_mode >= MODE_COUNT);
		mode = (Mode)override_mode;
	}

	switch (mode) {
		case MODE_ADD:
			paint_height(height_map, cell_pos, 50.0 * delta);
			break;

		case MODE_SUBTRACT:
			paint_height(height_map, cell_pos, -50.0 * delta);
			break;

		// TODO Other modes

		default:
			break;
	}
}

void HeightMapBrush::paint_height(HeightMap &height_map, Point2i cell_pos, float speed) {
	// TODO Take undo/redo into account
	// TODO Factor iteration and use operators, don't copy/paste!

	Point2i shape_radii = Point2i(_radius, _radius);
	height_map.set_area_dirty(cell_pos - shape_radii, shape_radii * 2);

	// TODO Eventually HeightMapData will become a resource so we won't need to access the HeightMap node directly
	HeightMapData &data = height_map.get_data();

	Point2i shape_size = _shape.size();
	Point2i pos;

	float s = _opacity * speed;

	for (pos.y = 0; pos.y < shape_size.y; ++pos.y) {
		for (pos.x = 0; pos.x < shape_size.x; ++pos.x) {

			float shape_value = _shape.get(pos);
			Point2i tpos = cell_pos + pos - shape_radii;

			// TODO We could get rid if this `if` by calculating proper bounds beforehands
			if (data.heights.is_valid_pos(tpos)) {
				float h = data.heights.get(tpos);
				h += s * shape_value;
				data.heights.set(tpos, h);
			}
		}
	}
}
