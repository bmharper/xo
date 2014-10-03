#include "pch.h"
#include "xoCanvas.h"

xoCanvas::xoCanvas()
{
}

xoCanvas::~xoCanvas()
{
	free(Buffer);
}

bool xoCanvas::Resize( uint32 width, uint32 height )
{
	free(Buffer);
	Buffer = nullptr;
	RenderBuff.attach( nullptr, 0, 0, 0 );

	Buffer = malloc( width * height * 4 );
	if ( Buffer == nullptr )
		return false;

	RenderBuff.attach( (uint8*) Buffer, width, height, width * 4 );
	PixFormatBGRA_Pre.attach( RenderBuff );
	RenderBaseBGRA_Pre.attach( PixFormatBGRA_Pre );
	RenderAA_BGRA_Pre.attach( RenderBaseBGRA_Pre );

	return true;
}

void xoCanvas::FillRect( xoBox box, xoColor color )
{
	RenderBaseBGRA_Pre.copy_bar( box.Left, box.Top, box.Right, box.Bottom, ColorToAggRgba(color) );
}

void xoCanvas::StrokeLine( bool closed, int nvx, const float* vx, int vx_stride_bytes, xoColor color, float linewidth )
{
	RasAA.reset();
	RasAA.filling_rule( agg::fill_even_odd );

	agg::path_storage path;
	TLineClipper						clipped( path );
	agg::conv_stroke<TLineClipper>		clipped_stroked( clipped );

	clipped_stroked.line_cap( agg::butt_cap );
	clipped_stroked.width( linewidth );

	clipped.clip_box( -linewidth, -linewidth, Width() + linewidth, Height() + linewidth );

	RasAA.add_path( clipped_stroked );
	RenderAA_BGRA_Pre.color( ColorToAggRgba(color) );

	agg::render_scanlines( RasAA, Scanline, RenderAA_BGRA_Pre );
}

agg::rgba xoCanvas::ColorToAggRgba( xoColor c )
{	
	return agg::rgba( c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f );
}
