#include "height_map.h"
#include <scene/3d/camera.h>

#define DEFAULT_RESOLUTION 256

// Chunk actions that don't need private access to HeightMap
static void s_set_material_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
static void s_delete_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
static void s_enter_world_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
static void s_exit_world_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
static void s_transform_changed_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
static void s_visibility_changed_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);


HeightMap::HeightMap() {
	set_notify_transform(true);

	_collision_enabled = true;

	_lodder.make_func = s_make_chunk_cb;
	_lodder.recycle_func = s_recycle_chunk_cb;

	set_resolution(DEFAULT_RESOLUTION);

	_data.update_all_normals();

	_lodder.create_from_sizes(CHUNK_SIZE, _data.size().x);
}

HeightMap::~HeightMap() {
	// Free chunks
	_lodder.for_all_chunks(s_delete_chunk_cb, this);
}

void HeightMap::set_material(Ref<Material> p_material) {
	if(_material != p_material) {
		_material = p_material;
		_lodder.for_all_chunks(s_set_material_cb, this);
	}
}

void HeightMap::set_collision_enabled(bool enabled) {
	_collision_enabled = enabled;
	// TODO Update chunks
}

void HeightMap::set_resolution(int p_res) {

	if (p_res < CHUNK_SIZE)
		p_res = CHUNK_SIZE;

	// Power of two is important for LOD.
	// Also, grid data is off by one,
	// because for an even number of quads you need an odd number of vertices
	p_res = nearest_power_of_2(p_res-1) + 1;

	if (p_res != get_resolution()) {
		_data.resize(p_res);
		_lodder.create_from_sizes(CHUNK_SIZE, _data.size().x);
	}
}

int HeightMap::get_resolution() const {
	return _data.size().x;
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
			_lodder.for_all_chunks(s_enter_world_cb, *world);
		} break;

		case NOTIFICATION_EXIT_WORLD:
			_lodder.for_all_chunks(s_exit_world_cb, NULL);
			break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			Transform world_transform = get_global_transform();
			_lodder.for_all_chunks(s_transform_changed_cb, &world_transform);
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			bool visible = is_visible();
			_lodder.for_all_chunks(s_visibility_changed_cb, &visible);
		} break;

		// TODO TEST, WONT REMAIN HERE
		case NOTIFICATION_READY:
		{
			Point2i size = _data.size();
			Point2i pos;
			for (pos.y = 0; pos.y < size.y; ++pos.y) {
				for (pos.x = 0; pos.x < size.x; ++pos.x) {
					float h = 2.0 * (Math::cos(pos.x * 0.2) + Math::sin(pos.y * 0.2));
					_data.heights.set(pos, h);
				}
			}
		}
		break;

		case NOTIFICATION_PROCESS:
			_process();
			break;
	}
}

void HeightMap::_process() {

	// Get viewer pos
	Vector3 viewer_pos;
	Viewport *viewport = get_viewport();
	if (viewport) {
		Camera *camera = viewport->get_camera();
		if (camera) {
			viewer_pos = camera->get_global_transform().origin;
		}
	}

	// Update LOD container
	_lodder.update(viewer_pos, this);

	// Update chunks
	for (int lod = 0; lod < _pending_chunk_updates.size(); ++lod) {
		HashMap<Point2i, bool> &pending_chunks = _pending_chunk_updates[lod];
		if (pending_chunks.size() != 0) {

			const Point2i *key = NULL;
			while (key = pending_chunks.next(key)) {

				HeightMapChunk *chunk = NULL;
				_lodder.try_get_chunk_at(chunk, *key, lod);
				ERR_FAIL_COND(chunk == NULL);
				update_chunk(*chunk, lod);
			}
		}
		pending_chunks.clear();
	}
}

void HeightMap::update_chunk(HeightMapChunk &chunk, int lod) {

	HeightMapMesher::Params mesher_params;
	mesher_params.lod = lod;
	mesher_params.origin = chunk.cell_origin;
	mesher_params.size = Point2i(CHUNK_SIZE, CHUNK_SIZE);
	mesher_params.smooth = true; // TODO Implement this option

	if (mesher_params.smooth) {
		Point2i cell_size = mesher_params.size;
		cell_size.x <<= lod;
		cell_size.y <<= lod;

		// Padding is needed because normals are calculated using neighboring,
		// so a change in height X also requires normals in X-1 and X+1 to be updated
		Point2i pad(1,1);

		_data.update_normals(chunk.cell_origin-pad, cell_size+2*pad);
	}

	Ref<Mesh> mesh = _mesher.make_chunk(mesher_params, _data);
	chunk.set_mesh(mesh);

	if (get_tree()->is_editor_hint() == false) {
		// TODO Generate collider
	}
}

