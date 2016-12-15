#include "pch.h"
#if XO_BUILD_OPENGL
#include "RenderGL_Defs.h"

namespace xo {

GLProg::GLProg() {
	ResetBase();
}

GLProg::~GLProg() {
}

void        GLProg::Reset() { ResetBase(); }
const char* GLProg::VertSrc() { return NULL; }
const char* GLProg::FragSrc() { return NULL; }
const char* GLProg::Name() { return "<unnamed shader>"; }
void        GLProg::ResetBase() { Vert = Frag = Prog = 0; }
bool        GLProg::LoadVariablePositions() { return false; }
uint32_t      GLProg::PlatformMask() { return Platform_All; }
VertexType  GLProg::VertexType() { return VertexType_NULL; }
}
#endif
