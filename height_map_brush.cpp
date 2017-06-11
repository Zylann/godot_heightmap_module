#include "height_map_brush.h"

HeightMapBrush::HeightMapBrush() {
	_opacity = 1;
	_radius = 0;
	_shape_sum = 0;
	_mode = MODE_ADD;
	_flatten_height = 0;
}

void HeightMapBrush::set_mode(Mode mode) {
	_mode = mode;
}

void HeightMapBrush::set_radius(int p_radius) {
	if (p_radius != _radius) {
		ERR_FAIL_COND(p_radius <= 0);
		_radius = p_radius;
		generate_procedural(_radius);
		// TODO Allow to set a texture as shape
	}
}

void HeightMapBrush::set_flatten_height(float flatten_height) {
	_flatten_height = flatten_height;
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
			_shape.set(x + radius, y + radius, v);
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

		case MODE_SMOOTH:
			smooth_height(height_map, cell_pos, delta);
			break;

		case MODE_FLATTEN:
			flatten_height(height_map, cell_pos);
			break;

		case MODE_TEXTURE:
			paint_indexed_texture(height_map, cell_pos);
			break;

		default:
			break;
	}
}

template <typename Operator_T>
void foreach_xy(
		Operator_T &op,
		HeightMap &height_map,
		Point2i cell_center,
		float speed,
		float opacity,
		const Grid2D<float> &shape,
		bool readonly = false) {

	Point2i shape_size = shape.size();
	Point2i origin = cell_center - shape_size / 2;

	if (readonly == false)
		height_map.set_area_dirty(origin, shape_size);

	// TODO Eventually HeightMapData will become a resource so we won't need to access the HeightMap node directly
	HeightMapData &data = height_map.get_data();

	Point2i pos;

	float s = opacity * speed;

	for (pos.y = 0; pos.y < shape_size.y; ++pos.y) {
		for (pos.x = 0; pos.x < shape_size.x; ++pos.x) {

			float shape_value = shape.get(pos);
			Point2i tpos = origin + pos;

			// TODO We could get rid of this `if` by calculating proper bounds beforehands
			if (data.heights.is_valid_pos(tpos)) {
				op(data, tpos, s * shape_value);
			}
		}
	}
}

struct OperatorAdd {
	void operator()(HeightMapData &data, Point2i pos, float v) {
		data.heights.set(pos, data.heights.get(pos) + v);
	}
};

struct OperatorSum {
	float sum;
	OperatorSum()
		: sum(0) {}
	void operator()(HeightMapData &data, Point2i pos, float v) {
		sum += data.heights.get(pos) * v;
	}
};

struct OperatorLerp {
	float target;
	OperatorLerp(float p_target)
		: target(p_target) {}
	void operator()(HeightMapData &data, Point2i pos, float v) {
		data.heights.set(pos, Math::lerp(data.heights.get(pos), target, v));
	}
};

void HeightMapBrush::paint_height(HeightMap &height_map, Point2i cell_pos, float speed) {
	OperatorAdd op;
	foreach_xy(op, height_map, cell_pos, speed, _opacity, _shape);
}

void HeightMapBrush::smooth_height(HeightMap &height_map, Point2i cell_pos, float speed) {
	OperatorSum sum_op;
	foreach_xy(sum_op, height_map, cell_pos, 1, _opacity, _shape, false);
	float target_value = sum_op.sum / _shape_sum;
	OperatorLerp lerp_op(target_value);
	foreach_xy(lerp_op, height_map, cell_pos, speed, _opacity, _shape);
}

void HeightMapBrush::flatten_height(HeightMap &height_map, Point2i cell_pos) {
	OperatorLerp op(_flatten_height);
	foreach_xy(op, height_map, cell_pos, 1, 1, _shape);
}

void HeightMapBrush::paint_indexed_texture(HeightMap &height_map, Point2i cell_pos) {

	// TODO Implement texture paint using blend indices
	// TODO Implement texture arrays or 3D textures in shaders so that we can see the result

	// The idea is that, in a fragment shader, we can do this for a cheap cost and any number of textures:

	// c += texture3D(s, vec3(uv.xy, indices.x)) * weigths.x;
	// c += texture3D(s, vec3(uv.xy, indices.y)) * weigths.y;
	// c += texture3D(s, vec3(uv.xy, indices.z)) * weigths.z;
	// c += texture3D(s, vec3(uv.xy, indices.w)) * weigths.w;

	// or

	// c += textureArray(s, uv.xy, int(indices.x)) * weigths.x;
	// c += textureArray(s, uv.xy, int(indices.y)) * weigths.y;
	// c += textureArray(s, uv.xy, int(indices.z)) * weigths.z;
	// c += textureArray(s, uv.xy, int(indices.w)) * weigths.w;

	// Note: binary combination of 16 textures works too but the method above is more efficient and scalable in GLES3

	// So what we do in the editor is:
	// paint textures as blend factors, automatically choose indices based on
	// which textures are the most visible, and discard the others.
	// (a bit like a local dynamic color palette, but with textures)
	// Most of the time there will be only 1 or 2 textures used in a given point,
	// but to handle transitions correctly there should be at least 4 slots.
	// Then this information will be translated into vertices at meshing time.
}
