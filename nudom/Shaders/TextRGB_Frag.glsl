#NU_PLATFORM_WIN_DESKTOP
#version 330

#ifdef NU_PLATFORM_ANDROID
precision mediump float;
#endif

// This looks absolutely terrible without sRGB blending, so we simply don't even try subpixel text
// on those platforms. As it turns out, there is great correlation between low res screen (Windows)
// and a GPU capable of sRGB blending.

uniform sampler2D	tex0;
varying vec4		color;
varying vec2		texuv0;
varying vec4		texClamp;
//layout(location = 0, index = 0) out vec4 outputColor0;
//layout(location = 0, index = 1) out vec4 outputColor1;
// On NVidia (9.18.13.3165 (10-23-2013), R331.65 (branch: r331_00-146)),
// one doesn't need the layout qualification, nor glBindFragDataLocationIndexed. The order in which you
// declare the output variables is sufficient to make an affinity with "color0" or "color1".
out		vec4		outputColor0;
out		vec4		outputColor1;
void main()
{
	float offset = 1.0 / NU_GLYPH_ATLAS_SIZE;
	vec2 uv = texuv0;

	float tap0 = texture2D(tex0, vec2(clamp(uv.s - offset * 3.0, texClamp.x, texClamp.z), uv.t)).r;
	float tap1 = texture2D(tex0, vec2(clamp(uv.s - offset * 2.0, texClamp.x, texClamp.z), uv.t)).r;
	float tap2 = texture2D(tex0, vec2(clamp(uv.s - offset * 1.0, texClamp.x, texClamp.z), uv.t)).r;
	float tap3 = texture2D(tex0, vec2(clamp(uv.s             ,   texClamp.x, texClamp.z), uv.t)).r;
	float tap4 = texture2D(tex0, vec2(clamp(uv.s + offset * 1.0, texClamp.x, texClamp.z), uv.t)).r;
	float tap5 = texture2D(tex0, vec2(clamp(uv.s + offset * 2.0, texClamp.x, texClamp.z), uv.t)).r;
	float tap6 = texture2D(tex0, vec2(clamp(uv.s + offset * 3.0, texClamp.x, texClamp.z), uv.t)).r;

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

	outputColor0 = vec4(color.rgb, avgA);
	outputColor1 = vec4(aR, aG, aB, avgA);
}
