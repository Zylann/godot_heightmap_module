#ifndef HEIGHT_MAP_MESHER_H
#define HEIGHT_MAP_MESHER_H

#include <math/math_2d.h>
#include <scene/resources/mesh.h>

#include "height_map_data.h"

class HeightMapMesher {

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

	Ref<Mesh> make_chunk(Params params, const HeightMapData &data);

private:
	Vector<Vector3> _output_vertices;
	Vector<Vector3> _output_normals;
	Vector<Color> _output_colors;
	//Vector<Vector2> uv;
	Vector<int> _output_indices;
};

#endif // HEIGHT_MAP_MESHER_H
