#pragma once
#if XO_BUILD_OPENGL

#include "../xoDefs.h"
#include "xoVertexTypes.h"

class XOAPI xoGLProg : public xoProgBase
{
public:
	GLuint Vert;
	GLuint Frag;
	GLuint Prog;

							xoGLProg();
	virtual					~xoGLProg();
	
	// All of these virtual functions are overridden by auto-generated shader objects
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();
	virtual uint32			PlatformMask(); 			// Combination of xoPlatformMask bits.
	virtual xoVertexType	VertexType();				// Not used on GL

	bool					UseOnThisPlatform() { return !!(PlatformMask() & XO_PLATFORM); }

protected:
	void					ResetBase();
};

#endif