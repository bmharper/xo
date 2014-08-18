#pragma once

#include "../nuDefs.h"

class NUAPI nuImage : public nuTexture
{
public:
					nuImage();
					~nuImage();
	
	nuImage*		Clone() const;
	void			Free();
	void			Set( nuTexFormat format, u32 width, u32 height, const void* bytes );
	void			Alloc( nuTexFormat format, u32 width, u32 height );
	u32				GetWidth() const	{ return TexWidth; }
	u32				GetHeight() const	{ return TexHeight; }
	const void*		GetData() const		{ return TexData; }
};

