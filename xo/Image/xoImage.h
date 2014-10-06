#pragma once

#include "../xoDefs.h"

class XOAPI xoImage : public xoTexture
{
public:
					xoImage();
					~xoImage();		// Destructor calls Free()
	
	xoImage*		Clone() const;
	void			Free();
	bool			Set( xoTexFormat format, u32 width, u32 height, const void* bytes );	// Returns false if memory allocation fails
	bool			Alloc( xoTexFormat format, u32 width, u32 height );						// Returns false if memory allocation fails
	u32				GetWidth() const	{ return TexWidth; }
	u32				GetHeight() const	{ return TexHeight; }
	const void*		GetData() const		{ return TexData; }
};

