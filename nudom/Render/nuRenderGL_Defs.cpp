#include "pch.h"
#include "nuRenderGL_Defs.h"

nuGLProg::nuGLProg()
{
	ResetBase();
}

nuGLProg::~nuGLProg()
{
}

void			nuGLProg::Reset()					{ ResetBase(); }
const char*		nuGLProg::VertSrc()					{ return NULL; }
const char*		nuGLProg::FragSrc()					{ return NULL; }
const char*		nuGLProg::Name()					{ return "<unnamed shader>"; }
void			nuGLProg::ResetBase()				{ Vert = Frag = Prog = 0; }
bool			nuGLProg::LoadVariablePositions()	{ return false; }
uint32			nuGLProg::PlatformMask()			{ return nuPlatform_All; }
nuVertexType	nuGLProg::VertexType()				{ return nuVertexType_NULL; }
