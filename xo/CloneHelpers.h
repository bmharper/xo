#pragma once
namespace xo {

template <typename T>
void ClonePvectPrepare(cheapvec<T>& dst, const cheapvec<T>& src, Pool* pool) {
	if (src.count != 0)
		dst.data = (void**) pool->Alloc(sizeof(T) * src.count, true);
	dst.count    = src.count;
	dst.capacity = src.capacity;
}

template <typename T>
void ClonePodvecPrepare(cheapvec<T>& dst, const cheapvec<T>& src, Pool* pool) {
	if (src.count != 0)
		dst.data = (T*) pool->Alloc(sizeof(T) * src.count, true);
	dst.count    = src.count;
	dst.capacity = src.capacity;
}

template <typename T>
void ClonePodvecWithMemCopy(cheapvec<T>& dst, const cheapvec<T>& src, Pool* pool) {
	ClonePodvecPrepare(dst, src, pool);
	memcpy(dst.data, src.data, src.size() * sizeof(T));
}

template <typename T, size_t N>
void CloneStaticArrayWithCloneSlowInto(T (&dst)[N], const T (&src)[N]) {
	for (size_t i = 0; i < N; i++)
		src[i].CloneSlowInto(dst[i]);
}
}
