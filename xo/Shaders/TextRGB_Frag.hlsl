
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;
	float2 uv		: TEXCOORD0;
	float4 uvClamp	: TEXCOORD1;
};

struct PSOutput
{
	float4 color0	: SV_Target0;
	float4 color1	: SV_Target1;
};

PSOutput main(VSOutput input)
{
	//float4 col;
	//col = input.color * float4(1,1,1, shader_texture.Sample(sample_type, input.texuv0).r);
    //return col;

	float offset = 1.0 / XO_GLYPH_ATLAS_SIZE;
	float2 uv = input.uv;
	float4 clamps = input.uvClamp;

	float tap0 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 3.0, clamps.x, clamps.z), uv.y)).r;
	float tap1 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 2.0, clamps.x, clamps.z), uv.y)).r;
	float tap2 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 1.0, clamps.x, clamps.z), uv.y)).r;
	float tap3 = shader_texture.Sample(sample_type, float2(clamp(uv.x             ,   clamps.x, clamps.z), uv.y)).r;
	float tap4 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 1.0, clamps.x, clamps.z), uv.y)).r;
	float tap5 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 2.0, clamps.x, clamps.z), uv.y)).r;
	float tap6 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 3.0, clamps.x, clamps.z), uv.y)).r;

	float w0 = 0.56;
	float w1 = 0.28;
	float w2 = 0.16;
	//float w0 = 0.60;
	//float w1 = 0.28;
	//float w2 = 0.12;
	//float w0 = 0.98;
	//float w1 = 0.01;
	//float w2 = 0.01;

	float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);
	float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);
	float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);
	float aR = r * input.color.a;
	float aG = g * input.color.a;
	float aB = b * input.color.a;
	float avgA = (r + g + b) / 3.0;
	//float minA = min(r,g,min(g,b));
	// ONE MINUS SRC COLOR
	//float alpha = min(min(red, green), blue);
	//gl_FragColor = vec4(aR, aG, aB, avgA);

	PSOutput output;
	output.color0 = float4(input.color.rgb, avgA);
	output.color1 = float4(aR, aG, aB, avgA);
	return output;
}
