#include "pch.h"
#if XO_BUILD_DIRECTX
#include "RectShader.h"

xoDXProg_Rect::xoDXProg_Rect()
{
	Reset();
}

void xoDXProg_Rect::Reset()
{
	ResetBase();

}

const char* xoDXProg_Rect::VertSrc()
{
	return
	"\n"
	"cbuffer PerFrame : register(b0)\n"
	"{\n"
	"	float4x4		mvproj;\n"
	"	float2			vport_hsize;\n"
	"\n"
	"	Texture2D		shader_texture;\n"
	"	SamplerState	sample_type;\n"
	"};\n"
	"\n"
	"cbuffer PerObject : register(b1)\n"
	"{\n"
	"	float4		box;\n"
	"	float4		border;\n"
	"	float4		border_color;\n"
	"	float		radius;\n"
	"};\n"
	"\n"
	"struct VertexType_PTC\n"
	"{\n"
	"	float4 pos		: POSITION;\n"
	"	float2 uv		: TEXCOORD0;\n"
	"	float4 color	: COLOR;\n"
	"};\n"
	"\n"
	"struct VertexType_PTCV4\n"
	"{\n"
	"	float4 pos		: POSITION;\n"
	"	float2 uv		: TEXCOORD1;\n"
	"	float4 color	: COLOR;\n"
	"	float4 v4		: TEXCOORD2;\n"
	"};\n"
	"\n"
	"float fromSRGB_Component(float srgb)\n"
	"{\n"
	"	float sRGB_Low	= 0.0031308;\n"
	"	float sRGB_a	= 0.055;\n"
	"\n"
	"	if (srgb <= 0.04045)\n"
	"		return srgb / 12.92;\n"
	"	else\n"
	"		return pow(abs((srgb + sRGB_a) / (1.0 + sRGB_a)), 2.4);\n"
	"}\n"
	"\n"
	"float4 fromSRGB(float4 c)\n"
	"{\n"
	"	float4 linear_c;\n"
	"	linear_c.r = fromSRGB_Component(c.r);\n"
	"	linear_c.g = fromSRGB_Component(c.g);\n"
	"	linear_c.b = fromSRGB_Component(c.b);\n"
	"	linear_c.a = c.a;\n"
	"	return linear_c;\n"
	"}\n"
	"\n"
	"float4 premultiply(float4 c)\n"
	"{\n"
	"	return float4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);\n"
	"}\n"
	"\n"
	"// SV_Position is in screen space, but in GLSL it is in normalized device space\n"
	"float2 frag_to_screen(float2 unit_pt)\n"
	"{\n"
	"	return unit_pt;\n"
	"}\n"
	"\n"
	"struct VSOutput\n"
	"{\n"
	"	float4 pos		: SV_Position;\n"
	"	float4 color	: COLOR;\n"
	"};\n"
	"\n"
	"VSOutput main(VertexType_PTC vertex)\n"
	"{\n"
	"	VSOutput output;\n"
	"	output.pos = mul(mvproj, vertex.pos);\n"
	"	output.color = fromSRGB(vertex.color);\n"
	"	return output;\n"
	"}\n"
;
}

