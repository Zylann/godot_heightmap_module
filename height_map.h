#ifndef HEIGHT_MAP_H
#define HEIGHT_MAP_H

#include <scene/3d/spatial.h>
#include "height_map_data.h"
#include "height_map_chunk.h"
#include "height_map_mesher.h"
#include "quad_tree_lod.h"


// Heightmap-based 3D terrain
class HeightMap : public Spatial {
	GDCLASS(HeightMap, Spatial)
public:
	//static const int CHUNK_SIZE = 16;
	// Workaround because GCC doesn't links the line above properly
	enum { CHUNK_SIZE = 16 };

	HeightMap();
	~HeightMap();

	void set_material(Ref<Material> p_material);
	inline Ref<Material> get_material() const { return _material; }

	void set_collision_enabled(bool enabled);
	inline bool is_collision_enabled() const { return _collision_enabled; }

	void set_resolution(int p_res);
	int get_resolution() const;

	void set_area_dirty(Point2i origin_in_cells, Point2i size_in_cells);
	bool cell_raycast(Vector3 origin_world, Vector3 dir_world, Point2i &out_cell_pos);

	HeightMapData &get_data() { return _data; }

protected:
	void _notification(int p_what);

private:
	void _process();
	HeightMapChunk *_make_chunk_cb(Point2i origin, int lod);
	void _recycle_chunk_cb(HeightMapChunk *chunk);

	void set_chunk_dirty(Point2i pos, int lod);
	void update_chunk(HeightMapChunk & chunk, int lod);

	Point2i local_pos_to_cell(Vector3 local_pos) const;

	static void _bind_methods();

	static HeightMapChunk *s_make_chunk_cb(void *context, Point2i origin, int lod);
	static void s_recycle_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
	static void s_delete_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);
	static void s_set_chunk_dirty_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);

private:
	Ref<Material> _material;
	bool _collision_enabled;
	HeightMapData _data;
	HeightMapMesher _mesher;
	QuadTreeLod<HeightMapChunk*> _lodder;

	// Pending chunk updates indexed by lod
	// Note: needed Vector<HashSet<Point2i>> but HashSet doesn't exist so I use a placeholder bool.
	Vector<HashMap<Point2i,bool> > _pending_chunk_updates;
};


#endif // HEIGHT_MAP_H

