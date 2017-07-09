#include <scene/3d/camera.h>

#include "height_map.h"
#include "utility.h"
#include "resources.gen.cpp"

const char *HeightMap::SHADER_PARAM_HEIGHT_TEXTURE = "height_texture";
const char *HeightMap::SHADER_PARAM_NORMAL_TEXTURE = "normal_texture";
const char *HeightMap::SHADER_PARAM_COLOR_TEXTURE = "color_texture";
const char *HeightMap::SHADER_PARAM_RESOLUTION = "heightmap_resolution";
const char *HeightMap::SHADER_PARAM_INVERSE_TRANSFORM = "heightmap_inverse_transform";

namespace {

	struct SetMaterialAction {
		Ref<Material> material;
		void operator()(HeightMapChunk &chunk) {
			chunk.set_material(material);
		}
	};

	struct EnterWorldAction {
		Ref<World> world;
		void operator()(HeightMapChunk &chunk) {
			chunk.enter_world(**world);
		}
	};

	struct ExitWorldAction {
		void operator()(HeightMapChunk &chunk) {
			chunk.exit_world();
		}
	};

	struct TransformChangedAction {
		Transform *transform;
		void operator()(HeightMapChunk &chunk) {
			chunk.parent_transform_changed(*transform);
		}
	};

	struct VisibilityChangedAction {
		bool visible;
		void operator()(HeightMapChunk &chunk) {
			chunk.set_visible(visible);
		}
	};

	struct DeleteChunkAction {
		void operator()(HeightMapChunk &chunk) {
			memdelete(&chunk);
		}
	};

	Ref<Shader> s_default_shader;
}

HeightMap::HeightMap() {

	set_notify_transform(true);
	_collision_enabled = true;
	_lodder.set_callbacks(s_make_chunk_cb, s_recycle_chunk_cb, this);
	_updated_chunks = 0;
}

HeightMap::~HeightMap() {
	clear_chunk_cache();
}

void HeightMap::init_default_resources() {
	ERR_FAIL_COND(s_default_shader.is_valid());
	s_default_shader.instance();
	s_default_shader->set_code(s_default_shader_code);
}

void HeightMap::free_default_resources() {
	ERR_FAIL_COND(s_default_shader.is_null());
	s_default_shader.unref();
}

void HeightMap::clear_chunk_cache() {

	// The lodder has to be cleared because otherwise it will reference dangling pointers
	_lodder.clear();

	for_all_chunks(DeleteChunkAction());
	_chunk_cache.clear();
}

HeightMapChunk *HeightMap::get_chunk_at(Point2i pos, int lod) const {
	if(lod < _chunk_cache.size()) {
		HeightMapChunk *const *pptr = _chunk_cache[lod].getptr(pos);
		if(pptr)
			return *pptr;
	}
	return NULL;
}

void HeightMap::set_data(Ref<HeightMapData> data) {

	if(data == _data)
		return;

	if(_data.is_valid()) {
		_data->disconnect(HeightMapData::SIGNAL_RESOLUTION_CHANGED, this, "_on_data_resolution_changed");
		_data->disconnect(HeightMapData::SIGNAL_REGION_CHANGED, this, "_on_data_region_changed");
	}

	_data = data;

	// Note: the order of these two is important
	clear_chunk_cache();

	if(_data.is_valid()) {

#ifdef TOOLS_ENABLED
		// This is a small UX improvement so that the user sees a default terrain
		if(is_inside_tree() && get_tree()->is_editor_hint()) {
			if(data->get_resolution() == 0) {
				data->load_default();
			}
		}
#endif
		_data->connect(HeightMapData::SIGNAL_RESOLUTION_CHANGED, this, "_on_data_resolution_changed");
		_data->connect(HeightMapData::SIGNAL_REGION_CHANGED, this, "_on_data_region_changed");
		_on_data_resolution_changed();

		update_material();
	}
}

void HeightMap::_on_data_resolution_changed() {

	clear_chunk_cache();

	_lodder.create_from_sizes(CHUNK_SIZE, _data->get_resolution());

	_pending_chunk_updates.clear();
	_chunk_cache.resize(_lodder.get_lod_count());
	_mesher.configure(Point2i(CHUNK_SIZE, CHUNK_SIZE), _lodder.get_lod_count());
	update_material();
}


void HeightMap::_on_data_region_changed(int min_x, int min_y, int max_x, int max_y, int channel) {
	//print_line(String("_on_data_region_changed {0}, {1}, {2}, {3}").format(varray(min_x, min_y, max_x, max_y)));
	set_area_dirty(Point2i(min_x, min_y), Point2i(max_x - min_x, max_y - min_y));
}

