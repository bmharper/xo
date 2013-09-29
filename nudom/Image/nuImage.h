#pragma once

#include "../nuDefs.h"

class NUAPI nuImage : public nuTexture
{
public:
					nuImage();
					~nuImage();
	
	nuImage*		Clone() const;
	void			Free();
	void			Set( u32 width, u32 height, const void* bytes ); // set RGBA 8888
	u32				GetWidth() const { return TexWidth; }
	u32				GetHeight() const { return TexHeight; }
	const void*		GetData() const { return TexData; }

protected:
	//u32				Width;
	//u32				Height;
	//void*			Bytes;

};

