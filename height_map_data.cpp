#include <core/os/file_access.h>
#include <core/io/file_access_compressed.h>
#include <core/array.h>

#include "height_map.h"
#include "utility.h"

#define DEFAULT_RESOLUTION 256
#define HEIGHTMAP_EXTENSION "heightmap"

const char *HeightMapData::SIGNAL_RESOLUTION_CHANGED = "resolution_changed";
const char *HeightMapData::SIGNAL_REGION_CHANGED = "region_changed";

const int HeightMapData::MAX_RESOLUTION = 4096 + 1;

// For serialization
const char *HEIGHTMAP_MAGIC_V1 = "GDHM";
//const char *HEIGHTMAP_SUB_V1 = "v1__";
const char *HEIGHTMAP_SUB_V = "v2__";


HeightMapData::HeightMapData() {

	_resolution = 0;

//#ifdef TOOLS_ENABLED
	_disable_apply_undo = false;
//#endif
}

void HeightMapData::load_default() {

	set_resolution(DEFAULT_RESOLUTION);
	update_all_normals();
}

int HeightMapData::get_resolution() const {
	return _resolution;
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

	_resolution = p_res;

	// Resize heights
	if(_images[CHANNEL_HEIGHT].is_null()) {
		_images[CHANNEL_HEIGHT].instance();
		_images[CHANNEL_HEIGHT]->create(_resolution, _resolution, false, get_channel_format(CHANNEL_HEIGHT));
	} else {
		_images[CHANNEL_HEIGHT]->resize(_resolution, _resolution);
	}

	// Resize heights
	if(_images[CHANNEL_NORMAL].is_null()) {
		_images[CHANNEL_NORMAL].instance();
	}
	_images[CHANNEL_NORMAL]->create(_resolution, _resolution, false, get_channel_format(CHANNEL_NORMAL));
	update_all_normals();

	// Resize colors
	if(_images[CHANNEL_COLOR].is_null()) {
		_images[CHANNEL_COLOR].instance();
		_images[CHANNEL_COLOR]->create(_resolution, _resolution, false, get_channel_format(CHANNEL_COLOR));
	} else {
		_images[CHANNEL_COLOR]->resize(_resolution, _resolution);
	}

//	for (int i = 0; i < TEXTURE_INDEX_COUNT; ++i) {
//		// Sum of all weights must be 1, so we fill first slot with 1 and others with 0
//		texture_weights[i].resize(size, true, i == 0 ? 1 : 0);
//		texture_indices[i].resize(size, true, 0);
//	}

	emit_signal(SIGNAL_RESOLUTION_CHANGED);
}

void HeightMapData::update_all_normals() {
	update_normals(Point2i(), Point2i(_resolution, _resolution));
}

inline Color get_clamped(const Image &im, int x, int y) {

	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	if (x >= im.get_width())
		x = im.get_width() - 1;
	if (y >= im.get_height())
		y = im.get_height() - 1;

	return im.get_pixel(x, y);
}

void HeightMapData::update_normals(Point2i min, Point2i size) {

	ERR_FAIL_COND(_images[CHANNEL_HEIGHT].is_null());
	ERR_FAIL_COND(_images[CHANNEL_NORMAL].is_null());

	Image &heights = **_images[CHANNEL_HEIGHT];
	Image &normals = **_images[CHANNEL_NORMAL];

	Point2i max = min + size;
	Point2i pos;

	clamp_min_max_excluded(min, max, Point2i(0,0), Point2i(heights.get_width(), heights.get_height()));

	heights.lock();
	normals.lock();

	for (pos.y = min.y; pos.y < max.y; ++pos.y) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {

			float left = get_clamped(heights, pos.x - 1, pos.y).r;
			float right = get_clamped(heights, pos.x + 1, pos.y).r;
			float fore = get_clamped(heights, pos.x, pos.y + 1).r;
			float back = get_clamped(heights, pos.x, pos.y - 1).r;

			Vector3 n = Vector3(left - right, 2.0, back - fore).normalized();

			normals.set_pixel(pos.x, pos.y, encode_normal(n));
		}
	}

	heights.unlock();
	normals.unlock();
}

