varying 	vec4	pos;
varying 	vec4	color;
varying 	float	border_width;
varying 	float	border_distance;
varying 	vec4	border_color;

void main()
{
	// Distance from edge.
	// We target fragments that are targeted at pixel centers, so if border_distance = 0.5, then alpha must be 1.0.
	// Our alpha must drop off within a single pixel, so then at border_distance = -0.5, alpha must be 0.
	// What we're thus looking for is a line of slope = 1.0, which passes through 0.5.
	float edge_alpha = clamp(border_distance + 0.5, 0, 1);

	// The +0.5 here is the same as above
	float dclamped = clamp(border_width - border_distance + 0.5, 0, 1);

	vec4 color = mix(color, border_color, dclamped);
	color *= edge_alpha;
	gl_FragColor = color;
}