const char* xoDXProg_Rect::FragSrc()
{
	return
	"\n"
	"cbuffer PerFrame : register(b0)\n"
	"{\n"
	"	float4x4		mvproj;\n"
	"	float2			vport_hsize;\n"
	"\n"
	"	Texture2D		shader_texture;\n"
	"	SamplerState	sample_type;\n"
	"};\n"
	"\n"
	"cbuffer PerObject : register(b1)\n"
	"{\n"
	"	float4		box;\n"
	"	float4		border;\n"
	"	float4		border_color;\n"
	"	float		radius;\n"
	"};\n"
	"\n"
	"struct VertexType_PTC\n"
	"{\n"
	"	float4 pos		: POSITION;\n"
	"	float2 uv		: TEXCOORD0;\n"
	"	float4 color	: COLOR;\n"
	"};\n"
	"\n"
	"struct VertexType_PTCV4\n"
	"{\n"
	"	float4 pos		: POSITION;\n"
	"	float2 uv		: TEXCOORD1;\n"
	"	float4 color	: COLOR;\n"
	"	float4 v4		: TEXCOORD2;\n"
	"};\n"
	"\n"
	"float fromSRGB_Component(float srgb)\n"
	"{\n"
	"	float sRGB_Low	= 0.0031308;\n"
	"	float sRGB_a	= 0.055;\n"
	"\n"
	"	if (srgb <= 0.04045)\n"
	"		return srgb / 12.92;\n"
	"	else\n"
	"		return pow(abs((srgb + sRGB_a) / (1.0 + sRGB_a)), 2.4);\n"
	"}\n"
	"\n"
	"float4 fromSRGB(float4 c)\n"
	"{\n"
	"	float4 linear_c;\n"
	"	linear_c.r = fromSRGB_Component(c.r);\n"
	"	linear_c.g = fromSRGB_Component(c.g);\n"
	"	linear_c.b = fromSRGB_Component(c.b);\n"
	"	linear_c.a = c.a;\n"
	"	return linear_c;\n"
	"}\n"
	"\n"
	"float4 premultiply(float4 c)\n"
	"{\n"
	"	return float4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);\n"
	"}\n"
	"\n"
	"// SV_Position is in screen space, but in GLSL it is in normalized device space\n"
	"float2 frag_to_screen(float2 unit_pt)\n"
	"{\n"
	"	return unit_pt;\n"
	"}\n"
	"\n"
	"struct VSOutput\n"
	"{\n"
	"	float4 pos		: SV_Position;\n"
	"	float4 color	: COLOR;\n"
	"};\n"
	"\n"
	"#define LEFT x\n"
	"#define TOP y\n"
	"#define RIGHT z\n"
	"#define BOTTOM w\n"
	"\n"
	"// NOTE: This is a stupid way of doing rectangles.\n"
	"// Instead of rendering one big rectangle, we should be rendering it as 4 quadrants.\n"
	"// This \"one big rectangle\" approach forces you to evaluate all 4 corners for every pixel.\n"
	"// NOTE ALSO: This is broken, in the sense that it assumes a uniform border width.\n"
	"float4 main(VSOutput input) : SV_Target\n"
	"{\n"
	"	float2 screenxy = frag_to_screen(input.pos.xy);\n"
	"	float radius_out = radius;\n"
	"	float4 mybox = box;\n"
	"	float4 mycolor = input.color;\n"
	"	float radius_in = max(border.x, radius); // This is why different border widths screw up.. because we're bound to border.left\n"
	"	float4 out_box = mybox + float4(radius_out, radius_out, -radius_out, -radius_out);\n"
	"	float4 in_box = mybox + float4(max(border.LEFT, radius), max(border.TOP, radius), -max(border.RIGHT, radius), -max(border.BOTTOM, radius));\n"
	"\n"
	"	float2 cent_in = screenxy;\n"
	"	float2 cent_out = screenxy;\n"
	"	cent_in.x = clamp(cent_in.x, in_box.LEFT, in_box.RIGHT);\n"
	"	cent_in.y = clamp(cent_in.y, in_box.TOP, in_box.BOTTOM);\n"
	"	cent_out.x = clamp(cent_out.x, out_box.LEFT, out_box.RIGHT);\n"
	"	cent_out.y = clamp(cent_out.y, out_box.TOP, out_box.BOTTOM);\n"
	"\n"
	"	// If you draw the pixels out on paper, and take cognisance of the fact that\n"
	"	// our samples are at pixel centers, then this -0.5 offset makes perfect sense.\n"
	"	// This offset is correct regardless of whether you're blending linearly or in gamma space.\n"
	"	// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset\n"
	"	// that is fed into the shader's \"radius\" uniform, we effectively get rectangles to be sharp\n"
	"	// when they are aligned to an integer grid. I haven't thought this through carefully enough,\n"
	"	// but it does feel right.\n"
	"	float dist_out = length(screenxy - cent_out) - 0.5f;\n"
	"\n"
	"	float dist_in = length(screenxy - cent_in) - (radius_in - border.x);\n"
	"\n"
	"	// I don't like this \"if (borderWidth > 0.5f)\". I feel like I'm doing something\n"
	"	// wrong here, but I haven't drawn it out carefully enough to know what that something is.\n"
	"	// When you rewrite this function to render quadrants, then perhaps fix this up too.\n"
	"	float borderWidthX = max(border.x, border.z);\n"
	"	float borderWidthY = max(border.y, border.w);\n"
	"	float borderWidth = max(borderWidthX, borderWidthY);\n"
	"	float borderMix = clamp(dist_in, 0.0f, 1.0f);\n"
	"	if (borderWidth > 0.5f)\n"
	"		mycolor = lerp(mycolor, border_color, borderMix);\n"
	"\n"
	"	float4 output;\n"
	"	output.rgb = mycolor.rgb;\n"
	"	output.a = mycolor.a * clamp(radius_out - dist_out, 0.0f, 1.0f);\n"
	"	return premultiply(output);\n"
	"}\n"
;
}

const char* xoDXProg_Rect::Name()
{
	return "Rect";
}


bool xoDXProg_Rect::LoadVariablePositions()
{
	int nfail = 0;

	if ( nfail != 0 )
		XOTRACE( "Failed to bind %d variables of shader Rect\n", nfail );

	return nfail == 0;
}

uint32 xoDXProg_Rect::PlatformMask()
{
	return xoPlatform_All;
}

xoVertexType xoDXProg_Rect::VertexType()
{
	return xoVertexType_PTC;
}

#endif // XO_BUILD_DIRECTX

