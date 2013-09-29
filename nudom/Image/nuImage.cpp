#include "pch.h"
#include "nuImage.h"

nuImage::nuImage()
{
	//Width = 0;
	//Height = 0;
	//Bytes = NULL;
}

nuImage::~nuImage()
{
	Free();
}

nuImage* nuImage::Clone() const
{
	nuImage* c = new nuImage();
	c->Set( GetWidth(), GetHeight(), GetData() );
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

void nuImage::Set( u32 width, u32 height, const void* bytes )
{
	if ( TexWidth != width || TexWidth != height )
		Free();
	TexWidth = width;
	TexHeight = height;
	if ( TexWidth != 0 && TexHeight != 0 && bytes != NULL )
	{
		TexInvalidate();
		size_t size = TexWidth * TexHeight * 4;
		TexData = AbcAlignedMalloc( size, 16 );
		TexStride = TexWidth * 4;
		AbcCheckAlloc( TexData );
		memcpy( TexData, bytes, size );
	}
}
