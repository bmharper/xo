uniform		vec2	vport_hsize;

varying		vec4	pos;
varying		vec4	center;
varying		vec4	color;
varying		vec4	border_color;
varying		float	radius1;		// Start of border
varying		float	radius2;		// End of border (and everything else). radius1 = radius2 - border_width

vec2 to_screen(vec2 unit_pt)
{
	return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;
}

void main()
{
	vec2 screen_pos = to_screen(pos.xy);
	float distance = length(screen_pos - to_screen(center.xy));
	vec4 out_color = color;
	float color_blend = clamp(distance - radius1 + 0.5, 0, 1);
	float alpha_blend = clamp(radius2 - distance + 0.5, 0, 1);
	out_color = mix(color, border_color, color_blend);
	out_color *= alpha_blend;
	gl_FragColor = out_color;
}
