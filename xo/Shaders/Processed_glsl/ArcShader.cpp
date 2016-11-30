#include "pch.h"
#if XO_BUILD_OPENGL
#include "ArcShader.h"

xoGLProg_Arc::xoGLProg_Arc()
{
	Reset();
}

void xoGLProg_Arc::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcenter = -1;
	v_vcolor = -1;
	v_vborder_color = -1;
	v_vradius1 = -1;
	v_vradius2 = -1;
	v_vport_hsize = -1;
}

const char* xoGLProg_Arc::VertSrc()
{
	return
		"uniform		mat4	mvproj;\n"
		"\n"
		"attribute	vec4	vpos;\n"
		"attribute	vec4	vcenter;\n"
		"attribute	vec4	vcolor;\n"
		"attribute	vec4	vborder_color;\n"
		"attribute	float	vradius1;\n"
		"attribute	float	vradius2;\n"
		"\n"
		"varying		vec4	pos;\n"
		"varying		vec2	center1;\n"
		"varying		vec2	center2;\n"
		"varying		vec4	color;\n"
		"varying		vec4	border_color;\n"
		"varying		float	radius1;\n"
		"varying		float	radius2;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	pos = mvproj * vpos;\n"
		"	center1 = vcenter.xy;\n"
		"	center2 = vcenter.zw;\n"
		"	gl_Position = pos;\n"
		"	color = fromSRGB(vcolor);\n"
		"	border_color = fromSRGB(vborder_color);\n"
		"	radius1 = vradius1;\n"
		"	radius2 = vradius2;\n"
		"}\n"
;
}

const char* xoGLProg_Arc::FragSrc()
{
	return
		"uniform		vec2	vport_hsize;\n"
		"\n"
		"varying		vec4	pos;\n"
		"varying		vec2	center1;		// Center of arc1\n"
		"varying		vec2	center2;		// Center of arc2\n"
		"varying		vec4	color;\n"
		"varying		vec4	border_color;\n"
		"varying		float	radius1;		// Radius of arc1 = Start of border\n"
		"varying		float	radius2;		// Radius of arc2 = End of border (and everything else). radius1 = radius2 - border_width\n"
		"\n"
		"vec2 to_screen(vec2 unit_pt)\n"
		"{\n"
		"	return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec2 screen_pos = to_screen(pos.xy);\n"
		"	float distance1 = length(screen_pos - center1);\n"
		"	float distance2 = length(screen_pos - center2);\n"
		"	vec4 out_color = color;\n"
		"	float color_blend = clamp(distance1 - radius1 + 0.5, 0, 1);\n"
		"	float alpha_blend = clamp(radius2 - distance2 + 0.5, 0, 1);\n"
		"	out_color = mix(color, border_color, color_blend);\n"
		"	//out_color.r *= 0.5;\n"
		"	out_color *= alpha_blend;\n"
		"	gl_FragColor = out_color;\n"
		"}\n"
;
}

const char* xoGLProg_Arc::Name()
{
	return "Arc";
}


bool xoGLProg_Arc::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcenter = glGetAttribLocation( Prog, "vcenter" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_vborder_color = glGetAttribLocation( Prog, "vborder_color" )) == -1;
	nfail += (v_vradius1 = glGetAttribLocation( Prog, "vradius1" )) == -1;
	nfail += (v_vradius2 = glGetAttribLocation( Prog, "vradius2" )) == -1;
	nfail += (v_vport_hsize = glGetUniformLocation( Prog, "vport_hsize" )) == -1;
	if (nfail != 0)
		XOTRACE("Failed to bind %d variables of shader Arc\n", nfail);

	return nfail == 0;
}

uint32 xoGLProg_Arc::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoGLProg_Arc::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

