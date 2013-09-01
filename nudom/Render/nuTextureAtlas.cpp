#include "pch.h"
#include "nuTextureAtlas.h"

nuTextureAtlas::nuTextureAtlas()
{
	memset( this, 0, sizeof(*this) );
}

nuTextureAtlas::~nuTextureAtlas()
{
}

void nuTextureAtlas::Initialize( uint width, uint height, uint bytes_per_texel )
{
	Width = width;
	Height = height;
	BytesPerTexel = bytes_per_texel;
	size_t nbytes = width * height * bytes_per_texel;
	Data = (byte*) nuMallocOrDie( nbytes );
	//memset( Data, 0xcc, nbytes );
	//memset( Data, 0, nbytes );
	Stride = width * bytes_per_texel;
	PosTop = 0;
	PosBottom = 0;
	PosRight = 0;
}

void nuTextureAtlas::Free()
{
	free(Data);
	memset( this, 0, sizeof(*this) );
}

bool nuTextureAtlas::Alloc( uint16 width, uint16 height, uint16& x, uint16& y )
{
	if ( width > Width )
		return false;
	if ( PosRight + width > Width )
	{
		// move onto next line
		PosTop = PosBottom;
		PosRight = 0;
	}
	// can't fit vertically
	if ( PosTop + height > Height )
		return false;
	x = PosRight;
	y = PosTop;
	PosRight += width;
	PosBottom = std::max(PosBottom, PosTop + height);
	return true;
}
