#include <scene/3d/camera.h>
#include "height_map.h"

#define DEFAULT_RESOLUTION 256


HeightMap::HeightMap() {
	_collision_enabled = true;

	// TODO TEST
	set_resolution(DEFAULT_RESOLUTION);
	Point2i size = _data.size();
	Point2i pos;
	for(pos.y = 0; pos.y < size.y; ++pos.y) {
		for(pos.x = 0; pos.x < size.x; ++pos.x) {
			float h = 2.0 * (Math::cos(pos.x*0.2) + Math::sin(pos.y*0.2));
			_data.heights.set(pos, h);
		}
	}

	_data.update_all_normals();

	_lodder.create_from_sizes(CHUNK_SIZE, _data.size().x);
	_lodder.make_func = s_make_chunk_cb;
	_lodder.recycle_func = s_recycle_chunk_cb;
}

HeightMap::~HeightMap() {

}

void HeightMap::set_material(Ref<Material> p_material) {
	_material = p_material;
	// TODO Update chunks
}

void HeightMap::set_collision_enabled(bool enabled) {
	_collision_enabled = enabled;
	// TODO Update chunks
}

void HeightMap::set_resolution(int p_res) {

	if(p_res < CHUNK_SIZE)
		p_res = CHUNK_SIZE;

	// Power of two is important for LOD.
	// Also, grid data is off by one,
	// because for an even number of quads you need an odd number of vertices
	p_res = nearest_power_of_2(p_res) + 1;

	if(p_res != get_resolution()) {
		_data.resize(p_res);
		// TODO Update chunks
	}
}

int HeightMap::get_resolution() const {
	return _data.size().x;
}

void HeightMap::_notification(int p_what) {
	switch(p_what) {

	case NOTIFICATION_ENTER_TREE:
		set_process(true);
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
	if(viewport) {
		Camera *camera = viewport->get_camera();
		if(camera) {
			viewer_pos = camera->get_global_transform().origin;
		}
	}

	// Update LOD container
	_lodder.update(viewer_pos, this);

	// Update chunks
	for(int lod = 0; lod < _pending_chunk_updates.size(); ++lod) {
		HashMap<Point2i,bool> & pending_chunks = _pending_chunk_updates[lod];
		if(pending_chunks.size() != 0) {

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

void HeightMap::update_chunk(HeightMapChunk & chunk, int lod) {

	HeightMapMesher::Params mesher_params;
	mesher_params.lod = lod;
	mesher_params.origin = chunk.cell_origin;
	mesher_params.size = Point2i(CHUNK_SIZE, CHUNK_SIZE);
	mesher_params.smooth = true; // TODO Implement this option

	Ref<Mesh> mesh = _mesher.make_chunk(mesher_params, _data);
	chunk.set_mesh(mesh);

	if(get_tree()->is_editor_hint() == false) {
		// TODO Generate collider
	}
}

HeightMapChunk *HeightMap::_make_chunk_cb(Point2i origin, int lod) {
	int lod_size = _lodder.get_lod_size(lod);
	Point2i origin_in_cells = origin * CHUNK_SIZE * lod_size;
	HeightMapChunk *chunk = memnew(HeightMapChunk(this));
	chunk->create(origin_in_cells, _material);

	set_chunk_dirty(origin, lod);

	return chunk;
}

void HeightMap::set_chunk_dirty(Point2i pos, int lod) {
	if(_pending_chunk_updates.size() <= lod)
		_pending_chunk_updates.resize(lod+1);
	HashMap<Point2i,bool> &pending_chunks = _pending_chunk_updates[lod];
	// Note: if the chunk has already been made dirty,
	// nothing will change and the chunk will be updated only once when the update step comes.
	pending_chunks[pos] = true;
}

void HeightMap::_recycle_chunk_cb(HeightMapChunk *chunk) {
	chunk->clear();
	memdelete(chunk);
}

void HeightMap::_bind_methods() {

	// TODO API subject to change as the module heads to a more efficient implementation

	ClassDB::bind_method(D_METHOD("get_material:Material"), &HeightMap::get_material);
	ClassDB::bind_method(D_METHOD("set_material(material:Material)"), &HeightMap::set_material);

	ClassDB::bind_method(D_METHOD("is_collision_enabled"), &HeightMap::is_collision_enabled);
	ClassDB::bind_method(D_METHOD("set_collision_enabled"), &HeightMap::set_collision_enabled);

	ClassDB::bind_method(D_METHOD("set_resolution"), &HeightMap::set_resolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &HeightMap::get_resolution);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material"), "set_material", "get_material");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_enabled"), "set_collision_enabled", "is_collision_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "resolution"), "set_resolution", "get_resolution");
}

// static
HeightMapChunk *HeightMap::s_make_chunk_cb(void *context, Point2i origin, int lod) {
	HeightMap *self = reinterpret_cast<HeightMap*>(context);
	return self->_make_chunk_cb(origin, lod);
}

// static
void HeightMap::s_recycle_chunk_cb(void *context, HeightMapChunk *chunk) {
	HeightMap *self = reinterpret_cast<HeightMap*>(context);
	self->_recycle_chunk_cb(chunk);
}

