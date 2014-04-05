
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
	float		radius;
};

struct VSOutput
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR;
	float2 texuv0	: TEXCOORD0;
	float2 texClamp;
};

float4 main(VSOutput input) : SV_Target
{
	float4 col;
	col = input.color * float4(1,1,1, shader_texture.Sample(sample_type, input.texuv0).r);
    return col;
}
