#include "height_map_brush.h"

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

		case MODE_TEXTURE:
			// TODO Undo for this when we are sure it works
			paint_indexed_texture(data, origin);
			break;

		case MODE_COLOR:
			paint_color(data, origin);
			break;

		default:
			break;
	}

	data.notify_region_change(origin, _shape.size());
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

	data.heights.clamp_min_max_excluded(min, max);

	Point2i pos;
	for (pos.y = min.y; pos.y < max.y; ++pos.y) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {

			float shape_value = shape.get(pos - origin);
			op(data, pos, s * shape_value);
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

struct OperatorLerpColor {
	Color target;
	OperatorLerpColor(Color p_target)
		: target(p_target) {}
	void operator()(HeightMapData &data, Point2i pos, float v) {
		data.colors.set(pos, data.colors.get(pos).linear_interpolate(target, v));
	}
};

struct OperatorIndexedTexture {

	int texture_index;

	OperatorIndexedTexture(int p_texture_index)
		: texture_index(p_texture_index) {}

	void operator()(HeightMapData &data, Point2i pos, float v) {

		// TODO There is potential for bad blending using this local algorithm,
		// neighbors should be taken into account I think

		int loc = data.texture_weights[0].index(pos);

		int slot_index = -1;

		// Find if the texture already has a slot
		for (int i = 0; i < HeightMapData::TEXTURE_INDEX_COUNT; ++i) {
			if (data.texture_indices[slot_index][loc] == texture_index) {
				slot_index = i;
				break;
			}
		}

		if (slot_index == -1) {
			// Need to assign a slot to the texture

			// Find the lowest weight
			float lowest_weight = 2;
			for (int i = 0; i < HeightMapData::TEXTURE_INDEX_COUNT; ++i) {
				float w = data.texture_weights[i][loc];
				if (w < lowest_weight) {
					lowest_weight = w;
					slot_index = i;
				}
			}

			// Replace old weight by new one
			data.texture_indices[slot_index][loc] = texture_index;
			data.texture_weights[slot_index][loc] = 0;
		}

		// Weights sum should always initially be 1
		float weight_sum = 1;

		// Exclude the slot we are modifying
		weight_sum -= data.texture_weights[slot_index][loc];

		// Modify slot's weight
		float w = data.texture_weights[slot_index][loc];
		w = Math::lerp(w, 1, v);
		data.texture_weights[slot_index][loc] = w;

		// Make sure the other slots give a sum of 1
		float k = 0;
		if (weight_sum > 0.01)
			k = (1.f - w) / weight_sum;
		for (int i = 0; i < HeightMapData::TEXTURE_INDEX_COUNT; ++i) {
			if (i != slot_index)
				data.texture_weights[i][loc] *= k;
		}

		// Debug check:
		// Really make sure weights sum is 1
		weight_sum = 0;
		for (int i = 0; i < HeightMapData::TEXTURE_INDEX_COUNT; ++i) {
			weight_sum += data.texture_weights[i][loc];
		}
		if (weight_sum > 1.001)
			print_line(String("Sum is above 1: {0}").format(varray(weight_sum)));
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

template <typename T>
void backup_for_undo(const Grid2D<T> &grid, HeightMapBrush::UndoCache &undo_cache, Point2i rect_origin, Point2i rect_size) {

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

			bool invalid_min = !grid.is_valid_pos(min);
			bool invalid_max = !grid.is_valid_pos(max - Point2i(1,1)); // Note: max is excluded

			if(invalid_min || invalid_max) {
				// Out of bounds
				if(invalid_min ^ invalid_max)
					print_line("Wut? Grid might not be multiple of chunk size!");
				continue;
			}

			PoolByteArray data = grid.dump_region(min, max);
			undo_cache.chunks[cpos] = data;

			//debug_print_cache(undo_cache, cpos);
		}
	}
}

void HeightMapBrush::paint_height(HeightMapData &data, Point2i origin, float speed) {

	backup_for_undo(data.heights, _undo_cache, origin, _shape.size());

	OperatorAdd op;
	foreach_xy(op, data, origin, speed, _opacity, _shape);

	data.update_normals(origin, _shape.size());
}

void HeightMapBrush::smooth_height(HeightMapData &data, Point2i origin, float speed) {

	backup_for_undo(data.heights, _undo_cache, origin, _shape.size());

	OperatorSum sum_op;
	foreach_xy(sum_op, data, origin, 1, _opacity, _shape);
	float target_value = sum_op.sum / _shape_sum;
	OperatorLerp lerp_op(target_value);
	foreach_xy(lerp_op, data, origin, speed, _opacity, _shape);

	data.update_normals(origin, _shape.size());
}

void HeightMapBrush::flatten_height(HeightMapData &data, Point2i origin) {

	backup_for_undo(data.heights, _undo_cache, origin, _shape.size());

	OperatorLerp op(_flatten_height);
	foreach_xy(op, data, origin, 1, 1, _shape);

	data.update_normals(origin, _shape.size());
}

void HeightMapBrush::paint_indexed_texture(HeightMapData &data, Point2i origin) {

	OperatorIndexedTexture op(_texture_index);
	foreach_xy(op, data, origin, 1, _opacity, _shape);

	// TODO Implement texture arrays or 3D textures in shaders so that we can see the result

	// The idea is that, in a fragment shader, we can do this for a cheap cost and any number of textures:

	// c += texture3D(s, vec3(uv.xy, indices.x)) * weigths.x;
	// c += texture3D(s, vec3(uv.xy, indices.y)) * weigths.y;
	// c += texture3D(s, vec3(uv.xy, indices.z)) * weigths.z;
	// c += texture3D(s, vec3(uv.xy, indices.w)) * weigths.w;

	// or

	// c += textureArray(s, vec3(uv.xy, indices.x)) * weigths.x;
	// c += textureArray(s, vec3(uv.xy, indices.y)) * weigths.y;
	// c += textureArray(s, vec3(uv.xy, indices.z)) * weigths.z;
	// c += textureArray(s, vec3(uv.xy, indices.w)) * weigths.w;

	// Note: binary combination of 16 textures works too but the method above is more efficient and scalable in GLES3

	// So what we do in the editor is:
	// paint textures as blend factors, automatically choose indices based on
	// which textures are the most visible, and discard the others.
	// (a bit like a local dynamic color palette, but with textures)
	// Most of the time there will be only 1 or 2 textures used in a given point,
	// but to handle transitions correctly there should be at least 4 slots.
	// Then this information will be translated into vertices at meshing time.
}

void HeightMapBrush::paint_color(HeightMapData &data, Point2i origin) {

	backup_for_undo(data.colors, _undo_cache, origin, _shape.size());

	OperatorLerpColor op(_color);
	foreach_xy(op, data, origin, 1, _opacity, _shape);
}

template <typename T>
Array fetch_redo_chunks(const Grid2D<T> &grid, const List<Point2i> &keys) {
	Array output;
	for (const List<Point2i>::Element *E = keys.front(); E; E = E->next()) {
		Point2i cpos = E->get();
		Point2i min = cpos * HeightMap::CHUNK_SIZE;
		Point2i max = min + Point2i(1,1)*HeightMap::CHUNK_SIZE;
		PoolByteArray data = grid.dump_region(min, max);
		output.append(data);
	}
	return output;
}

HeightMapBrush::UndoData HeightMapBrush::pop_undo_redo_data(const HeightMapData &heightmap_data) {

	// TODO If possible, use a custom Reference class to store this data into the UndoRedo API,
	// but WITHOUT exposing it to scripts (so we won't need the following conversions!)

	List<Point2i> chunk_positions_list;
	_undo_cache.chunks.get_key_list(&chunk_positions_list);

	HeightMapData::Channel channel = HeightMapData::CHANNEL_HEIGHT;

	// Copy heightmap data after operation for redo
	Array redo_data;
	switch(_mode) {
		case MODE_ADD:
		case MODE_SUBTRACT:
		case MODE_SMOOTH:
		case MODE_FLATTEN:
			redo_data = fetch_redo_chunks(heightmap_data.heights, chunk_positions_list);
			channel = HeightMapData::CHANNEL_HEIGHT;
			break;
		case MODE_TEXTURE:
			// TODO
			print_line("No undo for this mode! (yet)");
			break;
		case MODE_COLOR:
			redo_data = fetch_redo_chunks(heightmap_data.colors, chunk_positions_list);
			channel = HeightMapData::CHANNEL_COLOR;
			break;
		default:
			print_line("No undo for this mode!");
			break;
	}

	Array undo_data;

	// Convert chunk positions to flat int array
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

	UndoData data;
	data.undo = undo_data;
	data.redo = redo_data;
	data.chunk_positions = chunk_positions;
	data.channel = channel;

	_undo_cache.clear();

	return data;
}


