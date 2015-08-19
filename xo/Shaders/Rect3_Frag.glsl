varying 	vec4	pos;
varying 	vec4	color;
varying 	float	border_width;
varying 	float	distance;
varying 	vec4	border_color;

void main()
{
	float dclamped = clamp(distance - border_width, 0, 1);
	gl_FragColor = mix(color, border_color, dclamped);
}
