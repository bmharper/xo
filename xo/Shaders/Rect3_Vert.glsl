uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
attribute 	float	vborder_width;
attribute 	float	vborder_distance;
attribute 	vec4	vborder_color;

varying 	vec4	pos;
varying 	vec4	color;
varying 	float	border_width;
varying 	float	border_distance;
varying 	vec4	border_color;

void main()
{
	pos = mvproj * vpos;
	gl_Position = pos;
	border_width = vborder_width;
	border_distance = vborder_distance;
	border_color = fromSRGB(vborder_color);
	color = fromSRGB(vcolor);
}
