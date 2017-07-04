#include "height_map_chunk.h"

//int s_chunk_count = 0;

HeightMapChunk::HeightMapChunk(Spatial *p_parent, Point2i p_cell_pos, Ref<Material> p_material) {
	cell_origin = p_cell_pos;

	VisualServer &vs = *VisualServer::get_singleton();

	_mesh_instance = vs.instance_create();

	parent_transform_changed(p_parent->get_global_transform());

	if (p_material.is_valid()) {
		vs.instance_geometry_set_material_override(_mesh_instance, p_material->get_rid());
	}

	Ref<World> world = p_parent->get_world();
	if (world.is_valid()) {
		vs.instance_set_scenario(_mesh_instance, world->get_scenario());
	}

	// TODO Is this needed?
	vs.instance_set_visible(_mesh_instance, true);

	_visible = true;
	_active = true;
	_dirty = true;
	_pending_update = false;

//	++s_chunk_count;
//	print_line(String("Chunk count: ") + String::num(s_chunk_count));
}

HeightMapChunk::~HeightMapChunk() {
	VisualServer &vs = *VisualServer::get_singleton();
	if (_mesh_instance.is_valid()) {
		vs.free(_mesh_instance);
		_mesh_instance = RID();
	}
	//	if(collider)
	//		collider->queue_delete();
//	--s_chunk_count;
//	print_line(String("Chunk count: ") + String::num(s_chunk_count));
}

void HeightMapChunk::enter_world(World &world) {
	ERR_FAIL_COND(_mesh_instance.is_valid() == false);
	VisualServer &vs = *VisualServer::get_singleton();
	vs.instance_set_scenario(_mesh_instance, world.get_scenario());
}

void HeightMapChunk::exit_world() {
	ERR_FAIL_COND(_mesh_instance.is_valid() == false);
	VisualServer &vs = *VisualServer::get_singleton();
	vs.instance_set_scenario(_mesh_instance, RID());
}

void HeightMapChunk::parent_transform_changed(const Transform &parent_transform) {
	ERR_FAIL_COND(_mesh_instance.is_valid() == false);
	VisualServer &vs = *VisualServer::get_singleton();
	Transform local_transform(Basis(), Vector3(cell_origin.x, 0, cell_origin.y));
	Transform world_transform = parent_transform * local_transform;
	vs.instance_set_transform(_mesh_instance, world_transform);
}

void HeightMapChunk::set_mesh(Ref<Mesh> mesh) {
	ERR_FAIL_COND(_mesh_instance.is_valid() == false);
	VisualServer &vs = *VisualServer::get_singleton();
	vs.instance_set_base(_mesh_instance, mesh.is_valid() ? mesh->get_rid() : RID());
	_mesh = mesh;
}

void HeightMapChunk::set_material(Ref<Material> material) {
	ERR_FAIL_COND(_mesh_instance.is_valid() == false);
	VisualServer &vs = *VisualServer::get_singleton();
	vs.instance_geometry_set_material_override(_mesh_instance, material.is_valid() ? material->get_rid() : RID());
}

void HeightMapChunk::set_visible(bool visible) {
	ERR_FAIL_COND(_mesh_instance.is_valid() == false);
	VisualServer &vs = *VisualServer::get_singleton();
	vs.instance_set_visible(_mesh_instance, visible);
}
