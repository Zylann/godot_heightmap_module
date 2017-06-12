#ifndef HEIGHT_MAP_CHUNK_H
#define HEIGHT_MAP_CHUNK_H

#include <math/math_2d.h>
#include <scene/3d/mesh_instance.h>

// Container for chunk objects
class HeightMapChunk {
public:
	Point2i cell_origin;

	HeightMapChunk(Node *parent);

	void create(Point2i pos, Ref<Material> material);
	void set_mesh(Ref<Mesh> mesh);
	void clear();
	void set_material(Ref<Material> material);

private:
	// TODO In the future it won't rely on nodes anymore,
	// you shouldn't assume chunks use nodes so it's private
	MeshInstance *get_mesh_instance();

private:
	Node *_parent;
	NodePath _mesh_instance_path;
};

#endif // HEIGHT_MAP_CHUNK_H
