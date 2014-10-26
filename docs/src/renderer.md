Goals
-----
Do not update texture atlases during a render. Instead, mark the render as invalid and queue the glyph
load for the next render.