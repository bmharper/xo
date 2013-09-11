Gamma, Linear, sRGB
===================

When working with old OpenGL implementations, there is no place in
the pipeline where the GL performs conversion between a gamma space
(sRGB) and a linear space. These concepts were introduced by various
OpenGL extensions, and later became part of the mandated standard,
sometime around OpenGL 3.2 (I think) on the desktop, and OpenGL ES 3.0.

Without these concepts, the GL pipeline doesn't actually care what
color space you're working in. You typically send in colors and texels
in the range of [0..255]. Before these reach your shaders, they are
scaled to the [0..1] range. Your shader's output to gl_FragColor is
also in the [0..1] range, and that is scaled back to the [0..255]
range by simply multiplying by 255. None of this had any knowledge
of a gamma ramp - it was all simply dividing by 255 or multiplying
by 255.

The significant difference when you're paying attention to this stuff
is that you can get the GL pipeline to perform various non-linear
scalings to your inputs and outputs. Of particular relevance are
texture formats such as GL_SRGB (affecting the input before it hits
your shader), and extensions such as ARB_framebuffer_sRGB, which
perform non-linear scaling from your [0..1] shader outputs back
into sRGB [0..255]. Another critical aspect of the ARB_framebuffer_sRGB
extension is that it performs blending in linear space, before
converting to sRGB.

So what does nuDom do with this? Where possible, we use an sRGB
framebuffer, because linear blending yields better results. You
can fake these kinds of results if you know that your background
is always light and your text (or whatever) is always dark, but
in order to get consistently good results, linear blending is the
only way.

Where an sRGB framebuffer is not available (Galaxy S3, i.e. Mali400),
then we simply blend non-linearly.

All vertex color inputs are non-premultiplied sRGB. The alpha, however,
is linear. This is consistent with what Freetype produces, or indeed
what most antialiasing rasterizers give you. When we input a rasterized
glyph from Freetype, we send it in as a GL_LUMINANCE texture. I
initially made the mistake of thinking that it should be GL_SLUMINANCE
(ie non-linear), but that is wrong.

Our vertex shaders expect sRGB colors, so they perform the conversion
from sRGB to linear, with the approximation pow(color, 2.2). What do
we do with alpha though? It gets a little tricky here. If we use alpha
as a linear coverage value, which is exactly what Freetype does, then
we should not raise our alpha component to 2.2 the same way we do
for the RGB channels. This is exactly what we do with GL_LUMINANCE
textures of Freetype glyphs. i.e. That 8-bit glyph texture is consumed
by our fragment shader and turned into an alpha value between 0 and 1,
which is then sent to the blender. We do no raising of that alpha value
to any power, nor do any other non-linear operations on it. The same
is not true for vertex colors though.

Vertex colors in nuDom almost always originate NOT from a
coverage-generating rasterizer, but from a human being setting a color
such as #ff000080 = rgba(255,0,0,128). People are accustomed by now to
seeing the alpha channel cause blending in gamma space. In other words,
if you compose a red of 255 onto a black background, with an alpha of
128, then you expect to get a red of 128 as your final color. This is
gamma-space blending. It certainly is wrong from a certain point of view,
but it is what's expected. More important than that however, is the
fact that many mobile GPUs do not support linear framebuffer blending,
and it would not earn nuDom any thanks if it rendered differently on
devices that claim to be supported.

So we have a problem: We want to use linear blending when rendering
text, but we don't want it when rendering solid objects, such as
solid-filled backgrounds. The solution is fortunately very simple:
Raise alpha to 2.2 along with RGB, and you're back to gamma-space
blending. My old nVidia 460 GTX actually seems to adhere to the
sRGB spec better than simply doing a 2.2 exponential, because the
numbers I get are always a little off (like 2 or 3 out of 255).

Having said all that, I do believe that having linear blending
turned on for all operations would actually be a good thing. Since
nuDom is embedded, applications are free to choose the mode they
want. I'm going to leave a global switch in there that enables it
for all operations (not only text).

Having this switchable *would* make it painful for the ecosystem
though, since some controls might be written for gamma-space blending,
and other controls for linear-space blending. Still... you might
be able to flip between these two with relatively little changes.
Right now all it entails is flipping the pow(alpha,2.2) on or
off in our vertex shaders, and hopefully it will remain this simple.
