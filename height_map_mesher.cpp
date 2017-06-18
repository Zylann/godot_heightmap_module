#include "height_map_mesher.h"

// TODO Is this already in engine?
template <typename T>
void copy_to(PoolVector<T> &to, Vector<T> &from) {

	to.resize(from.size());

	typename PoolVector<T>::Write w = to.write();

	for (int i = 0; i < from.size(); ++i) {
		w[i] = from[i];
	}
}

Ref<Mesh> HeightMapMesher::make_chunk(Params params, const HeightMapData &data) {

	if (!params.smooth) {
		; // TODO Faceted version
		//return make_chunk_faceted(params, data)
	}

	int stride = 1 << params.lod;

	Point2i max = params.origin + params.size * stride;

	Point2i terrain_size(data.get_resolution(), data.get_resolution());

	if (max.y >= terrain_size.y)
		max.y = terrain_size.y;
	if (max.x >= terrain_size.x)
		max.x = terrain_size.x;

	//	Vector2 uv_scale = Vector2(
	//		1.0 / static_cast<real_t>(terrain_size_x),
	//		1.0 / static_cast<real_t>(terrain_size_y));
	// Note: UVs aren't needed because they can be deduced from world positions

	make_regular(data, params.origin, max, stride);

	if (_output_vertices.size() == 0) {
		print_line("No vertices generated!");
		return Ref<Mesh>();
	}

	return commit();
}

void HeightMapMesher::make_regular(const HeightMapData &data, Point2i min, Point2i max, int stride) {

	// Make central part
	Point2i pos;
	for (pos.y = min.y; pos.y <= max.y; pos.y += stride) {
		for (pos.x = min.x; pos.x <= max.x; pos.x += stride) {

			int loc = data.heights.index(pos);

			_output_vertices.push_back(Vector3(
					pos.x - min.x,
					data.heights[loc],
					pos.y - min.y));

			_output_colors.push_back(data.colors[loc]);

			_output_normals.push_back(data.normals[loc]);

			// Texture arrays
			//			_output_uv.push_back(Vector2(data.texture_indices[0][loc], data.texture_indices[1][loc]));
			//			_output_uv2.push_back(Vector2(data.texture_indices[2][loc], data.texture_indices[3][loc]));
			//			_output_colors.push_back(Color(data.texture_weights[0][loc], data.texture_weights[1][loc], data.texture_weights[2][loc], data.texture_weights[3][loc]));
			// Err... no more attributes??
		}
	}

	Point2i size = (max - min) / stride;

	int i = 0;
	for (pos.y = 0; pos.y < size.y; ++pos.y) {
		for (pos.x = 0; pos.x < size.x; ++pos.x) {

			int i00 = i;
			int i10 = i + 1;
			int i01 = i + size.x + 1;
			int i11 = i01 + 1;

			float h00 = _output_vertices[i00].y;
			float h10 = _output_vertices[i10].y;
			float h01 = _output_vertices[i01].y;
			float h11 = _output_vertices[i11].y;

			// Put the diagonal between vertices that have the less height differences
			bool flip = Math::abs(h10 - h01) < Math::abs(h11 - h00);

			if(flip) {

				// 01---11
				//  |\  |
				//  | \ |
				//  |  \|
				// 00---10

				_output_indices.push_back( i00 );
				_output_indices.push_back( i10 );
				_output_indices.push_back( i01 );
				_output_indices.push_back( i10 );
				_output_indices.push_back( i11 );
				_output_indices.push_back( i01 );

			} else {

				// 01---11
				//  |  /|
				//  | / |
				//  |/  |
				// 00---10

				_output_indices.push_back( i00 );
				_output_indices.push_back( i11 );
				_output_indices.push_back( i01 );
				_output_indices.push_back( i00 );
				_output_indices.push_back( i10 );
				_output_indices.push_back( i11 );
			}
			++i;
		}
		++i;
	}
}

void HeightMapMesher::reset() {
	_output_vertices.clear();
	_output_normals.clear();
	_output_colors.clear();
	//	_output_uv.clear();
	//	_output_uv2.clear();
	//	_output_bones.clear();
	//	_output_tangents.clear();
	//	_output_weights.clear();
	_output_indices.clear();
}

Ref<Mesh> HeightMapMesher::commit() {

	PoolVector<Vector3> pool_vertices;
	PoolVector<Vector3> pool_normals;
	PoolVector<Color> pool_colors;
	PoolVector<int> pool_indices;

	copy_to(pool_vertices, _output_vertices);
	copy_to(pool_normals, _output_normals);
	copy_to(pool_colors, _output_colors);
	copy_to(pool_indices, _output_indices);

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = pool_vertices;
	arrays[Mesh::ARRAY_NORMAL] = pool_normals;
	arrays[Mesh::ARRAY_COLOR] = pool_colors;
	arrays[Mesh::ARRAY_INDEX] = pool_indices;

	Ref<ArrayMesh> mesh_ref(memnew(ArrayMesh));
	mesh_ref->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	reset();

	return mesh_ref;
}



