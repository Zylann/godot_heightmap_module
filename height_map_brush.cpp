#include "height_map_brush.h"
#include "utility.h"

HeightMapBrush::HeightMapBrush() {
	_opacity = 1;
	_radius = 0;
	_shape_sum = 0;
	_mode = MODE_ADD;
	_flatten_height = 0;
	_texture_index = 0;
	_color = Color(1, 0, 0, 0);
}

void HeightMapBrush::set_mode(Mode mode) {
	_mode = mode;
	// Different mode might affect other channels,
	// so we need to clear the current data otherwise it wouldn't make sense
	_undo_cache.clear();
}

void HeightMapBrush::set_radius(int p_radius) {
	if (p_radius != _radius) {
		ERR_FAIL_COND(p_radius <= 0);
		_radius = p_radius;
		generate_procedural(_radius);
		// TODO Allow to set a texture as shape
	}
}

void HeightMapBrush::set_opacity(float opacity) {
	if (opacity < 0)
		opacity = 0;
	if (opacity > 1)
		opacity = 1;
	_opacity = opacity;
}

void HeightMapBrush::set_flatten_height(float flatten_height) {
	_flatten_height = flatten_height;
}

void HeightMapBrush::set_texture_index(int tid) {
	ERR_FAIL_COND(tid < 0);
	ERR_FAIL_COND(tid > 255);
	_texture_index = tid;
}

void HeightMapBrush::set_color(Color c) {
	// Color might be useful for custom shading
	_color = c;
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

HeightMapData::Channel HeightMapBrush::get_mode_channel(Mode mode) {
	switch(mode) {
	case MODE_ADD:
	case MODE_SUBTRACT:
	case MODE_SMOOTH:
	case MODE_FLATTEN:
		return HeightMapData::CHANNEL_HEIGHT;
	case MODE_COLOR:
		return HeightMapData::CHANNEL_COLOR;
	case MODE_SPLAT:
		return HeightMapData::CHANNEL_SPLAT;
	case MODE_MASK:
		return HeightMapData::CHANNEL_MASK;
	default:
		print_line("This mode has no channel");
		break;
	}
	return HeightMapData::CHANNEL_COUNT; // Error
}

void HeightMapBrush::paint(HeightMap &height_map, Point2i cell_pos, int override_mode) {

	ERR_FAIL_COND(height_map.get_data().is_null());
	HeightMapData &data = **height_map.get_data();

	float delta = _opacity * 1.f / 60.f;
	Mode mode = _mode;

	if (override_mode != -1) {
		ERR_FAIL_COND(override_mode < 0 || override_mode >= MODE_COUNT);
		mode = (Mode)override_mode;
	}

	Point2i origin = cell_pos - _shape.size() / 2;

	height_map.set_area_dirty(origin, _shape.size());

	switch (mode) {

		case MODE_ADD:
			paint_height(data, origin, 50.0 * delta);
			break;

		case MODE_SUBTRACT:
			paint_height(data, origin, -50.0 * delta);
			break;

		case MODE_SMOOTH:
			smooth_height(data, origin, delta);
			break;

		case MODE_FLATTEN:
			flatten_height(data, origin);
			break;

		case MODE_SPLAT:
			paint_splat(data, origin);
			break;

		case MODE_COLOR:
			paint_color(data, origin);
			break;

		case MODE_MASK:
			paint_mask(data, origin);
			break;

		default:
			break;
	}

	data.notify_region_change(origin, _shape.size(), get_mode_channel(mode));
}

template <typename Operator_T>
void foreach_xy(
		Operator_T &op,
		HeightMapData &data,
		Point2i origin,
		float speed,
		float opacity,
		const Grid2D<float> &shape) {

	Point2i shape_size = shape.size();

	float s = opacity * speed;

	Point2i min = origin;
	Point2i max = min + shape_size;

	clamp_min_max_excluded(min, max, Point2i(0,0), Point2i(data.get_resolution(), data.get_resolution()));

	Point2i pos;
	for (pos.y = min.y; pos.y < max.y; ++pos.y) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {

			float shape_value = shape.get(pos - min);
			op(data, pos, s * shape_value);
		}
	}
}

struct OperatorAdd {
	Image &_im;
	OperatorAdd(Image &im)
		: _im(im) {}
	void operator()(HeightMapData &data, Point2i pos, float v) {
		Color c = _im.get_pixel(pos.x, pos.y);
		c.r += v;
		_im.set_pixel(pos.x, pos.y, c);
	}
};

struct OperatorSum {
	float sum;
	const Image &_im;
	OperatorSum(const Image &im)
		: sum(0), _im(im) {}
	void operator()(HeightMapData &data, Point2i pos, float v) {
		sum += _im.get_pixel(pos.x, pos.y).r * v;
	}
};

struct OperatorLerp {

	float target;
	Image &_im;

