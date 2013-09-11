#NU_PLATFORM_WIN_DESKTOP
#version 330
precision mediump float;
uniform sampler2D	tex0;
varying vec4		color;
varying vec2		texuv0;
varying vec4		texClamp;
layout(location = 0, index = 0) out vec4 outputColor0;
layout(location = 0, index = 1) out vec4 outputColor1;
void main()
{
	float offset = 1.0 / NU_GLYPH_ATLAS_SIZE;
	vec2 uv = texuv0;

	float tap0 = texture2D(tex0, vec2(clamp(uv.s - offset * 3.0, texClamp.x, texClamp.z), uv.t));
	float tap1 = texture2D(tex0, vec2(clamp(uv.s - offset * 2.0, texClamp.x, texClamp.z), uv.t));
	float tap2 = texture2D(tex0, vec2(clamp(uv.s - offset * 1.0, texClamp.x, texClamp.z), uv.t));
	float tap3 = texture2D(tex0, vec2(clamp(uv.s             ,   texClamp.x, texClamp.z), uv.t));
	float tap4 = texture2D(tex0, vec2(clamp(uv.s + offset * 1.0, texClamp.x, texClamp.z), uv.t));
	float tap5 = texture2D(tex0, vec2(clamp(uv.s + offset * 2.0, texClamp.x, texClamp.z), uv.t));
	float tap6 = texture2D(tex0, vec2(clamp(uv.s + offset * 3.0, texClamp.x, texClamp.z), uv.t));

	float w0 = 0.55;
	float w1 = 0.32;
	float w2 = 0.13;
	//float w0 = 0.98;
	//float w1 = 0.01;
	//float w2 = 0.01;

	float r = (w2 * tap0 + w1 * tap1 + w0 * tap2 + w1 * tap3 + w2 * tap4);
	float g = (w2 * tap1 + w1 * tap2 + w0 * tap3 + w1 * tap4 + w2 * tap5);
	float b = (w2 * tap2 + w1 * tap3 + w0 * tap4 + w1 * tap5 + w2 * tap6);
	float aR = r * color.a;
	float aG = g * color.a;
	float aB = b * color.a;
	float avgA = (r + g + b) / 3.0;
	//float minA = min(r,g,min(g,b));
	// ONE MINUS SRC COLOR
	//float alpha = min(min(red, green), blue);
	//gl_FragColor = vec4(aR, aG, aB, avgA);
	outputColor0 = vec4(color.r, color.g, color.b, avgA);
	outputColor1 = vec4(aR, aG, aB, avgA);
}
