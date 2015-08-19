uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
attribute 	float	vborder_width;
attribute 	float	vdistance;
attribute 	vec4	vborder_color;

varying 	vec4	pos;
varying 	vec4	color;
varying 	float	border_width;
varying 	float	distance;			// distance from center
varying 	vec4	border_color;

void main()
{
	pos = mvproj * vpos;
	gl_Position = pos;
	border_width = vborder_width;
	distance = vdistance;
	border_color = fromSRGB(vborder_color);
	color = fromSRGB(vcolor);
}
