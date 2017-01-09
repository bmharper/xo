#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/RenderGL_Defs.h"

namespace xo {

class GLProg_Rect : public GLProg
{
public:
	GLProg_Rect();
	virtual void            Reset();
	virtual const char*     VertSrc();
	virtual const char*     FragSrc();
	virtual const char*     Name();
	virtual bool            LoadVariablePositions();  // Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32_t        PlatformMask();           // Combination of Platform bits.
	virtual xo::VertexType  VertexType();             // Only meaningful on DirectX

	GLint v_mvproj;                          // uniform mat4
	GLint v_vpos;                            // attribute vec4
	GLint v_vcolor;                          // attribute vec4
	GLint v_radius;                          // uniform float
	GLint v_box;                             // uniform vec4
	GLint v_border;                          // uniform vec4
	GLint v_border_color;                    // uniform vec4
	GLint v_vport_hsize;                     // uniform vec2
};

} // namespace xo

#endif // XO_BUILD_OPENGL

