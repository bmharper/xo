Shaders
=======

Adding a new shader
-------------------
1. Create a new pair of .glsl files inside nudom/Shaders (it must be a pair unfortunately - no support in 'build/shaders.rb' for mixing and matching vert/frag)
2. From the root, run `build/shaders.rb`
3. You will have a .cpp and .h file inside nudom/Shaders/Processed. Add that .cpp file to units.lua.
4. Add your shader to the nuRenderGL class. This should be straightforward. Just follow the example of the other shaders.
5. You will probably want to setup the uniform variables (such as mvproj) for your shader in the same place where all the other shaders do their initialization.


Writing Shaders
---------------
At the top of your script, you can use these lines
#NU_PLATFORM_WIN_DESKTOP
#NU_PLATFORM_ANDROID
to specify that the shader is only used on those platforms. If you omit any such
declaration, then the shader is used on all platforms.
This approach won't work forever, especially if we start to support the new
OpenGL ES 3+ devices out there.

Before compiling shaders, they are prefixed with a bunch of #defines.
You can use those tokens inside your shaders.

The following constants are defined:

	NU_PLATFORM_WIN_DESKTOP		Defined when target is Windows Desktop
	NU_PLATFORM_ANDROID			Defined when target is Android
	NU_GLYPH_ATLAS_SIZE			The size of glyph texture atlasses.
	NU_SRGB_FRAMEBUFFER			glEnable(GL_FRAMEBUFFER_SRGB) has been called, and is available
								on this platform (set via nuGlobal()->EnableSRGBFramebuffer).
	
	<DELETED - This is going to be too tricky>
	NU_EMULATE_GAMMA_BLENDING	You should raise your alpha to 2.2 (set via nuGlobal()->EmulateGammaBlending).
	<DELETED>
