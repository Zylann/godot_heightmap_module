#include "register_types.h"
#ifndef _3D_DISABLED
#include "height_map.h"
#include "height_map_editor_plugin.h"
#endif

void register_hterrain_types() {

#ifndef _3D_DISABLED
	ClassDB::register_class<HeightMap>();
	ClassDB::register_class<HeightMapData>();

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<HeightMapEditorPlugin>();
#endif
#endif
}

void unregister_hterrain_types() {
}
