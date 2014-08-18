#include "pch.h"
#include "nuImage.h"

nuImage::nuImage()
{
}

nuImage::~nuImage()
{
	Free();
}

nuImage* nuImage::Clone() const
{
	nuImage* c = new nuImage();
	c->Set( TexFormat, GetWidth(), GetHeight(), GetData() );
	return c;
}

void nuImage::Free()
{
	if ( TexData )
	{
		AbcAlignedFree( TexData );
		TexData = NULL;
	}
	TexID = nuTextureIDNull;
}

void nuImage::Set( nuTexFormat format, u32 width, u32 height, const void* bytes )
{
	Alloc( format, width, height );
	size_t size = TexWidth * TexHeight * nuTexFormatBytesPerPixel(format);
	if ( size != 0 )
		memcpy( TexData, bytes, size );
}

void nuImage::Alloc( nuTexFormat format, u32 width, u32 height )
{
	size_t existingFormatBPP = nuTexFormatBytesPerPixel(TexFormat);
	size_t requiredFormatBPP = nuTexFormatBytesPerPixel(format);

	if ( width == TexWidth && height == TexHeight && existingFormatBPP == requiredFormatBPP )
	{
		if ( TexFormat != format )
			TexFormat = format;
		return;
	}

	if ( TexWidth != width || TexWidth != height )
		Free();
	TexWidth = width;
	TexHeight = height;
	TexFormat = format;
	if ( TexWidth != 0 && TexHeight != 0 )
	{
		TexInvalidate();
		TexStride = TexWidth * (uint32) TexBytesPerPixel();
		size_t size = TexHeight * TexStride;
		TexData = AbcAlignedMalloc( size, 16 );
		AbcCheckAlloc( TexData );
	}
}
