#pragma once

#include "xoDefs.h"
#include "xoVertexTypes.h"

#if XO_BUILD_DIRECTX

class XOAPI xoDXProg : public xoProgBase
{
public:
	ID3D11VertexShader*		Vert;
	ID3D11PixelShader*		Frag;
	ID3D11InputLayout*      VertLayout;

	xoDXProg();
	virtual					~xoDXProg();

	// All of these virtual functions are overridden by auto-generated shader objects
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();
	virtual uint32			PlatformMask(); 			// Combination of xoPlatformMask bits.
	virtual xoVertexType	VertexType();

	bool					UseOnThisPlatform() { return !!(PlatformMask() & XO_PLATFORM); }

protected:
	void					ResetBase();
};

#endif