#ifndef HEIGHT_MAP_DATA_H
#define HEIGHT_MAP_DATA_H

#include <core/color.h>
#include <core/math/vector3.h>
#include <core/resource.h>
#include <core/io/resource_loader.h>
#include <core/io/resource_saver.h>
#include <core/dictionary.h>
#include <scene/resources/texture.h>
#include <core/os/file_access.h>

#include "grid.h"

class HeightMapData : public Resource {
	GDCLASS(HeightMapData, Resource)
public:
	//enum { TEXTURE_INDEX_COUNT = 4 };

	enum Channel {
		CHANNEL_HEIGHT = 0,
		CHANNEL_NORMAL,
		CHANNEL_COLOR,
//		CHANNEL_TEXTURE_WEIGHT,
//		CHANNEL_TEXTURE_INDEX,
		CHANNEL_COUNT
	};

	static const int MAX_RESOLUTION;

	static const char *SIGNAL_RESOLUTION_CHANGED;
	static const char *SIGNAL_REGION_CHANGED;

	HeightMapData();

	void load_default();

	void set_resolution(int p_res);
	int get_resolution() const;

	void update_all_normals();
	void update_normals(Point2i min, Point2i size);

	void notify_region_change(Point2i min, Point2i max, Channel channel);

	Ref<Texture> get_texture(Channel channel);
	Ref<Image> get_image(Channel channel) const;

	static Color encode_normal(Vector3 n);
	static Vector3 decode_normal(Color c);

	static Image::Format get_channel_format(Channel channel);

	Error _load(FileAccess &f);
	Error _save(FileAccess &f);

//#ifdef TOOLS_ENABLED
	bool _disable_apply_undo;
//#endif

private:

//#ifdef TOOLS_ENABLED
	void _apply_undo(Dictionary undo_data);
//#endif

	static void _bind_methods();

	void upload_channel(Channel channel);
	void upload_region(Channel channel, Point2i min, Point2i max);

private:
	int _resolution;

	Ref<ImageTexture> _textures[CHANNEL_COUNT];
	Ref<Image> _images[CHANNEL_COUNT];
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

VARIANT_ENUM_CAST(HeightMapData::Channel)


#endif // HEIGHT_MAP_DATA_H

