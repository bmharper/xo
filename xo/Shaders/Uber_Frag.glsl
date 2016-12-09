#extension GL_EXT_gpu_shader4 : enable

uniform		vec2	Frame_VPort_HSize;

uniform sampler2D	f_tex0;

varying		vec2	f_pos;
varying		vec4	f_uv1;
varying		vec4	f_uv2;
varying		vec4	f_color1;
varying		vec4	f_color2;
varying 	float	f_shader;

#if defined(XO_PLATFORM_WIN_DESKTOP) || defined(XO_PLATFORM_LINUX_DESKTOP)
//layout(location = 0, index = 0) out vec4 outputColor0;
//layout(location = 0, index = 1) out vec4 outputColor1;
// On NVidia (9.18.13.3165 (10-23-2013), R331.65 (branch: r331_00-146)),
// one doesn't need the layout qualification, nor glBindFragDataLocationIndexed. The order in which you
// declare the output variables is sufficient to make an affinity with "color0" or "color1".
// It is needed though, on Intel Haswell drivers on Linux
varying out		vec4		out_color0;
varying out		vec4		out_color1;

void write_color(vec4 color)
{
	out_color0 = color;
	out_color1 = color;
}
#else
void write_color(vec4 color)
{
	gl_FragColor = color;
}
#endif

vec2 to_screen(vec2 pos)
{
	return (vec2(pos.x, -pos.y) + vec2(1,1)) * Frame_VPort_HSize;
}

void main()
{
	int shader = int(f_shader);
	bool enableBGTex = (shader & SHADER_FLAG_TEXBG) != 0;
	shader = shader & SHADER_TYPE_MASK;

	if (shader == SHADER_ARC)
	{
		vec2 center1 = f_uv1.xy;
		vec2 center2 = f_uv1.zw;
		float radius1 = f_uv2.x;
		float radius2 = f_uv2.y;
		vec4 bg_color = f_color1;
		vec4 border_color = f_color2;

		vec2 screen_pos = to_screen(f_pos.xy);
		float distance1 = length(screen_pos - center1);
		float distance2 = length(screen_pos - center2);
		float color_blend = clamp(distance1 - radius1 + 0.5, 0.0, 1.0);
		float alpha_blend = clamp(radius2 - distance2 + 0.5, 0.0, 1.0);
		vec4 color = mix(bg_color, border_color, color_blend);
		color *= alpha_blend;
		write_color(color);
	}
	else if (shader == SHADER_RECT)
	{
		float border_width = f_uv1.x;
		float border_distance = f_uv1.y;
		vec2 uv = f_uv1.zw;
		vec4 bg_color = f_color1;
		vec4 border_color = f_color2;

		// Distance from edge.
		// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.
		// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.
		// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.
		float edge_alpha = clamp(border_distance + 0.5, 0.0, 1.0);

		// The +0.5 here is the same as above
		float dclamped = clamp(border_width - border_distance + 0.5, 0.0, 1.0);

		if (enableBGTex)
			bg_color *= texture2D(f_tex0, uv);

		vec4 color = mix(bg_color, border_color, dclamped);
		color *= edge_alpha;
		write_color(color);
	}
	else if (shader == SHADER_TEXT_SIMPLE)
	{
		vec4 texCol = texture2D(f_tex0, f_uv1.xy);
		write_color(texCol.rrrr * premultiply(f_color1));
	}
#if defined(XO_PLATFORM_WIN_DESKTOP) || defined(XO_PLATFORM_LINUX_DESKTOP)
	else if (shader == SHADER_TEXT_SUBPIXEL)
	{
		float offset = 1.0 / XO_GLYPH_ATLAS_SIZE;
		vec2 uv = f_uv1.xy;
		vec4 tex_clamp = f_uv2;

		float tap0 = texture2D(f_tex0, vec2(clamp(uv.s - offset * 3.0, tex_clamp.x, tex_clamp.z), uv.t)).r;
		float tap1 = texture2D(f_tex0, vec2(clamp(uv.s - offset * 2.0, tex_clamp.x, tex_clamp.z), uv.t)).r;
		float tap2 = texture2D(f_tex0, vec2(clamp(uv.s - offset * 1.0, tex_clamp.x, tex_clamp.z), uv.t)).r;
		float tap3 = texture2D(f_tex0, vec2(clamp(uv.s             ,   tex_clamp.x, tex_clamp.z), uv.t)).r;
		float tap4 = texture2D(f_tex0, vec2(clamp(uv.s + offset * 1.0, tex_clamp.x, tex_clamp.z), uv.t)).r;
		float tap5 = texture2D(f_tex0, vec2(clamp(uv.s + offset * 2.0, tex_clamp.x, tex_clamp.z), uv.t)).r;
		float tap6 = texture2D(f_tex0, vec2(clamp(uv.s + offset * 3.0, tex_clamp.x, tex_clamp.z), uv.t)).r;

		float w0 = 0.56;
		float w1 = 0.28;
		float w2 = 0.16;
		//float w0 = 0.60;
		//float w1 = 0.28;
		//float w2 = 0.12;

		float r_mask = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);
		float g_mask = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);
		float b_mask = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);
		float aR = r_mask * f_color1.a;
		float aG = g_mask * f_color1.a;
		float aB = b_mask * f_color1.a;
		float avgA = (r_mask + g_mask + b_mask) / 3.0;

		out_color0 = vec4(f_color1.rgb, avgA);
		out_color1 = vec4(aR, aG, aB, avgA);
	}
#endif
	else
	{
		write_color(vec4(1, 1, 0, 1));
	}
}
