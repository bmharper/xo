#pragma once
#if XO_BUILD_DIRECTX

#include "../../Render/xoRenderDX_Defs.h"

class xoDXProg_FillTex : public xoDXProg
{
public:
	xoDXProg_FillTex();
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32			PlatformMask();				// Combination of xoPlatform bits.
	virtual xoVertexType	VertexType();				// Only meaningful on DirectX

};

#endif // XO_BUILD_DIRECTX

