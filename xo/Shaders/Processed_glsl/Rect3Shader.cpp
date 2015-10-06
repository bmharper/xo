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
	v_vborder_distance = -1;
	v_vborder_color = -1;
}

const char* xoGLProg_Rect3::VertSrc()
{
	return
		"uniform		mat4	mvproj;\n"
		"attribute	vec4	vpos;\n"
		"attribute	vec4	vcolor;\n"
		"attribute 	float	vborder_width;\n"
		"attribute 	float	vborder_distance;\n"
		"attribute 	vec4	vborder_color;\n"
		"\n"
		"varying 	vec4	pos;\n"
		"varying 	vec4	color;\n"
		"varying 	float	border_width;\n"
		"varying 	float	border_distance;\n"
		"varying 	vec4	border_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	pos = mvproj * vpos;\n"
		"	gl_Position = pos;\n"
		"	border_width = vborder_width;\n"
		"	border_distance = vborder_distance;\n"
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
		"varying 	float	border_distance;\n"
		"varying 	vec4	border_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	// Distance from edge.\n"
		"	// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.\n"
		"	// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.\n"
		"	// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.\n"
		"	float edge_alpha = clamp(border_distance + 0.5, 0, 1);\n"
		"\n"
		"	// The -0.5 here is the same as above, just reversed\n"
		"	float dclamped = clamp(border_width - border_distance + 0.5, 0, 1);\n"
		"\n"
		"	vec4 color = mix(color, border_color, dclamped);\n"
		"	color *= edge_alpha;\n"
		"	gl_FragColor = color;\n"
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
	nfail += (v_vborder_distance = glGetAttribLocation( Prog, "vborder_distance" )) == -1;
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

