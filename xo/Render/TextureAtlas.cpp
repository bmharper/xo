#include "pch.h"
#include "TextureAtlas.h"

namespace xo {

TextureAtlas::TextureAtlas() {
	memset(this, 0, sizeof(*this));
}

TextureAtlas::~TextureAtlas() {
}

void TextureAtlas::Initialize(uint32_t width, uint32_t height, xo::TexFormat format, uint32_t padding) {
	TexWidth      = width;
	TexHeight     = height;
	Padding       = padding;
	TexFormat     = format;
	TexStride     = (int) (width * TexFormatBytesPerPixel(format));
	size_t nbytes = height * TexStride;
	TexData       = (uint8_t*) MallocOrDie(nbytes);
	PosTop        = Padding;
	PosBottom     = Padding;
	PosRight      = Padding;
}

void TextureAtlas::Zero() {
	memset(TexData, 0, std::abs(TexStride) * TexHeight);
}

void TextureAtlas::Free() {
	free(TexData);
	memset(this, 0, sizeof(*this));
	TexID = TextureIDNull;
}

bool TextureAtlas::Alloc(uint16_t width, uint16_t height, uint16_t& x, uint16_t& y) {
	if (width > TexWidth)
		return false;
	if (PosRight + width > TexWidth) {
		// move onto next line
		PosTop   = PosBottom;
		PosRight = 0;
	}
	// can't fit vertically
	if (PosTop + height + Padding > TexHeight)
		return false;
	x = PosRight;
	y = PosTop;
	PosRight += width + Padding;
	PosBottom = std::max(PosBottom, PosTop + height + Padding);
	TexInvalidRect.ExpandToFit(Box(x, y, x + width, y + height));
	return true;
}
}
