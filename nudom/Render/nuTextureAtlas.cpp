#include "pch.h"
#include "nuTextureAtlas.h"

nuTextureAtlas::nuTextureAtlas()
{
	memset( this, 0, sizeof(*this) );
}

nuTextureAtlas::~nuTextureAtlas()
{
}

void nuTextureAtlas::Initialize( uint width, uint height, nuTexFormat format, uint padding )
{
	TexWidth = width;
	TexHeight = height;
	Padding = padding;
	TexFormat = format;
	TexStride = (int) (width * nuTexFormatBytesPerPixel(format));
	size_t nbytes = height * TexStride;
	TexData = (byte*) nuMallocOrDie( nbytes );
	PosTop = Padding;
	PosBottom = Padding;
	PosRight = Padding;
}

void nuTextureAtlas::Zero()
{
	memset( TexData, 0, std::abs(TexStride) * TexHeight );
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
	if ( PosTop + height + Padding > TexHeight )
		return false;
	x = PosRight;
	y = PosTop;
	PosRight += width + Padding;
	PosBottom = std::max(PosBottom, PosTop + height + Padding);
	TexInvalidRect.ExpandToFit( nuBox(x, y, x + width, y + height) );
	return true;
}
