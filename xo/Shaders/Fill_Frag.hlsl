
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

float4 main(VSOutput input) : SV_Target
{
    return input.color;
}
