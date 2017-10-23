# xo

The idea behind xo is to take inspiration from the good parts of HTML/CSS, and use those to build a
small cross-platform GUI framework that is usable from a compiled language, and statically
linkable to build native applications for a wide range of platforms.

Some "native" applications actually use a browser as their presentation system, and many of these
applications seem to use substantially more memory and CPU than I think is necessary. Part of the
goal with this project is to allow you to build a lightweight application that launches instantly,
consumes next to zero resources when idle, and has a small size footprint.

### Using xo

The only platform that I'm actively working on right now is Windows Desktop. 
Every few months I make sure that Android and Linux (X11) still runs.

The easiest way to use xo is to build with the "amalgamation". The amalgamation is a set of 3 files:

* xo-amalgamation.h
* xo-amalgamation.cpp
* xo-amalgamation-freetype.c

Simply add `xo-amalgamation.cpp` and `xo-amalgamation-freetype.c` into your project. If you are building a cross-platform application, and you want xo to handle the event loop for you, then you will need to include at least one other file (xoWinMain.cpp). On Android, there is a bit more stuff to add, such as JNI bindings.

The amalgamation is hosted [here](https://github.com/bmharper/xo-amalgamation), but you can also build it yourself by running `meta\create-amalgamation.rb`.

### Building from full source

* Install Visual Studio 2015, or GCC on Linux.
* Install [tundra2](https://github.com/deplinenoise/tundra/releases).

You should now be able to build and run:
	
	tundra2 HelloWorld
	t2-output\win64-msvc2015-debug-default\HelloWorld.exe

If you move the cursor around on the screen, then the green square and text will move with it.

There is another sample application called `KitchenSink` that stresses the layout system more.

If you change shaders, then you must run `meta\shaders.rb` before building again.

### Using Visual Studio

Tundra can generate Visual Studio IDE projects for you.  
Use `meta\genide.bat` to generate IDE projects that you can open in Visual Studio.

## Design goals
* Keep the entire library small. At last count, xo compiles to 220 KB of code, but dependencies
such as Freetype bring the size up to around 2 MB.
* Render the entire scene through the GPU (OpenGL ES 2.0, or DirectX 11).
* Parallelize layout and rendering (the pre-GPU phases).
* Strive to keep latency low.
* Make it easy for games to integrate xo.
* Separate the *DOM manipulation* and *render* threads. This allows animations to run on the render thread
at 60 hz, while you're free to take significantly more time than 16ms to perform your computation and update the DOM.
While a moot point for games, this has material benefits for non-realtime applications.
* Try to come up with a re-imagined layout language that is simple and predictable.

## Target platforms
* Android
* Linux
* Windows Desktop
* Windows Apps (not yet)
* iOS (not yet)
* OSX (not yet)

xo is written in C++, but it would be nice to have bindings from other languages.

Sample
------

	auto btn = root->ParseAppendNode("<div class='button'>Click Me</div>");
	btn->OnClick([btn]() { btn->SetText("You clicked me"); });

Status
------

You can build some basic things with xo. I have gone through three iterations of the layout system,
and I think the current design is going to be OK. Windows Desktop support is good, but I haven't
built Linux or Android in a long time, although there's nothing in principle preventing me reviving
those platforms.

There is an OpenGL and a DirectX 11 backend, but when I moved over to an uber-shader,
I only ported OpenGL, so DirectX needs work there.

* Purely horizontal text is rendered well on Windows, where you typically have low resolution (< 100 dpi) monitors.

Why?
----
We want to write an application once, and have it run on many platforms. GUI is often the hardest thing to make cross platform.
Browsers have become an excellent GUI system, but every now and then you're better suited by building a native application.
