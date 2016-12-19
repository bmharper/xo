#pragma once
#if XO_BUILD_DIRECTX

#include "../../Render/RenderDX_Defs.h"

namespace xo {

class DXProg_TextRGB : public DXProg {
public:
	DXProg_TextRGB();
	virtual void           Reset();
	virtual const char*    VertSrc();
	virtual const char*    FragSrc();
	virtual const char*    Name();
	virtual bool           LoadVariablePositions(); // Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32_t       PlatformMask();          // Combination of Platform bits.
	virtual xo::VertexType VertexType();            // Only meaningful on DirectX
};

} // namespace xo

#endif // XO_BUILD_DIRECTX
