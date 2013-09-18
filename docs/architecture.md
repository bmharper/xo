
Threads
-------

Nudom splits the world up into two logical thread groups: We call them Render and UI.
The Render thread is responsible for performing document layout, resolving all the
styles, and finally putting pixels onto the screen. There can in fact be multiple
render threads, but you can think of those as worker threads, that are coordinated
by the single main render thread. In these docs, we will frequently refer to
"The Render Thread", but what we really mean there is all of the threads that
perform rendering. For the most part, it is good enough to simply think of rendering
as happening on one thread, and UI on another thread. The Render thread only runs
nudom code. It never runs 'user' code, which is what the UI thread is for. 

The UI thread fetches input messages from the OS, and calls the appropriate event
handlers. These event handlers are what we call 'user' code, because this is code
that is not part of nudom. The document is only mutated from the UI thread.
The Renderer never alters the document - it merely creates structures necessary
to perform it's job. As far as the renderer is concerned, the document is immutable.

Why split the system up into two thread groups?
There is only one reason, and that is to maintain a high frame rate for animations.
When we say animations here, we're actually talking about what CSS calls 'transitions'.
That is, implicit rules that allow style property changes to occur over a short duration.
For example, if you change the background color of a button, you want that to happen
over a 50 millisecond timeframe, instead of instantly. These transitions are a very
powerful asset in building great UI. It is worth while to make them easy to do.
If we didn't have the split thread model, then the 'user' (programmer) would have to
make sure that every single UI operation completes well under 16ms (for 60hz). One of our
explicit target platforms is wearable computing, and in that space 120hz is desirable,
so it is worth spending some effort to make 120hz easily achievable.

Since the UI and Renderer thread groups operate at different cadences, we need
to maintain two copies of the document. One of them is owned by the UI thread,
and the other is owned by the Renderer thread. Because of this frequent copying,
we make an effort to ensure that a document copy is pretty much limited by RAM
bandwidth.

An interlude with some numbers.
A Samsung Galaxy SII can do 700 MB/s of memcpy.
Assuming the worst case of 'user' code altering the document 60 times a second,
we have 700 / 60 = 11.6 MB/frame. Of course we need to do a lot of other work,
so let's say a conservative estimate for total document size then is 1 MB.
sizeof(nuDomEl) = 128.
1 MB / 128 = 8192 naked document elements fit into a 1 MB document.

Update: An S3 has very similar memory bandwidth - around 800 MB/s.
Update: (August 2013) Intel Atom devices are repored to have up to 17 GB/s memory bandwidth.
Update: The iPhone 5S has 5 GB/s memory bandwidth.

Style resolution
----------------

Style resolution is the process of resolving the full set of style attributes
for every DOM element. I don't know how best to do this. One solution is to
perform a pass over all elements and store a resolved list of attributes for
every node, but I'm pretty sure that approach will burn too much RAM bandwidth.
Remember that each attributes is around 8 bytes, and we'll easily have 256
such attributes, which comes to 2048 bytes per DOM element. Let's say our target
DOM size is 10k, then that comes to 20 MB, which I'm pretty sure is going to be
worse than incrementally calculating the style for each element as we go.

Remember that we need to support attributes that are inherited by child
DOM elements. Font-family is one such attribute. If you set font-family on
the root level ('body' in HTML) element, then all children inherit that same
font by default.

Locks
-----

The UI document has a single giant lock. Before UI code runs, it obtains that lock.
The lock is kept for the duration of the UI processing code.

Box Model
---------

Right now I'm toying with implementing the Flex Box model first, and the traditional
(Block, Inline) ones later. Eh.. screw it maybe there is something simpler that
can cater for both styles. Maybe.

Coding Style
------------
Inside .h:

	void	Function();		// A one-liner description

Inside .cpp:

// More detailed multi-line descriptions go here.
// Don't duplicate the one-liner from the .h file here.
void Klass::Function()
{
}
