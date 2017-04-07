#include "pch.h"
#include "TextureAtlas.h"

namespace xo {

TextureAtlas::TextureAtlas() {
	memset(this, 0, sizeof(*this));
}

TextureAtlas::~TextureAtlas() {
}

void TextureAtlas::Initialize(uint32_t width, uint32_t height, xo::TexFormat format, uint32_t padding) {
	Width         = width;
	Height        = height;
	Padding       = padding;
	Format        = format;
	Stride        = (int) (width * TexFormatBytesPerPixel(format));
	size_t nbytes = height * Stride;
	Data          = (uint8_t*) MallocOrDie(nbytes);
	PosTop        = Padding;
	PosBottom     = Padding;
	PosRight      = Padding;
}

void TextureAtlas::Zero() {
	memset(Data, 0, std::abs(Stride) * Height);
}

void TextureAtlas::Free() {
	free(Data);
	memset(this, 0, sizeof(*this));
	TexID = TextureIDNull;
}

bool TextureAtlas::Alloc(uint16_t width, uint16_t height, uint16_t& x, uint16_t& y) {
	if (width > Width)
		return false;
	if (PosRight + width > Width) {
		// move onto next line
		PosTop   = PosBottom;
		PosRight = 0;
	}
	// can't fit vertically
	if (PosTop + height + Padding > Height)
		return false;
	x = PosRight;
	y = PosTop;
	PosRight += width + Padding;
	PosBottom = std::max(PosBottom, PosTop + height + Padding);
	InvalidRect.ExpandToFit(Box(x, y, x + width, y + height));
	return true;
}
}
