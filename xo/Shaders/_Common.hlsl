
cbuffer PerFrame : register(b0)
{
	float4x4		mvproj;
	float2			vport_hsize;

	Texture2D		shader_texture;
	SamplerState	sample_type;	
};

cbuffer PerObject : register(b1)
{
	float4		box;
	float4		border;
	float4		border_color;
	float		radius;
};

struct VertexType_PTC
{
	float4 pos		: POSITION;
	float2 uv		: TEXCOORD0;
	float4 color	: COLOR;
};

struct VertexType_PTCV4
{
	float4 pos		: POSITION;
	float2 uv		: TEXCOORD1;
	float4 color	: COLOR;
	float4 v4		: TEXCOORD2;
};

float fromSRGB_Component(float srgb)
{
	float sRGB_Low	= 0.0031308;
	float sRGB_a	= 0.055;

	if (srgb <= 0.04045)
		return srgb / 12.92;
	else
		return pow(abs((srgb + sRGB_a) / (1.0 + sRGB_a)), 2.4);
}

float4 fromSRGB(float4 c)
{
	float4 linear_c;
	linear_c.r = fromSRGB_Component(c.r);
	linear_c.g = fromSRGB_Component(c.g);
	linear_c.b = fromSRGB_Component(c.b);
	linear_c.a = c.a;
	return linear_c;
}

float4 premultiply(float4 c)
{
	return float4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
}

// SV_Position is in screen space, but in GLSL it is in normalized device space
float2 frag_to_screen(float2 unit_pt)
{
	return unit_pt;
}

#define SHADER_TYPE_MASK     15
#define SHADER_FLAG_TEXBG    16

#define SHADER_ARC           1
#define SHADER_RECT          2
#define SHADER_TEXT_SIMPLE   3
#define SHADER_TEXT_SUBPIXEL 4
