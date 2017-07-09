#ifndef HEIGHTMAP_UTILITY_H
#define HEIGHTMAP_UTILITY_H

#include <core/math/math_2d.h>
#include <core/image.h>

// TODO Is this already in engine?
template <typename T>
void copy_to(PoolVector<T> &to, Vector<T> &from) {

	to.resize(from.size());

	typename PoolVector<T>::Write w = to.write();

	for (int i = 0; i < from.size(); ++i) {
		w[i] = from[i];
	}
}

void clamp_min_max_excluded(Point2i &out_min, Point2i &out_max, Point2i min, Point2i max);

struct LockImage {
	LockImage(Ref<Image> im) {
		_im = im;
		_im->lock();
	}
	~LockImage() {
		_im->unlock();
	}
	Ref<Image> _im;
};

#endif // HEIGHTMAP_UTILITY_H
