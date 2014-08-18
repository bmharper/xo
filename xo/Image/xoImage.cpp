#include "pch.h"
#include "xoImage.h"

xoImage::xoImage()
{
}

xoImage::~xoImage()
{
	Free();
}

xoImage* xoImage::Clone() const
{
	xoImage* c = new xoImage();
	c->Set( TexFormat, GetWidth(), GetHeight(), GetData() );
	return c;
}

void xoImage::Free()
{
	if ( TexData )
	{
		AbcAlignedFree( TexData );
		TexData = NULL;
	}
	TexID = xoTextureIDNull;
}

void xoImage::Set( xoTexFormat format, u32 width, u32 height, const void* bytes )
{
	Alloc( format, width, height );
	size_t size = TexWidth * TexHeight * xoTexFormatBytesPerPixel(format);
	if ( size != 0 )
		memcpy( TexData, bytes, size );
}

void xoImage::Alloc( xoTexFormat format, u32 width, u32 height )
{
	size_t existingFormatBPP = xoTexFormatBytesPerPixel(TexFormat);
	size_t requiredFormatBPP = xoTexFormatBytesPerPixel(format);

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
