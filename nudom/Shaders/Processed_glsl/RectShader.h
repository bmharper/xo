#pragma once

#include "../../Render/nuRenderGL_Defs.h"

class nuGLProg_Rect : public nuGLProg
{
public:
	nuGLProg_Rect();
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
	GLint v_radius;           // uniform float
	GLint v_box;              // uniform vec4
	GLint v_vport_hsize;      // uniform vec2
};

