#ifndef HEIGHT_MAP_DATA_H
#define HEIGHT_MAP_DATA_H

#include <core/color.h>
#include <core/math/vector3.h>

#include "grid.h"

class HeightMapData {
public:
	enum { TEXTURE_INDEX_COUNT = 4 };

	void update_all_normals();
	void update_normals(Point2i min, Point2i size);

	inline Point2i size() const { return heights.size(); }
	void resize(int size);

	// Public for convenience.
	// Do not attempt to resize them!
	Grid2D<float> heights;
	Grid2D<Vector3> normals;
	Grid2D<Color> colors;
	Grid2D<float> texture_weights[TEXTURE_INDEX_COUNT];
	Grid2D<char> texture_indices[TEXTURE_INDEX_COUNT];
};

#endif // HEIGHT_MAP_DATA_H
