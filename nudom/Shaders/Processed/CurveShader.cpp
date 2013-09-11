#include "pch.h"
#include "CurveShader.h"

nuGLProg_Curve::nuGLProg_Curve()
{
	Reset();
}

void nuGLProg_Curve::Reset()
{
	ResetBase();

}

const char* nuGLProg_Curve::VertSrc()
{
	return
"	varying vec4 pos;\n"
"	varying vec2 texuv0;\n"
"	void main()\n"
"	{\n"
"		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"		gl_FrontColor = gl_Color;\n"
"		pos = gl_Position;\n"
"		texuv0 = gl_MultiTexCoord0.xy;\n"
"	}\n"
"	\n"
;
}

const char* nuGLProg_Curve::FragSrc()
{
	return
"	// This is from Jim Blinn and Charles Loop's paper \"Resolution Independent Curve Rendering using Programmable Graphics Hardware\"\n"
"	// We don't need this complexity here.. and if I recall correctly, this technique aliases under minification faster than\n"
"	// our simpler rounded-rectangle alternative.\n"
"	varying vec4 pos;\n"
"	varying vec2 texuv0;\n"
"	\n"
"	void main()\n"
"	{\n"
"		vec2 p = texuv0;\n"
"	\n"
"		// Gradients\n"
"		vec2 px = dFdx(p);\n"
"		vec2 py = dFdy(p);\n"
"	\n"
"		// Chain rule\n"
"		float fx = (2 * p.x) * px.x - px.y;\n"
"		float fy = (2 * p.x) * py.x - py.y;\n"
"	\n"
"		// Signed distance\n"
"		float sd = (p.x * p.x - p.y) / sqrt(fx * fx + fy * fy);\n"
"	\n"
"		// Linear alpha\n"
"		float alpha = 0.5 - sd;\n"
"	\n"
"		gl_FragColor = gl_Color;\n"
"	\n"
"		if ( alpha > 1 )\n"
"			gl_FragColor.a = 1;\n"
"		else if ( alpha < 0 )\n"
"			discard;\n"
"		else\n"
"			gl_FragColor.a = alpha;\n"
"	}\n"
"	\n"
;
}

const char* nuGLProg_Curve::Name()
{
	return "Curve";
}


bool nuGLProg_Curve::LoadVariablePositions()
{
	int nfail = 0;

	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader Curve\n", nfail );

	return nfail == 0;
}

uint32 nuGLProg_Curve::PlatformMask()
{
	return nuPlatform_All;
}


