precision mediump float;
varying vec4	pos;
varying vec4	color;
uniform float	radius;
uniform vec4	box;
uniform vec2	vport_hsize;

vec2 to_screen( vec2 unit_pt )
{
	return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;
}

void main()
{
	vec2 screenxy = to_screen(pos.xy);
	float left = box.x + radius;
	float right = box.z - radius;
	float top = box.y + radius;
	float bottom = box.w - radius;
	vec2 cent = screenxy;
	float iradius = radius;
	if (		screenxy.x < left && screenxy.y < top )		{ cent = vec2(left, top); }
	else if (	screenxy.x < left && screenxy.y > bottom )	{ cent = vec2(left, bottom); }
	else if (	screenxy.x > right && screenxy.y < top )	{ cent = vec2(right, top); }
	else if (	screenxy.x > right && screenxy.y > bottom )	{ cent = vec2(right, bottom); }
	else iradius = 10.0;

	// If you draw the pixels out on paper, and take cognisance of the fact that
	// our samples are at pixel centers, then this -0.5 offset makes perfect sense.
	// This offset is correct regardless of whether you're blending linearly or in gamma space.
	float dist = length( screenxy - cent ) - 0.5;

	vec4 outcolor = color;
	outcolor.a *= clamp(iradius - dist, 0.0, 1.0);

#ifdef NU_SRGB_FRAMEBUFFER
	gl_FragColor = outcolor;
#else
	float igamma = 1.0/2.2;
	gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));
	gl_FragColor.a = outcolor.a;
#endif
}
