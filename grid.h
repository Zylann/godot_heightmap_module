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

	void resize(Point2i new_size) {
		ERR_FAIL_COND(new_size.x < 0 || new_size.y < 0);
		_data.clear();
		_data.resize(new_size.width * new_size.height);
		_size = new_size;
		// TODO resize preserve values
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
