Measurement Units
=================

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
