#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/xoRenderGL_Defs.h"

class xoGLProg_Uber : public xoGLProg
{
public:
	xoGLProg_Uber();
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32			PlatformMask();				// Combination of xoPlatform bits.
	virtual xoVertexType	VertexType();				// Only meaningful on DirectX

	GLint v_mvproj;                          // uniform mat4
	GLint v_v_pos;                           // attribute vec2
	GLint v_v_uv1;                           // attribute vec4
	GLint v_v_uv2;                           // attribute vec4
	GLint v_v_color1;                        // attribute vec4
	GLint v_v_color2;                        // attribute vec4
	GLint v_v_shader;                        // attribute float
	GLint v_Frame_VPort_HSize;               // uniform vec2
	GLint v_f_tex0;                          // uniform sampler2D
};

#endif // XO_BUILD_OPENGL

