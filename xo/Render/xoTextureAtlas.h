#pragma once

#include "../xoDefs.h"

// This is brain dead naive. We obviously need a better box packer. The one from freetype-gl looks decent.
class XOAPI xoTextureAtlas : public xoTexture
{
public:
	xoTextureAtlas();
	~xoTextureAtlas();
	void		Initialize(uint width, uint height, xoTexFormat format, uint padding);
	void		Zero();
	void		Free();
	bool		Alloc(uint16 width, uint16 height, uint16& x, uint16& y);

	int			GetStride() const					{ return TexStride; }
	uint		GetWidth() const					{ return TexWidth; }
	uint		GetHeight() const					{ return TexHeight; }

protected:
	uint	Padding;

	uint	PosTop;
	uint	PosBottom;
	uint	PosRight;
};