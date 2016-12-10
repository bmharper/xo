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
	v_f_tex0 = -1;
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
		"uniform sampler2D	f_tex0;\n"
		"\n"
		"varying		vec2	f_pos;\n"
		"varying		vec4	f_uv1;\n"
		"varying		vec4	f_uv2;\n"
		"varying		vec4	f_color1;\n"
		"varying		vec4	f_color2;\n"
		"varying 	float	f_shader;\n"
		"\n"
		"#if defined(XO_PLATFORM_WIN_DESKTOP) || defined(XO_PLATFORM_LINUX_DESKTOP)\n"
		"// This needs at least #version 130\n"
		"//layout(location = 0, index = 0) out vec4 outputColor0;\n"
		"//layout(location = 0, index = 1) out vec4 outputColor1;\n"
		"// On NVidia (9.18.13.3165 (10-23-2013), R331.65 (branch: r331_00-146)),\n"
		"// one doesn't need the layout qualification, nor glBindFragDataLocationIndexed. The order in which you\n"
		"// declare the output variables is sufficient to make an affinity with \"color0\" or \"color1\".\n"
		"// It is needed though, on Intel Haswell drivers on Linux\n"
		"out		vec4		out_color0;\n"
		"out		vec4		out_color1;\n"
		"\n"
		"void write_color(vec4 color)\n"
		"{\n"
		"	out_color0 = color;\n"
		"	out_color1 = color.aaaa;\n"
		"}\n"
		"#else\n"
		"void write_color(vec4 color)\n"
		"{\n"
		"	gl_FragColor = color;\n"
		"}\n"
		"#endif\n"
		"\n"
		"vec4 blend_over(vec4 a, vec4 b)\n"
		"{\n"
		"	return (1.0 - b.a) * a + b;\n"
		"}\n"
		"\n"
		"vec2 to_screen(vec2 pos)\n"
		"{\n"
		"	return (vec2(pos.x, -pos.y) + vec2(1,1)) * Frame_VPort_HSize;\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"	int shader = int(f_shader);\n"
		"	bool enableBGTex = (shader & SHADER_FLAG_TEXBG) != 0;\n"
		"	shader = shader & SHADER_TYPE_MASK;\n"
		"\n"
		"	if (shader == SHADER_ARC)\n"
		"	{\n"
		"		vec2 center1 = f_uv1.xy;\n"
		"		vec2 center2 = f_uv1.zw;\n"
		"		float radius1 = f_uv2.x;\n"
		"		float radius2 = f_uv2.y;\n"
		"		vec2 uv = f_uv2.zw;\n"
		"		vec4 bg_color = f_color1;\n"
		"		vec4 border_color = f_color2;\n"
		"\n"
		"		vec4 bg_tex = vec4(0, 0, 0, 0);\n"
		"		if (enableBGTex)\n"
		"		{\n"
		"			bg_tex = premultiply(texture2D(f_tex0, uv));\n"
		"			bg_color = blend_over(bg_color, bg_tex);\n"
		"		}\n"
		"\n"
		"		vec2 screen_pos = to_screen(f_pos.xy);\n"
		"		float distance1 = length(screen_pos - center1);\n"
		"		float distance2 = length(screen_pos - center2);\n"
		"		float color_blend = clamp(distance1 - radius1 + 0.5, 0.0, 1.0);\n"
		"		float alpha_blend = clamp(radius2 - distance2 + 0.5, 0.0, 1.0);\n"
		"		vec4 color = mix(bg_color, border_color, color_blend);\n"
		"		color *= alpha_blend;\n"
		"		write_color(color);\n"
		"	}\n"
		"	else if (shader == SHADER_RECT)\n"
		"	{\n"
		"		float border_width = f_uv1.x;\n"
		"		float border_distance = f_uv1.y;\n"
		"		vec2 uv = f_uv1.zw;\n"
		"		vec4 bg_color = f_color1;\n"
		"		vec4 border_color = f_color2;\n"
		"\n"
		"		// Distance from edge.\n"
		"		// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.\n"
		"		// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.\n"
		"		// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.\n"
		"		float edge_alpha = clamp(border_distance + 0.5, 0.0, 1.0);\n"
		"\n"
		"		// The +0.5 here is the same as above\n"
		"		float dclamped = clamp(border_width - border_distance + 0.5, 0.0, 1.0);\n"
		"\n"
		"		vec4 bg_tex = vec4(0, 0, 0, 0);\n"
		"		if (enableBGTex)\n"
		"		{\n"
		"			bg_tex = premultiply(texture2D(f_tex0, uv));\n"
		"			bg_color = blend_over(bg_color, bg_tex);\n"
		"		}\n"
		"\n"
		"		vec4 color = mix(bg_color, border_color, dclamped);\n"
		"		color *= edge_alpha;\n"
		"		write_color(color);\n"
		"	}\n"
		"	else if (shader == SHADER_TEXT_SIMPLE)\n"
		"	{\n"
		"		vec4 texCol = texture2D(f_tex0, f_uv1.xy);\n"
		"		write_color(texCol.rrrr * premultiply(f_color1));\n"
		"	}\n"
		"#if defined(XO_PLATFORM_WIN_DESKTOP) || defined(XO_PLATFORM_LINUX_DESKTOP)\n"
		"	else if (shader == SHADER_TEXT_SUBPIXEL)\n"
		"	{\n"
		"		float offset = 1.0 / XO_GLYPH_ATLAS_SIZE;\n"
		"		vec2 uv = f_uv1.xy;\n"
		"		vec4 tex_clamp = f_uv2;\n"
		"\n"
		"		float tap0 = texture2D(f_tex0, vec2(clamp(uv.s - offset * 3.0, tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"		float tap1 = texture2D(f_tex0, vec2(clamp(uv.s - offset * 2.0, tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"		float tap2 = texture2D(f_tex0, vec2(clamp(uv.s - offset * 1.0, tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"		float tap3 = texture2D(f_tex0, vec2(clamp(uv.s             ,   tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"		float tap4 = texture2D(f_tex0, vec2(clamp(uv.s + offset * 1.0, tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"		float tap5 = texture2D(f_tex0, vec2(clamp(uv.s + offset * 2.0, tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"		float tap6 = texture2D(f_tex0, vec2(clamp(uv.s + offset * 3.0, tex_clamp.x, tex_clamp.z), uv.t)).r;\n"
		"\n"
		"		float w0 = 0.56;\n"
		"		float w1 = 0.28;\n"
		"		float w2 = 0.16;\n"
		"		//float w0 = 0.60;\n"
		"		//float w1 = 0.28;\n"
		"		//float w2 = 0.12;\n"
		"\n"
		"		float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);\n"
		"		float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);\n"
		"		float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);\n"
		"		float aR = r * f_color1.a;\n"
		"		float aG = g * f_color1.a;\n"
		"		float aB = b * f_color1.a;\n"
		"		float avgA = (r + g + b) / 3.0;\n"
		"\n"
		"		out_color0 = vec4(f_color1.rgb, avgA);\n"
		"		out_color1 = vec4(aR, aG, aB, avgA);\n"
		"	}\n"
		"#endif\n"
		"	else\n"
		"	{\n"
		"		write_color(vec4(1, 1, 0, 1));\n"
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
	nfail += (v_f_tex0 = glGetUniformLocation( Prog, "f_tex0" )) == -1;
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

