#pragma once

template<typename T>
void nuClonePvectPrepare( pvect<T>& dst, const pvect<T>& src, nuPool* pool )
{
	if ( src.count != 0 ) dst.data = (void**) pool->Alloc( sizeof(T) * src.count, true );
	dst.count = src.count;
	dst.capacity = src.capacity;
}

template<typename T>
void nuClonePodvecPrepare( podvec<T>& dst, const podvec<T>& src, nuPool* pool )
{
	if ( src.count != 0 ) dst.data = (T*) pool->Alloc( sizeof(T) * src.count, true );
	dst.count = src.count;
	dst.capacity = src.capacity;
}

template<typename T>
void nuClonePodvecWithCloneFastInto( podvec<T>& dst, const podvec<T>& src, nuPool* pool )
{
	nuClonePodvecPrepare( dst, src, pool );
	for ( uintp i = 0; i < src.count; i++ )
	{
		src[i].CloneFastInto( dst[i], pool );
	}
}

template<typename T>
void nuClonePodvecWithMemCopy( podvec<T>& dst, const podvec<T>& src, nuPool* pool )
{
	nuClonePodvecPrepare( dst, src, pool );
	memcpy( dst.data, src.data, src.size() * sizeof(T) );
}

template<typename T, size_t N>
void nuCloneStaticArrayWithCloneFastInto( T (&dst)[N], const T (&src)[N], nuPool* pool )
{
	for ( size_t i = 0; i < N; i++ )
		src[i].CloneFastInto( dst[i], pool );
}

template<typename T, size_t N>
void nuCloneStaticArrayWithCloneSlowInto( T (&dst)[N], const T (&src)[N] )
{
	for ( size_t i = 0; i < N; i++ )
		src[i].CloneSlowInto( dst[i] );
}
