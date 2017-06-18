#include "height_map.h"

#define DEFAULT_RESOLUTION 256

const char *HeightMapData::SIGNAL_RESOLUTION_CHANGED = "resolution_changed";

HeightMapData::HeightMapData() {

	set_resolution(DEFAULT_RESOLUTION);

	// TODO Test, won't remain here
	Point2i size = heights.size();
	Point2i pos;
	for (pos.y = 0; pos.y < size.y; ++pos.y) {
		for (pos.x = 0; pos.x < size.x; ++pos.x) {
			float h = 8.0 * (Math::cos(pos.x * 0.2) + Math::sin(pos.y * 0.2));
			heights.set(pos, h);
		}
	}
	update_all_normals();
}

int HeightMapData::get_resolution() const {
	return heights.size().x;
}

void HeightMapData::set_resolution(int p_res) {

	if(p_res == get_resolution())
		return;

	if (p_res < HeightMap::CHUNK_SIZE)
		p_res = HeightMap::CHUNK_SIZE;

	// Power of two is important for LOD.
	// Also, grid data is off by one,
	// because for an even number of quads you need an odd number of vertices
	p_res = nearest_power_of_2(p_res - 1) + 1;

	Point2i size(p_res, p_res);
	heights.resize(size, true, 0);
	normals.resize(size, true, Vector3(0, 1, 0));
	colors.resize(size, true, Color(1, 1, 1, 1));

	for (int i = 0; i < TEXTURE_INDEX_COUNT; ++i) {
		// Sum of all weights must be 1, so we fill first slot with 1 and others with 0
		texture_weights[i].resize(size, true, i == 0 ? 1 : 0);
		texture_indices[i].resize(size, true, 0);
	}

	emit_signal(SIGNAL_RESOLUTION_CHANGED);
}

void HeightMapData::update_all_normals() {
	update_normals(Point2i(), heights.size());
}

void HeightMapData::update_normals(Point2i min, Point2i size) {

	Point2i max = min + size;
	Point2i pos;

	if (min.x < 0)
		min.x = 0;
	if (min.y < 0)
		min.y = 0;

	if (min.x > normals.size().x)
		min.x = normals.size().x;
	if (min.y > normals.size().y)
		min.y = normals.size().y;

	if (max.x < 0)
		max.x = 0;
	if (max.y < 0)
		max.y = 0;

	if (max.x > normals.size().x)
		max.x = normals.size().x;
	if (max.y > normals.size().y)
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

void HeightMapData::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_resolution", "p_res"), &HeightMapData::set_resolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &HeightMapData::get_resolution);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "resolution"), "set_resolution", "get_resolution");

	ADD_SIGNAL(MethodInfo(SIGNAL_RESOLUTION_CHANGED));
}




