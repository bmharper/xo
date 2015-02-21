#include "pch.h"
#if XO_BUILD_OPENGL
#include "Rect2Shader.h"

xoGLProg_Rect2::xoGLProg_Rect2()
{
	Reset();
}

void xoGLProg_Rect2::Reset()
{
	ResetBase();
	v_mvproj = -1;
	v_vpos = -1;
	v_vcolor = -1;
	v_vradius = -1;
	v_vborder_width = -1;
	v_vborder_color = -1;
	v_vport_hsize = -1;
	v_out_vector = -1;
	v_shadow_offset = -1;
	v_shadow_color = -1;
	v_shadow_size_inv = -1;
	v_edges = -1;
}

const char* xoGLProg_Rect2::VertSrc()
{
	return
		"uniform		mat4	mvproj;\n"
		"attribute	vec4	vpos;\n"
		"attribute	vec4	vcolor;\n"
		"attribute	float	vradius;\n"
		"attribute 	float	vborder_width;\n"
		"attribute 	vec4	vborder_color;\n"
		"\n"
		"varying 	vec4	pos;\n"
		"varying 	vec4	color;\n"
		"varying 	float	radius;\n"
		"varying 	float	border_width;\n"
		"varying 	vec4	border_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	pos = mvproj * vpos;\n"
		"	gl_Position = pos;\n"
		"	radius = vradius;\n"
		"	border_width = vborder_width;\n"
		"	border_color = fromSRGB(vborder_color);\n"
		"	color = fromSRGB(vcolor);\n"
		"}\n"
;
}

