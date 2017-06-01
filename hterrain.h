#ifndef HTERRAIN_H
#define HTERRAIN_H

#include <scene/3d/spatial.h>
#include "hterrain_data.h"
#include "hterrain_chunk.h"
#include "hterrain_mesher.h"
#include "quad_tree_lod.h"


// Heightmap-based 3D terrain
class HTerrain : public Spatial {
	GDCLASS(HTerrain, Spatial)
public:
	//static const int CHUNK_SIZE = 16;
	// Workaround because GCC doesn't links the line above properly
	enum { CHUNK_SIZE = 16 };

	HTerrain();
	~HTerrain();

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
	HTerrainChunk *_make_chunk_cb(Point2i origin, int lod);
	void _recycle_chunk_cb(HTerrainChunk *chunk);

	void update_chunk(HTerrainChunk & chunk, int lod);

	static void _bind_methods();
	static HTerrainChunk *s_make_chunk_cb(void *context, Point2i origin, int lod);
	static void s_recycle_chunk_cb(void *context, HTerrainChunk *chunk);

private:
	struct PendingChunkUpdate {
		Point2i pos;
		int lod;
	};

	Ref<Material> _material;
	bool _collision_enabled;
	HTerrainData _data;
	HTerrainMesher _mesher;
	QuadTreeLod<HTerrainChunk*> _lodder;
	Vector<PendingChunkUpdate> _pending_chunk_updates;
};


#endif // HTERRAIN_H