HeightMapChunk *HeightMap::_make_chunk_cb(Point2i origin, int lod) {
	int lod_size = _lodder.get_lod_size(lod);
	Point2i origin_in_cells = origin * CHUNK_SIZE * lod_size;
	HeightMapChunk *chunk = memnew(HeightMapChunk(this, origin_in_cells, _material));

	set_chunk_dirty(origin, lod);

	return chunk;
}

void HeightMap::set_chunk_dirty(Point2i pos, int lod) {
	if (_pending_chunk_updates.size() <= lod)
		_pending_chunk_updates.resize(lod + 1);
	HashMap<Point2i, bool> &pending_chunks = _pending_chunk_updates[lod];
	// Note: if the chunk has already been made dirty,
	// nothing will change and the chunk will be updated only once when the update step comes.
	pending_chunks[pos] = true;
	// TODO Neighboring chunks might need an update too because of normals and seams being updated
}

void HeightMap::set_area_dirty(Point2i origin_in_cells, Point2i size_in_cells) {
	Point2i cmin = origin_in_cells / CHUNK_SIZE;
	Point2i csize = size_in_cells / CHUNK_SIZE + Point2i(1, 1);

	// TODO take undo/redo into account here?

	_lodder.for_chunks_in_rect(s_set_chunk_dirty_cb, cmin, csize, this);
}

void HeightMap::_recycle_chunk_cb(HeightMapChunk *chunk) {
	memdelete(chunk);
}

Point2i HeightMap::local_pos_to_cell(Vector3 local_pos) const {
	return Point2i(
			static_cast<int>(local_pos.x),
			static_cast<int>(local_pos.z));
}

bool HeightMap::cell_raycast(Vector3 origin_world, Vector3 dir_world, Point2i &out_cell_pos) {

	Transform to_local = get_global_transform().affine_inverse();
	Vector3 origin = to_local.xform(origin_world);
	Vector3 dir = to_local.xform(dir_world);

	if (origin.y < _data.heights.get_or_default(local_pos_to_cell(origin))) {
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
		if (_data.heights.get_or_default(local_pos_to_cell(pos)) > pos.y) {
			out_cell_pos = local_pos_to_cell(pos - dir * unit);
			return true;
		}
		d += unit;
	}

	return false;
}

void HeightMap::_bind_methods() {

	// TODO API subject to change as the module heads to a more efficient implementation

	ClassDB::bind_method(D_METHOD("get_material:Material"), &HeightMap::get_material);
	ClassDB::bind_method(D_METHOD("set_material", "material:Material"), &HeightMap::set_material);

	ClassDB::bind_method(D_METHOD("is_collision_enabled"), &HeightMap::is_collision_enabled);
	ClassDB::bind_method(D_METHOD("set_collision_enabled", "enabled"), &HeightMap::set_collision_enabled);

	ClassDB::bind_method(D_METHOD("set_resolution", "resolution"), &HeightMap::set_resolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &HeightMap::get_resolution);

	ClassDB::bind_method(D_METHOD("set_lod_scale", "scale"), &HeightMap::set_lod_scale);
	ClassDB::bind_method(D_METHOD("get_lod_scale"), &HeightMap::get_lod_scale);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_enabled"), "set_collision_enabled", "is_collision_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "resolution"), "set_resolution", "get_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "lod_scale"), "set_lod_scale", "get_lod_scale");
}

HeightMapChunk *HeightMap::s_make_chunk_cb(void *context, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap *>(context);
	return self->_make_chunk_cb(origin, lod);
}

void HeightMap::s_recycle_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap *>(context);
	self->_recycle_chunk_cb(chunk);
}

void HeightMap::s_set_chunk_dirty_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap *>(context);
	self->set_chunk_dirty(origin, lod);
}

void s_set_material_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap *>(context);
	chunk->set_material(self->get_material());
}

void s_delete_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	memdelete(chunk);
}

void s_enter_world_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	World *world = reinterpret_cast<World *>(context);
	chunk->enter_world(*world);
}

void s_exit_world_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	chunk->exit_world();
}

void s_transform_changed_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	Transform *transform = reinterpret_cast<Transform*>(context);
	chunk->parent_transform_changed(*transform);
}

void s_visibility_changed_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod) {
	bool *visible = reinterpret_cast<bool*>(context);
	chunk->set_visible(*visible);
}

