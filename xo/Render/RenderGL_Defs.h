#pragma once
#if XO_BUILD_OPENGL

#include "../Defs.h"
#include "VertexTypes.h"

namespace xo {

class XO_API GLProg : public ProgBase {
public:
	GLuint Vert;
	GLuint Frag;
	GLuint Prog;

	GLProg();
	virtual ~GLProg();

	// All of these virtual functions are overridden by auto-generated shader objects
	virtual void        Reset();
	virtual const char* VertSrc();
	virtual const char* FragSrc();
	virtual const char* Name();
	virtual bool        LoadVariablePositions();
	virtual uint32_t    PlatformMask(); // Combination of PlatformMask bits.
	virtual VertexType  VertexType();   // Not used on GL

	bool UseOnThisPlatform() { return !!(PlatformMask() & XO_PLATFORM); }

protected:
	void ResetBase();
};
}
#endif
