#ifndef QUAD_TREE_LOD_H
#define QUAD_TREE_LOD_H

#include <core/hash_map.h>
#include <core/math/math_2d.h>
#include <core/math/vector3.h>
#include <core/variant.h>


template <typename T>
class QuadTreeLod {

private:
	struct Node {
		Node *children[4];
		Point2i origin;

		// Userdata.
		// Note: the tree doesn't own this field,
		// if it's a pointer make sure you free it when you don't need it anymore
		T chunk;

		Node() {
			chunk = T();
			for(int i = 0; i < 4; ++i) {
				children[i] = NULL;
			}
		}

		~Node() {
			clear_children();
		}

		void clear() {
			clear_children();
			chunk = T();
		}

		void clear_children() {
			if(has_children()) {
				for (int i = 0; i < 4; ++i) {
					memdelete(children[i]);
					children[i] = NULL;
				}
			}
		}

		bool has_children() {
			return children[0] != NULL;
		}
	};

public:
	typedef T (*MakeFunc)(void *context, Point2i origin, int lod);
	typedef void (*QueryFunc)(void *context, T chunk, Point2i origin, int lod);
	typedef QueryFunc RecycleFunc;

	MakeFunc make_func;
	RecycleFunc recycle_func;
	void *callbacks_context;

	QuadTreeLod() {
		_max_depth = 0;
		_base_size = 0;
		callbacks_context = NULL;
		make_func = NULL;
		recycle_func = NULL;
	}

	//~QuadTreeLod() {}

	void create_from_sizes(int base_size, int full_size) {
		for_all_chunks(recycle_func, callbacks_context);

		_grids.clear();
		_tree.clear_children();

		_base_size = base_size;

		int po = 0;
		while (full_size > base_size) {
			full_size = full_size >> 1;
			++po;
		}

		_max_depth = po;

		_grids.resize(po + 1);
	}

	inline int get_lod_count() {
		return _max_depth;
	}

	bool try_get_chunk_at(T &out_chunk, Point2i pos, int lod) {
		HashMap<Point2i,T> &grid = _grids[lod];
		T *chunk = grid.getptr(pos);
		if (chunk) {
			out_chunk = *chunk;
			return true;
		}
		return false;
	}

	void update(Vector3 viewer_pos, void *callbacks_context) {
		update_nodes_recursive(_tree, _max_depth, viewer_pos, callbacks_context);
		make_chunks_recursively(_tree, _max_depth, callbacks_context);
	}

	inline int get_lod_size(int lod) const {
		return 1 << lod;
	}

	inline int get_split_distance(int lod) const {
		return _base_size * get_lod_size(lod) * 2.0;
	}

	void for_all_chunks(QueryFunc action_cb, void *callback_context) {
		for_all_chunks_recursive(action_cb, callback_context, _tree, _max_depth);
	}

	// Takes a rectangle in highest LOD coordinates,
	// and calls a function on all chunks of that LOD or higher LODs.
	void for_chunks_in_rect(QueryFunc action_cb, Point2i cpos0, Point2i csize, void *callback_context) const {

		// For each lod
		for (int lod = 0; lod < _grids.size(); ++lod) {

			// Get grid and chunk size
			const HashMap<Point2i, T> &grid = _grids[lod];
			int s = get_lod_size(lod);

			// Convert rect into this lod's coordinates
			Point2i min = cpos0 / s;
			Point2i max = cpos0 + csize + Point2i(1, 1);

			// Find which chunks are within
			Point2i cpos;
			for (cpos.y = min.y; cpos.y < max.y; ++cpos.y) {
				for (cpos.x = min.x; cpos.x < max.x; ++cpos.x) {

					T const *chunk_ptr = grid.getptr(cpos);

					if (chunk_ptr) {
						T chunk = *chunk_ptr;
						action_cb(callback_context, chunk, cpos, lod);
					}
				}
			}
		}
	}

private:
	T make_chunk(void *callbacks_context, int lod, Point2i origin) {
		T chunk = T();
		if (make_func) {
			chunk = make_func(callbacks_context, origin, lod);
			if (chunk)
				_grids[lod][origin] = chunk;
		}
		return chunk;
	}

	void recycle_chunk(void *callbacks_context, T chunk, Point2i origin, int lod) {
		if (recycle_func)
			recycle_func(callbacks_context, chunk, origin, lod);
		_grids[lod].erase(origin);
	}

	void join_recursively(Node &node, int lod, void *callbacks_context) {
		if (node.has_children()) {
			for (int i = 0; i < 4; ++i) {
				Node *child = node.children[i];
				join_recursively(*child, lod - 1, callbacks_context);
			}
			node.clear_children();
		} else if (node.chunk) {
			recycle_chunk(callbacks_context, node.chunk, node.origin, lod);
			node.chunk = T();
		}
	}

	void update_nodes_recursive(Node &node, int lod, Vector3 viewer_pos, void *callbacks_context) {
		//print_line(String("update_nodes_recursive lod={0}, o={1}, {2} ").format(varray(lod, node.origin.x, node.origin.y)));

		int lod_size = get_lod_size(lod);
		Vector3 world_center = (_base_size * lod_size) * (Vector3(node.origin.x, 0, node.origin.y) + Vector3(0.5, 0, 0.5));
		real_t split_distance = get_split_distance(lod);

		if (node.has_children()) {
			// Test if it should be joined
			// TODO Distance should take the chunk's Y dimension into account
			if (world_center.distance_to(viewer_pos) > split_distance) {
				join_recursively(node, lod, callbacks_context);
			}

		} else if (lod > 0) {
			// Test if it should split
			if (world_center.distance_to(viewer_pos) < split_distance) {
				// Split

				for (int i = 0; i < 4; ++i) {
					Node *child = memnew(Node);
					Point2i origin = node.origin * 2 + Point2i(i & 1, (i & 2) >> 1);
					child->origin = origin;
					node.children[i] = child;
				}

				if (node.chunk) {
					recycle_chunk(callbacks_context, node.chunk, node.origin, lod);
				}

				node.chunk = T();
			}
		}

		// TODO This will check all chunks every frame,
		// we could find a way to recursively update chunks as they get joined/split,
		// but in C++ that would be not even needed.
		if (node.has_children()) {
			for (int i = 0; i < 4; ++i) {
				update_nodes_recursive(*node.children[i], lod - 1, viewer_pos, callbacks_context);
			}
		}
	}

	void make_chunks_recursively(Node &node, int lod, void *callbacks_context) {
		ERR_FAIL_COND(lod < 0);
		if (node.has_children()) {
			for (int i = 0; i < 4; ++i) {
				Node *child = node.children[i];
				make_chunks_recursively(*child, lod - 1, callbacks_context);
			}
		} else {
			if (!node.chunk) {
				node.chunk = make_chunk(callbacks_context, lod, node.origin);
				// Note: if you don't return anything here,
				// make_chunk will continue being called
			}
		}
	}

	void for_all_chunks_recursive(QueryFunc action_cb, void *callback_context, Node &node, int lod) {
		ERR_FAIL_COND(lod < 0);
		if(node.has_children()) {
			for(int i = 0; i < 4; ++i) {
				Node *child = node.children[i];
				for_all_chunks_recursive(action_cb, callback_context, *child, lod -1);
			}
		} else {
			if(node.chunk) {
				action_cb(callback_context, node.chunk, node.origin, lod);
			}
		}
	}

private:
	Node _tree;
	Vector<HashMap<Point2i, T> > _grids;
	int _max_depth;
	int _base_size;
};

#endif // QUAD_TREE_LOD_H
