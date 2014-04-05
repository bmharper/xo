#pragma once

#include "nuDefs.h"

#if NU_BUILD_DIRECTX

class NUAPI nuDXProg : public nuProgBase
{
public:
	ID3D11VertexShader*		Vert;
	ID3D11PixelShader*		Frag;
	ID3D11InputLayout*      VertLayout;

							nuDXProg();
	virtual					~nuDXProg();
	
	// All of these virtual functions are overridden by auto-generated shader objects
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();
	virtual uint32			PlatformMask(); 			// Combination of nuPlatformMask bits.

	bool					UseOnThisPlatform() { return !!(PlatformMask() & NU_PLATFORM); }

protected:
	void					ResetBase();
};

#endif