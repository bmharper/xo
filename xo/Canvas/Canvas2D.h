#pragma once

#include "../Defs.h"

namespace xo {

/*
	Simple canvas that uses AGG for rendering.
	Pixel format is RGBA 8 bits/sample, premultiplied alpha, sRGB.
	Blending is done in gamma space (ie not good).
	NOTE: If you modify the contents of the buffer without using the supplied
	functions, then you must call Invalidate() to let the system know what
	parts of the image have changed. When uploading textures to the GPU, we
	only send the modified region.
*/
class XO_API Canvas2D {
public:
	Canvas2D(Texture* backingImage);
	~Canvas2D();

	// Buffer/State access (use Invalidate if you modify contents directly)
	void*       Buffer() { return RenderBuff.buf(); }
	const void* Buffer() const { return RenderBuff.buf(); }
	void*       RowPtr(int line) { return RenderBuff.row_ptr(line); }
	const void* RowPtr(int line) const { return RenderBuff.row_ptr(line); }
	void*       PixelPtr(int x, int y) { return RenderBuff.row_ptr(y) + 4 * (uint32_t) x; }
	const void* PixelPtr(int x, int y) const { return RenderBuff.row_ptr(y) + 4 * (uint32_t) x; }
	int32_t     Stride() const { return RenderBuff.stride(); }
	uint32_t    StrideAbs() const { return RenderBuff.stride_abs(); }
	uint32_t    Width() const { return RenderBuff.width(); }
	uint32_t    Height() const { return RenderBuff.height(); }
	Box         GetInvalidRect() const { return InvalidRect; }               // Retrieve the bounding rectangle of all pixels that have been modified
	void        Invalidate(Box box) { InvalidRect.ExpandToFit(box); }        // Call this if you modify the buffer by directly accessing its memory
	void        Invalidate() { InvalidRect = Box(0, 0, Width(), Height()); } // Call this if you modify the buffer by directly accessing its memory
	Texture*    GetImage() { return Image; }
	// Drawing functions
	void Fill(Color color);
	void FillRect(Box box, Color color);
	void StrokeRect(Box box, Color color, float linewidth);
	void StrokeLine(bool closed, int nvx, const float* vx, int vx_stride_bytes, Color color, float linewidth);
	void StrokeLine(float x1, float y1, float x2, float y2, Color color, float linewidth);
	void StrokeCircle(float x, float y, float radius, Color color, float linewidth);
	void FillCircle(float x, float y, float radius, Color color);
	void RenderSVG(const char* svg);

	void SetPixel(int x, int y, RGBA c) { ((uint32_t*) RenderBuff.row_ptr(y))[x] = c.u; }

protected:
	typedef agg::renderer_base<agg::pixfmt_rgba32_pre>           TRenderBaseRGBA_Pre;
	typedef agg::renderer_scanline_aa_solid<TRenderBaseRGBA_Pre> TRendererAA_RGBA_Pre;
	//typedef agg::rasterizer_scanline_aa<>							TRasterScanlineAA;
	typedef agg::rasterizer_scanline_aa_nogamma<>      TRasterScanlineAA;
	typedef agg::conv_clip_polyline<agg::path_storage> TLineClipper;
	typedef agg::conv_clip_polygon<agg::path_storage>  TFillClipper;

	agg::scanline_u8       Scanline;
	TRasterScanlineAA      RasAA;
	agg::rendering_buffer  RenderBuff;
	TRenderBaseRGBA_Pre    RenderBaseRGBA_Pre;
	TRendererAA_RGBA_Pre   RenderAA_RGBA_Pre;
	agg::pixfmt_rgba32_pre PixFormatRGBA_Pre;
	Texture*               Image;
	Box                    InvalidRect;
	bool                   IsAlive; // We have a valid Image, and non-zero width and height

	agg::rgba  ColorToAgg(Color c);
	agg::rgba8 ColorToAgg8(Color c);
	void       RenderScanlines();
};
}
