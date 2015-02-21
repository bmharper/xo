#include "pch.h"
#if XO_BUILD_OPENGL
#include "CurveShader.h"

xoGLProg_Curve::xoGLProg_Curve()
{
	Reset();
}

void xoGLProg_Curve::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
	v_vtexuv0 = -1;
}

const char* xoGLProg_Curve::VertSrc()
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
		"	color = fromSRGB(vcolor);\n"
		"}\n"
		"\n"
;
}

const char* xoGLProg_Curve::FragSrc()
{
	return
		"// This is from Jim Blinn and Charles Loop's paper \"Resolution Independent Curve Rendering using Programmable Graphics Hardware\"\n"
		"// We don't need this complexity here.. and if I recall correctly, this technique aliases under minification faster than\n"
		"// our simpler rounded-rectangle alternative.\n"
		"varying vec2 texuv0;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec2 p = texuv0;\n"
		"\n"
		"	// Gradients\n"
		"	vec2 px = dFdx(p);\n"
		"	vec2 py = dFdy(p);\n"
		"\n"
		"	// Chain rule\n"
		"	float fx = (2 * p.x) * px.x - px.y;\n"
		"	float fy = (2 * p.x) * py.x - py.y;\n"
		"\n"
		"	// Signed distance\n"
		"	float sd = (p.x * p.x - p.y) / sqrt(fx * fx + fy * fy);\n"
		"\n"
		"	// Linear alpha\n"
		"	float alpha = 0.5 - sd;\n"
		"\n"
		"	gl_FragColor = gl_Color;\n"
		"\n"
		"	if ( alpha > 1 )\n"
		"		gl_FragColor.a = 1;\n"
		"	else if ( alpha < 0 )\n"
		"		discard;\n"
		"	else\n"
		"		gl_FragColor.a = alpha;\n"
		"}\n"
		"\n"
;
}

const char* xoGLProg_Curve::Name()
{
	return "Curve";
}


bool xoGLProg_Curve::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_vtexuv0 = glGetAttribLocation( Prog, "vtexuv0" )) == -1;
	if (nfail != 0)
		XOTRACE("Failed to bind %d variables of shader Curve\n", nfail);

	return nfail == 0;
}

uint32 xoGLProg_Curve::PlatformMask()
{
	return xoPlatform_WinDesktop | xoPlatform_LinuxDesktop;
}

xoVertexType xoGLProg_Curve::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