	OperatorLerp(float p_target, Image &im)
		: target(p_target), _im(im) {}

	void operator()(HeightMapData &data, Point2i pos, float v) {
		Color c = _im.get_pixel(pos.x, pos.y);
		c.r = Math::lerp(c.r, target, v);
		_im.set_pixel(pos.x, pos.y, c);
	}
};

struct OperatorLerpColor {

	Color target;
	Image &_im;

	OperatorLerpColor(Color p_target, Image &im)
		: target(p_target), _im(im) {}

	void operator()(HeightMapData &data, Point2i pos, float v) {
		Color c = _im.get_pixel(pos.x, pos.y);
		c = c.linear_interpolate(target, v);
		_im.set_pixel(pos.x, pos.y, c);
	}
};

/*static void debug_print_cache(const HeightMapBrush::UndoCache &cache, Point2i anchor) {
	print_line(" ");
	for(int y = 8; y >= 0; --y) {
		String str;
		for(int x = 0; x < 16; ++x) {
			Point2i p(x,y);
			if(cache.chunks.getptr(p) != NULL) {
				if(p == anchor)
					str += "X";
				else
					str += "O";
			} else {
				str += "-";
			}
		}
		print_line(str);
	}
}*/

static inline bool is_valid_pos(Point2i pos, const Image &im) {
	return !(pos.x < 0 || pos.y < 0 || pos.x >= im.get_width() || pos.y >= im.get_height());
}

void HeightMapBrush::backup_for_undo(const Image &im, HeightMapBrush::UndoCache &undo_cache, Point2i rect_origin, Point2i rect_size) {

	// Backup cells before they get changed,
	// using chunks so that we don't save the entire grid everytime.
	// This function won't do anything if all concerned chunks got backupped already.

	Point2i cmin = rect_origin / HeightMap::CHUNK_SIZE;
	Point2i cmax = (rect_origin + rect_size - Point2i(1,1)) / HeightMap::CHUNK_SIZE + Point2i(1,1);

	Point2i cpos;
	for(cpos.y = cmin.y; cpos.y < cmax.y; ++cpos.y) {
		for(cpos.x = cmin.x; cpos.x < cmax.x; ++cpos.x) {

			if(undo_cache.chunks.getptr(cpos) != NULL) {
				// Already backupped
				continue;
			}

			Point2i min = cpos * HeightMap::CHUNK_SIZE;
			Point2i max = min + Point2i(HeightMap::CHUNK_SIZE, HeightMap::CHUNK_SIZE);

			bool invalid_min = !is_valid_pos(min, im);
			bool invalid_max = !is_valid_pos(max - Point2i(1,1), im); // Note: max is excluded

			if(invalid_min || invalid_max) {
				// Out of bounds

				// Note: this error check isn't working because data grids are intentionally off-by-one
				//if(invalid_min ^ invalid_max)
				//	print_line("Wut? Grid might not be multiple of chunk size!");

				continue;
			}

			Ref<Image> sub_image = im.get_rect(Rect2(min, max - min));
			undo_cache.chunks[cpos] = sub_image;
		}
	}
}

void HeightMapBrush::paint_height(HeightMapData &data, Point2i origin, float speed) {

	Ref<Image> im_ref = data.get_image(HeightMapData::CHANNEL_HEIGHT);
	ERR_FAIL_COND(im_ref.is_null());

	LockImage lock(im_ref);

	backup_for_undo(**im_ref, _undo_cache, origin, _shape.size());

	OperatorAdd op(**im_ref);
	foreach_xy(op, data, origin, speed, _opacity, _shape);

	data.update_normals(origin, _shape.size());
}

void HeightMapBrush::smooth_height(HeightMapData &data, Point2i origin, float speed) {

	Ref<Image> im_ref = data.get_image(HeightMapData::CHANNEL_HEIGHT);
	ERR_FAIL_COND(im_ref.is_null());

	LockImage lock(im_ref);

	backup_for_undo(**im_ref, _undo_cache, origin, _shape.size());

	OperatorSum sum_op(**im_ref);
	foreach_xy(sum_op, data, origin, 1, _opacity, _shape);
	float target_value = sum_op.sum / _shape_sum;

	OperatorLerp lerp_op(target_value, **im_ref);
	foreach_xy(lerp_op, data, origin, speed, _opacity, _shape);

	data.update_normals(origin, _shape.size());
}

void HeightMapBrush::flatten_height(HeightMapData &data, Point2i origin) {

	Ref<Image> im_ref = data.get_image(HeightMapData::CHANNEL_HEIGHT);
	ERR_FAIL_COND(im_ref.is_null());

	LockImage lock(im_ref);

	backup_for_undo(**im_ref, _undo_cache, origin, _shape.size());

	OperatorLerp op(_flatten_height, **im_ref);
	foreach_xy(op, data, origin, 1, 1, _shape);

	data.update_normals(origin, _shape.size());
}

