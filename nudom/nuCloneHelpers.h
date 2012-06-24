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
void nuClonePodvec( podvec<T>& dst, const podvec<T>& src, nuPool* pool )
{
	nuClonePodvecPrepare( dst, src, pool );
	for ( uintp i = 0; i < src.count; i++ )
	{
		src[i].CloneFastInto( dst[i], pool );
	}
}
