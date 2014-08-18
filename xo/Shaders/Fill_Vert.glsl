uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
varying		vec4	color;
void main()
{
	gl_Position = mvproj * vpos;
	color = vec4(pow(vcolor.rgb, vec3(2.2, 2.2, 2.2)), vcolor.a);
}
