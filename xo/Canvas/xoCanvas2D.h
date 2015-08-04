#pragma once

#include "../xoDefs.h"

/*
	Simple canvas that uses AGG for rendering.
	Pixel format is RGBA 8 bits/sample, premultiplied alpha, sRGB.
	Blending is done in gamma space (ie not good).
	NOTE: If you modify the contents of the buffer without using the supplied
	functions, then you must call Invalidate() to let the system know what
	parts of the image have changed. When uploading textures to the GPU, we
	only send the modified region.
*/
class XOAPI xoCanvas2D
{
public:
	xoCanvas2D(xoImage* backingImage);
	~xoCanvas2D();

	// Buffer/State access (use Invalidate if you modify contents directly)
	void*		Buffer()						{ return RenderBuff.buf(); }
	const void*	Buffer() const					{ return RenderBuff.buf(); }
	void*		RowPtr(int line)				{ return RenderBuff.row_ptr(line); }
	const void*	RowPtr(int line) const			{ return RenderBuff.row_ptr(line); }
	int32		Stride() const					{ return RenderBuff.stride(); }
	uint32		StrideAbs() const				{ return RenderBuff.stride_abs(); }
	uint32		Width() const					{ return RenderBuff.width(); }
	uint32		Height() const					{ return RenderBuff.height(); }
	xoBox		GetInvalidRect() const			{ return InvalidRect; }								// Retrieve the bounding rectangle of all pixels that have been modified
	void		Invalidate(xoBox box)			{ InvalidRect.ExpandToFit(box); }					// Call this if you modify the buffer by directly accessing its memory
	void		Invalidate()					{ InvalidRect = xoBox(0, 0, Width(), Height()); }	// Call this if you modify the buffer by directly accessing its memory
	xoImage*	GetImage()						{ return Image; }

	// Drawing functions
	void		Fill(xoColor color);
	void		FillRect(xoBox box, xoColor color);
	void		StrokeRect(xoBox box, xoColor color, float linewidth);
	void		StrokeLine(bool closed, int nvx, const float* vx, int vx_stride_bytes, xoColor color, float linewidth);

protected:
	typedef agg::renderer_base< agg::pixfmt_rgba32_pre >			TRenderBaseRGBA_Pre;
	typedef agg::renderer_scanline_aa_solid< TRenderBaseRGBA_Pre >	TRendererAA_RGBA_Pre;
	//typedef agg::rasterizer_scanline_aa<>							TRasterScanlineAA;
	typedef agg::rasterizer_scanline_aa_nogamma<>					TRasterScanlineAA;
	typedef agg::conv_clip_polyline< agg::path_storage >			TLineClipper;
	typedef agg::conv_clip_polygon< agg::path_storage >				TFillClipper;

	agg::scanline_u8		Scanline;
	TRasterScanlineAA		RasAA;
	agg::rendering_buffer 	RenderBuff;
	TRenderBaseRGBA_Pre		RenderBaseRGBA_Pre;
	TRendererAA_RGBA_Pre	RenderAA_RGBA_Pre;
	agg::pixfmt_rgba32_pre	PixFormatRGBA_Pre;
	xoImage*				Image;
	xoBox					InvalidRect;
	bool					IsAlive;			// We have a valid Image, and non-zero width and height

	agg::rgba	ColorToAgg(xoColor c);
	agg::rgba8	ColorToAgg8(xoColor c);
	void		RenderScanlines();
};
