uniform		vec2	vport_hsize;

varying		vec4	pos;
varying		vec2	center1;		// Center of arc1
varying		vec2	center2;		// Center of arc2
varying		vec4	color;
varying		vec4	border_color;
varying		float	radius1;		// Radius of arc1 = Start of border
varying		float	radius2;		// Radius of arc2 = End of border (and everything else). radius1 = radius2 - border_width

vec2 to_screen(vec2 unit_pt)
{
	return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;
}

void main()
{
	vec2 screen_pos = to_screen(pos.xy);
	float distance1 = length(screen_pos - center1);
	float distance2 = length(screen_pos - center2);
	vec4 out_color = color;
	float color_blend = clamp(distance1 - radius1 + 0.5, 0, 1);
	float alpha_blend = clamp(radius2 - distance2 + 0.5, 0, 1);
	out_color = mix(color, border_color, color_blend);
	//out_color.r *= 0.5;
	out_color *= alpha_blend;
	gl_FragColor = out_color;
}
