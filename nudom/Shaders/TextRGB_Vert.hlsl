
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;
	float2 uv		: TEXCOORD0;
	float4 uvClamp	: TEXCOORD1;
};

VSOutput main(VertexType_PTCV4 vertex)
{
	VSOutput output;
	output.pos = mul(mvproj, vertex.pos);
	output.color = vertex_color_in(vertex.color);
	output.uv = vertex.uv;
	output.uvClamp = vertex.v4;
	return output;
}
