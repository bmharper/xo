uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
attribute	vec2	vtexuv0;
attribute	vec4	vtexClamp;
varying		vec4	color;
varying		vec2	texuv0;
varying		vec4	texClamp;
void main()
{
	gl_Position = mvproj * vpos;
	texuv0 = vtexuv0;
	texClamp = vtexClamp;
	color = fromSRGB(vcolor);
}
