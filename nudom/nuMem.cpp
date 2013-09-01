#include "pch.h"
#include "nuDefs.h"
#include "nuMem.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6001)		// using uninitialized memory. False static analysis warning
#endif

nuPool::nuPool()
{
	ChunkSize = 64 * 1024;
	TopRemain = 0;
}

nuPool::~nuPool()
{
	FreeAll();
}

void nuPool::SetChunkSize( size_t size )
{
	// If this were not true, then FreeAllExceptOne would be wrong.
	// Also, this just seems like sane behaviour (i.e. initializing chunk size a priori).
	NUASSERT( Chunks.size() == 0 );
	ChunkSize = size;
}

void nuPool::FreeAll()
{
	for ( intp i = 0; i < Chunks.size(); i++ )
		free(Chunks[i]);
	for ( intp i = 0; i < BigBlocks.size(); i++ )
		free(BigBlocks[i]);
	Chunks.clear();
	BigBlocks.clear();
	TopRemain = 0;
}

/* This is an optimization for a pool that is frequently re-used.
The pool must have quite a predictable size for this to be effective.
*/
void nuPool::FreeAllExceptOne()
{
	if ( Chunks.size() == 1 && BigBlocks.size() == 0 )
		TopRemain = ChunkSize;
	else
		FreeAll();
}

void* nuPool::Alloc( size_t bytes, bool zeroInit )
{
	NUASSERT(bytes != 0);
	if ( bytes > ChunkSize )
	{
		BigBlocks += nuMallocOrDie( bytes );
		if ( zeroInit ) memset( BigBlocks.back(), 0, bytes );
		return BigBlocks.back();
	}
	else
	{
		if ( (intp) (TopRemain - bytes) < 0 )
		{
			Chunks += nuMallocOrDie( ChunkSize );
			TopRemain = ChunkSize;
		}
		byte* p = ((byte*) Chunks.back()) + ChunkSize - TopRemain;
		if ( zeroInit ) memset( p, 0, bytes );
		TopRemain -= bytes;
		return p;
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
