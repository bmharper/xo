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
You can use the following pre-processor constructs inside your shaders:

### Constants
	NU_GLYPH_ATLAS_SIZE		This is replaced by the fixed size of glyph texture atlasses.

If any line in the shader contains a platform specifier string then the shader will be
initialized and used on that platform.
The only two currently supported are:

	#NU_PLATFORM_WIN_DESKTOP
	#NU_PLATFORM_ANDROID

If you do not include any platform specifiers, then the shader is used on all platforms.