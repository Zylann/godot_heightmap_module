#include "height_map_mesher.h"
#include "utility.h"


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

	make_indices(size, 0);
}

// size: chunk size in quads (there are N+1 vertices)
// seams: Bitfield for which seams are present
void HeightMapMesher::make_indices(Point2i chunk_size, unsigned int seams) {

	// LOD seams can't be made properly on uneven chunk sizes
	ERR_FAIL_COND(chunk_size.x % 2 != 0 || chunk_size.y % 2 != 0);

	Point2i reg_origin;
	Point2i reg_size = chunk_size;
	int reg_hstride = 1;

	if(seams & SEAM_LEFT) {
		reg_origin.x += 1;
		reg_size.x -= 1;
		++reg_hstride;
	}
	if(seams & SEAM_BOTTOM) {
		reg_origin.y += 1;
		reg_size.y -= 1;
	}
	if(seams & SEAM_RIGHT) {
		reg_size.x -= 1;
		++reg_hstride;
	}
	if(seams & SEAM_TOP) {
		reg_size.y -= 1;
	}

	// Regular triangles
	int i = reg_origin.x + reg_origin.y * (chunk_size.x + 1);
	Point2i pos;
	for (pos.y = 0; pos.y < reg_size.y; ++pos.y) {
		for (pos.x = 0; pos.x < reg_size.x; ++pos.x) {

			int i00 = i;
			int i10 = i + 1;
			int i01 = i + chunk_size.x + 1;
			int i11 = i01 + 1;

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

			++i;
		}
		i += reg_hstride;
	}

	// Left seam
	if(seams & SEAM_LEFT) {

		//     4 . 5
		//     |\  .
		//     | \ .
		//     |  \.
		//  (2)|   3
		//     |  /.
		//     | / .
		//     |/  .
		//     0 . 1

		int i = 0;
		int n = chunk_size.y / 2;

		for(int j = 0; j < n; ++j) {

			int i0 = i;
			int i1 = i + 1;
			int i3 = i + chunk_size.x + 2;
			int i4 = i + 2 * (chunk_size.x + 1);
			int i5 = i4 + 1;

			_output_indices.push_back( i0 );
			_output_indices.push_back( i3 );
			_output_indices.push_back( i4 );

			if(j != 0 || seams & SEAM_BOTTOM == 0) {
				_output_indices.push_back( i0 );
				_output_indices.push_back( i1 );
				_output_indices.push_back( i3 );
			}

			if(j != n-1 || seams & SEAM_TOP == 0) {
				_output_indices.push_back( i3 );
				_output_indices.push_back( i5 );
				_output_indices.push_back( i4 );
			}

			i = i4;
		}
	}

	if(seams & SEAM_RIGHT) {

		//     4 . 5
		//     .  /|
		//     . / |
		//     ./  |
		//     2   |(3)
		//     .\  |
		//     . \ |
		//     .  \|
		//     0 . 1

		int i = chunk_size.x - 1;
		int n = chunk_size.y / 2;

		for(int j = 0; j < n; ++j) {

			int i0 = i;
			int i1 = i + 1;
			int i2 = i + chunk_size.x + 1;
			int i4 = i + 2 * (chunk_size.x + 1);
			int i5 = i4 + 1;

			_output_indices.push_back( i1 );
			_output_indices.push_back( i5 );
			_output_indices.push_back( i2 );

			if(j != 0 || seams & SEAM_BOTTOM == 0) {
				_output_indices.push_back( i0 );
				_output_indices.push_back( i1 );
				_output_indices.push_back( i2 );
			}

			if(j != n-1 || seams & SEAM_TOP == 0) {
				_output_indices.push_back( i2 );
				_output_indices.push_back( i5 );
				_output_indices.push_back( i4 );
			}

			i = i4;
		}
	}

	if(seams & SEAM_BOTTOM) {

		//  3 . 4 . 5
		//  .  / \  .
		//  . /   \ .
		//  ./     \.
		//  0-------2
		//     (1)

		int i = 0;
		int n = chunk_size.x / 2;

		for(int j = 0; j < n; ++j) {

			int i0 = i;
			int i2 = i + 2;
			int i3 = i + chunk_size.x + 1;
			int i4 = i3 + 1;
			int i5 = i4 + 1;

			_output_indices.push_back( i0 );
			_output_indices.push_back( i2 );
			_output_indices.push_back( i4 );

			if(j != 0 || seams & SEAM_LEFT == 0) {
				_output_indices.push_back( i0 );
				_output_indices.push_back( i4 );
				_output_indices.push_back( i3 );
			}

			if(j != n-1 || seams & SEAM_RIGHT == 0) {
				_output_indices.push_back( i2 );
				_output_indices.push_back( i5 );
				_output_indices.push_back( i4 );
			}

			i = i2;
		}
	}

	if(seams & SEAM_TOP) {

		//     (4)
		//  3-------5
		//  .\     /.
		//  . \   / .
		//  .  \ /  .
		//  0 . 1 . 2

		int i = (chunk_size.y - 1) * (chunk_size.x + 1);
		int n = chunk_size.x / 2;

		for(int j = 0; j < n; ++j) {

			int i0 = i;
			int i1 = i + 1;
			int i2 = i + 2;
			int i3 = i + chunk_size.x + 1;
			int i5 = i3 + 2;

			_output_indices.push_back( i3 );
			_output_indices.push_back( i1 );
			_output_indices.push_back( i5 );

			if(j != 0 || seams & SEAM_LEFT == 0) {
				_output_indices.push_back( i0 );
				_output_indices.push_back( i1 );
				_output_indices.push_back( i3 );
			}

			if(j != n-1 || seams & SEAM_RIGHT == 0) {
				_output_indices.push_back( i1 );
				_output_indices.push_back( i2 );
				_output_indices.push_back( i5 );
			}

			i = i2;
		}
	}
}

void HeightMapMesher::make_indices_legacy(Point2i chunk_size) {

	// TODO If glDrawElementsIndirect was supported, we could support LOD
	// while preserving the quad flip trick... currently we can't

	int i = 0;
	Point2i pos;
	for (pos.y = 0; pos.y < chunk_size.y; ++pos.y) {
		for (pos.x = 0; pos.x < chunk_size.x; ++pos.x) {

			int i00 = i;
			int i10 = i + 1;
			int i01 = i + chunk_size.x + 1;
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



