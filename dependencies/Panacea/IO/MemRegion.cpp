#include "pch.h"
/*
#include "MemRegion.h"

namespace Panacea
{
namespace IO
{

MemRegion::MemRegion()
{
	OwnMapping = true;
	Mem = NULL;
	Bytes = 0;
}

MemRegion::~MemRegion()
{
	ASSERT( Mem == NULL );
	ASSERT( !OwnMapping );
}

bool MemRegion::MapAndInit( HANDLE hfilemap, UINT64 start, size_t bytes )
{
	void* m = MapViewOfFile( hfilemap, FILE_MAP_COPY, start >> 32, start & 0xFFFFFFFF, bytes );
	if ( m == NULL ) return false;

	if ( Init( m, bytes ) )
	{
		OwnMapping = true;
		return true;
	}
	else
	{
		UnmapViewOfFile( m );
		return false;
	}
}

bool MemRegion::Init( void* mem, size_t bytes )
{
	if ( mem == NULL || bytes == 0 ) return false;
	Mem = mem;
	Bytes = bytes;
	return true;
}

void MemRegion::GetModifiedPages( IntVector& indices )
{
	//GetWriteWatch( 

}

}
}
*/
