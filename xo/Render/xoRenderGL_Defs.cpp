#include "pch.h"
#if XO_BUILD_OPENGL
#include "xoRenderGL_Defs.h"

xoGLProg::xoGLProg()
{
	ResetBase();
}

xoGLProg::~xoGLProg()
{
}

void			xoGLProg::Reset()					{ ResetBase(); }
const char*		xoGLProg::VertSrc()					{ return NULL; }
const char*		xoGLProg::FragSrc()					{ return NULL; }
const char*		xoGLProg::Name()					{ return "<unnamed shader>"; }
void			xoGLProg::ResetBase()				{ Vert = Frag = Prog = 0; }
bool			xoGLProg::LoadVariablePositions()	{ return false; }
uint32			xoGLProg::PlatformMask()			{ return xoPlatform_All; }
xoVertexType	xoGLProg::VertexType()				{ return xoVertexType_NULL; }

#endif