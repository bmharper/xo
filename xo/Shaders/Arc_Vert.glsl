uniform		mat4	mvproj;

attribute	vec4	vpos;
attribute	vec4	vcenter;
attribute	vec4	vcolor;
attribute	vec4	vborder_color;
attribute	float	vborder_width;
attribute	float	vradius;

varying		vec4	pos;
varying		vec4	center;
varying		vec4	color;
varying		vec4	border_color;
varying		float	radius1;
varying		float	radius2;

void main()
{
	pos = mvproj * vpos;
	center = mvproj * vcenter;
	gl_Position = pos;
	color = fromSRGB(vcolor);
	border_color = fromSRGB(vborder_color);
	radius1 = vradius - vborder_width;
	radius2 = vradius;
}
