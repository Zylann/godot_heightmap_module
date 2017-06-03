#ifndef GRID_H
#define GRID_H

#include <core/math/math_2d.h>
#include <core/vector.h>

template <typename T>
class Grid2D {
public:
	inline Point2i size() const { return _size; }

	inline int area() const {
		return _size.width * _size.height;
	}

	inline T get(Point2i pos) const {
		return _data[index(pos)];
	}

	inline T get(int x, int y) const {
		return _data[index(x, y)];
	}

	inline void set(Point2i pos, T v) {
		_data[index(pos)] = v;
	}

	inline T get_or_default(int x, int y) {
		if (is_valid_pos(x, y))
			return get(x, y);
		return T();
	}

	inline T get_or_default(Point2i pos) {
		return get_or_default(pos.x, pos.y);
	}

	inline T get_clamped(int x, int y) {
		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		if (x >= _size.x)
			x = _size.x - 1;
		if (y >= _size.y)
			y = _size.y - 1;
		return get(x, y);
	}

	inline bool is_valid_pos(int x, int y) const {
		return x >= 0 && y >= 0 && x < _size.width && y < _size.height;
	}

	inline bool is_valid_pos(Point2i pos) const {
		return is_valid_pos(pos.x, pos.y);
	}

	inline int index(Point2i pos) const {
		return index(pos.x, pos.y);
	}

	inline int index(int x, int y) const {
		return y * _size.width + x;
	}

	void resize(Point2i new_size, bool preserve_data, T defval=T()) {
		ERR_FAIL_COND(new_size.x < 0 || new_size.y < 0);

		Point2i old_size = _size;
		int new_area = new_size.x * new_size.y;

		if(preserve_data) {

			if(old_size.x == new_size.x) {
				// Column count didn't change, no need to offset any data
				_data.resize(new_area);

			} else {
				// The number of columns did change

				if(new_area > _data.size()) {
					// The array becomes bigger, enlarge it first so that we can offset the data
					_data.resize(new_area);
				}

				// Now we need to offset rows

				if(new_size.x < old_size.x) {
					// Shrink columns
					// (the first column doesn't change)
					for(int y = 0; y < old_size.y; ++y) {
						int old_row_begin = y * old_size.x;
						int new_row_begin = y * new_size.x;

						for(int x = 0; x < new_size.x; ++x) {
							_data[new_row_begin + x] = _data[old_row_begin + x];
						}
					}

				} else if(new_size.x > old_size.x) {
					// Offset columns at bigger intervals:
					// Iterate backwards because otherwise we would overwrite the data we want to move
					// (The first column doesn't change either)

					// .     .     .
					// 1 2 3 4 5 6 7 8 9
					// .       .       .       .
					// 1 2 3 4 5 6 7 8 9 _ _ _ _ _ _ _
					// 1 2 3 _ 4 5 6 _ 7 8 9 _ _ _ _ _

					int y = old_size.y - 1;
					if(y >= new_size.y)
						y = new_size.y-1;

					while(y >= 0) {
						int old_row_begin = y * old_size.x;
						int new_row_begin = y * new_size.x;

						int x = old_size.x - 1;
						while(x >= 0) {
							_data[new_row_begin + x] = _data[old_row_begin + x];
							x -= 1;
						}

						// Fill gaps with default values
						for(int x = old_size.x; x < new_size.x; ++x) {
							_data[new_row_begin + x] = defval;
						}

						y -= 1;
					}
				}

				if(new_area < _data.size()) {
					// The array becomes smaller, shrink it at the end so that we can offset the data in place
					_data.resize(new_area);
				}
			}

			// Fill new rows with default color
			for(int y = old_size.y; y < new_size.y; ++y) {
				for(int x = 0; x < new_size.x; ++x) {
					_data[x + y * new_size.x] = defval;
				}
			}

		} else {
			// Don't care about the data, just resize
			_data.clear();
			_data.resize(new_area);
		}

		_size = new_size;
	}

	inline void fill(T value) {
		for (int i = 0; i < _data.size(); ++i) {
			_data[i] = value;
		}
	}

private:
	Vector<T> _data;
	Point2i _size;
};

#endif // GRID_H
