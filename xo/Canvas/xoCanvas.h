#pragma once

#include "../xoDefs.h"

// Simple canvas that uses AGG for rendering
// Pixel format is BGRA 8 bits/sample, premultiplied alpha
class XOAPI xoCanvas
{
public:
				xoCanvas();
				~xoCanvas();

	bool		Resize( uint32 width, uint32 height );		// Returns false if malloc() fails. Memory after allocation is uninitialized.

	void		FillRect( xoBox box, xoColor color );
	
	const void*	Scanline0() const	{ return RenderBuff.row_ptr(0); }
	int32		Stride() const		{ return RenderBuff.stride(); }

	uint32		Width() const	{ return RenderBuff.width(); }
	uint32		Height() const	{ return RenderBuff.height(); }

	void		StrokeLine( bool closed, int nvx, const float* vx, int vx_stride_bytes, xoColor color, float linewidth );

protected:
	typedef agg::renderer_base< agg::pixfmt_bgra32_pre >			TRenderBaseBGRA_Pre;
	typedef agg::renderer_scanline_aa_solid< TRenderBaseBGRA_Pre >	TRendererAA_BGRA_Pre;
	typedef agg::rasterizer_scanline_aa<>							TRasterScanlineAA;
	typedef agg::conv_clip_polyline< agg::path_storage >			TLineClipper;
	typedef agg::conv_clip_polygon< agg::path_storage >				TFillClipper;

	void*					Buffer		= nullptr;
	agg::scanline_u8		Scanline;
	TRasterScanlineAA		RasAA;
	agg::rendering_buffer 	RenderBuff;
	TRenderBaseBGRA_Pre		RenderBaseBGRA_Pre;
	TRendererAA_BGRA_Pre	RenderAA_BGRA_Pre;
	agg::pixfmt_bgra32_pre	PixFormatBGRA_Pre;

	agg::rgba ColorToAggRgba( xoColor c );

};
