#include "pch.h"
#if XO_BUILD_OPENGL
#include "FillTexShader.h"

xoGLProg_FillTex::xoGLProg_FillTex()
{
	Reset();
}

void xoGLProg_FillTex::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
	v_vtexuv0 = -1;
	v_tex0 = -1;
}

const char* xoGLProg_FillTex::VertSrc()
{
	return
	"uniform		mat4	mvproj;\n"
	"attribute	vec4	vpos;\n"
	"attribute	vec4	vcolor;\n"
	"attribute	vec2	vtexuv0;\n"
	"varying		vec4	color;\n"
	"varying		vec2	texuv0;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = mvproj * vpos;\n"
	"	texuv0 = vtexuv0;\n"
	"	color = vec4(pow(vcolor.rgb, vec3(2.2, 2.2, 2.2)), vcolor.a);\n"
	"}\n"
;
}

const char* xoGLProg_FillTex::FragSrc()
{
	return
	"#ifdef XO_PLATFORM_ANDROID\n"
	"precision mediump float;\n"
	"#endif\n"
	"uniform sampler2D	tex0;\n"
	"varying vec4		color;\n"
	"varying vec2		texuv0;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = color * texture2D(tex0, texuv0.st);\n"
	"}\n"
;
}

const char* xoGLProg_FillTex::Name()
{
	return "FillTex";
}


bool xoGLProg_FillTex::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_vtexuv0 = glGetAttribLocation( Prog, "vtexuv0" )) == -1;
	nfail += (v_tex0 = glGetUniformLocation( Prog, "tex0" )) == -1;
	if ( nfail != 0 )
		XOTRACE( "Failed to bind %d variables of shader FillTex\n", nfail );

	return nfail == 0;
}

uint32 xoGLProg_FillTex::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoGLProg_FillTex::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

