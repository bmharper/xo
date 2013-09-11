#pragma once

#include "nuDefs.h"

class NUAPI nuGLProg
{
public:
	GLuint Vert;
	GLuint Frag;
	GLuint Prog;

							nuGLProg();
	virtual					~nuGLProg();
	
	// All of these virtual functions are overridden by auto-generated shader objects
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();
	virtual uint32			PlatformMask(); 			// Combination of nuPlatformMask bits.

	bool					UseOnThisPlatform() { return !!(PlatformMask() & NU_PLATFORM); }

protected:
	void					ResetBase();
};

// Position, UV, Color
struct NUAPI nuVx_PTC
{
	// Note that nuRenderGL::DrawQuad assumes that we share our base layout with nuVx_PTCV4
	nuVec3	Pos;
	nuVec2	UV;
	uint32	Color;
};

// Position, UV, Color, Vec4
struct NUAPI nuVx_PTCV4
{
	// Note that nuRenderGL::DrawQuad assumes that we share our base layout with nuVx_PTC
	nuVec3	Pos;
	nuVec2	UV;
	uint32	Color;
	nuVec4	V4;
};

