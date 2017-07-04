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
		SEAM_TOP = 4,
		SEAM_BOTTOM = 8
	};

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
	void make_indices(Point2i chunk_size, unsigned int seams);
	void make_indices_legacy(Point2i chunk_size);

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
