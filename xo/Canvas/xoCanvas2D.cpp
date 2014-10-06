#include "pch.h"
#include "xoCanvas2D.h"
#include "../Image/xoImage.h"

xoCanvas2D::xoCanvas2D( xoImage* backingImage )
	: Image(backingImage)
{
	if ( Image != nullptr && Image->TexFormat == xoTexFormatRGBA8 )
	{
		RenderBuff.attach( (uint8*) Image->GetData(), Image->GetWidth(), Image->GetHeight(), Image->TexStride );
		PixFormatRGBA_Pre.attach( RenderBuff );
		RenderBaseRGBA_Pre.attach( PixFormatRGBA_Pre );
		RenderAA_RGBA_Pre.attach( RenderBaseRGBA_Pre );
		IsAlive = Image->GetData() != nullptr && Image->GetWidth() != 0 && Image->GetHeight() != 0;
	}
	else
	{
		IsAlive = false;
	}
}

xoCanvas2D::~xoCanvas2D()
{
	//Reset();
	//delete Image;
}

/*
bool xoCanvas2D::Resize( uint32 width, uint32 height )
{
	if ( width == Width() && height == Height() )
		return true;

	Reset();

	if ( Image == nullptr )
		Image = new xoImage();

	if ( !Image->Alloc( xoTexFormatRGBA8, width, height ) )
		return false;

	RenderBuff.attach( (uint8*) Image->GetData(), width, height, Image->TexStride );
	PixFormatBGRA_Pre.attach( RenderBuff );
	RenderBaseBGRA_Pre.attach( PixFormatBGRA_Pre );
	RenderAA_BGRA_Pre.attach( RenderBaseBGRA_Pre );

	return true;
}

bool xoCanvas2D::CloneInto( xoCanvas2D& clone )
{
	if ( clone.Width() != Width() || clone.Height() != Height() )
	{
		clone.Reset();
		if ( !clone.Resize( Width(), Height() ) )
			return false;
	}

	for ( uint i = 0; i < Height(); i++ )
		memcpy( clone.RowPtr(i), RowPtr(i), StrideAbs() );

	return true;
}

void xoCanvas2D::Reset()
{
	if ( Image != nullptr )
		Image->Free();
	RenderBuff.attach( nullptr, 0, 0, 0 );
}
*/

void xoCanvas2D::Fill( xoColor color )
{
	if ( !IsAlive )
		return;

	RenderBaseRGBA_Pre.copy_bar( 0, 0, Width(), Height(), ColorToAgg8(color) );
}

void xoCanvas2D::FillRect( xoBox box, xoColor color )
{
	if ( !IsAlive )
		return;

	RenderBaseRGBA_Pre.copy_bar( box.Left, box.Top, box.Right, box.Bottom, ColorToAgg8(color) );
}

void xoCanvas2D::StrokeLine( bool closed, int nvx, const float* vx, int vx_stride_bytes, xoColor color, float linewidth )
{
	if ( !IsAlive )
		return;

	RasAA.reset();
	RasAA.filling_rule( agg::fill_even_odd );

	agg::path_storage path;

	path.start_new_path();
	
	// emit first vertex
	path.move_to( vx[0], vx[1] );
	(char*&) vx += vx_stride_bytes;
	nvx--;

	// emit remaining vertices
	for ( int i = 0; i < nvx; i++, (char*&) vx += vx_stride_bytes )
		path.line_to( vx[0], vx[1] );
	
	if ( closed )
		path.close_polygon();

	TLineClipper						clipped_line( path );
	TFillClipper						clipped_fill( path );
	agg::conv_stroke<TLineClipper>		clipped_line_stroked( clipped_line );
	agg::conv_stroke<TFillClipper>		clipped_fill_stroked( clipped_fill );

	if ( closed )
	{
		clipped_fill.clip_box( -linewidth, -linewidth, Width() + linewidth, Height() + linewidth );
		clipped_fill_stroked.line_cap( agg::butt_cap );
		clipped_fill_stroked.line_join( agg::miter_join );
		clipped_fill_stroked.width( linewidth );
		RasAA.add_path( clipped_fill_stroked );
	}
	else
	{
		clipped_line.clip_box( -linewidth, -linewidth, Width() + linewidth, Height() + linewidth );
		clipped_line_stroked.line_cap( agg::butt_cap );
		clipped_line_stroked.line_join( agg::miter_join );
		clipped_line_stroked.width( linewidth );
		RasAA.add_path( clipped_line_stroked );
	}

	RenderAA_RGBA_Pre.color( ColorToAgg8(color) );

	agg::render_scanlines( RasAA, Scanline, RenderAA_RGBA_Pre );
}

agg::rgba xoCanvas2D::ColorToAgg( xoColor c )
{	
	return agg::rgba(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

agg::rgba8 xoCanvas2D::ColorToAgg8( xoColor c )
{	
	return agg::rgba8(c.r, c.g, c.b, c.a);
}