const char* xoGLProg_Rect2::FragSrc()
{
	return
		"//#define SHADERTOY\n"
		"\n"
		"// !!! I have given up on this approach, since I can't figure out how to use it\n"
		"// to draw rounded borders when the border widths are different. For example,\n"
		"// left border is 5 pixels, and top border is 10 pixels, with a radius of 5.\n"
		"// Also, I don't know how to construct an implicit function for an ellipse\n"
		"// that produces a correctly antialiased result. The problem is that the\n"
		"// distance function for an ellipse ends up biasing you so that the shorter\n"
		"// axis is much \"tighter\".\n"
		"// Elliptical arc rendering is necessary to cover non-uniform border radii,\n"
		"// as specified in http://www.w3.org/TR/css3-background/#corners.\n"
		"// I'm going to try cubic spline rendering instead.\n"
		"\n"
		"#ifdef SHADERTOY\n"
		"    #define TOP_LEFT 0.0\n"
		"    #define TOP_RIGHT 1.0\n"
		"    #define BOTTOM_LEFT 2.0\n"
		"    #define BOTTOM_RIGHT 3.0\n"
		"\n"
		"    vec4 premultiply(vec4 c)\n"
		"    {\n"
		"        return vec4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);\n"
		"    }\n"
		"#else\n"
		"    uniform vec2    vport_hsize;\n"
		"    uniform vec2    out_vector;\n"
		"    uniform vec2    shadow_offset;\n"
		"    uniform vec4    shadow_color;\n"
		"    uniform float   shadow_size_inv;\n"
		"    uniform vec2    edges;\n"
		"    varying vec4    pos;\n"
		"    varying vec4    color;\n"
		"    varying float   radius;\n"
		"    varying float   border_width;\n"
		"    varying vec4    border_color;\n"
		"\n"
		"    vec2 to_screen(vec2 unit_pt)\n"
		"    {\n"
		"        return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;\n"
		"    }\n"
		"#endif\n"
		"\n"
		"vec2 clamp_to_edges(vec2 pos, vec2 edges, vec2 out_vector)\n"
		"{\n"
		"    // Clamp our distance from the outside edge to [-inf, 0],\n"
		"    // then add that distance back to the edge.\n"
		"    vec2 distance_outside = (pos - edges) * out_vector;\n"
		"    return edges + out_vector * min(distance_outside, vec2(0.0, 0.0));\n"
		"}\n"
		"\n"
		"// Our boxes lie on integer coordinates. For example, a box that has perfectly sharp edges\n"
		"// will have coordinates (5,5,10,10). However, fragment coordinates here are the centers\n"
		"// of pixels, so the origin fragment lies at (0.5,0.5).\n"
		"// In order to calculate blending factors, we first clamp the fragment location to our\n"
		"// rectangle's box. Then, we measure the distance from the clamped location to the original\n"
		"// location. A distance of 0 means the fragment is completely inside the box.\n"
		"// Because our boxes lie on integer coordinates, but our fragment centers are at 0.5 offsets,\n"
		"// we need to shrink our box by 0.5 on all sides, so that a fragment on the edge is at\n"
		"// a distance of exactly 0, and any fragment outside that has an increasing distance.\n"
		"\n"
		"// I suspect it is going to be better to have a single shader that gets issued on 4 primitives\n"
		"// (ie 4 corners of rectangle) instead of a single invocation. But HOW do we communicate\n"
		"// this...\n"
		"void main()\n"
		"{\n"
		"#ifdef SHADERTOY\n"
		"    vec4 color = vec4(0.0, 0.5, 0.0, 1.0);\n"
		"    vec4 border_color = vec4(0.5, 0.0, 0.0, 1.0);\n"
		"    float border_width = 35.0;\n"
		"    vec2 edges = vec2(124.0, 124.0);\n"
		"\n"
		"    float type = TOP_LEFT;\n"
		"\n"
		"    // screen pos\n"
		"    vec2 spos = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y);\n"
		"\n"
		"    float radius = 35.0;\n"
		"    float shadow_size = 0.0;\n"
		"    float shadow_size_inv = 0.0;\n"
		"    if (shadow_size != 0.0)\n"
		"        shadow_size_inv = 1.0 / shadow_size;\n"
		"    vec4 shadow_color = vec4(0.0, 0.0, 0.0, 0.5);\n"
		"    vec2 shadow_offset = vec2(1.0, 1.0);\n"
		"\n"
		"    // The 'out vector' points outward from our box's circle.\n"
		"    vec2 out_vector;\n"
		"    if (type == TOP_LEFT)           { out_vector = vec2(-1.0, -1.0); }\n"
		"    else if (type == TOP_RIGHT)     { out_vector = vec2( 1.0, -1.0); }\n"
		"    else if (type == BOTTOM_LEFT)   { out_vector = vec2(-1.0,  1.0); }\n"
		"    else if (type == BOTTOM_RIGHT)  { out_vector = vec2( 1.0,  1.0); }\n"
		"#else\n"
		"    vec2 spos = to_screen(pos.xy);\n"
		"#endif\n"
		"\n"
		"    // Fragment samples are at pixel centers, so we have no choice but\n"
		"    // to shrink our edges back by half a pixel. If we don't do this,\n"
		"    // then the pixels immediately outside our border end up being a\n"
		"    // distance of 0.5 from our edge, which gives it some color. We need\n"
		"    // any pixels outside of our box to be completely unlit.\n"
		"\n"
		"    float border_shrink = max(border_width, radius) + 0.5;\n"
		"\n"
		"    vec2 edges_circle = edges - (0.5 + radius) * out_vector;\n"
		"    vec2 edges_border = edges - border_shrink * out_vector;\n"
		"    vec2 edges_shadow_circle = edges_circle + shadow_offset;\n"
		"\n"
		"    vec2 pos_circle = clamp_to_edges(spos, edges_circle, out_vector);\n"
		"    vec2 pos_border = clamp_to_edges(spos, edges_border, out_vector);\n"
		"    vec2 pos_shadow_circle = clamp_to_edges(spos, edges_shadow_circle, out_vector);\n"
		"\n"
		"    float dist_circle = length(spos - pos_circle);\n"
		"    float dist_border = length(spos - pos_border);\n"
		"\n"
		"    vec4 shadow_out = vec4(0,0,0,0);\n"
		"    if (shadow_size_inv != 0.0)\n"
		"    {\n"
		"        float dist_shadow_circle = length(spos - pos_shadow_circle);\n"
		"        float shdist = (dist_shadow_circle - radius - 1.0) * shadow_size_inv;\n"
		"        shdist = clamp(shdist, 0.0, 1.0);\n"
		"        // cheap gaussian approximation (1 - x^2)^2 - I'm definitely not satisfied with this!\n"
		"        float shadow_strength = 1.0 - shdist * shdist;\n"
		"        shadow_strength *= shadow_strength;\n"
		"        shadow_out = mix(shadow_out, premultiply(shadow_color), shadow_strength);\n"
		"    }\n"
		"\n"
		"    vec4 outcolor = premultiply(color);\n"
		"\n"
		"    // blend border onto outcolor\n"
		"    float border_blend = dist_border - (border_shrink - border_width) + 0.5;\n"
		"    border_blend = clamp(border_blend, 0.0, 1.0);\n"
		"    vec4 border_out = premultiply(border_color);\n"
		"    outcolor = mix(outcolor, border_out, border_blend);\n"
		"\n"
		"    float bgblend = 1.0 + radius - dist_circle;\n"
		"    outcolor *= clamp(bgblend, 0.0, 1.0);\n"
		"\n"
		"    // blend outcolor onto shadow_out\n"
		"    outcolor = (1.0 - outcolor.a) * shadow_out + outcolor;\n"
		"\n"
		"#ifdef SHADERTOY\n"
		"    // basecolor is for shadertoy\n"
		"    vec4 basecolor = vec4(1,1,1,1);\n"
		"    outcolor = (1.0 - outcolor.a) * basecolor + outcolor;\n"
		"\n"
		"    float igamma = 1.0/2.2;\n"
		"    gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));\n"
		"    gl_FragColor.a = outcolor.a;\n"
		"#else\n"
		"    #ifdef XO_SRGB_FRAMEBUFFER\n"
		"        gl_FragColor = outcolor;\n"
		"    #else\n"
		"        float igamma = 1.0/2.2;\n"
		"        gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));\n"
		"        gl_FragColor.a = outcolor.a;\n"
		"    #endif\n"
		"#endif\n"
		"}\n"
;
}

