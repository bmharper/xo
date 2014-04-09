
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;
	float2 texuv0	: TEXCOORD0;
};

VSOutput main(VertexType_PTC vertex)
{
	VSOutput output;
	output.pos = mul(mvproj, vertex.pos);
	output.color = vertex_color_in(vertex.color);
	output.texuv0 = vertex.uv;
	return output;
}
