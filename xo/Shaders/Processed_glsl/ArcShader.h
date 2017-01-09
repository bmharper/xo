#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/RenderGL_Defs.h"

namespace xo {

class GLProg_Arc : public GLProg
{
public:
	GLProg_Arc();
	virtual void            Reset();
	virtual const char*     VertSrc();
	virtual const char*     FragSrc();
	virtual const char*     Name();
	virtual bool            LoadVariablePositions();  // Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32_t        PlatformMask();           // Combination of Platform bits.
	virtual xo::VertexType  VertexType();             // Only meaningful on DirectX

	GLint v_mvproj;                          // uniform mat4
	GLint v_vpos;                            // attribute vec4
	GLint v_vcenter;                         // attribute vec4
	GLint v_vcolor;                          // attribute vec4
	GLint v_vborder_color;                   // attribute vec4
	GLint v_vradius1;                        // attribute float
	GLint v_vradius2;                        // attribute float
	GLint v_vport_hsize;                     // uniform vec2
};

} // namespace xo

#endif // XO_BUILD_OPENGL

