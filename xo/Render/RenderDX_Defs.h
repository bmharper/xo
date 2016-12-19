#pragma once
#include "Defs.h"
#include "VertexTypes.h"

#if XO_BUILD_DIRECTX
namespace xo {

class XO_API DXProg : public ProgBase {
public:
	ID3D11VertexShader* Vert;
	ID3D11PixelShader*  Frag;
	ID3D11InputLayout*  VertLayout;

	DXProg();
	virtual ~DXProg();

	// All of these virtual functions are overridden by auto-generated shader objects
	virtual void        Reset();
	virtual const char* VertSrc();
	virtual const char* FragSrc();
	virtual const char* Name();
	virtual bool        LoadVariablePositions();
	virtual uint32_t    PlatformMask(); // Combination of PlatformMask bits.
	virtual VertexType  VertexType();

	bool UseOnThisPlatform() { return !!(PlatformMask() & XO_PLATFORM); }

protected:
	void ResetBase();
};
}
#endif
