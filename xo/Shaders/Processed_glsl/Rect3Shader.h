#pragma once
#if XO_BUILD_OPENGL

#include "../../Render/RenderGL_Defs.h"

class GLProg_Rect3 : public GLProg {
public:
	GLProg_Rect3();
	virtual void        Reset();
	virtual const char* VertSrc();
	virtual const char* FragSrc();
	virtual const char* Name();
	virtual bool        LoadVariablePositions(); // Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32_t      PlatformMask();          // Combination of Platform bits.
	virtual VertexType  VertexType();            // Only meaningful on DirectX

	GLint v_mvproj;           // uniform mat4
	GLint v_vpos;             // attribute vec4
	GLint v_vcolor;           // attribute vec4
	GLint v_vborder_width;    // attribute float
	GLint v_vborder_distance; // attribute float
	GLint v_vborder_color;    // attribute vec4
};

#endif // XO_BUILD_OPENGL
