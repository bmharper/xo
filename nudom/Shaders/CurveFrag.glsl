// This is from Jim Blinn and Charles Loop's paper "Resolution Independent Curve Rendering using Programmable Graphics Hardware"
// We don't need this complexity here.. and if I recall correctly, this technique aliases under minification faster than
// our simpler rounded-rectangle alternative.
varying vec4 pos;
varying vec2 texuv0;

void main()
{
	vec2 p = texuv0;

	// Gradients
	vec2 px = dFdx(p);
	vec2 py = dFdy(p);

	// Chain rule
	float fx = (2 * p.x) * px.x - px.y;
	float fy = (2 * p.x) * py.x - py.y;

	// Signed distance
	float sd = (p.x * p.x - p.y) / sqrt(fx * fx + fy * fy);

	// Linear alpha
	float alpha = 0.5 - sd;

	gl_FragColor = gl_Color;

	if ( alpha > 1 )
		gl_FragColor.a = 1;
	else if ( alpha < 0 )
		discard;
	else
		gl_FragColor.a = alpha;
}

