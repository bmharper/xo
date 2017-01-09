#include "pch.h"
#if XO_BUILD_DIRECTX
#include "TextRGBShader.h"

namespace xo {

DXProg_TextRGB::DXProg_TextRGB()
{
	Reset();
}

void DXProg_TextRGB::Reset()
{
	ResetBase();

}

const char* DXProg_TextRGB::VertSrc()
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
		"	float2 uv		: TEXCOORD0;\n"
		"	float4 uvClamp	: TEXCOORD1;\n"
		"};\n"
		"\n"
		"VSOutput main(VertexType_PTCV4 vertex)\n"
		"{\n"
		"	VSOutput output;\n"
		"	output.pos = mul(mvproj, vertex.pos);\n"
		"	output.color = fromSRGB(vertex.color);\n"
		"	output.uv = vertex.uv;\n"
		"	output.uvClamp = vertex.v4;\n"
		"	return output;\n"
		"}\n"
;
}

const char* DXProg_TextRGB::FragSrc()
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
		"	float2 uv		: TEXCOORD0;\n"
		"	float4 uvClamp	: TEXCOORD1;\n"
		"};\n"
		"\n"
		"struct PSOutput\n"
		"{\n"
		"	float4 color0	: SV_Target0;\n"
		"	float4 color1	: SV_Target1;\n"
		"};\n"
		"\n"
		"PSOutput main(VSOutput input)\n"
		"{\n"
		"	//float4 col;\n"
		"	//col = input.color * float4(1,1,1, shader_texture.Sample(sample_type, input.texuv0).r);\n"
		"    //return col;\n"
		"\n"
		"	float offset = 1.0 / XO_GLYPH_ATLAS_SIZE;\n"
		"	float2 uv = input.uv;\n"
		"	float4 clamps = input.uvClamp;\n"
		"\n"
		"	float tap0 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 3.0, clamps.x, clamps.z), uv.y)).r;\n"
		"	float tap1 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 2.0, clamps.x, clamps.z), uv.y)).r;\n"
		"	float tap2 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 1.0, clamps.x, clamps.z), uv.y)).r;\n"
		"	float tap3 = shader_texture.Sample(sample_type, float2(clamp(uv.x             ,   clamps.x, clamps.z), uv.y)).r;\n"
		"	float tap4 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 1.0, clamps.x, clamps.z), uv.y)).r;\n"
		"	float tap5 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 2.0, clamps.x, clamps.z), uv.y)).r;\n"
		"	float tap6 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 3.0, clamps.x, clamps.z), uv.y)).r;\n"
		"\n"
		"	float w0 = 0.56;\n"
		"	float w1 = 0.28;\n"
		"	float w2 = 0.16;\n"
		"	//float w0 = 0.60;\n"
		"	//float w1 = 0.28;\n"
		"	//float w2 = 0.12;\n"
		"	//float w0 = 0.98;\n"
		"	//float w1 = 0.01;\n"
		"	//float w2 = 0.01;\n"
		"\n"
		"	float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);\n"
		"	float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);\n"
		"	float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);\n"
		"	float aR = r * input.color.a;\n"
		"	float aG = g * input.color.a;\n"
		"	float aB = b * input.color.a;\n"
		"	float avgA = (r + g + b) / 3.0;\n"
		"	//float minA = min(r,g,min(g,b));\n"
		"	// ONE MINUS SRC COLOR\n"
		"	//float alpha = min(min(red, green), blue);\n"
		"	//gl_FragColor = vec4(aR, aG, aB, avgA);\n"
		"\n"
		"	PSOutput output;\n"
		"	output.color0 = float4(input.color.rgb, avgA);\n"
		"	output.color1 = float4(aR, aG, aB, avgA);\n"
		"	return output;\n"
		"}\n"
;
}

const char* DXProg_TextRGB::Name()
{
	return "TextRGB";
}


bool DXProg_TextRGB::LoadVariablePositions()
{
	int nfail = 0;

	if (nfail != 0)
		Trace("Failed to bind %d variables of shader TextRGB\n", nfail);

	return nfail == 0;
}

uint32_t DXProg_TextRGB::PlatformMask()
{
	return Platform_All;
}

xo::VertexType DXProg_TextRGB::VertexType()
{
	return VertexType_PTCV4;
}

} // namespace xo

#endif // XO_BUILD_DIRECTX

