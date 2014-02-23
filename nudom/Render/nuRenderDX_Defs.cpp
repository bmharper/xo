#include "pch.h"
#include "nuRenderDX_Defs.h"

nuDXProg::nuDXProg()
{
	ResetBase();
}

nuDXProg::~nuDXProg()
{
}

void		nuDXProg::Reset()					{ ResetBase(); }
const char* nuDXProg::VertSrc()					{ return NULL; }
const char* nuDXProg::FragSrc()					{ return NULL; }
const char* nuDXProg::Name()					{ return "<unnamed shader>"; }
void		nuDXProg::ResetBase()				{ Vert = NULL; Frag = NULL; VertLayout = NULL; }
bool		nuDXProg::LoadVariablePositions()	{ return false; }
uint32		nuDXProg::PlatformMask()			{ return nuPlatform_All; }
