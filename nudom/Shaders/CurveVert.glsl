varying vec4 pos;
varying vec2 texuv0;
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_FrontColor = gl_Color;
	pos = gl_Position;
	texuv0 = gl_MultiTexCoord0.xy;
}
 
