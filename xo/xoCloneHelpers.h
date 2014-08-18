#pragma once

template<typename T>
void xoClonePvectPrepare( pvect<T>& dst, const pvect<T>& src, xoPool* pool )
{
	if ( src.count != 0 ) dst.data = (void**) pool->Alloc( sizeof(T) * src.count, true );
	dst.count = src.count;
	dst.capacity = src.capacity;
}

template<typename T>
void xoClonePodvecPrepare( podvec<T>& dst, const podvec<T>& src, xoPool* pool )
{
	if ( src.count != 0 ) dst.data = (T*) pool->Alloc( sizeof(T) * src.count, true );
	dst.count = src.count;
	dst.capacity = src.capacity;
}

//template<typename T>
//void xoClonePodvecWithCloneFastInto( podvec<T>& dst, const podvec<T>& src, xoPool* pool )
//{
//	xoClonePodvecPrepare( dst, src, pool );
//	for ( uintp i = 0; i < src.count; i++ )
//	{
//		src[i].CloneFastInto( dst[i], pool );
//	}
//}

template<typename T>
void xoClonePodvecWithMemCopy( podvec<T>& dst, const podvec<T>& src, xoPool* pool )
{
	xoClonePodvecPrepare( dst, src, pool );
	memcpy( dst.data, src.data, src.size() * sizeof(T) );
}

//template<typename T, size_t N>
//void xoCloneStaticArrayWithCloneFastInto( T (&dst)[N], const T (&src)[N], xoPool* pool )
//{
//	for ( size_t i = 0; i < N; i++ )
//		src[i].CloneFastInto( dst[i], pool );
//}

template<typename T, size_t N>
void xoCloneStaticArrayWithCloneSlowInto( T (&dst)[N], const T (&src)[N] )
{
	for ( size_t i = 0; i < N; i++ )
		src[i].CloneSlowInto( dst[i] );
}
