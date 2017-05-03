
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color0	: COLOR0;
	float4 color1	: COLOR1;
	float4 uv0	    : TEXCOORD0;
	float4 uv1	    : TEXCOORD1;
	uint   shader   : BLENDINDICES;
};

struct PSOutput
{
	float4 color0	: SV_Target0;
	float4 color1	: SV_Target1;
};

PSOutput write_color(float4 color)
{
	PSOutput pout;
	pout.color0 = color;
	pout.color1 = color.aaaa;
	return pout;
}

float4 blend_over(float4 a, float4 b)
{
	return (1.0 - b.a) * a + b;
}

PSOutput main(VSOutput input)
{
	//int shader = int(input.shader);
	uint shader = input.shader;
	bool enableBGTex = (shader & SHADER_FLAG_TEXBG) != 0;
	shader = shader & SHADER_TYPE_MASK;
	PSOutput output;

	if (shader == SHADER_ARC)
	{
		float2 center1 = input.uv0.xy;
		float2 center2 = input.uv0.zw;
		float radius1 = input.uv1.x;
		float radius2 = input.uv1.y;
		float2 uv = input.uv1.zw;
		float4 bg_color = premultiply(input.color0);
		float4 border_color = premultiply(input.color1);

		float4 bg_tex = float4(0, 0, 0, 0);
		if (enableBGTex)
		{
			bg_tex = premultiply(shader_texture.Sample(sample_type, uv));
			bg_color = blend_over(bg_color, bg_tex);
		}

		float2 screen_pos = frag_to_screen(input.pos.xy);
		float distance1 = length(screen_pos - center1);
		float distance2 = length(screen_pos - center2);
		float color_blend = clamp(distance1 - radius1 + 0.5, 0.0, 1.0);
		float alpha_blend = clamp(radius2 - distance2 + 0.5, 0.0, 1.0);
		float4 color = lerp(bg_color, border_color, color_blend);
		color *= alpha_blend;
		output = write_color(color);
	}
	else if (shader == SHADER_RECT)
	{
		float border_width = input.uv0.x;
		float border_distance = input.uv0.y;
		float2 uv = input.uv0.zw;
		float4 bg_color = premultiply(input.color0);
		float4 border_color = premultiply(input.color1);

		// Distance from edge.
		// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.
		// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.
		// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.
		float edge_alpha = clamp(border_distance + 0.5, 0.0, 1.0);

		// The +0.5 here is the same as above
		float dclamped = clamp(border_width - border_distance + 0.5, 0.0, 1.0);

		float4 bg_tex = float4(0, 0, 0, 0);
		if (enableBGTex)
		{
			bg_tex = premultiply(shader_texture.Sample(sample_type, uv));
			bg_color = blend_over(bg_color, bg_tex);
		}

		float4 color = lerp(bg_color, border_color, dclamped);
		color *= edge_alpha;
		output = write_color(color);
	}
	else if (shader == SHADER_TEXT_SIMPLE)
	{
		float4 texCol = shader_texture.Sample(sample_type, input.uv0.xy);
		output = write_color(texCol.rrrr * premultiply(input.color0));
	}
	else if (shader == SHADER_TEXT_SUBPIXEL)
	{
		float offset = 1.0 / XO_GLYPH_ATLAS_SIZE;
		float2 uv = input.uv0.xy;
		float4 tex_clamp = input.uv1;

		float tap0 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 3.0, tex_clamp.x, tex_clamp.z), uv.y)).r;
		float tap1 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 2.0, tex_clamp.x, tex_clamp.z), uv.y)).r;
		float tap2 = shader_texture.Sample(sample_type, float2(clamp(uv.x - offset * 1.0, tex_clamp.x, tex_clamp.z), uv.y)).r;
		float tap3 = shader_texture.Sample(sample_type, float2(clamp(uv.x             ,   tex_clamp.x, tex_clamp.z), uv.y)).r;
		float tap4 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 1.0, tex_clamp.x, tex_clamp.z), uv.y)).r;
		float tap5 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 2.0, tex_clamp.x, tex_clamp.z), uv.y)).r;
		float tap6 = shader_texture.Sample(sample_type, float2(clamp(uv.x + offset * 3.0, tex_clamp.x, tex_clamp.z), uv.y)).r;

		float w0 = 0.56;
		float w1 = 0.28;
		float w2 = 0.16;
		//float w0 = 0.60;
		//float w1 = 0.28;
		//float w2 = 0.12;

		// Note that input.color0 needs to be non-premultiplied here.

		float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);
		float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);
		float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);
		float aR = r * input.color0.a;
		float aG = g * input.color0.a;
		float aB = b * input.color0.a;
		float avgA = (r + g + b) / 3.0;

		// See the long explanation inside write_color for more details. In order to be consistent with regular
		// (non-sub-pixel) rendering, we need to premultiply our RGB here with their respective alpha values.
		output.color0 = float4(input.color0.r * aR, input.color0.g * aG, input.color0.b * aB, avgA);
		output.color1 = float4(aR, aG, aB, avgA);
	}
	else
	{
		output = write_color(float4(1, 1, 0, 1));
	}
	return output;
}
