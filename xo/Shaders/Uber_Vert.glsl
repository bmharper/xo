uniform		mat4	mvproj; // Change to Frame_MVProj once we've gotten rid of the old ones

// CPU -> Vertex Shader: 8 + 2 * 16 + 2 * 4 = 48 bytes per vertex
// Vertex -> Fragment:   8 + 4 * 16         = 68 bytes per fragment

attribute	vec2	v_pos;
attribute	vec4	v_uv1;
attribute	vec4	v_uv2;
attribute	vec4	v_color1;
attribute	vec4	v_color2;
attribute	float	v_shader; 

varying			vec2	f_pos;
varying			vec4	f_uv1;
varying			vec4	f_uv2;
varying			vec4	f_color1;
varying			vec4	f_color2;
varying 	float		f_shader;

void main()
{
	vec4 pos = mvproj * vec4(v_pos.x, v_pos.y, 0, 1);
	gl_Position = pos;
	f_pos = pos.xy;
	f_uv1 = v_uv1;
	f_uv2 = v_uv2;
	f_color1 = fromSRGB(v_color1);
	f_color2 = fromSRGB(v_color2);
	f_shader = v_shader;
}
