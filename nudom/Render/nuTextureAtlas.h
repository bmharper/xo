#pragma once

#include "../nuDefs.h"

// This is brain dead naive. We obviously need a better box packer. The one from freetype-gl looks decent.
class NUAPI nuTextureAtlas : public nuTexture
{
public:
				nuTextureAtlas();
				~nuTextureAtlas();
	void		Initialize( uint width, uint height, uint bytes_per_texel );
	void		Free();
	bool		Alloc( uint16 width, uint16 height, uint16& x, uint16& y );
	
	void*		DataAt( int x, int y )				{ return (char*) TexData + y * TexStride + x * BytesPerTexel; }
	const void*	DataAt( int x, int y ) const		{ return (char*) TexData + y * TexStride + x * BytesPerTexel; }
	int			GetBytesPerTexel() const			{ return BytesPerTexel; }
	int			GetStride() const					{ return TexStride; }
	uint		GetWidth() const					{ return TexWidth; }
	uint		GetHeight() const					{ return TexHeight; }

protected:
	uint	BytesPerTexel;

	uint	PosTop;
	uint	PosBottom;
	uint	PosRight;
};