void HeightMapData::notify_region_change(Point2i min, Point2i max, HeightMapData::Channel channel) {

	// TODO Hmm not sure if that belongs here
	switch(channel) {
		case CHANNEL_HEIGHT:
			upload_region(channel, min, max);
			upload_region(CHANNEL_NORMAL, min, max);
			break;
		case CHANNEL_NORMAL:
		case CHANNEL_COLOR:
			upload_region(channel, min, max);
			break;
		default:
			print_line("Unrecognized channel");
			break;
	}

	emit_signal(SIGNAL_REGION_CHANGED, min.x, min.y, max.x, max.y, channel);
}

//#ifdef TOOLS_ENABLED

// Very specific to the editor.
// undo_data contains chunked grids of modified terrain in a given channel.
void HeightMapData::_apply_undo(Dictionary undo_data) {

	if(_disable_apply_undo)
		return;
	// TODO Update to use images

	Array chunk_positions = undo_data["chunk_positions"];
	Array chunk_datas = undo_data["data"];
	int channel = undo_data["channel"];

	// Validate input

	ERR_FAIL_COND(channel < 0 || channel >= CHANNEL_COUNT);
	ERR_FAIL_COND(chunk_positions.size()/2 != chunk_datas.size());

	ERR_FAIL_COND(chunk_positions.size() % 2 != 0);
	for(int i = 0; i < chunk_positions.size(); ++i) {
		Variant p = chunk_positions[i];
		ERR_FAIL_COND(p.get_type() != Variant::INT);
	}
	for(int i = 0; i < chunk_datas.size(); ++i) {
		Variant d = chunk_datas[i];
		ERR_FAIL_COND(d.get_type() != Variant::POOL_BYTE_ARRAY);
	}

	// Apply

	for(int i = 0; i < chunk_datas.size(); ++i) {
		Point2i cpos;
		cpos.x = chunk_positions[2 * i];
		cpos.y = chunk_positions[2 * i + 1];

		Point2i min = cpos * HeightMap::CHUNK_SIZE;
		Point2i max = min + Point2i(1, 1) * HeightMap::CHUNK_SIZE;

		Ref<Image> data = chunk_datas[i];
		ERR_FAIL_COND(data.is_null());

		Rect2 data_rect(0, 0, data->get_width(), data->get_height());

		switch(channel) {

			case CHANNEL_HEIGHT:
				ERR_FAIL_COND(_images[channel].is_null())
				_images[channel]->blit_rect(data, data_rect, min);
				// Padding is needed because normals are calculated using neighboring,
				// so a change in height X also requires normals in X-1 and X+1 to be updated
				update_normals(min - Point2i(1,1), max + Point2i(1,1));
				break;

			case CHANNEL_COLOR:
				ERR_FAIL_COND(_images[channel].is_null())
				_images[channel]->blit_rect(data, data_rect, min);
				break;

			case CHANNEL_NORMAL:
				print_line("This is a calculated channel!, no undo on this one");
				break;

			default:
				print_line("Wut? Unsupported undo channel");
				break;
		}

		// TODO This one might be very slow even with partial texture update, due to rebinding...?
		notify_region_change(min, max, (Channel)channel);
	}
}

//#endif

void HeightMapData::upload_channel(Channel channel) {
	upload_region(channel, Point2i(0,0), Point2i(_resolution, _resolution));
}

void HeightMapData::upload_region(Channel channel, Point2i min, Point2i max) {

	ERR_FAIL_COND(_images[channel].is_null());

	if(_textures[channel].is_null()) {
		_textures[channel].instance();
	}

	int flags = 0;

	if(channel == CHANNEL_NORMAL) {
		// To allow smooth shading in fragment shader
		flags |= Texture::FLAG_FILTER;
	}

	//               ..ooo@@@XXX%%%xx..
	//            .oo@@XXX%x%xxx..     ` .
	//          .o@XX%%xx..               ` .
	//        o@X%..                  ..ooooooo
	//      .@X%x.                 ..o@@^^   ^^@@o
	//    .ooo@@@@@@ooo..      ..o@@^          @X%
	//    o@@^^^     ^^^@@@ooo.oo@@^             %
	//   xzI    -*--      ^^^o^^        --*-     %
	//   @@@o     ooooooo^@@^o^@X^@oooooo     .X%x
	//  I@@@@@@@@@XX%%xx  ( o@o )X%x@ROMBASED@@@X%x
	//  I@@@@XX%%xx  oo@@@@X% @@X%x   ^^^@@@@@@@X%x
	//   @X%xx     o@@@@@@@X% @@XX%%x  )    ^^@X%x
	//    ^   xx o@@@@@@@@Xx  ^ @XX%%x    xxx
	//          o@@^^^ooo I^^ I^o ooo   .  x
	//          oo @^ IX      I   ^X  @^ oo
	//          IX     U  .        V     IX
	//           V     .           .     V
	//
	// TODO Partial update pleaaase! SLOOOOOOOOOOWNESS AHEAD !!
	_textures[channel]->create_from_image(_images[channel], flags);
	//print_line(String("Channel updated ") + String::num(channel));
}