void HeightMap::set_custom_shader(Ref<Shader> p_shader) {

	if (_custom_shader != p_shader) {
		_custom_shader = p_shader;

#ifdef TOOLS_ENABLED
		// When the new shader is empty, allows to fork from the default shader
		if(is_inside_tree() && get_tree()->is_editor_hint()) {
			if(p_shader.is_valid()) {
				if(p_shader->get_code().empty()) {
					p_shader->set_code(s_default_shader_code);
				}
			}
		}
#endif

		update_material();
	}
}

void HeightMap::update_material() {

	if(_material.is_null()) {
		_material.instance();
	}
	Ref<Shader> sh = s_default_shader;
	_material->set_shader(_custom_shader.is_null() ? sh : _custom_shader);

	Ref<Texture> height_texture;
	Ref<Texture> normal_texture;
	Ref<Texture> color_texture;
	Vector2 res(-1,-1);

	if(_data.is_valid()) {
		height_texture = _data->get_texture(HeightMapData::CHANNEL_HEIGHT);
		normal_texture = _data->get_texture(HeightMapData::CHANNEL_NORMAL);
		color_texture = _data->get_texture(HeightMapData::CHANNEL_COLOR);
		res.x = _data->get_resolution();
		res.y = res.x;
	}

	if(is_inside_tree()) {
		Transform gt = get_global_transform();
		Transform t = gt.affine_inverse();
		_material->set_shader_param(SHADER_PARAM_INVERSE_TRANSFORM, t);
	}

	_material->set_shader_param(SHADER_PARAM_HEIGHT_TEXTURE, height_texture);
	_material->set_shader_param(SHADER_PARAM_NORMAL_TEXTURE, normal_texture);
	_material->set_shader_param(SHADER_PARAM_COLOR_TEXTURE, color_texture);
	_material->set_shader_param(SHADER_PARAM_RESOLUTION, res);
}

void HeightMap::set_collision_enabled(bool enabled) {
	_collision_enabled = enabled;
	// TODO Update chunks / enable heightmap collider (or will be done through a different node perhaps)
}

void HeightMap::set_lod_scale(float lod_scale) {
	_lodder.set_split_scale(lod_scale);
}

float HeightMap::get_lod_scale() const {
	return _lodder.get_split_scale();
}

void HeightMap::_notification(int p_what) {
	switch (p_what) {

		case NOTIFICATION_ENTER_TREE:
			set_process(true);
			break;

		case NOTIFICATION_ENTER_WORLD: {
			Ref<World> world = get_world();
			for_all_chunks(EnterWorldAction { world });
		} break;

		case NOTIFICATION_EXIT_WORLD:
			for_all_chunks(ExitWorldAction());
			break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			Transform world_transform = get_global_transform();
			for_all_chunks(TransformChangedAction { &world_transform });
			update_material();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			bool visible = is_visible();
			for_all_chunks(VisibilityChangedAction { visible });
		} break;

		case NOTIFICATION_PROCESS:
			_process();
			break;
	}
}

//        3
//      o---o
//    0 |   | 1
//      o---o
//        2
// Directions to go to neighbor chunks
Point2i s_dirs[4] = {
	Point2i(-1, 0), // SEAM_LEFT
	Point2i(1, 0), // SEAM_RIGHT
	Point2i(0, -1), // SEAM_BOTTOM
	Point2i(0, 1) // SEAM_TOP
};

//       7   6
//     o---o---o
//   0 |       | 5
//     o       o
//   1 |       | 4
//     o---o---o
//       2   3
//
// Directions to go to neighbor chunks of higher LOD
Point2i s_rdirs[8] = {
	Point2i(-1, 0),
	Point2i(-1, 1),
	Point2i(0, 2),
	Point2i(1, 2),
	Point2i(2, 1),
	Point2i(2, 0),
	Point2i(1, -1),
	Point2i(0, -1)
};

