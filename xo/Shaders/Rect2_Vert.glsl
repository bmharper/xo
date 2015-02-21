uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
attribute	float	vradius;
attribute 	float	vborder_width;
attribute 	vec4	vborder_color;

varying 	vec4	pos;
varying 	vec4	color;
varying 	float	radius;
varying 	float	border_width;
varying 	vec4	border_color;

void main()
{
	pos = mvproj * vpos;
	gl_Position = pos;
	radius = vradius;
	border_width = vborder_width;
	border_color = fromSRGB(vborder_color);
	color = fromSRGB(vcolor);
}
