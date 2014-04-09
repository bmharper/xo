#include "pch.h"
#include "RectShader.h"

nuGLProg_Rect::nuGLProg_Rect()
{
	Reset();
}

void nuGLProg_Rect::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
	v_radius = -1;
	v_box = -1;
	v_vport_hsize = -1;
}

const char* nuGLProg_Rect::VertSrc()
{
	return
"	uniform		mat4	mvproj;\n"
"	attribute	vec4	vpos;\n"
"	attribute	vec4	vcolor;\n"
"	varying		vec4	pos;\n"
"	varying		vec4	color;\n"
"	void main()\n"
"	{\n"
"		pos = mvproj * vpos;\n"
"		gl_Position = pos;\n"
"		color = vec4(pow(vcolor.rgb, vec3(2.2, 2.2, 2.2)), vcolor.a);\n"
"	}\n"
;
}

const char* nuGLProg_Rect::FragSrc()
{
	return
"	#ifdef NU_PLATFORM_ANDROID\n"
"	precision mediump float;\n"
"	#endif\n"
"	varying vec4	pos;\n"
"	varying vec4	color;\n"
"	uniform float	radius;\n"
"	uniform vec4	box;\n"
"	uniform vec2	vport_hsize;\n"
"	\n"
"	vec2 to_screen( vec2 unit_pt )\n"
"	{\n"
"		return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;\n"
"	}\n"
"	\n"
"	void main()\n"
"	{\n"
"		vec2 screenxy = to_screen(pos.xy);\n"
"		float left = box.x + radius;\n"
"		float right = box.z - radius;\n"
"		float top = box.y + radius;\n"
"		float bottom = box.w - radius;\n"
"		vec2 cent = screenxy;\n"
"	\n"
"		cent.x = clamp(cent.x, left, right);\n"
"		cent.y = clamp(cent.y, top, bottom);\n"
"	\n"
"		// If you draw the pixels out on paper, and take cognisance of the fact that\n"
"		// our samples are at pixel centers, then this -0.5 offset makes perfect sense.\n"
"		// This offset is correct regardless of whether you're blending linearly or in gamma space.\n"
"		// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset\n"
"		// that is fed into the shader's \"radius\" uniform, we effectively get rectangles to be sharp\n"
"		// when they are aligned to an integer grid. I haven't thought this through carefully enough,\n"
"		// but it does feel right.\n"
"		float dist = length(screenxy - cent) - 0.5;\n"
"	\n"
"		vec4 outcolor = color;\n"
"		outcolor.a *= clamp(radius - dist, 0.0, 1.0);\n"
"	\n"
"	#ifdef NU_SRGB_FRAMEBUFFER\n"
"		gl_FragColor = outcolor;\n"
"	#else\n"
"		float igamma = 1.0/2.2;\n"
"		gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));\n"
"		gl_FragColor.a = outcolor.a;\n"
"	#endif\n"
"	}\n"
;
}

const char* nuGLProg_Rect::Name()
{
	return "Rect";
}


bool nuGLProg_Rect::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_radius = glGetUniformLocation( Prog, "radius" )) == -1;
	nfail += (v_box = glGetUniformLocation( Prog, "box" )) == -1;
	nfail += (v_vport_hsize = glGetUniformLocation( Prog, "vport_hsize" )) == -1;
	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader Rect\n", nfail );

	return nfail == 0;
}

uint32 nuGLProg_Rect::PlatformMask()
{
	return nuPlatform_All;
}

nuVertexType nuGLProg_Rect::VertexType()
{
	return nuVertexType_NULL;
}