void HeightMapBrush::paint_splat(HeightMapData &data, Point2i origin) {

	Ref<Image> im_ref = data.get_image(HeightMapData::CHANNEL_SPLAT);
	ERR_FAIL_COND(im_ref.is_null());
	Image &im = **im_ref;

	im.lock();
	backup_for_undo(im, _undo_cache, origin, _shape.size());
	im.unlock();

	Point2i shape_size = _shape.size();

	Point2i min = origin;
	Point2i max = min + shape_size;

	clamp_min_max_excluded(min, max, Point2i(0,0), Point2i(data.get_resolution(), data.get_resolution()));

	Point2i pos;
	const float shape_threshold = 0.1;

	LockImage lock(im_ref);

	for (pos.y = min.y; pos.y < max.y; ++pos.y) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {

			float shape_value = _shape.get(pos - min);

			if(shape_value > shape_threshold) {

				// TODO Improve weight blending, it looks meh

				Color c;
				c.r = static_cast<float>(_texture_index) / 256.0;
				c.g = CLAMP(_opacity, 0.0, 1.0);
				im.set_pixel(pos.x, pos.y, c);
			}
		}
	}
}

void HeightMapBrush::paint_color(HeightMapData &data, Point2i origin) {

	Ref<Image> im_ref = data.get_image(HeightMapData::CHANNEL_COLOR);
	ERR_FAIL_COND(im_ref.is_null());

	LockImage lock(im_ref);

	backup_for_undo(**im_ref, _undo_cache, origin, _shape.size());

	OperatorLerpColor op(_color, **im_ref);
	foreach_xy(op, data, origin, 1, _opacity, _shape);
}

void HeightMapBrush::paint_mask(HeightMapData &data, Point2i origin) {

	Ref<Image> im_ref = data.get_image(HeightMapData::CHANNEL_MASK);
	ERR_FAIL_COND(im_ref.is_null());
	Image &im = **im_ref;

	im.lock();
	backup_for_undo(im, _undo_cache, origin, _shape.size());
	im.unlock();

	Point2i shape_size = _shape.size();

	Point2i min = origin;
	Point2i max = min + shape_size;

	clamp_min_max_excluded(min, max, Point2i(0,0), Point2i(data.get_resolution(), data.get_resolution()));

	Point2i pos;
	const float shape_threshold = 0.1;
	const Color value = _opacity > 0.5 ? Color(1.0, 0.0, 0.0, 0.0) : Color();

	LockImage lock(im_ref);

	for (pos.y = min.y; pos.y < max.y; ++pos.y) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {

			float shape_value = _shape.get(pos - min);

			if(shape_value > shape_threshold) {
				im.set_pixel(pos.x, pos.y, value);
			}
		}
	}
}

static Array fetch_redo_chunks(const Image &im, const List<Point2i> &keys) {
	Array output;
	for (const List<Point2i>::Element *E = keys.front(); E; E = E->next()) {
		Point2i cpos = E->get();
		Point2i min = cpos * HeightMap::CHUNK_SIZE;
		Point2i max = min + Point2i(1,1)*HeightMap::CHUNK_SIZE;
		Ref<Image> sub_image = im.get_rect(Rect2(min, max - min));
		output.append(sub_image);
	}
	return output;
}

HeightMapBrush::UndoData HeightMapBrush::pop_undo_redo_data(const HeightMapData &heightmap_data) {

	// TODO If possible, use a custom Reference class to store this data into the UndoRedo API,
	// but WITHOUT exposing it to scripts (so we won't need the following conversions!)

	UndoData data;

	List<Point2i> chunk_positions_list;
	_undo_cache.chunks.get_key_list(&chunk_positions_list);

	HeightMapData::Channel channel = get_mode_channel(_mode);
	ERR_FAIL_COND_V(channel == HeightMapData::CHANNEL_COUNT, data);

	Ref<Image> im_ref = heightmap_data.get_image(channel);
	ERR_FAIL_COND_V(im_ref.is_null(), data);
	Array redo_data = fetch_redo_chunks(**im_ref, chunk_positions_list);

	// Convert chunk positions to flat int array
	Array undo_data;
	PoolIntArray chunk_positions;
	chunk_positions.resize(chunk_positions_list.size() * 2);
	{
		int i = 0;
		PoolIntArray::Write r = chunk_positions.write();
		for (List<Point2i>::Element *E = chunk_positions_list.front(); E; E = E->next()) {

			Point2i cpos = E->get();
			r[i] = cpos.x;
			r[i+1] = cpos.y;
			i += 2;

			// Also gather pre-cached data for undo, in the same order
			undo_data.append(_undo_cache.chunks[cpos]);
		}
	}

	data.undo = undo_data;
	data.redo = redo_data;
	data.chunk_positions = chunk_positions;
	data.channel = channel;

	_undo_cache.clear();

	return data;
}


