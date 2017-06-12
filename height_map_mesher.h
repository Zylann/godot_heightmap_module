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
			lod = 0;
			for (int i = 0; i < 4; ++i)
				seams[i] = 0;
		}
	};

	Ref<Mesh> make_chunk(Params params, const HeightMapData &data);

private:
	Vector<Vector3> _output_vertices;
	Vector<Vector3> _output_normals;
	Vector<Color> _output_colors;
//	Vector<Vector2> _output_uv;
//	Vector<Vector2> _output_uv2;
//	Vector<float> _output_tangents;
//	Vector<float> _output_bones;
//	Vector<float> _output_weights;

	Vector<int> _output_indices;
};

#endif // HEIGHT_MAP_MESHER_H
