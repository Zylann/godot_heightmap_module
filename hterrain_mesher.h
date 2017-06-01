#ifndef HTERRAIN_MESHER_H
#define HTERRAIN_MESHER_H

#include <math/math_2d.h>
#include <scene/resources/mesh.h>

#include "hterrain_data.h"

class HTerrainMesher {

public:
	struct Params {
		Point2i origin;
		Point2i size;
		bool smooth;
		int lod;
		int seams[4];

		Params() {
			smooth = true;
			for (int i = 0; i < 4; ++i)
				seams[i] = 0;
		}
	};

	Ref<Mesh> make_chunk(Params params, const HTerrainData &data);

private:
	Vector<Vector3> _output_vertices;
	Vector<Vector3> _output_normals;
	Vector<Color> _output_colors;
	//Vector<Vector2> uv;
	Vector<int> _output_indices;
};

#endif // HTERRAIN_MESHER_H
