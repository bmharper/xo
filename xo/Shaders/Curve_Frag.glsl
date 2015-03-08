#XO_PLATFORM_WIN_DESKTOP
#XO_PLATFORM_LINUX_DESKTOP
// This is from Jim Blinn and Charles Loop's paper "Resolution Independent Curve Rendering using Programmable Graphics Hardware"
// We don't need this complexity here.. and if I recall correctly, this technique aliases under minification faster than
// our simpler rounded-rectangle alternative.
varying vec2		texuv0;
varying vec4		color;
varying float		flip;

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
	float alpha = 0.5 - (flip * sd);
	alpha = min(alpha, 1.0);

	vec4 col = color;
	col.a *= alpha;

	gl_FragColor = premultiply(col);
}

