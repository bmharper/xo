#pragma once

#include "../xoDefs.h"

class XOAPI xoImage : public xoTexture
{
public:
					xoImage();
					~xoImage();
	
	xoImage*		Clone() const;
	void			Free();
	void			Set( xoTexFormat format, u32 width, u32 height, const void* bytes );
	void			Alloc( xoTexFormat format, u32 width, u32 height );
	u32				GetWidth() const	{ return TexWidth; }
	u32				GetHeight() const	{ return TexHeight; }
	const void*		GetData() const		{ return TexData; }
};

