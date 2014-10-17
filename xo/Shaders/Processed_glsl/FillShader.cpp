#include "pch.h"
#if XO_BUILD_OPENGL
#include "FillShader.h"

xoGLProg_Fill::xoGLProg_Fill()
{
	Reset();
}

void xoGLProg_Fill::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
}

const char* xoGLProg_Fill::VertSrc()
{
	return
	"uniform		mat4	mvproj;\n"
	"attribute	vec4	vpos;\n"
	"attribute	vec4	vcolor;\n"
	"varying		vec4	color;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = mvproj * vpos;\n"
	"	color = fromSRGB(vcolor);\n"
	"}\n"
;
}

const char* xoGLProg_Fill::FragSrc()
{
	return
	"varying vec4	color;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = color;\n"
	"}\n"
;
}

const char* xoGLProg_Fill::Name()
{
	return "Fill";
}


bool xoGLProg_Fill::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	if ( nfail != 0 )
		XOTRACE( "Failed to bind %d variables of shader Fill\n", nfail );

	return nfail == 0;
}

uint32 xoGLProg_Fill::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoGLProg_Fill::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

