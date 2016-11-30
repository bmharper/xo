#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/xoRenderGL_Defs.h"

class xoGLProg_Arc : public xoGLProg
{
public:
	xoGLProg_Arc();
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32			PlatformMask();				// Combination of xoPlatform bits.
	virtual xoVertexType	VertexType();				// Only meaningful on DirectX

	GLint v_mvproj;                          // uniform mat4
	GLint v_vpos;                            // attribute vec4
	GLint v_vcenter;                         // attribute vec4
	GLint v_vcolor;                          // attribute vec4
	GLint v_vborder_color;                   // attribute vec4
	GLint v_vradius1;                        // attribute float
	GLint v_vradius2;                        // attribute float
	GLint v_vport_hsize;                     // uniform vec2
};

#endif // XO_BUILD_OPENGL

