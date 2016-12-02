#include "pch.h"
#if XO_BUILD_OPENGL
#include "UberShader.h"

xoGLProg_Uber::xoGLProg_Uber()
{
	Reset();
}

void xoGLProg_Uber::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_v_pos = -1;
	v_v_uv1 = -1;
	v_v_uv2 = -1;
	v_v_color1 = -1;
	v_v_color2 = -1;
	v_v_shader = -1;
	v_Frame_VPort_HSize = -1;
}

const char* xoGLProg_Uber::VertSrc()
{
	return
		"uniform		mat4	mvproj; // Change to Frame_MVProj once we've gotten rid of the old ones\n"
		"\n"
		"// CPU -> Vertex Shader: 8 + 2 * 16 + 3 * 4 = 52 bytes per vertex\n"
		"// Vertex -> Fragment:   8 + 4 * 16 + 4     = 72 bytes per fragment\n"
		"\n"
		"attribute	vec2	v_pos;\n"
		"attribute	vec4	v_uv1;\n"
		"attribute	vec4	v_uv2;\n"
		"attribute	vec4	v_color1;\n"
		"attribute	vec4	v_color2;\n"
		"attribute	float	v_shader;\n"
		"\n"
		"varying		vec2	f_pos;\n"
		"varying		vec4	f_uv1;\n"
		"varying		vec4	f_uv2;\n"
		"varying		vec4	f_color1;\n"
		"varying		vec4	f_color2;\n"
		"varying 	float	f_shader;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec4 pos = mvproj * vec4(v_pos.x, v_pos.y, 0, 1);\n"
		"	gl_Position = pos;\n"
		"	f_pos = pos.xy;\n"
		"	f_uv1 = v_uv1;\n"
		"	f_uv2 = v_uv2;\n"
		"	f_color1 = fromSRGB(v_color1);\n"
		"	f_color2 = fromSRGB(v_color2);\n"
		"	f_shader = v_shader;\n"
		"}\n"
;
}

const char* xoGLProg_Uber::FragSrc()
{
	return
		"uniform		vec2	Frame_VPort_HSize;\n"
		"\n"
		"varying		vec2	f_pos;\n"
		"varying		vec4	f_uv1;\n"
		"varying		vec4	f_uv2;\n"
		"varying		vec4	f_color1;\n"
		"varying		vec4	f_color2;\n"
		"varying 	float	f_shader;\n"
		"\n"
		"vec2 to_screen(vec2 pos)\n"
		"{\n"
		"	return (vec2(pos.x, -pos.y) + vec2(1,1)) * Frame_VPort_HSize;\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"	if (f_shader == SHADER_ARC)\n"
		"	{\n"
		"		vec2 center1 = f_uv1.xy;\n"
		"		vec2 center2 = f_uv1.zw;\n"
		"		float radius1 = f_uv2.x;\n"
		"		float radius2 = f_uv2.y;\n"
		"		vec4 bg_color = f_color1;\n"
		"		vec4 border_color = f_color2;\n"
		"\n"
		"		vec2 screen_pos = to_screen(f_pos.xy);\n"
		"		float distance1 = length(screen_pos - center1);\n"
		"		float distance2 = length(screen_pos - center2);\n"
		"		float color_blend = clamp(distance1 - radius1 + 0.5, 0, 1);\n"
		"		float alpha_blend = clamp(radius2 - distance2 + 0.5, 0, 1);\n"
		"		vec4 color = mix(bg_color, border_color, color_blend);\n"
		"		color *= alpha_blend;\n"
		"		gl_FragColor = color;\n"
		"	}\n"
		"	else if (f_shader == SHADER_RECT)\n"
		"	{\n"
		"		float border_distance = f_uv1.y;\n"
		"		float border_width = f_uv1.x;\n"
		"\n"
		"		// Distance from edge.\n"
		"		// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.\n"
		"		// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.\n"
		"		// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.\n"
		"		float edge_alpha = clamp(border_distance + 0.5, 0, 1);\n"
		"\n"
		"		// The +0.5 here is the same as above\n"
		"		float dclamped = clamp(border_width - border_distance + 0.5, 0, 1);\n"
		"\n"
		"		vec4 color = mix(f_color1, f_color2, dclamped);\n"
		"		color *= edge_alpha;\n"
		"		gl_FragColor = color;\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		gl_FragColor = vec4(1, 1, 0, 1);\n"
		"	}\n"
		"}\n"
;
}

const char* xoGLProg_Uber::Name()
{
	return "Uber";
}


bool xoGLProg_Uber::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_v_pos = glGetAttribLocation( Prog, "v_pos" )) == -1;
	nfail += (v_v_uv1 = glGetAttribLocation( Prog, "v_uv1" )) == -1;
	nfail += (v_v_uv2 = glGetAttribLocation( Prog, "v_uv2" )) == -1;
	nfail += (v_v_color1 = glGetAttribLocation( Prog, "v_color1" )) == -1;
	nfail += (v_v_color2 = glGetAttribLocation( Prog, "v_color2" )) == -1;
	nfail += (v_v_shader = glGetAttribLocation( Prog, "v_shader" )) == -1;
	nfail += (v_Frame_VPort_HSize = glGetUniformLocation( Prog, "Frame_VPort_HSize" )) == -1;
	if (nfail != 0)
		XOTRACE("Failed to bind %d variables of shader Uber\n", nfail);

	return nfail == 0;
}

uint32 xoGLProg_Uber::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoGLProg_Uber::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

