#include "height_map.h"


void HeightMapData::resize(int p_size) {
	Point2i size(p_size, p_size);
	heights.resize(size);
	normals.resize(size);
	colors.resize(size);
}

void HeightMapData::update_all_normals() {
	update_normals(Point2i(), heights.size());
}

void HeightMapData::update_normals(Point2i min, Point2i size) {

	Point2i max = min + size;
	Point2i pos;

	for(pos.y = min.y; pos.y < max.y; ++pos.y) {
		for(pos.x = min.x; pos.x < max.x; ++pos.x) {

			float left = heights.get_clamped(pos.x - 1, pos.y);
			float right = heights.get_clamped(pos.x + 1, pos.y);
			float fore = heights.get_clamped(pos.x, pos.y + 1);
			float back = heights.get_clamped(pos.x, pos.y - 1);

			normals.set(pos, Vector3(left - right, 2.0, back - fore).normalized());
		}
	}
}


