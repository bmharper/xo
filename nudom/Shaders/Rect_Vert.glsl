uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
varying		vec4	pos;
varying		vec4	color;
void main()
{
	pos = mvproj * vpos;
	gl_Position = pos;
	color = fromSRGB(vcolor);
}
