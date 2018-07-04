#include "pch.h"
#include "Image.h"

namespace xo {

Image::Image() {
}

Image::~Image() {
	Free();
}

Image* Image::Clone() const {
	Image* c = new Image();
	c->Set(Format, Width, Height, Data);
	return c;
}

void Image::CloneMetadataFrom(const Image& b) {
	*this  = b;
	Data   = nullptr;
	Stride = 0;
}

void Image::Free() {
	if (Data) {
		AlignedFree(Data);
		Data = NULL;
	}
	TexID = TextureIDNull;
}

bool Image::Set(xo::TexFormat format, uint32_t width, uint32_t height, const void* bytes) {
	if (!Alloc(format, width, height))
		return false;
	size_t size = Width * Height * TexFormatBytesPerPixel(format);
	if (size != 0)
		memcpy(Data, bytes, size);
	return true;
}

bool Image::Alloc(xo::TexFormat format, uint32_t width, uint32_t height) {
	size_t existingFormatBPP = TexFormatBytesPerPixel(Format);
	size_t requiredFormatBPP = TexFormatBytesPerPixel(format);

	if (width == Width && height == Height && existingFormatBPP == requiredFormatBPP) {
		if (Format != format)
			Format = format;
		return true;
	}

	if (Width != width || Width != height)
		Free();
	Width     = width;
	Height    = height;
	Format = format;
	if (Width != 0 && Height != 0) {
		InvalidateWholeSurface();
		Stride      = Width * (uint32_t) BytesPerPixel();
		size_t size = Height * Stride;
		Data        = AlignedAlloc(size, 16);
		return Data != nullptr;
	} else {
		return true;
	}
}

bool Image::SaveToPng(const char* filename) const {
	return stbi_write_png(filename, Width, Height, TexFormatChannelCount(Format), Data, Stride) != 0;
}
}
