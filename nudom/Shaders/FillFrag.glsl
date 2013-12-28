#ifdef NU_PLATFORM_ANDROID
precision mediump float;
#endif
varying vec4	color;
void main()
{
	gl_FragColor = color;
}
