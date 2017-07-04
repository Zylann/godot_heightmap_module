#include "height_map.h"
#include <scene/3d/camera.h>

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
}

HeightMap::HeightMap() {

	set_notify_transform(true);
	_collision_enabled = true;
	_lodder.set_callbacks(s_make_chunk_cb, s_recycle_chunk_cb, this);
	_remeshed_chunks = 0;
	//print_line("HeightMap()");
}

HeightMap::~HeightMap() {
	clear_chunk_cache();
	//print_line("~HeightMap()");
}

void HeightMap::clear_chunk_cache() {
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

	clear_chunk_cache();

	if(_data.is_valid()) {

#ifdef TOOLS_ENABLED
		// This is a small UX improvement so that the user sees a default terrain
		if(data->get_resolution() == 0) {
			data->load_default();
		}
#endif
		_data->connect(HeightMapData::SIGNAL_RESOLUTION_CHANGED, this, "_on_data_resolution_changed");
		_data->connect(HeightMapData::SIGNAL_REGION_CHANGED, this, "_on_data_region_changed");
		_on_data_resolution_changed();

	} else {
		_lodder.clear();
	}
}

void HeightMap::_on_data_resolution_changed() {
	clear_chunk_cache();
	_pending_chunk_updates.clear();
	_lodder.create_from_sizes(CHUNK_SIZE, _data->get_resolution());
	_chunk_cache.resize(_lodder.get_lod_count() + 1);
}

void HeightMap::_on_data_region_changed(int min_x, int min_y, int max_x, int max_y) {	
	//print_line(String("_on_data_region_changed {0}, {1}, {2}, {3}").format(varray(min_x, min_y, max_x, max_y)));
	set_area_dirty(Point2i(min_x, min_y), Point2i(max_x - min_x, max_y - min_y));
}

void HeightMap::set_material(Ref<Material> p_material) {
	if (_material != p_material) {
		_material = p_material;
		for_all_chunks(SetMaterialAction { p_material });
	}
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

	_remeshed_chunks = 0;

	// Update chunks
	for(int i = 0; i < _pending_chunk_updates.size(); ++i) {

		PendingChunkUpdate u = _pending_chunk_updates[i];
		HeightMapChunk *chunk = get_chunk_at(u.pos, u.lod);
		ERR_FAIL_COND(chunk == NULL);
		update_chunk(*chunk, u.lod);
	}

	_pending_chunk_updates.clear();

	// DEBUG
	if(_remeshed_chunks > 0) {
		print_line(String("Remeshed {0} chunks").format(varray(_remeshed_chunks)));
	}
}

void HeightMap::update_chunk(HeightMapChunk &chunk, int lod) {
	ERR_FAIL_COND(_data.is_null())

	if(chunk.is_dirty()) {

		HeightMapMesher::Params mesher_params;
		mesher_params.lod = lod;
		mesher_params.origin = chunk.cell_origin;
		mesher_params.size = Point2i(CHUNK_SIZE, CHUNK_SIZE);
		mesher_params.smooth = true; // TODO Implement this option

		Ref<Mesh> mesh = _mesher.make_chunk(mesher_params, **_data);
		chunk.set_mesh(mesh);
		chunk.set_dirty(false);

		++_remeshed_chunks;
	}

	// TODO Update seams

	chunk.set_visible(is_visible());
	chunk.set_pending_update(false);

	if (get_tree()->is_editor_hint() == false) {
		// TODO Generate collider? Or delegate this to another node
	}
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

					// Make chunk dirty
					chunk->set_dirty(true);
					if(chunk->is_active()) {
						add_chunk_update(*chunk, cpos, lod);
					}
				}
			}
		}
	}
}

// Called when a chunk is needed to be seen
HeightMapChunk *HeightMap::_make_chunk_cb(Point2i origin, int lod) {

	HeightMapChunk *chunk = get_chunk_at(origin, lod);

	if(chunk == NULL) {

		// This is the first time this chunk is required at this lod, generate it
		int lod_factor = _lodder.get_lod_size(lod);
		Point2i origin_in_cells = origin * CHUNK_SIZE * lod_factor;
		chunk = memnew(HeightMapChunk(this, origin_in_cells, _material));
		chunk->set_dirty(true);
		_chunk_cache[lod][origin] = chunk;

	}

	// Make sure it gets updated
	add_chunk_update(*chunk, origin, lod);

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

bool HeightMap::cell_raycast(Vector3 origin_world, Vector3 dir_world, Point2i &out_cell_pos) {
	if(_data.is_null())
		return false;

	Transform to_local = get_global_transform().affine_inverse();
	Vector3 origin = to_local.xform(origin_world);
	Vector3 dir = to_local.xform(dir_world);

	if (origin.y < _data->heights.get_or_default(local_pos_to_cell(origin))) {
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
		if (_data->heights.get_or_default(local_pos_to_cell(pos)) > pos.y) {
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

	ClassDB::bind_method(D_METHOD("get_material:Material"), &HeightMap::get_material);
	ClassDB::bind_method(D_METHOD("set_material", "material:Material"), &HeightMap::set_material);

	ClassDB::bind_method(D_METHOD("is_collision_enabled"), &HeightMap::is_collision_enabled);
	ClassDB::bind_method(D_METHOD("set_collision_enabled", "enabled"), &HeightMap::set_collision_enabled);

	ClassDB::bind_method(D_METHOD("set_lod_scale", "scale"), &HeightMap::set_lod_scale);
	ClassDB::bind_method(D_METHOD("get_lod_scale"), &HeightMap::get_lod_scale);

	ClassDB::bind_method(D_METHOD("_on_data_resolution_changed"), &HeightMap::_on_data_resolution_changed);
	ClassDB::bind_method(D_METHOD("_on_data_region_changed"), &HeightMap::_on_data_region_changed);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data", PROPERTY_HINT_RESOURCE_TYPE, "HeightMapData"), "set_data", "get_data");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");
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