void HeightMap::_process() {

	// Get viewer pos
	Vector3 viewer_pos = _manual_viewer_pos;
	Viewport *viewport = get_viewport();
	if (viewport) {
		Camera *camera = viewport->get_camera();
		if (camera) {
			viewer_pos = camera->get_global_transform().origin;
		}
	}

	if(_data.is_valid())
		_lodder.update(viewer_pos);

	_updated_chunks = 0;

	// Add more chunk updates for neighboring (seams):
	// This adds updates to higher-LOD chunks around lower-LOD ones,
	// because they might not needed to update by themselves, but the fact a neighbor
	// chunk got joined or split requires them to create or revert seams
	int precount = _pending_chunk_updates.size();
	for(int i = 0; i < precount; ++i) {
		PendingChunkUpdate u = _pending_chunk_updates[i];

		// In case the chunk got split
		for(int d = 0; d < 4; ++d) {

			Point2i ncpos = u.pos + s_dirs[d];
			HeightMapChunk *nchunk = get_chunk_at(ncpos, u.lod);

			if(nchunk && nchunk->is_active()) {
				// Note: this will append elements to the array we are iterating on,
				// but we iterate only on the previous count so it should be fine
				add_chunk_update(*nchunk, ncpos, u.lod);
			}
		}

		// In case the chunk got joined
		if(u.lod > 0) {
			Point2i cpos_upper = u.pos * 2;
			int nlod = u.lod - 1;

			for(int rd = 0; rd < 8; ++rd) {

				Point2i ncpos_upper = cpos_upper + s_rdirs[rd];
				HeightMapChunk *nchunk = get_chunk_at(ncpos_upper, nlod);

				if(nchunk && nchunk->is_active()) {
					add_chunk_update(*nchunk, ncpos_upper, nlod);
				}
			}
		}
	}

	// Update chunks
	for(int i = 0; i < _pending_chunk_updates.size(); ++i) {

		PendingChunkUpdate u = _pending_chunk_updates[i];
		HeightMapChunk *chunk = get_chunk_at(u.pos, u.lod);
		ERR_FAIL_COND(chunk == NULL);
		update_chunk(*chunk, u.lod);
	}

	_pending_chunk_updates.clear();

	// DEBUG
//	if(_updated_chunks > 0) {
//		print_line(String("Remeshed {0} chunks").format(varray(_updated_chunks)));
//	}
}

void HeightMap::update_chunk(HeightMapChunk &chunk, int lod) {
	ERR_FAIL_COND(_data.is_null())

	// Check for my own seams
	int seams = 0;
	Point2i cpos = chunk.cell_origin / (CHUNK_SIZE << lod);
	Point2i cpos_lower = cpos / 2;

	// Check for lower-LOD chunks around me
	for(int d = 0; d < 4; ++d) {
		Point2i ncpos_lower = (cpos + s_dirs[d]) / 2;
		if(ncpos_lower != cpos_lower) {
			HeightMapChunk *nchunk = get_chunk_at(ncpos_lower, lod + 1);
			if(nchunk && nchunk->is_active()) {
				seams |= (1 << d);
			}
		}
	}

	Ref<Mesh> mesh = _mesher.get_chunk(lod, seams);
	chunk.set_mesh(mesh);

	++_updated_chunks;

	chunk.set_visible(is_visible());
	chunk.set_pending_update(false);

//	if (get_tree()->is_editor_hint() == false) {
//		// TODO Generate collider? Or delegate this to another node
//	}
}

void HeightMap::add_chunk_update(HeightMapChunk &chunk, Point2i pos, int lod) {

	if(chunk.is_pending_update()) {
		//print_line("Chunk update is already pending!");
		return;
	}

	// No update pending for this chunk, create one
	PendingChunkUpdate u;
	u.pos = pos;
	u.lod = lod;
	_pending_chunk_updates.push_back(u);

	chunk.set_pending_update(true);

	// TODO Neighboring chunks might need an update too because of normals and seams being updated
}

void HeightMap::set_area_dirty(Point2i origin_in_cells, Point2i size_in_cells) {

	Point2i cpos0 = origin_in_cells / CHUNK_SIZE;
	Point2i csize = (origin_in_cells + size_in_cells - Point2i(1,1)) / CHUNK_SIZE + Point2i(1,1);

	// For each lod
	for (int lod = 0; lod < _chunk_cache.size(); ++lod) {

		// Get grid and chunk size
		const HashMap<Point2i, HeightMapChunk*> &grid = _chunk_cache[lod];
		int s = _lodder.get_lod_size(lod);

		// Convert rect into this lod's coordinates:
		// Pick min and max (included), divide them, then add 1 to max so it's excluded again
		Point2i min = cpos0 / s;
		Point2i max = (cpos0 + csize - Point2i(1 ,1)) / s + Point2i(1, 1);

		// Find which chunks are within
		Point2i cpos;
		for (cpos.y = min.y; cpos.y < max.y; ++cpos.y) {
			for (cpos.x = min.x; cpos.x < max.x; ++cpos.x) {

				HeightMapChunk *const *chunk_ptr = grid.getptr(cpos);

				if (chunk_ptr) {
					HeightMapChunk *chunk = *chunk_ptr;

					if(chunk->is_active()) {
						add_chunk_update(*chunk, cpos, lod);
					}
				}
			}
		}
	}
}

