uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
attribute	vec2	vtexuv0;
varying		vec4	pos;
varying		vec4	color;
varying		vec2	texuv0;
void main()
{
	pos = mvproj * vpos;
	gl_Position = pos;
	texuv0 = vtexuv0;
	color = fromSRGB(vcolor);
}
