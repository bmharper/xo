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
The shaders are run through an extremely limited pre-processor.

You can use the following:

#ifdef SOMETHING
#else
#endif

And also
#if defined(SOMETHING) || defined(SOMETHING_ELSE)

...and that's it.

There is also the heinous restriction that all logical operators in an #if statement
must be || or &&. You cannot mix them, and you cannot use parentheses.

All tokens that are #defined are done so outside of the script. You cannot #define
something inside your script.

The following constants are defined:

	NU_PLATFORM_WIN_DESKTOP		Defined when target is Windows Desktop
	NU_PLATFORM_ANDROID			Defined when target is Android
	NU_GLYPH_ATLAS_SIZE			The size of glyph texture atlasses.
