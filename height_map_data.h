#ifndef HEIGHT_MAP_DATA_H
#define HEIGHT_MAP_DATA_H

#include <core/color.h>
#include <core/math/vector3.h>
#include <core/resource.h>

#include "grid.h"

class HeightMapData : public Resource {
	GDCLASS(HeightMapData, Resource)
public:
	enum { TEXTURE_INDEX_COUNT = 4 };

	static const char *SIGNAL_RESOLUTION_CHANGED;

	HeightMapData();

	void set_resolution(int p_res);
	int get_resolution() const;

	void update_all_normals();
	void update_normals(Point2i min, Point2i size);

	// Public for convenience.
	// Do not attempt to resize them!
	Grid2D<float> heights;
	Grid2D<Vector3> normals;
	Grid2D<Color> colors;
	Grid2D<float> texture_weights[TEXTURE_INDEX_COUNT];
	Grid2D<char> texture_indices[TEXTURE_INDEX_COUNT];

private:
	static void _bind_methods();
};

#endif // HEIGHT_MAP_DATA_H
