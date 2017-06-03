#include "height_map_chunk.h"

HeightMapChunk::HeightMapChunk(Node *parent) {
	_parent = parent;
}

void HeightMapChunk::create(Point2i cell_pos, Ref<Material> material) {
	cell_origin = cell_pos;

	MeshInstance *mesh_instance = get_mesh_instance();
	if(mesh_instance == NULL) {
		mesh_instance = memnew(MeshInstance);
	}

	//mesh_instance.set_name("chunk_" + str(x) + "_" + str(y))
	mesh_instance->set_translation(Vector3(cell_pos.x, 0, cell_pos.y));
	if(material.is_valid()) {
		mesh_instance->set_material_override(material);
	}

	mesh_instance->show();
	if(mesh_instance->is_inside_tree() == false) {
		_parent->add_child(mesh_instance);
		_mesh_instance_path = mesh_instance->get_path();
	}
}

void HeightMapChunk::clear() {
	MeshInstance *mesh_instance = get_mesh_instance();
	if(mesh_instance)
		mesh_instance->queue_delete();
//	if(collider)
//		collider->queue_delete();
}

void HeightMapChunk::set_mesh(Ref<Mesh> mesh) {
	MeshInstance *mesh_instance = get_mesh_instance();
	if(mesh_instance)
		mesh_instance->set_mesh(mesh);
}

MeshInstance *HeightMapChunk::get_mesh_instance() {
	if (_mesh_instance_path.is_empty())
		return NULL;
	Node * n = _parent->get_node(_mesh_instance_path);
	if (n == NULL)
		return NULL;
	return n->cast_to<MeshInstance>();
}



