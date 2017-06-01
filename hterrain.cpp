#include <scene/3d/camera.h>
#include "hterrain.h"

HTerrain::HTerrain() {
	_collision_enabled = true;

	// TODO TEST
	set_resolution(4096);
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

HTerrain::~HTerrain() {

}

void HTerrain::set_material(Ref<Material> p_material) {
	_material = p_material;
	// TODO Update chunks
}

void HTerrain::set_collision_enabled(bool enabled) {
	_collision_enabled = enabled;
	// TODO Update chunks
}

void HTerrain::set_resolution(int p_res) {

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

int HTerrain::get_resolution() const {
	return _data.size().x;
}

void HTerrain::_notification(int p_what) {
	switch(p_what) {

	case NOTIFICATION_ENTER_TREE:
		set_process(true);
		break;

	case NOTIFICATION_PROCESS:
		_process();
		break;
	}
}

void HTerrain::_process() {

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
	if(_pending_chunk_updates.size() != 0)
		print_line(String("Updating {0} chunks").format(varray(_pending_chunk_updates.size())));
	for(int i = 0; i < _pending_chunk_updates.size(); ++i) {
		PendingChunkUpdate u = _pending_chunk_updates[i];
		HTerrainChunk *chunk = NULL;
		_lodder.try_get_chunk_at(chunk, u.pos, u.lod);
		ERR_FAIL_COND(chunk == NULL);
		update_chunk(*chunk, u.lod);
	}
	_pending_chunk_updates.clear();
}

void HTerrain::update_chunk(HTerrainChunk & chunk, int lod) {

	HTerrainMesher::Params mesher_params;
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

HTerrainChunk *HTerrain::_make_chunk_cb(Point2i origin, int lod) {
	int lod_size = _lodder.get_lod_size(lod);
	Point2i cell_origin = origin * CHUNK_SIZE * lod_size;
	HTerrainChunk *chunk = memnew(HTerrainChunk(this));
	chunk->create(cell_origin, _material);

	PendingChunkUpdate u;
	u.lod = lod;
	u.pos = origin;
	_pending_chunk_updates.push_back(u);

	return chunk;
}

void HTerrain::_recycle_chunk_cb(HTerrainChunk *chunk) {
	chunk->clear();
	memdelete(chunk);
}

void HTerrain::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_material:Material"), &HTerrain::get_material);
	ClassDB::bind_method(D_METHOD("set_material(material:Material)"), &HTerrain::set_material);

	ClassDB::bind_method(D_METHOD("is_collision_enabled"), &HTerrain::is_collision_enabled);
	ClassDB::bind_method(D_METHOD("set_collision_enabled"), &HTerrain::set_collision_enabled);
}

// static
HTerrainChunk *HTerrain::s_make_chunk_cb(void *context, Point2i origin, int lod) {
	HTerrain *self = reinterpret_cast<HTerrain*>(context);
	return self->_make_chunk_cb(origin, lod);
}

// static
void HTerrain::s_recycle_chunk_cb(void *context, HTerrainChunk *chunk) {
	HTerrain *self = reinterpret_cast<HTerrain*>(context);
	self->_recycle_chunk_cb(chunk);
}

