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
	TexWidth = width;
	TexHeight = height;
	BytesPerTexel = bytes_per_texel;
	size_t nbytes = width * height * bytes_per_texel;
	TexData = (byte*) nuMallocOrDie( nbytes );
	//memset( Data, 0xcc, nbytes );
	//memset( Data, 0, nbytes );
	TexStride = width * bytes_per_texel;
	TexFormat = nuTexFormatGrey8;
	PosTop = 0;
	PosBottom = 0;
	PosRight = 0;
}

void nuTextureAtlas::Free()
{
	free(TexData);
	memset( this, 0, sizeof(*this) );
	TexID = nuTextureIDNull;
}

bool nuTextureAtlas::Alloc( uint16 width, uint16 height, uint16& x, uint16& y )
{
	if ( width > TexWidth )
		return false;
	if ( PosRight + width > TexWidth )
	{
		// move onto next line
		PosTop = PosBottom;
		PosRight = 0;
	}
	// can't fit vertically
	if ( PosTop + height > TexHeight )
		return false;
	x = PosRight;
	y = PosTop;
	PosRight += width;
	PosBottom = std::max(PosBottom, PosTop + height);
	TexInvalidRect.ExpandToFit( nuBox(x, y, x + width, y + height) );
	return true;
}
