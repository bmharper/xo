#XO_PLATFORM_WIN_DESKTOP
uniform		mat4	mvproj;
attribute	vec4	vpos;
attribute	vec4	vcolor;
attribute	float	vflip;
attribute	vec2	vtexuv0;
varying		vec4	color;
varying		vec2	texuv0;
varying		float	flip;
void main()
{
	gl_Position = mvproj * vpos;
	texuv0 = vtexuv0;
	flip = vflip;
	color = fromSRGB(vcolor);
}
 
