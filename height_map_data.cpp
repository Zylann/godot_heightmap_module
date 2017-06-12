#include "height_map.h"

void HeightMapData::resize(int p_size) {

	Point2i size(p_size, p_size);
	heights.resize(size, true, 0);
	normals.resize(size, true, Vector3(0, 1, 0));
	colors.resize(size, true, Color(1, 1, 1, 1));

	for(int i = 0; i < TEXTURE_INDEX_COUNT; ++i) {
		// Sum of all weights must be 1, so we fill first slots with 1 and others with 0
		texture_weights[i].resize(size, true, i == 0 ? 1 : 0);
		texture_indices[i].resize(size, true, 0);
	}
}

void HeightMapData::update_all_normals() {
	update_normals(Point2i(), heights.size());
}

void HeightMapData::update_normals(Point2i min, Point2i size) {

	Point2i max = min + size;
	Point2i pos;

	if(min.x < 0)
		min.x = 0;
	if(min.y < 0)
		min.y = 0;

	if(min.x > normals.size().x)
		min.x = normals.size().x;
	if(min.y > normals.size().y)
		min.y = normals.size().y;

	if(max.x < 0)
		max.x = 0;
	if(max.y < 0)
		max.y = 0;

	if(max.x > normals.size().x)
		max.x = normals.size().x;
	if(max.y > normals.size().y)
		max.y = normals.size().y;

	for (pos.y = min.y; pos.y < max.y; ++pos.y) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {

			float left = heights.get_clamped(pos.x - 1, pos.y);
			float right = heights.get_clamped(pos.x + 1, pos.y);
			float fore = heights.get_clamped(pos.x, pos.y + 1);
			float back = heights.get_clamped(pos.x, pos.y - 1);

			normals.set(pos, Vector3(left - right, 2.0, back - fore).normalized());
		}
	}
}
