#include "pch.h"
#if XO_BUILD_DIRECTX
#include "RenderDX_Defs.h"

namespace xo {

DXProg::DXProg() {
	ResetBase();
}

DXProg::~DXProg() {
}

void        DXProg::Reset() { ResetBase(); }
const char* DXProg::VertSrc() { return NULL; }
const char* DXProg::FragSrc() { return NULL; }
const char* DXProg::Name() { return "<unnamed shader>"; }
void        DXProg::ResetBase() {
    Vert       = NULL;
    Frag       = NULL;
    VertLayout = NULL;
}
bool       DXProg::LoadVariablePositions() { return false; }
uint32_t     DXProg::PlatformMask() { return Platform_All; }
VertexType DXProg::VertexType() { return VertexType_NULL; }
}
#endif
