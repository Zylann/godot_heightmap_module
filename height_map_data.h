#ifndef HEIGHT_MAP_DATA_H
#define HEIGHT_MAP_DATA_H

#include <core/color.h>
#include <core/math/vector3.h>
#include <core/resource.h>
#include <core/io/resource_loader.h>
#include <core/io/resource_saver.h>

#include "grid.h"

class HeightMapData : public Resource {
	GDCLASS(HeightMapData, Resource)
public:
	enum { TEXTURE_INDEX_COUNT = 4 };

	static const char *SIGNAL_RESOLUTION_CHANGED;

	HeightMapData();

	void load_default();

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


class HeightMapDataSaver : public ResourceFormatSaver {
public:
	Error save(const String &p_path, const Ref<Resource> &p_resource, uint32_t p_flags);
	bool recognize(const Ref<Resource> &p_resource) const;
	void get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const;
};


class HeightMapDataLoader : public ResourceFormatLoader {
public:
	Ref<Resource> load(const String &p_path, const String &p_original_path, Error *r_error);
	void get_recognized_extensions(List<String> *p_extensions) const;
	bool handles_type(const String &p_type) const;
	String get_resource_type(const String &p_path) const;
};


#endif // HEIGHT_MAP_DATA_H

