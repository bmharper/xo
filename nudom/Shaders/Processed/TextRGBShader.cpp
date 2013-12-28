#include "pch.h"
#include "TextRGBShader.h"

nuGLProg_TextRGB::nuGLProg_TextRGB()
{
	Reset();
}

void nuGLProg_TextRGB::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
	v_vtexuv0 = -1;
	v_vtexClamp = -1;
	v_tex0 = -1;
}

const char* nuGLProg_TextRGB::VertSrc()
{
	return
"	uniform		mat4	mvproj;\n"
"	attribute	vec4	vpos;\n"
"	attribute	vec4	vcolor;\n"
"	attribute	vec2	vtexuv0;\n"
"	attribute	vec4	vtexClamp;\n"
"	varying		vec4	color;\n"
"	varying		vec2	texuv0;\n"
"	varying		vec4	texClamp;\n"
"	void main()\n"
"	{\n"
"		gl_Position = mvproj * vpos;\n"
"		texuv0 = vtexuv0;\n"
"		texClamp = vtexClamp;\n"
"	\n"
"		color.rgb = pow( vcolor.rgb, vec3(2.2, 2.2, 2.2) );\n"
"		color.a = vcolor.a;\n"
"	}\n"
;
}

const char* nuGLProg_TextRGB::FragSrc()
{
	return
"	#version 330\n"
"	\n"
"	#ifdef NU_PLATFORM_ANDROID\n"
"	precision mediump float;\n"
"	#endif\n"
"	\n"
"	// This looks absolutely terrible without sRGB blending, so we simply don't even try subpixel text\n"
"	// on those platforms. As it turns out, there is great correlation between low res screen (Windows)\n"
"	// and a GPU capable of sRGB blending.\n"
"	\n"
"	uniform sampler2D	tex0;\n"
"	varying vec4		color;\n"
"	varying vec2		texuv0;\n"
"	varying vec4		texClamp;\n"
"	//layout(location = 0, index = 0) out vec4 outputColor0;\n"
"	//layout(location = 0, index = 1) out vec4 outputColor1;\n"
"	// On NVidia (9.18.13.3165 (10-23-2013), R331.65 (branch: r331_00-146)),\n"
"	// one doesn't need the layout qualification, nor glBindFragDataLocationIndexed. The order in which you\n"
"	// declare the output variables is sufficient to make an affinity with \"color0\" or \"color1\".\n"
"	out		vec4		outputColor0;\n"
"	out		vec4		outputColor1;\n"
"	void main()\n"
"	{\n"
"		float offset = 1.0 / NU_GLYPH_ATLAS_SIZE;\n"
"		vec2 uv = texuv0;\n"
"	\n"
"		float tap0 = texture2D(tex0, vec2(clamp(uv.s - offset * 3.0, texClamp.x, texClamp.z), uv.t)).r;\n"
"		float tap1 = texture2D(tex0, vec2(clamp(uv.s - offset * 2.0, texClamp.x, texClamp.z), uv.t)).r;\n"
"		float tap2 = texture2D(tex0, vec2(clamp(uv.s - offset * 1.0, texClamp.x, texClamp.z), uv.t)).r;\n"
"		float tap3 = texture2D(tex0, vec2(clamp(uv.s             ,   texClamp.x, texClamp.z), uv.t)).r;\n"
"		float tap4 = texture2D(tex0, vec2(clamp(uv.s + offset * 1.0, texClamp.x, texClamp.z), uv.t)).r;\n"
"		float tap5 = texture2D(tex0, vec2(clamp(uv.s + offset * 2.0, texClamp.x, texClamp.z), uv.t)).r;\n"
"		float tap6 = texture2D(tex0, vec2(clamp(uv.s + offset * 3.0, texClamp.x, texClamp.z), uv.t)).r;\n"
"	\n"
"		float w0 = 0.55;\n"
"		float w1 = 0.32;\n"
"		float w2 = 0.13;\n"
"		//float w0 = 0.98;\n"
"		//float w1 = 0.01;\n"
"		//float w2 = 0.01;\n"
"	\n"
"		float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);\n"
"		float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);\n"
"		float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);\n"
"		float aR = r * color.a;\n"
"		float aG = g * color.a;\n"
"		float aB = b * color.a;\n"
"		float avgA = (r + g + b) / 3.0;\n"
"		//float minA = min(r,g,min(g,b));\n"
"		// ONE MINUS SRC COLOR\n"
"		//float alpha = min(min(red, green), blue);\n"
"		//gl_FragColor = vec4(aR, aG, aB, avgA);\n"
"	\n"
"		outputColor0 = vec4(color.rgb, avgA);\n"
"		outputColor1 = vec4(aR, aG, aB, avgA);\n"
"	}\n"
;
}

const char* nuGLProg_TextRGB::Name()
{
	return "TextRGB";
}


bool nuGLProg_TextRGB::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_vtexuv0 = glGetAttribLocation( Prog, "vtexuv0" )) == -1;
	nfail += (v_vtexClamp = glGetAttribLocation( Prog, "vtexClamp" )) == -1;
	nfail += (v_tex0 = glGetUniformLocation( Prog, "tex0" )) == -1;
	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader TextRGB\n", nfail );

	return nfail == 0;
}

uint32 nuGLProg_TextRGB::PlatformMask()
{
	return nuPlatform_WinDesktop;
}


