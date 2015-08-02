varying vec4	pos;
varying vec4	color;
varying vec2	texuv0;
uniform float	radius;
uniform vec4	box;
uniform vec4	border;
uniform vec4	border_color;
uniform vec2	vport_hsize;

vec2 to_screen(vec2 unit_pt)
{
	return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;
}

#define LEFT x
#define TOP y
#define RIGHT z
#define BOTTOM w

void main()
{
	vec2 screenxy = to_screen(pos.xy);
	float radius_in = max(border.x, radius);
	float radius_out = radius;
	vec4 out_box = box + vec4(radius, radius, -radius, -radius);
	vec4 in_box = box + vec4(max(border.LEFT, radius), max(border.TOP, radius), -max(border.RIGHT, radius), -max(border.BOTTOM, radius));

	vec2 cent_in = screenxy;
	vec2 cent_out = screenxy;
	cent_in.x = clamp(cent_in.x, in_box.LEFT, in_box.RIGHT);
	cent_in.y = clamp(cent_in.y, in_box.TOP, in_box.BOTTOM);
	cent_out.x = clamp(cent_out.x, out_box.LEFT, out_box.RIGHT);
	cent_out.y = clamp(cent_out.y, out_box.TOP, out_box.BOTTOM);

	// If you draw the pixels out on paper, and take cognisance of the fact that
	// our samples are at pixel centers, then this -0.5 offset makes perfect sense.
	// This offset is correct regardless of whether you're blending linearly or in gamma space.
	// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset
	// that is fed into the shader's "radius" uniform, we effectively get rectangles to be sharp
	// when they are aligned to an integer grid. I haven't thought this through carefully enough,
	// but it does feel right.
	float dist_out = length(screenxy - cent_out) - 0.5;

	float dist_in = length(screenxy - cent_in) - (radius_in - border.x);

	vec4 outcolor = color;

	float borderWidthX = max(border.x, border.z);
	float borderWidthY = max(border.y, border.w);
	float borderWidth = max(borderWidthX, borderWidthY);
	float borderMix = clamp(dist_in, 0.0, 1.0);
	if (borderWidth > 0.5)
		outcolor = mix(outcolor, border_color, borderMix);

	outcolor.a *= clamp(radius_out - dist_out, 0.0, 1.0);
	outcolor.r = texuv0.s;
	outcolor = premultiply(outcolor);

#ifdef XO_SRGB_FRAMEBUFFER
	gl_FragColor = outcolor;
#else
	float igamma = 1.0/2.2;
	gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));
	gl_FragColor.a = outcolor.a;
#endif
}
