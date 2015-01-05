#include "pch.h"
#include "xoTextureAtlas.h"

xoTextureAtlas::xoTextureAtlas()
{
	memset(this, 0, sizeof(*this));
}

xoTextureAtlas::~xoTextureAtlas()
{
}

void xoTextureAtlas::Initialize(uint width, uint height, xoTexFormat format, uint padding)
{
	TexWidth = width;
	TexHeight = height;
	Padding = padding;
	TexFormat = format;
	TexStride = (int)(width * xoTexFormatBytesPerPixel(format));
	size_t nbytes = height * TexStride;
	TexData = (byte*) xoMallocOrDie(nbytes);
	PosTop = Padding;
	PosBottom = Padding;
	PosRight = Padding;
}

void xoTextureAtlas::Zero()
{
	memset(TexData, 0, std::abs(TexStride) * TexHeight);
}

void xoTextureAtlas::Free()
{
	free(TexData);
	memset(this, 0, sizeof(*this));
	TexID = xoTextureIDNull;
}

bool xoTextureAtlas::Alloc(uint16 width, uint16 height, uint16& x, uint16& y)
{
	if (width > TexWidth)
		return false;
	if (PosRight + width > TexWidth)
	{
		// move onto next line
		PosTop = PosBottom;
		PosRight = 0;
	}
	// can't fit vertically
	if (PosTop + height + Padding > TexHeight)
		return false;
	x = PosRight;
	y = PosTop;
	PosRight += width + Padding;
	PosBottom = std::max(PosBottom, PosTop + height + Padding);
	TexInvalidRect.ExpandToFit(xoBox(x, y, x + width, y + height));
	return true;
}
