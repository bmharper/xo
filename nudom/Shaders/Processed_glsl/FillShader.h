#pragma once

#include "../../Render/nuRenderGL_Defs.h"

class nuGLProg_Fill : public nuGLProg
{
public:
	nuGLProg_Fill();
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
};

