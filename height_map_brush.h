#ifndef HEIGHT_MAP_BRUSH_H
#define HEIGHT_MAP_BRUSH_H

#include "grid.h"
#include "height_map.h"

class HeightMapBrush {
public:
	enum Mode {
		MODE_ADD = 0,
		MODE_SUBTRACT,
		MODE_SMOOTH,
		MODE_FLATTEN,
		MODE_TEXTURE,
		MODE_COUNT
	};

	HeightMapBrush();

	void set_mode(Mode mode);
	Mode get_mode() const { return _mode; }

	void set_radius(int p_radius);
	int get_radius() const { return _radius; }

	void paint_world_pos(HeightMap &height_map, Point2i cell_pos, int override_mode = -1);

private:
	void generate_procedural(int radius);

	void paint_height(HeightMap &height_map, Point2i cell_pos, float speed);

private:
	int _radius;
	float _opacity;
	Grid2D<float> _shape;
	float _shape_sum;
	Mode _mode;
};

#endif // HEIGHT_MAP_BRUSH_H
