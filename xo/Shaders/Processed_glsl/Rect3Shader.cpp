#include "pch.h"
#if XO_BUILD_OPENGL
#include "Rect3Shader.h"

xoGLProg_Rect3::xoGLProg_Rect3()
{
	Reset();
}

void xoGLProg_Rect3::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
	v_vborder_width = -1;
	v_vdistance = -1;
	v_vborder_color = -1;
}

const char* xoGLProg_Rect3::VertSrc()
{
	return
		"uniform		mat4	mvproj;\n"
		"attribute	vec4	vpos;\n"
		"attribute	vec4	vcolor;\n"
		"attribute 	float	vborder_width;\n"
		"attribute 	float	vdistance;\n"
		"attribute 	vec4	vborder_color;\n"
		"\n"
		"varying 	vec4	pos;\n"
		"varying 	vec4	color;\n"
		"varying 	float	border_width;\n"
		"varying 	float	distance;			// distance from center\n"
		"varying 	vec4	border_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	pos = mvproj * vpos;\n"
		"	gl_Position = pos;\n"
		"	border_width = vborder_width;\n"
		"	distance = vdistance;\n"
		"	border_color = fromSRGB(vborder_color);\n"
		"	color = fromSRGB(vcolor);\n"
		"}\n"
;
}

const char* xoGLProg_Rect3::FragSrc()
{
	return
		"varying 	vec4	pos;\n"
		"varying 	vec4	color;\n"
		"varying 	float	border_width;\n"
		"varying 	float	distance;\n"
		"varying 	vec4	border_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	float dclamped = clamp(distance - border_width, 0, 1);\n"
		"	gl_FragColor = mix(color, border_color, dclamped);\n"
		"}\n"
;
}

const char* xoGLProg_Rect3::Name()
{
	return "Rect3";
}


bool xoGLProg_Rect3::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_vborder_width = glGetAttribLocation( Prog, "vborder_width" )) == -1;
	nfail += (v_vdistance = glGetAttribLocation( Prog, "vdistance" )) == -1;
	nfail += (v_vborder_color = glGetAttribLocation( Prog, "vborder_color" )) == -1;
	if (nfail != 0)
		XOTRACE("Failed to bind %d variables of shader Rect3\n", nfail);

	return nfail == 0;
}

uint32 xoGLProg_Rect3::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoGLProg_Rect3::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

