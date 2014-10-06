uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
varying		vec4	color;
void main()
{
	gl_Position = mvproj * vpos;
	color = fromSRGB(vcolor);
}
