struct VertexType_Uber
{
	float2	pos     : POSITION;
	float4	uv0     : TEXCOORD0;
	float4	uv1     : TEXCOORD1;
	float4	color0  : COLOR0;
	float4	color1  : COLOR1;
	uint	shader  : BLENDINDICES;
};

struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color0	: COLOR0;
	float4 color1	: COLOR1;
	float4 uv0	    : TEXCOORD0;
	float4 uv1	    : TEXCOORD1;
	uint   shader   : BLENDINDICES;
};

// CPU -> Vertex Shader: 8 + 2 * 16 + 3 * 4 = 52 bytes per vertex
// Vertex -> Fragment:   8 + 4 * 16 + 4     = 72 bytes per fragment

VSOutput main(VertexType_Uber vertex)
{
	VSOutput output;
	output.pos = mul(mvproj, float4(vertex.pos.x, vertex.pos.y, 0, 1));
	output.uv0 = vertex.uv0;
	output.uv1 = vertex.uv1;
	// You might be tempted to premultiply right here, but unfortunately the subpixel text renderer needs the RGB values unassociated.
	output.color0 = fromSRGB(vertex.color0);
	output.color1 = fromSRGB(vertex.color1);
	output.shader = vertex.shader;
	return output;
}