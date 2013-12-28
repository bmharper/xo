Below about 24 pixel text, the lack of sub-pixel rendering is very noticeable.
On a big 1920 x 1080 Windows screen, even 24px text is ugly without subpixel.
However, on my S3, 24 is about the number where text becomes decent.

Doing sub-pixel text on a mobile device would be absolutely insane.

Possible performance enhancement on high res devices
----------------------------------------------------
Instead of padding every glyph on high res devices with 3 texel on either side,
use clamping in the pixel shader, the same way we do it for sub-pixel rendering.
Given the rise of ALU/Bandwidth, this is likely the right choice.

Sub-pixel rendering
-------------------
See this link for Dual Source Blending, which is what we use: "http://www.opengl.org/wiki/Blending#Dual_Source_Blending"