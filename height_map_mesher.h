#ifndef HEIGHT_MAP_MESHER_H
#define HEIGHT_MAP_MESHER_H

#include <math/math_2d.h>
#include <scene/resources/mesh.h>

#include "height_map_data.h"

class HeightMapMesher {

public:
	enum SeamFlag {
		SEAM_LEFT = 1,
		SEAM_RIGHT = 2,
		SEAM_BOTTOM = 4,
		SEAM_TOP = 8,
		SEAM_CONFIG_COUNT = 16
	};

	void configure(Point2i chunk_size, int lod_count);
	Ref<Mesh> get_chunk(int lod, int seams);

private:
	void precalculate();
	Ref<Mesh> make_flat_chunk(Point2i chunk_size, int stride, int seams);
	PoolVector<int> make_indices(Point2i chunk_size, unsigned int seams);

private:
	Vector< Ref<Mesh> > _mesh_cache[SEAM_CONFIG_COUNT];
	Point2i _chunk_size;
};

#endif // HEIGHT_MAP_MESHER_H
