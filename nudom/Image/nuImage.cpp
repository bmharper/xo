#include "pch.h"
#include "nuImage.h"

nuImage::nuImage()
{
	Width = 0;
	Height = 0;
	Bytes = NULL;
}

nuImage::~nuImage()
{
	Free();
}

nuImage* nuImage::Clone() const
{
	nuImage* c = new nuImage();
	c->Set( Width, Height, Bytes );
	return c;
}

void nuImage::Free()
{
	if ( Bytes )
	{
		AbcAlignedFree( Bytes );
		Bytes = NULL;
	}
}

void nuImage::Set( u32 width, u32 height, const void* bytes )
{
	if ( Width != width || Height != height )
		Free();
	Width = width;
	Height = height;
	if ( Width != 0 && Height != 0 && bytes != NULL )
	{
		size_t size = Width * Height * 4;
		Bytes = AbcAlignedMalloc( size, 16 );
		AbcCheckAlloc( Bytes );
		memcpy( Bytes, bytes, size );
	}
}
