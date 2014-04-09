#pragma once

#include "../../Render/nuRenderGL_Defs.h"

class nuGLProg_TextWhole : public nuGLProg
{
public:
	nuGLProg_TextWhole();
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32			PlatformMask();				// Combination of nuPlatform bits.
	virtual nuVertexType	VertexType();				// Only meaningful on DirectX

	GLint v_mvproj;           // uniform mat4
	GLint v_vpos;             // attribute vec4
	GLint v_vcolor;           // attribute vec4
	GLint v_vtexuv0;          // attribute vec2
	GLint v_tex0;             // uniform sampler2D
};

