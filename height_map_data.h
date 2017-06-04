#ifndef HEIGHT_MAP_DATA_H
#define HEIGHT_MAP_DATA_H

#include <core/color.h>
#include <core/math/vector3.h>

#include "grid.h"

class HeightMapData {
public:
	void update_all_normals();
	void update_normals(Point2i min, Point2i size);

	inline Point2i size() const { return heights.size(); }
	void resize(int size);

	// Public for convenience.
	// Do not attempt to resize them!
	Grid2D<float> heights;
	Grid2D<Vector3> normals;
	Grid2D<Color> colors;
};

#endif // HEIGHT_MAP_DATA_H
