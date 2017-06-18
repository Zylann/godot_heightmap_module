#ifndef HEIGHT_MAP_CHUNK_H
#define HEIGHT_MAP_CHUNK_H

#include <math/math_2d.h>
#include <scene/3d/mesh_instance.h>

// Container for chunk objects
class HeightMapChunk {
public:
	Point2i cell_origin;

	HeightMapChunk(Spatial *p_parent, Point2i p_cell_pos, Ref<Material> p_material);
	~HeightMapChunk();

	void create(Point2i pos, Ref<Material> material);
	void set_mesh(Ref<Mesh> mesh);
	void clear();
	void set_material(Ref<Material> material);
	void enter_world(World &world);
	void exit_world();
	void parent_transform_changed(const Transform &parent_transform);
	void set_visible(bool visible);

private:
	RID _mesh_instance;
	// Need to keep a reference so that the mesh RID doesn't get freed
	Ref<Mesh> _mesh;
};

#endif // HEIGHT_MAP_CHUNK_H
