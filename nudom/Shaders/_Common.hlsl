
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

float4 vertex_color_in(float4 vertex_color)
{
	float4 color;
	color.rgb = pow(abs(vertex_color.rgb), float3(2.2, 2.2, 2.2));
	color.a = vertex_color.a;
	return color;
}

// SV_Position is in screen space, but in GLSL it is in normalized device space
float2 frag_to_screen(float2 unit_pt)
{
	return unit_pt;
}
