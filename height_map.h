#ifndef HEIGHT_MAP_H
#define HEIGHT_MAP_H

#include "height_map_chunk.h"
#include "height_map_data.h"
#include "height_map_mesher.h"
#include "quad_tree_lod.h"
#include <scene/3d/spatial.h>

// Heightmap-based 3D terrain
class HeightMap : public Spatial {
	GDCLASS(HeightMap, Spatial)
public:
	//static const int CHUNK_SIZE = 16;
	// Workaround because GCC doesn't links the line above properly
	enum { CHUNK_SIZE = 16 };

	static const char *SHADER_PARAM_HEIGHT_TEXTURE;
	static const char *SHADER_PARAM_NORMAL_TEXTURE;
	static const char *SHADER_PARAM_COLOR_TEXTURE;
	static const char *SHADER_PARAM_RESOLUTION;
	static const char *SHADER_PARAM_INVERSE_TRANSFORM;

	HeightMap();
	~HeightMap();

	void set_data(Ref<HeightMapData> data);
	Ref<HeightMapData> get_data() { return _data; }

	void set_custom_shader(Ref<Shader> p_shader);
	inline Ref<Shader> get_custom_shader() const { return _custom_shader; }

	void set_collision_enabled(bool enabled);
	inline bool is_collision_enabled() const { return _collision_enabled; }

	void set_lod_scale(float lod_scale);
	float get_lod_scale() const;

	void set_area_dirty(Point2i origin_in_cells, Point2i size_in_cells);
	bool cell_raycast(Vector3 origin_world, Vector3 dir_world, Point2i &out_cell_pos);

	static void init_default_resources();
	static void free_default_resources();

	Vector3 _manual_viewer_pos;

protected:
	void _notification(int p_what);

private:
	void _process();

	void update_material();

	HeightMapChunk *_make_chunk_cb(Point2i origin, int lod);
	void _recycle_chunk_cb(HeightMapChunk *chunk);

	void add_chunk_update(HeightMapChunk &chunk, Point2i pos, int lod);
	void update_chunk(HeightMapChunk &chunk, int lod);

	Point2i local_pos_to_cell(Vector3 local_pos) const;

	void _on_data_resolution_changed();
	void _on_data_region_changed(int min_x, int min_y, int max_x, int max_y, int channel);

	void clear_chunk_cache();

	HeightMapChunk *get_chunk_at(Point2i pos, int lod) const;

	static void _bind_methods();

	static HeightMapChunk *s_make_chunk_cb(void *context, Point2i origin, int lod);
	static void s_recycle_chunk_cb(void *context, HeightMapChunk *chunk, Point2i origin, int lod);

	template <typename Action_T>
	void for_all_chunks(Action_T action) {
		for(int lod = 0; lod < _chunk_cache.size(); ++lod) {
			const Point2i *key = NULL;
			while(key = _chunk_cache[lod].next(key)) {
				HeightMapChunk *chunk = _chunk_cache[lod][*key];
				action(*chunk);
			}
		}
	}

private:
	Ref<Shader> _custom_shader;
	Ref<ShaderMaterial> _material;
	bool _collision_enabled;
	Ref<HeightMapData> _data;
	HeightMapMesher _mesher;
	QuadTreeLod<HeightMapChunk *> _lodder;

	struct PendingChunkUpdate {
		Point2i pos;
		int lod;
		PendingChunkUpdate() : lod(0) {}
	};

	Vector<PendingChunkUpdate> _pending_chunk_updates;

	// [lod][pos]
	// This container owns chunks, so will be used to free them
	// TODO Change HashMaps to Grid2D
	Vector< HashMap< Point2i, HeightMapChunk* > > _chunk_cache;

	// Stats
	int _updated_chunks;
};

#endif // HEIGHT_MAP_H
