#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/xoRenderGL_Defs.h"

class xoGLProg_Rect2 : public xoGLProg
{
public:
	xoGLProg_Rect2();
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32			PlatformMask();				// Combination of xoPlatform bits.
	virtual xoVertexType	VertexType();				// Only meaningful on DirectX

	GLint v_mvproj;           // uniform mat4
	GLint v_vpos;             // attribute vec4
	GLint v_vcolor;           // attribute vec4
	GLint v_vradius;          // attribute float
	GLint v_vborder_width;    // attribute float
	GLint v_vborder_color;    // attribute vec4
	GLint v_vport_hsize;      // uniform vec2
	GLint v_out_vector;       // uniform vec2
	GLint v_shadow_offset;    // uniform vec2
	GLint v_shadow_color;     // uniform vec4
	GLint v_shadow_size_inv;  // uniform float
	GLint v_edges;            // uniform vec2
};

#endif // XO_BUILD_OPENGL

