uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
varying		vec4	pos;
varying		vec4	color;
void main()
{
	pos = mvproj * vpos;
	gl_Position = pos;
	
	color.rgb = pow( vcolor.rgb, vec3(2.2, 2.2, 2.2) );
	color.a = vcolor.a;
}
