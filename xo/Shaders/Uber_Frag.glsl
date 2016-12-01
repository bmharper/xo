uniform		vec2	Frame_VPort_HSize;

varying			vec2	f_pos;
varying			vec4	f_uv1;
varying			vec4	f_uv2;
varying			vec4	f_color1;
varying			vec4	f_color2;
varying 	float		f_shader;

vec2 to_screen(vec2 pos)
{
	return (vec2(pos.x, -pos.y) + vec2(1,1)) * Frame_VPort_HSize;
}

void main()
{
	if (f_shader == SHADER_ARC)
	{
		vec2 center1 = f_uv1.xy;
		vec2 center2 = f_uv1.zw;
		float radius1 = f_uv2.x;
		float radius2 = f_uv2.y;
		vec4 bg_color = f_color1;
		vec4 border_color = f_color2;

		vec2 screen_pos = to_screen(f_pos.xy);
		float distance1 = length(screen_pos - center1);
		float distance2 = length(screen_pos - center2);
		float color_blend = clamp(distance1 - radius1 + 0.5, 0, 1);
		float alpha_blend = clamp(radius2 - distance2 + 0.5, 0, 1);
		vec4 out_color = mix(bg_color, border_color, color_blend);
		out_color *= alpha_blend;
		gl_FragColor = out_color;
	}
	else
	{
		gl_FragColor = vec4(1, 1, 0, 1);
	}
}