const char* xoGLProg_Rect2::Name()
{
	return "Rect2";
}


bool xoGLProg_Rect2::LoadVariablePositions()
{
	int nfail = 0;

	nfail += (v_mvproj = glGetUniformLocation( Prog, "mvproj" )) == -1;
	nfail += (v_vpos = glGetAttribLocation( Prog, "vpos" )) == -1;
	nfail += (v_vcolor = glGetAttribLocation( Prog, "vcolor" )) == -1;
	nfail += (v_vradius = glGetAttribLocation( Prog, "vradius" )) == -1;
	nfail += (v_vborder_width = glGetAttribLocation( Prog, "vborder_width" )) == -1;
	nfail += (v_vborder_color = glGetAttribLocation( Prog, "vborder_color" )) == -1;
	nfail += (v_vport_hsize = glGetUniformLocation( Prog, "vport_hsize" )) == -1;
	nfail += (v_out_vector = glGetUniformLocation( Prog, "out_vector" )) == -1;
	nfail += (v_shadow_offset = glGetUniformLocation( Prog, "shadow_offset" )) == -1;
	nfail += (v_shadow_color = glGetUniformLocation( Prog, "shadow_color" )) == -1;
	nfail += (v_shadow_size_inv = glGetUniformLocation( Prog, "shadow_size_inv" )) == -1;
	nfail += (v_edges = glGetUniformLocation( Prog, "edges" )) == -1;
	if (nfail != 0)
		XOTRACE("Failed to bind %d variables of shader Rect2\n", nfail);

	return nfail == 0;
}

uint32 xoGLProg_Rect2::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoGLProg_Rect2::VertexType()
{
	return xoVertexType_NULL;
}

#endif // XO_BUILD_OPENGL

