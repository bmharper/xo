#pragma once

#include <malloc.h>
#include <stdlib.h>

// I don't yet know how to reliably detect for the presence of posix_memalign.
// Right now my only requirements are Windows/Android, and for my particular use on android, I don't need aligned malloc.
#ifndef ABC_HAVE_ALIGNED_MALLOC
#	ifdef _WIN32
#		define ABC_HAVE_ALIGNED_MALLOC 1
#	endif
#endif

#if defined(ABC_HAVE_ALIGNED_MALLOC)

#ifdef __cplusplus
#	define O_INLINE inline
#else
#	define O_INLINE
#endif

O_INLINE void* AbcAlignedMalloc( size_t bytes, size_t alignment )
{
#ifdef _WIN32
	return _aligned_malloc( bytes, alignment );
#else
	void* p = NULL;
	if ( 0 != posix_memalign( &p, alignment, bytes ) )
		return NULL;
	return p;
#endif
}

// alignment must be the same as the original block
O_INLINE void* AbcAlignedRealloc( size_t original_block_bytes, void* block, size_t bytes, size_t alignment )
{
#ifdef _WIN32
	return _aligned_realloc( block, bytes, alignment );
#else
	void* p = AbcAlignedMalloc( block, bytes, alignment );
	if ( !p ) return NULL;
	memcpy( p, block, original_block_bytes );
	free( block );
	return p;
#endif
}

O_INLINE void AbcAlignedFree( void* block )
{
#ifdef _WIN32
	_aligned_free( block );
#else
	free( block );
#endif
}

#undef O_INLINE
#endif