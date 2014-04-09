
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;
	float2 texuv0	: TEXCOORD0;
	float4 texClamp	: TEXCOORD1;
};

VSOutput main(VertexType_PTCV4 vertex)
{
	VSOutput output;
	output.pos = mul(mvproj, vertex.pos);
	output.color = vertex_color_in(vertex.color);
	output.texuv0 = vertex.uv;
	output.texClamp = vertex.v4;
	return output;
}
