#ifndef HEIGHTMAP_UTILITY_H
#define HEIGHTMAP_UTILITY_H

// TODO Is this already in engine?
template <typename T>
void copy_to(PoolVector<T> &to, Vector<T> &from) {

	to.resize(from.size());

	typename PoolVector<T>::Write w = to.write();

	for (int i = 0; i < from.size(); ++i) {
		w[i] = from[i];
	}
}

#endif // HEIGHTMAP_UTILITY_H
