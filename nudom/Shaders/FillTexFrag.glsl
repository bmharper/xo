#ifdef NU_PLATFORM_ANDROID
precision mediump float;
#endif
uniform sampler2D	tex0;
varying vec4		color;
varying vec2		texuv0;
void main()
{
	gl_FragColor = color * texture2D(tex0, texuv0.st);
}
