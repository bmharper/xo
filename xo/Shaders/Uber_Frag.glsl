uniform		vec2	Frame_VPort_HSize;

uniform sampler2D	f_tex0;

varying		vec2	f_pos;
varying		vec4	f_uv1;
varying		vec4	f_uv2;
varying		vec4	f_color1;
varying		vec4	f_color2;
varying 	float	f_shader;

#if defined(XO_PLATFORM_WIN_DESKTOP) || defined(XO_PLATFORM_LINUX_DESKTOP)
// This needs at least #version 130
//layout(location = 0, index = 0) out vec4 outputColor0;
//layout(location = 0, index = 1) out vec4 outputColor1;
// On NVidia (9.18.13.3165 (10-23-2013), R331.65 (branch: r331_00-146)),
// one doesn't need the layout qualification, nor glBindFragDataLocationIndexed. The order in which you
// declare the output variables is sufficient to make an affinity with "color0" or "color1".
// It is needed though, on Intel Haswell drivers on Linux
out		vec4		out_color0;
out		vec4		out_color1;

// color must be premultiplied
void write_color(vec4 color)
{
	//                                        srcColor   dstColor              srcAlpha   dstAlpha
	//                                           |       |                        |       |
	// Our blend equation is glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_COLOR, GL_ONE, GL_ONE_MINUS_SRC1_ALPHA),
	// which means source RGB is multiplied by ONE, and destination RGB is multiplied by (1 - SRC1_COLOR), individually for each component.
	// In the classic blend OVER equation, you have R = a*r + (1-a)*R, where R is the framebuffer Red. In this equation,
	// the 'a' is the same for each of the red, green, and blue channels.
	// However, glBlendFuncSeparate means that the 'a' in the above equation is no longer just a single value, but it can
	// vary per color component. This is key for doing sub-pixel text rendering, where the 'a' for a channel is computed as
	// the alpha value of the text color, multiplied by the filter value for that subpixel. This yields what we name aR for
	// alpha red, aG for alpha green, etc. Those are the final alpha values that we want to send the blender, and critically,
	// those are the ones that have meaning through the GL_ONE_MINUS_SRC1_COLOR, shown above.
	// So... if you need this setup to behave like the classic blend equation, then the red,green,blue "alpha" values that
	// you're sending to GL_ONE_MINUS_SRC1_COLOR, should all be the classic source alpha value. That is why we fill
	// out_color1 with color.aaaa.
	// There is one redundant piece of information here - and that is out_color0.a. That value is not used at all during blending,
	// so no matter what you make it, it won't change a thing.
	out_color0 = color;
	out_color1 = color.aaaa;
}
#else
void write_color(vec4 color)
{
	gl_FragColor = color;
}
#endif

vec4 blend_over(vec4 a, vec4 b)
{
	return (1.0 - b.a) * a + b;
}

vec2 to_screen(vec2 pos)
{
	return (vec2(pos.x, -pos.y) + vec2(1,1)) * Frame_VPort_HSize;
}

vec4 read_bgtex(vec2 uv, bool isAlreadyPremultiplied)
{
	vec4 c = texture2D(f_tex0, uv);
	if (!isAlreadyPremultiplied)
		c = premultiply(c);
	return c;
}

void main()
{
	int shader = int(f_shader);
	bool enableBGTex = (shader & SHADER_FLAG_TEXBG) != 0;
	bool bgTexPremul = (shader & SHADER_FLAG_TEXBG_PREMUL) != 0;
	shader = shader & SHADER_TYPE_MASK;

	if (shader == SHADER_ARC)
	{
		vec2 center1 = f_uv1.xy;
		vec2 center2 = f_uv1.zw;
		float radius1 = f_uv2.x;
		float radius2 = f_uv2.y;
		vec2 uv = f_uv2.zw;
		vec4 bg_color = premultiply(f_color1);
		vec4 border_color = premultiply(f_color2);

		vec4 bg_tex = vec4(0, 0, 0, 0);
		if (enableBGTex)
		{
			bg_tex = read_bgtex(uv, bgTexPremul);
			bg_color = blend_over(bg_color, bg_tex);
		}

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
		vec4 bg_color = premultiply(f_color1);
		vec4 border_color = premultiply(f_color2);

		// Distance from edge.
		// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.
		// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.
		// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.
		float edge_alpha = clamp(border_distance + 0.5, 0.0, 1.0);

		// The +0.5 here is the same as above
		float dclamped = clamp(border_width - border_distance + 0.5, 0.0, 1.0);

		vec4 bg_tex = vec4(0, 0, 0, 0);
		if (enableBGTex)
		{
			bg_tex = read_bgtex(uv, bgTexPremul);
			bg_color = blend_over(bg_color, bg_tex);
		}

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

		// Note that f_color1 needs to be non-premultiplied here.

		float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);
		float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);
		float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);
		float aR = r * f_color1.a;
		float aG = g * f_color1.a;
		float aB = b * f_color1.a;
		float avgA = (r + g + b) / 3.0;

		// See the long explanation inside write_color for more details. In order to be consistent with regular
		// (non-sub-pixel) rendering, we need to premultiply our RGB here with their respective alpha values.
		out_color0 = vec4(f_color1.r * aR, f_color1.g * aG, f_color1.b * aB, avgA);
		out_color1 = vec4(aR, aG, aB, avgA);
	}
#endif
	else
	{
		write_color(vec4(1, 1, 0, 1));
	}
}
