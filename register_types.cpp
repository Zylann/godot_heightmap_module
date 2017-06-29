#include "register_types.h"

#ifndef _3D_DISABLED

#include "height_map.h"
#include "height_map_editor_plugin.h"

HeightMapDataSaver *s_heightmap_data_saver = NULL;
HeightMapDataLoader *s_heightmap_data_loader = NULL;

#endif


void register_hterrain_types() {

#ifndef _3D_DISABLED
	ClassDB::register_class<HeightMap>();
	ClassDB::register_class<HeightMapData>();

	s_heightmap_data_saver = memnew(HeightMapDataSaver());
	ResourceSaver::add_resource_format_saver(s_heightmap_data_saver);

	s_heightmap_data_loader = memnew(HeightMapDataLoader());
	ResourceLoader::add_resource_format_loader(s_heightmap_data_loader);

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<HeightMapEditorPlugin>();
#endif
#endif
}

void unregister_hterrain_types() {

#ifndef _3D_DISABLED
	if(s_heightmap_data_saver) {
		memdelete(s_heightmap_data_saver);
	}
	if(s_heightmap_data_loader) {
		memdelete(s_heightmap_data_loader);
	}
#endif
}
