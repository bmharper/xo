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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuLifoBuf::nuLifoBuf()
{
	Buffer = nullptr;
	Size = 0;
	Pos = 0;
}

nuLifoBuf::nuLifoBuf( size_t size ) : nuLifoBuf()
{
	Init( size );
}

nuLifoBuf::~nuLifoBuf()
{
	NUASSERT( Pos == 0 ); // This assertion might be a nuisance. If so, just remove it.
	free( Buffer );
}

void nuLifoBuf::Init( size_t size )
{
	NUASSERT( Pos == 0 );
	Size = size;
	free( Buffer );
	Buffer = nuMallocOrDie( size );
}

void* nuLifoBuf::Alloc( size_t _bytes )
{
	NUASSERT( (size_t) Pos + _bytes <= (size_t) Size );
	NUASSERT( (intp) _bytes >= 0 );
	intp bytes = _bytes;

	void* pos = (byte*) Buffer + Pos;
	ItemSizes += bytes;
	Pos += bytes;
	
	return pos;
}

void nuLifoBuf::Realloc( void* buf, size_t bytes )
{
	NUASSERT( ItemSizes.size() > 0 && buf == (byte*) Buffer + (Pos - ItemSizes.back()) );
	NUASSERT( (size_t) Pos - (size_t) ItemSizes.back() + bytes <= (size_t) Size );
	NUASSERT( (intp) bytes >= 0 );
	intp delta = (intp) bytes - ItemSizes.back();
	ItemSizes.back() += delta;
	Pos += delta;
}

void nuLifoBuf::GrowLast( size_t moreBytes )
{
	NUASSERT( (size_t) Pos + moreBytes <= (size_t) Size );
	ItemSizes.back() += (intp) moreBytes;
	Pos += (intp) moreBytes;
}

void nuLifoBuf::Free( void* buf )
{
	if ( buf == nullptr )
		return;
	NUASSERT( ItemSizes.size() > 0 && buf == (byte*) Buffer + (Pos - ItemSizes.back()) );
	Pos -= ItemSizes.back();
	ItemSizes.pop();
}
