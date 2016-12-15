#pragma once
#include "../Defs.h"

namespace xo {

class XO_API Image : public Texture {
public:
	Image();
	~Image(); // Destructor calls Free()

	Image*      Clone() const;
	Image*      CloneMetadata() const;
	void        Free();
	bool        Set(xo::TexFormat format, uint32_t width, uint32_t height, const void* bytes); // Returns false if memory allocation fails
	bool        Alloc(xo::TexFormat format, uint32_t width, uint32_t height);                  // Returns false if memory allocation fails
	uint32_t         GetWidth() const { return TexWidth; }
	uint32_t         GetHeight() const { return TexHeight; }
	const void* GetData() const { return TexData; }
};
}
