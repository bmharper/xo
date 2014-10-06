#pragma once

#include "../xoDefs.h"

// Simple canvas that uses AGG for rendering.
// Pixel format is RGBA 8 bits/sample, premultiplied alpha, sRGB.
// Blending is done in gamma space (ie not good).
// This is a temporary structure, much like HTML's Context2D. When you
// create an xoCanvas2D, you must bind it to an existing image. The drawing
// commands are then executed against that image.
class XOAPI xoCanvas2D
{
public:
				xoCanvas2D( xoImage* backingImage );
				~xoCanvas2D();

	// Management
	//bool		Resize( uint32 width, uint32 height );		// Returns false if malloc() fails. Memory after allocation is uninitialized.
	//bool		CloneInto( xoCanvas2D& clone );				// Returns false if malloc() fails.
	//void		Reset();									// Frees memory, resets all state

	// Buffer/State access
	void*		Buffer()					{ return RenderBuff.buf(); }
	const void*	Buffer() const				{ return RenderBuff.buf(); }
	void*		RowPtr( int line )			{ return RenderBuff.row_ptr(line); }
	const void*	RowPtr( int line ) const	{ return RenderBuff.row_ptr(line); }
	int32		Stride() const				{ return RenderBuff.stride(); }
	uint32		StrideAbs() const			{ return RenderBuff.stride_abs(); }
	uint32		Width() const				{ return RenderBuff.width(); }
	uint32		Height() const				{ return RenderBuff.height(); }

	// Drawing functions
	void		Fill( xoColor color );
	void		FillRect( xoBox box, xoColor color );
	void		StrokeLine( bool closed, int nvx, const float* vx, int vx_stride_bytes, xoColor color, float linewidth );

protected:
	typedef agg::renderer_base< agg::pixfmt_rgba32_pre >			TRenderBaseRGBA_Pre;
	typedef agg::renderer_scanline_aa_solid< TRenderBaseRGBA_Pre >	TRendererAA_RGBA_Pre;
	typedef agg::rasterizer_scanline_aa<>							TRasterScanlineAA;
	typedef agg::conv_clip_polyline< agg::path_storage >			TLineClipper;
	typedef agg::conv_clip_polygon< agg::path_storage >				TFillClipper;

	agg::scanline_u8		Scanline;
	TRasterScanlineAA		RasAA;
	agg::rendering_buffer 	RenderBuff;
	TRenderBaseRGBA_Pre		RenderBaseRGBA_Pre;
	TRendererAA_RGBA_Pre	RenderAA_RGBA_Pre;
	agg::pixfmt_rgba32_pre	PixFormatRGBA_Pre;
	xoImage*				Image;
	bool					IsAlive;			// We have a valid Image, and non-zero width and height

	agg::rgba	ColorToAgg( xoColor c );
	agg::rgba8	ColorToAgg8( xoColor c );
	
	//bool		IsAlive() const { return Buffer() != nullptr; }
};
