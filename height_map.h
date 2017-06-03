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

protected:
	void _notification(int p_what);

private:
	void _process();
	HeightMapChunk *_make_chunk_cb(Point2i origin, int lod);
	void _recycle_chunk_cb(HeightMapChunk *chunk);

	void update_chunk(HeightMapChunk & chunk, int lod);

	static void _bind_methods();
	static HeightMapChunk *s_make_chunk_cb(void *context, Point2i origin, int lod);
	static void s_recycle_chunk_cb(void *context, HeightMapChunk *chunk);

private:
	struct PendingChunkUpdate {
		Point2i pos;
		int lod;
	};

	Ref<Material> _material;
	bool _collision_enabled;
	HeightMapData _data;
	HeightMapMesher _mesher;
	QuadTreeLod<HeightMapChunk*> _lodder;
	Vector<PendingChunkUpdate> _pending_chunk_updates;
};


#endif // HEIGHT_MAP_H

