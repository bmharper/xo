#ifdef NU_PLATFORM_ANDROID
precision mediump float;
#endif
uniform sampler2D	tex0;
varying vec4		color;
varying vec2		texuv0;
void main()
{
	vec4 texCol = texture2D(tex0, texuv0.st);
	gl_FragColor = color * vec4(1,1,1, texCol.r);
}
 
