
cbuffer PerFrame : register(b0)
{
	float4x4	mvproj;
	float2		vport_hsize;
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
};

VSOutput main(float4 vpos : POSITION, float4 vcol : COLOR, float2 vtexuv0 : TEXCOORD0)
{
	VSOutput output;
	output.pos = mul(mvproj, vpos);
	output.color = vcol;
	output.texuv0 = vtexuv0;
	return output;
}
