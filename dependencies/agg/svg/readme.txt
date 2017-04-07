I started with the svg demo from the original agg sources that I found on sourceforge, in March 2017.
Then I added support for arcs, fixed dense number parsing, and fixed the relative move on closed
poly issue. I also commented out the code that re-orients all the polygons to be CCW, because
that just breaks paths with holes in them.

After doing that work, I discovered https://github.com/dov/agg-svg.
I should probably just be a grown-up and use that instead.
The only thing negative about that, is that the filling rull is always even-odd, instead of just
commenting out the re-orientation of polygons. Obviously that's a very easy thing to fix.
I think if I discover that I have a need for gradients, then I'll switch over to
https://github.com/dov/agg-svg.
