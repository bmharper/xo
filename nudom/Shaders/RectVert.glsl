uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
varying		vec4	pos;
varying		vec4	color;
void main()
{
	pos = mvproj * vpos;
	//pos = mvproj * gl_Vertex;
	//pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	//pos = vpos;
	gl_Position = pos;
	//gl_FrontColor = vcolor;
	//color = vcolor;
	//color = pow( vcolor, 2.2 );
	color.rgb = pow( vcolor.rgb, 2.2 );
	color.a = vcolor.a;
}
