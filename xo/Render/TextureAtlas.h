#pragma once
#include "../Defs.h"

namespace xo {

// This is brain dead naive. We obviously need a better box packer. The one from freetype-gl looks decent.
class XO_API TextureAtlas : public Texture {
public:
	TextureAtlas();
	~TextureAtlas();
	void Initialize(uint32_t width, uint32_t height, xo::TexFormat format, uint32_t padding);
	void Zero();
	void Free();
	bool Alloc(uint16_t width, uint16_t height, uint16_t& x, uint16_t& y);

	int      GetStride() const { return TexStride; }
	uint32_t GetWidth() const { return TexWidth; }
	uint32_t GetHeight() const { return TexHeight; }

protected:
	uint32_t Padding;

	uint32_t PosTop;
	uint32_t PosBottom;
	uint32_t PosRight;
};
}
