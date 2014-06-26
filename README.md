nudom
=====

A new DOM.

The idea behind nudom is to take the good parts of HTML/CSS, and use those to build a
cross-platform GUI framework that is usable from a compiled language, and statically
linkable to build native applications for a wide range of platforms.

Build instructions
------------------
The only platform that I'm actively working on right now is Windows Desktop. 
Every few weeks I make sure that Android still runs. I have had it running under Linux too,
but haven't bothered to document that yet.

Build requirements:

* The build system is [tundra2](https://github.com/deplinenoise/tundra)

### Building on Windows

* Install Visual Studio 2013.
* Install tundra2.
	* Install the [latest binary release](https://github.com/deplinenoise/tundra/releases)
	* Clone the latest tundra `master` branch onto your machine and compile a release build using the provided Visual Studio solution.
	* Overwrite the appropriate pieces of the official release with the exe's that you just compiled.
	* Overwrite the `scripts` directory with the latest scripts from `master`.

Once you have the latest tundra, you can do this:
	
	tundra2 HelloWorld
	t2-output\win64-msvc2013-debug-default\HelloWorld.exe

If you move the cursor around on the screen, then the green square and text will move with it.

There is another sample application called `KitchenSink` that stresses the layout system more.

If you change shaders, then you must run `build\shaders.rb` before building again.

### Using Visual Studio

Tundra can generate Visual Studio IDE projects for you.  
Use `build\genide.bat` to generate IDE projects that you can open in Visual Studio.

Design goals
------------
* Keep the entire library small. Right now the goal is 2MB of compiled code.
* Depend upon the GPU (OpenGL ES 2.0, or DirectX 11).
* Parallelize layout and rendering (the pre-GPU phases).
* Strive to keep latency low.
* Make it easy for games to integrate nudom.
* Separate the *DOM manipulation* and *render* threads. This allows animations to run on the render thread
at 60 hz, while you're free to take significantly more time than 16ms to perform your computation and update the DOM.
While a moot point for games, this has material benefits for non-realtime applications.
* Try to come up with a re-imagined CSS that is simple and predictable.

Target platforms
----------------
* Android
* Linux
* Windows Desktop
* Windows Metro (maybe)
* iOS
* OSX

nudom is written in C++, but it should be usable from other languages.

Sample
------

	nuDomEl* btn = root->AddNode( nuTagDiv );
	btn->AddClass( "button" );
	btn->SetText( "Click Me" );
	btn->OnClick( [](const nuEvent& ev) -> bool { /* do something */ } );

Status
------
*GARAGE EXPERIMENT*

I have three colored, rounded rectangles on the screen, that you can control with a finger or mouse move.  
Running on Android, Windows, and Linux (X-Windows).

There is an OpenGL and a DirectX 11 backend.

* Purely horizontal text is rendered well on Windows, where you typically have low resolution (< 100 dpi) monitors.
	Text through the low DPI path is vertically snapped to whole pixels, and horizontally snapped to 1/3 of a pixel,
	which effectively allows resolution-independent horizontal text layout. The lack of vertical independence is still
	a problem for achieving resolution-independent layout, but I believe this is the right tradeoff.

Why?
----
We want to write an application once, and have it run on many platforms. GUI is often the hardest thing to make cross platform.

This project is well and truly irrational. The rational thing to do would be to stick to the
browser as your UI medium.

Why not ... ?
-------------

* __HTML__ Writing code inside a browser imposes numerous constraints, including performance,
memory usage, access to hardware, generic network access. Some of these problems seem tantalizing close to
being solved by some browsers (PNaCL, asm.js, websockets, etc), however browsers are definitely
not "there yet" for a lot of applications. There is also the browser fragmentation problem: Only Chrome
supports PNaCL, only Firefox does asm.js, only IE does ActiveX!
* __libRocket__ This is painful to ignore, because we're obviously in dubious "Not Invented Here" territory.
libRocket is almost what we want. I am writing nudom mostly to scratch an itch, and much of the joy here is
in the journey. I wanted to start with a clean slate.
Also, I believe that the layout principles that have evolved inside HTML/CSS could benefit from a clean start.
* __QT__ Like I said, I'm scratching an itch.