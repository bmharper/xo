#include "pch.h"
#include "Image.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../dependencies/stb_image_write.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace xo {

Image::Image() {
}

Image::~Image() {
	Free();
}

Image* Image::Clone() const {
	Image* c = new Image();
	c->Set(TexFormat, GetWidth(), GetHeight(), GetData());
	return c;
}

Image* Image::CloneMetadata() const {
	Image* c     = new Image();
	*c           = *this;
	c->TexData   = nullptr;
	c->TexStride = 0;
	return c;
}

void Image::Free() {
	if (TexData) {
		AlignedFree(TexData);
		TexData = NULL;
	}
	TexID = TextureIDNull;
}

bool Image::Set(xo::TexFormat format, uint32_t width, uint32_t height, const void* bytes) {
	if (!Alloc(format, width, height))
		return false;
	size_t size = TexWidth * TexHeight * TexFormatBytesPerPixel(format);
	if (size != 0)
		memcpy(TexData, bytes, size);
	return true;
}

bool Image::Alloc(xo::TexFormat format, uint32_t width, uint32_t height) {
	size_t existingFormatBPP = TexFormatBytesPerPixel(TexFormat);
	size_t requiredFormatBPP = TexFormatBytesPerPixel(format);

	if (width == TexWidth && height == TexHeight && existingFormatBPP == requiredFormatBPP) {
		if (TexFormat != format)
			TexFormat = format;
		return true;
	}

	if (TexWidth != width || TexWidth != height)
		Free();
	TexWidth  = width;
	TexHeight = height;
	TexFormat = format;
	if (TexWidth != 0 && TexHeight != 0) {
		TexInvalidateWholeSurface();
		TexStride   = TexWidth * (uint32_t) TexBytesPerPixel();
		size_t size = TexHeight * TexStride;
		TexData     = AlignedAlloc(size, 16);
		return TexData != nullptr;
	} else {
		return true;
	}
}

bool Image::SaveToPng(const char* filename) const {
	return stbi_write_png(filename, TexWidth, TexHeight, TexFormatChannelCount(TexFormat), TexData, TexStride) != 0;
}

}
