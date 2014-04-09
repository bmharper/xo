#pragma once

#include "../nuDefs.h"
#include "nuVertexTypes.h"

class NUAPI nuGLProg : public nuProgBase
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
	virtual nuVertexType	VertexType();				// Not used on GL

	bool					UseOnThisPlatform() { return !!(PlatformMask() & NU_PLATFORM); }

protected:
	void					ResetBase();
};

