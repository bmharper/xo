#include "pch.h"
#include "FillShader.h"

nuGLProg_Fill::nuGLProg_Fill()
{
	Reset();
}

void nuGLProg_Fill::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
}

const char* nuGLProg_Fill::VertSrc()
{
	return
"	uniform		mat4	mvproj;\n"
"	attribute	vec4	vpos;\n"
"	attribute	vec4	vcolor;\n"
"	varying		vec4	color;\n"
"	void main()\n"
"	{\n"
"		gl_Position = mvproj * vpos;\n"
"		color = vcolor;\n"
"	}\n"
;
}

const char* nuGLProg_Fill::FragSrc()
{
	return
"	#ifdef NU_PLATFORM_ANDROID\n"
"	precision mediump float;\n"
"	#endif\n"
"	varying vec4	color;\n"
"	void main()\n"
"	{\n"
"		gl_FragColor = color;\n"
"	}\n"
;
}

const char* nuGLProg_Fill::Name()
{
	return "Fill";
}


bool nuGLProg_Fill::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader Fill\n", nfail );

	return nfail == 0;
}

uint32 nuGLProg_Fill::PlatformMask()
{
	return nuPlatform_All;
}


