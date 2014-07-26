#ifdef NU_PLATFORM_ANDROID
precision mediump float;
#endif
varying vec4	pos;
varying vec4	color;
uniform float	radius;
uniform vec4	box;
uniform vec4	border;
uniform vec4	border_Color;
uniform vec2	vport_hsize;

vec2 to_screen( vec2 unit_pt )
{
	return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;
}

void main()
{
	float left		= box.x + border.x + radius;
	float right		= box.z - border.z - radius;
	float top		= box.y + border.y + radius;
	float bottom	= box.w -+ border.w - radius;
	
	vec2 screenxy = to_screen(pos.xy);

	vec2 cent = screenxy;
	cent.x = clamp(cent.x, left, right);
	cent.y = clamp(cent.y, top, bottom);

	// If you draw the pixels out on paper, and take cognisance of the fact that
	// our samples are at pixel centers, then this -0.5 offset makes perfect sense.
	// This offset is correct regardless of whether you're blending linearly or in gamma space.
	// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset
	// that is fed into the shader's "radius" uniform, we effectively get rectangles to be sharp
	// when they are aligned to an integer grid. I haven't thought this through carefully enough,
	// but it does feel right.
	float dist = length(screenxy - cent) - 0.5;

	vec4 outcolor = color;
	outcolor.a *= clamp(radius - dist, 0.0, 1.0);

#ifdef NU_SRGB_FRAMEBUFFER
	gl_FragColor = outcolor;
#else
	float igamma = 1.0/2.2;
	gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));
	gl_FragColor.a = outcolor.a;
#endif
}
