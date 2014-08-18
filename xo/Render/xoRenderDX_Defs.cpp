#include "pch.h"
#if XO_BUILD_DIRECTX
#include "xoRenderDX_Defs.h"

xoDXProg::xoDXProg()
{
	ResetBase();
}

xoDXProg::~xoDXProg()
{
}

void			xoDXProg::Reset()					{ ResetBase(); }
const char*		xoDXProg::VertSrc()					{ return NULL; }
const char*		xoDXProg::FragSrc()					{ return NULL; }
const char*		xoDXProg::Name()					{ return "<unnamed shader>"; }
void			xoDXProg::ResetBase()				{ Vert = NULL; Frag = NULL; VertLayout = NULL; }
bool			xoDXProg::LoadVariablePositions()	{ return false; }
uint32			xoDXProg::PlatformMask()			{ return xoPlatform_All; }
xoVertexType	xoDXProg::VertexType()				{ return xoVertexType_NULL; }

#endif
