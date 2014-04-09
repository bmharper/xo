
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

VSOutput main(VertexType_PTC vertex)
{
	VSOutput output;
	output.pos = mul(mvproj, vertex.pos);
	output.color = vertex_color_in(vertex.color);
	return output;
}
