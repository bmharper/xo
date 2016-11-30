uniform		mat4	mvproj;

attribute	vec4	vpos;
attribute	vec4	vcenter;
attribute	vec4	vcolor;
attribute	vec4	vborder_color;
attribute	float	vradius1;
attribute	float	vradius2;

varying		vec4	pos;
varying		vec2	center1;
varying		vec2	center2;
varying		vec4	color;
varying		vec4	border_color;
varying		float	radius1;
varying		float	radius2;

void main()
{
	pos = mvproj * vpos;
	center1 = vcenter.xy;
	center2 = vcenter.zw;
	gl_Position = pos;
	color = fromSRGB(vcolor);
	border_color = fromSRGB(vborder_color);
	radius1 = vradius1;
	radius2 = vradius2;
}
