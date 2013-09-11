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

	gl_FragColor = color;
	gl_FragColor.a *= clamp(iradius - dist, 0.0, 1.0);

	//gl_FragColor.rgb = vec3(1.0, 0, 0);
	//gl_FragColor.a = pow(0.25, 2.2);

	//gl_FragColor.rgb = color.rgb;
	//gl_FragColor.a = 0.25;

	//gl_FragColor.r += 0.3;
	//gl_FragColor.a += 0.3;
}
