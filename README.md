nudom
=====

A new DOM.

The idea behind nudom is to take the good parts of HTML/CSS, and use those to build a
cross-platform GUI framework that is usable from a compiled language, and statically
linkable to build native applications for a wide range of platforms.

Design goals

* Keep the entire library small. Right now the goal is 1MB of compiled code.
* Depend upon the GPU.
* Parallelize layout and rendering (the pre-GPU phases).
* Make it easy to maintain 120 hz for wearable computing devices.
* Separate the *DOM manipulation* and *render* threads. This allows animations to run on the render thread
at 120 hz, while you're free to take significantly more time than 8ms to perform your computation and update the DOM.

Target platforms

* Android
* Linux
* Windows Desktop
* Windows Metro
* iOS
* OSX

nudom is written in C++, but it should be usable from other languages.

Sample
------

	nuDomEl* btn = doc->Append( "<div class='button'>Click Me</div>" );
	btn->OnClick( [](nuEvent* ev) => { /* do something */ } );

Status
------

*GARAGE EXPERIMENT*

I have three colored, rounded rectangles on the screen, that you can control with a finger or mouse move.  
Running on Android and Windows.

Why?
----

We want to write an application once, and have it run on many platforms.

Why not ... ?
-------------

* __HTML__ Writing code inside a browser imposes numerous constraints, including performance,
memory usage, access to hardware, generic network access.
* __libRocket__ This is painful to ignore, because we're obviously in dubious "Not Invented Here" territory.
libRocket is almost what we want. I am writing nudom mostly to scratch an itch, and much of the joy here is
in the journey. I wanted to start with a clean slate, and be sure that I could make this thing pretty tight code.
* __QT__ Not invented here!