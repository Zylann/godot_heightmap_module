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

		Params() {
			smooth = true;
			lod = 0;
		}
	};

	Ref<Mesh> make_chunk(Params params, const HeightMapData &data);

private:
	void reset();
	Ref<Mesh> commit();

	void make_regular(const HeightMapData &data, Point2i min, Point2i max, int stride);

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