Ref<Image> HeightMapData::get_image(Channel channel) const {
	return _images[channel];
}

Ref<Texture> HeightMapData::get_texture(Channel channel) {
	if(_textures[channel].is_null() && _images[channel].is_valid()) {
		upload_channel(channel);
	}
	return _textures[channel];
}

Color HeightMapData::encode_normal(Vector3 n) {
	return Color(
		0.5 * (n.x + 1.0),
		0.5 * (n.y + 1.0),
		0.5 * (n.z + 1.0), 1.0);
}

Vector3 HeightMapData::decode_normal(Color c) {
	return Vector3(
		2.0 * c.r - 1.0,
		2.0 * c.g - 1.0,
		2.0 * c.b - 1.0);
}

void HeightMapData::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_resolution", "p_res"), &HeightMapData::set_resolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &HeightMapData::get_resolution);

//#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("_apply_undo", "data"), &HeightMapData::_apply_undo);
//#endif

	ADD_PROPERTY(PropertyInfo(Variant::INT, "resolution"), "set_resolution", "get_resolution");

	ADD_SIGNAL(MethodInfo(SIGNAL_RESOLUTION_CHANGED));
	ADD_SIGNAL(MethodInfo(SIGNAL_REGION_CHANGED,
						  PropertyInfo(Variant::INT, "min_x"),
						  PropertyInfo(Variant::INT, "min_y"),
						  PropertyInfo(Variant::INT, "max_x"),
						  PropertyInfo(Variant::INT, "max_y"),
						  PropertyInfo(Variant::INT, "channel")));
}

Image::Format HeightMapData::get_channel_format(Channel channel) {
	switch(channel) {
		case CHANNEL_HEIGHT:
			return Image::FORMAT_RH;
		case CHANNEL_NORMAL:
			return Image::FORMAT_RGB8;
		case CHANNEL_COLOR:
			return Image::FORMAT_RGBA8;
	}
	print_line("Unrecognized channel");
	return Image::FORMAT_MAX;
}

static void write_channel(FileAccess &f, Ref<Image> img_ref) {

	PoolVector<uint8_t> data = img_ref->get_data();
	PoolVector<uint8_t>::Read r = data.read();

	f.store_buffer(r.ptr(), data.size());
}

Error HeightMapData::_save(FileAccess &f) {

	// Sub-version
	f.store_buffer((const uint8_t*)HEIGHTMAP_SUB_V, 4);

	// Size
	//print_line(String("String saving resolution ") + String::num(_resolution));
	f.store_32(_resolution);
	f.store_32(_resolution);

	for(int channel = 0; channel < CHANNEL_COUNT; ++channel) {

		Ref<Image> im = _images[channel];
		//print_line(String("Saving channel ") + String::num(channel));

		// Sanity checks
		ERR_FAIL_COND_V(im.is_null(), ERR_FILE_CORRUPT);
		ERR_FAIL_COND_V(im->get_width() != _resolution || im->get_height() != _resolution, ERR_FILE_CORRUPT);

		write_channel(f, _images[channel]);
	}

	// TODO Texture indices
	// TODO Texture weights

	return OK;
}

