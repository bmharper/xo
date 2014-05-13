Layout Concepts
===============

Horizontal vs Vertical
----------------------
When performing layout, a box can specify whether it uses
vertical or horizontal layout. Vertical is the regular thing
you expect, where new lines are underneath each other, and text
flows horizontally. In this case we say that the major axis
is vertical and the minor axis is horizontal.

A box can also specify that an axis direction is reversed.
The normal direction for vertical is top to bottom.
The normal direction for horizontal is left to right.
nuFlowDirection is the enum used to control this.

To set horizontal to reversed, use the following style:

	flow-direction-horizontal: reverse

To set horizontal to normal, use the following style:

	flow-direction-horizontal: normal

Size units
----------
* Pixels (px) - physical device pixels. These are physical pixels, at least
to the extent that such a thing exists. Screens with "PenTile" configurations
share subpixel elements with neighbouring pixels, so you don't actually
have the exact resolution that the device claims. Nevertheless, it is not
worth fussing at that level, because these devices typically have high
enough pixel density that it doesn't matter.

* Eye Pixels (ep) - pixel-inspired unit that is calibrated to the typical
viewing conditions of the device. This takes into account both the screen
resolution as well as the typical viewing distance. For example, a phone
and a 23" monitor may have the same screen resolution of 1920 x 1080.
The phone's screen is maybe 6" large, so its resolution is about 4 times
that of the desktop monitor. However, the phone is held much closer to the
eye than the monitor, so visual objects can be physicaller smaller and still
be readable. The eye pixel is a fudged unit, that is roughly 1:1 to
physical pixels of a 23" 1920x1080 desktop monitor.
On Android, we use [scaledDensity](http://developer.android.com/reference/android/util/DisplayMetrics.html#scaledDensity)
On Windows, we use `GetDeviceCaps(LOGPIXELSX)` / 96.0

* Percent (%) - percentage of parent content box.
* Points - TODO - considering leaving this out. Don't know how it adds value.
* EM - TODO
* Zero - this is represented as 0 pixels ("0px" or just "0").


"Display" types
---------------
* __Block__ Block implies a line break before and after the element.
* __Inline__ Inline has no implicit line break, before or after.

Hang on - maybe we can get away without defining "block" or "inline",
and rather implement those concepts with style classes.

Text tags
---------
Text must live inside 'text' tags. Text tags must be leaves - ie they
may not have any children.

Layout Algorithm
----------------

I'm still struggling to formulate this, so here are a bunch of principles:

* Containing Box is the Content Box of the parent. Starting at the top,
the Containing Box of the <body> element is the viewport dimensions.

* Width and Height are treated equally. The only thing that makes them
seem different is that the default flow mode is vertical.

* Instead of 'block' and 'inline' modes, we instead have a style
attribute called "break-after", which, if true, causes the flow to move
in the major flow direction. For convenience there is also a <br> element,
which by default has the "break-after" attribute set.

* A new line (which is \n only) in text elements causes a line break.

* If the width/height of an element is unspecified, then it grows forever,
regardless of the size of its parent content box. For example, if you had
text inside an element with no width (and assuming vertical flow), then
the only thing that would cause the text to wrap onto the next line would
be the presence of a <br> element inside the text.
On the contrary, if the width/height of an element is specified, then
its children wrap around inside it.

* We perform centering by introducing style attributes called "center-horizontal"
and "center-vertical". By setting "center-horizontal: 50%", you can center
horizontally. The "left" and "right" attributes will clobber the center
attributes.

* Our default box-sizing is "border". This just makes sense for UI layout.

* Width or Height specified as a percentage is a percentage of the size of
the Content Box of its parent.

* We do not collapse margins at all.