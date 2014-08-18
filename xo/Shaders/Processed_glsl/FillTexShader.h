#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/xoRenderGL_Defs.h"

class xoGLProg_FillTex : public xoGLProg
{
public:
	xoGLProg_FillTex();
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
	GLint v_vtexuv0;          // attribute vec2
	GLint v_tex0;             // uniform sampler2D
};

#endif // XO_BUILD_OPENGL