static void load_channel(Ref<Image> &img_ref, int channel, FileAccess &f, Point2i size) {

	if(img_ref.is_null()) {
		img_ref.instance();
	}

	Image::Format format = HeightMapData::get_channel_format((HeightMapData::Channel)channel);
	ERR_FAIL_COND(format == Image::FORMAT_MAX);

	//img_ref->create(size.x, size.y, false, format);
	// I can't create the image before because getting the data array afterwards will increase refcount to 2.
	// Because of this, using a Write to set the bytes will trigger copy-on-write, which will:
	// 1) Needlessly double the amount of memory needed to load the image, and that image can be big
	// 2) Loose any loaded data because it gets loaded on a copy, not the actual image

	PoolVector<uint8_t> data;
	data.resize(Image::get_image_data_size(size.x, size.y, format, false));
	PoolVector<uint8_t>::Write w = data.write();

	//print_line(String("Load channel {0}, size={1}").format(varray(channel, data.size())));
	f.get_buffer(w.ptr(), data.size());

	img_ref->create(size.x, size.y, false, format, data);
}

Error HeightMapData::_load(FileAccess &f) {

	char version[5] = {0};
	f.get_buffer((uint8_t*)version, 4);

	if(strncmp(version, HEIGHTMAP_SUB_V, 4) != 0) {
		print_line(String("Invalid version, found {0}, expected {1}").format(varray(version, HEIGHTMAP_SUB_V)));
		return ERR_FILE_UNRECOGNIZED;
	}

	Point2i size;
	size.x = f.get_32();
	size.y = f.get_32();

	// Note: maybe one day we'll support non-square heightmaps
	_resolution = size.x;
	size.y = size.x;
	//print_line(String("Loaded resolution ") + String::num(_resolution));

	ERR_FAIL_COND_V(size.x > MAX_RESOLUTION, ERR_FILE_CORRUPT);
	ERR_FAIL_COND_V(size.y > MAX_RESOLUTION, ERR_FILE_CORRUPT);

	for(int channel = 0; channel < CHANNEL_COUNT; ++channel) {
		load_channel(_images[channel], channel, f, size);
	}

	// TODO Texture indices
	// TODO Texture weights

	return OK;
}


//---------------------------------------
// Saver

Error HeightMapDataSaver::save(const String &p_path, const Ref<Resource> &p_resource, uint32_t p_flags) {
	//print_line("Saving heightmap data");

	Ref<HeightMapData> heightmap_data_ref = p_resource;
	ERR_FAIL_COND_V(heightmap_data_ref.is_null(), ERR_BUG);

	FileAccessCompressed *fac = memnew(FileAccessCompressed);
	fac->configure(HEIGHTMAP_MAGIC_V1);
	Error err = fac->_open(p_path, FileAccess::WRITE);
	if (err) {
		//print_line("Error saving heightmap data");
		memdelete(fac);
		return err;
	}

	Error e = heightmap_data_ref->_save(*fac);

	fac->close();
	// TODO I didn't see examples doing this after close()... how is this freed?
	//memdelete(fac);

	return e;
}

bool HeightMapDataSaver::recognize(const Ref<Resource> &p_resource) const {
	return p_resource->cast_to<HeightMapData>() != NULL;
}

void HeightMapDataSaver::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (p_resource->cast_to<HeightMapData>()) {
		p_extensions->push_back(HEIGHTMAP_EXTENSION);
	}
}


//---------------------------------------
// Loader

Ref<Resource> HeightMapDataLoader::load(const String &p_path, const String &p_original_path, Error *r_error) {
	//print_line("Loading heightmap data");

	FileAccessCompressed *fac = memnew(FileAccessCompressed);
	fac->configure(HEIGHTMAP_MAGIC_V1);
	Error err = fac->_open(p_path, FileAccess::READ);
	if (err) {
		//print_line("Error loading heightmap data");
		if(r_error)
			*r_error = err;
		memdelete(fac);
		return Ref<Resource>();
	}

	Ref<HeightMapData> heightmap_data_ref(memnew(HeightMapData));

	err = heightmap_data_ref->_load(*fac);
	if(err != OK) {
		if(r_error)
			*r_error = err;
		memdelete(fac);
		return Ref<Resource>();
	}

	fac->close();

	// TODO I didn't see examples doing this after close()... how is this freed?
	//memdelete(fac);

	if(r_error)
		*r_error = OK;
	return heightmap_data_ref;
}

void HeightMapDataLoader::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back(HEIGHTMAP_EXTENSION);
}

bool HeightMapDataLoader::handles_type(const String &p_type) const {
	return p_type == "HeightMapData";
}

String HeightMapDataLoader::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == HEIGHTMAP_EXTENSION)
		return "HeightMapData";
	return "";
}




