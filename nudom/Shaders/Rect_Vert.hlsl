
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
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

VSOutput main(float4 vpos : POSITION, float4 vcol : COLOR)
{
	VSOutput output;
	output.pos = mul(mvproj, vpos);
	output.color.rgb = pow(abs(vcol.rgb), float3(2.2, 2.2, 2.2));
	output.color.a = vcol.a;
	return output;
}