// Called when a chunk is needed to be seen
HeightMapChunk *HeightMap::_make_chunk_cb(Point2i cpos, int lod) {

	HeightMapChunk *chunk = get_chunk_at(cpos, lod);

	if(chunk == NULL) {

		// This is the first time this chunk is required at this lod, generate it
		int lod_factor = _lodder.get_lod_size(lod);
		Point2i origin_in_cells = cpos * CHUNK_SIZE * lod_factor;
		chunk = memnew(HeightMapChunk(this, origin_in_cells, _material));
		_chunk_cache[lod][cpos] = chunk;

	}

	// Make sure it gets updated
	add_chunk_update(*chunk, cpos, lod);

	chunk->set_active(true);

	return chunk;
}

// Called when a chunk is no longer seen
void HeightMap::_recycle_chunk_cb(HeightMapChunk *chunk) {
	chunk->set_visible(false);
	chunk->set_active(false);
}

Point2i HeightMap::local_pos_to_cell(Vector3 local_pos) const {
	return Point2i(
			static_cast<int>(local_pos.x),
			static_cast<int>(local_pos.z));
}

static float get_height_or_default(const Image &im, Point2i pos) {
	if(pos.x < 0 || pos.y < 0|| pos.x >= im.get_width() || pos.y >= im.get_height())
		return 0;
	return im.get_pixel(pos.x, pos.y).r;
}

bool HeightMap::cell_raycast(Vector3 origin_world, Vector3 dir_world, Point2i &out_cell_pos) {

	if(_data.is_null())
		return false;

	Ref<Image> heights_ref = _data->get_image(HeightMapData::CHANNEL_HEIGHT);
	if(heights_ref.is_null())
		return false;
	Image &heights = **heights_ref;

	Transform to_local = get_global_transform().affine_inverse();
	Vector3 origin = to_local.xform(origin_world);
	Vector3 dir = to_local.xform(dir_world);

	LockImage lock(heights_ref);

	if (origin.y < get_height_or_default(heights, local_pos_to_cell(origin))) {
		// Below
		return false;
	}

	float unit = 1;
	float d = 0;
	const real_t max_distance = 800;
	Vector3 pos = origin;

	// Slow, but enough for edition
	// TODO Could be optimized with a form of binary search
	while (d < max_distance) {
		pos += dir * unit;
		if (get_height_or_default(heights, local_pos_to_cell(pos)) > pos.y) {
			out_cell_pos = local_pos_to_cell(pos - dir * unit);
			return true;
		}
		d += unit;
	}

	return false;
}

void HeightMap::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_data:HeightMapData"), &HeightMap::get_data);
	ClassDB::bind_method(D_METHOD("set_data", "data:HeightMapData"), &HeightMap::set_data);

	ClassDB::bind_method(D_METHOD("get_custom_shader:Shader"), &HeightMap::get_custom_shader);
	ClassDB::bind_method(D_METHOD("set_custom_shader", "shader:Shader"), &HeightMap::set_custom_shader);

	ClassDB::bind_method(D_METHOD("is_collision_enabled"), &HeightMap::is_collision_enabled);
	ClassDB::bind_method(D_METHOD("set_collision_enabled", "enabled"), &HeightMap::set_collision_enabled);

	ClassDB::bind_method(D_METHOD("set_lod_scale", "scale"), &HeightMap::set_lod_scale);
	ClassDB::bind_method(D_METHOD("get_lod_scale"), &HeightMap::get_lod_scale);

	ClassDB::bind_method(D_METHOD("_on_data_resolution_changed"), &HeightMap::_on_data_resolution_changed);
	ClassDB::bind_method(D_METHOD("_on_data_region_changed", "x", "y", "w", "h", "c"), &HeightMap::_on_data_region_changed);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data", PROPERTY_HINT_RESOURCE_TYPE, "HeightMapData"), "set_data", "get_data");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_shader", PROPERTY_HINT_RESOURCE_TYPE, "Shader"), "set_custom_shader", "get_custom_shader");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_enabled"), "set_collision_enabled", "is_collision_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "lod_scale"), "set_lod_scale", "get_lod_scale");
}

// Callbacks configured for QuadTreeLod

HeightMapChunk *HeightMap::s_make_chunk_cb(void *context, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap *>(context);
	return self->_make_chunk_cb(origin, lod);
}

void HeightMap::s_recycle_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap *>(context);
	self->_recycle_chunk_cb(chunk);
}


