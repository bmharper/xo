#pragma once

#include "../Other/lmDefs.h"
#include "Point2I.h"

// This is the old badness (the new thing is down below in this file)
#define RECT2I_DEFINED 1
class PAPI Rect2I
{
public:
	int x1, y1, x2, y2;

	Rect2I(void)
	{
		x1 = INTMAX;
		y1 = INTMAX;
		x2 = INTMIN;
		y2 = INTMIN;
	}

	Rect2I(int _x1, int _y1, int _x2, int _y2)
	{
		x1 = _x1;
		y1 = _y1;
		x2 = _x2;
		y2 = _y2;
	}

	void Set(int _x1, int _y1, int _x2, int _y2)
	{
		x1 = _x1;
		y1 = _y1;
		x2 = _x2;
		y2 = _y2;
	}

	void ExpandToFit( int x, int y )
	{
		x1 = std::min(x, x1);
		y1 = std::min(y, y1);
		x2 = std::max(x, x2);
		y2 = std::max(y, y2);
	}

	void ExpandToFit( const Rect2I& b )
	{
		x1 = std::min(x1, b.x1);
		y1 = std::min(y1, b.y1);
		x2 = std::max(x2, b.x2);
		y2 = std::max(y2, b.y2);
	}

	void ClipTo( const Rect2I& b )
	{
		x1 = std::max( x1, b.x1 );
		y1 = std::max( y1, b.y1 );
		x2 = std::min( x2, b.x2 );
		y2 = std::min( y2, b.y2 );
	}

	void Expand( int w, int h )
	{
		x1 -= w;
		x2 += w;
		y1 -= h;
		y2 += h;
	}

	void Reset()
	{
		x1 = INTMAX;
		y1 = INTMAX;
		x2 = INTMIN;
		y2 = INTMIN;
	}

	/// Returns true if the rectangle is inverted (y1 > y2 OR x1 > x2)
	bool IsNull() const
	{
		return y1 > y2 || x1 > x2;
	}

	bool IsNotNullAndPositiveArea() const
	{
		return (x2 > x1) && (y2 > y1);
	}

	double Aspect() const { return (x2 - x1) / (double) (y2 - y1); }
	int Width() const { return x2 - x1; }
	int Height() const { return y2 - y1; }

	INT64 Area() const { return (INT64) Width() * (INT64) Height(); }

	void Offset( int x, int y )
	{
		x1 += x;
		y1 += y;
		x2 += x;
		y2 += y;
	}

	bool operator==( const Rect2I& b ) const
	{
		return	x1 == b.x1 && x2 == b.x2 && 
						y1 == b.y1 && y2 == b.y2;
	}

	bool operator!=( const Rect2I& b ) const
	{
		return	x1 != b.x1 || x2 != b.x2 || 
						y1 != b.y1 || y2 != b.y2;
	}

	bool PositiveUnion( const Rect2I& b ) const
	{
		/*return (	(b.x1 < x2 || x1 < b.x2) &&
							(b.x2 > x1 || x1 > b.x1) &&
							(b.y1 < y2 || y1 < b.y2) &&
							(b.y2 > y1 || y1 > b.y1) ); */
		int width	= std::min(x2, b.x2) - std::max(x1, b.x1);
		int height	= std::min(y2, b.y2) - std::max(y1, b.y1);
		return width >= 0 && height >= 0;
	}

	// inclusive of borders
	bool IsInside( int x, int y ) const
	{
		return	x >= x1 && y >= y1 &&
						x <= x2 && y <= y2;
	}


};



// This is the new thing (prefer over Rect2I). It's just a matter of aesthetics, coding and naming style, etc.
template< typename TINT >
struct TRectI
{
	TINT X1, Y1, X2, Y2;

	TRectI() {}
	TRectI( TINT x1, TINT y1, TINT x2, TINT y2 ) { X1 = x1; Y1 = y1; X2 = x2; Y2 = y2; }

	TRectI( TPointI<TINT> p1, TPointI<TINT> p2 ) { X1 = p1.X; Y1 = p1.Y;  X2 = p2.X; Y2 = p2.Y; }

#if defined(RECT2I_DEFINED)
	TRectI( Rect2I r )			{ X1 = r.x1; Y1 = r.y1; X2 = r.x2; Y2 = r.y2; }
	operator Rect2I() const		{ return Rect2I(X1, Y1, X2, Y2); }
#endif

#if defined(_WIN32)
	TRectI( RECT r ) { X1 = r.left; Y1 = r.top; X2 = r.right; Y2 = r.bottom; }
	operator RECT() const { RECT r = {X1, Y1, X2, Y2}; return r; }
#endif

#if defined(GDIPLUS_H) || defined(_GDIPLUS_H)
	TRectI( Gdiplus::Rect r ) { X1 = r.X; Y1 = r.Y; X2 = r.X + r.Width; Y2 = r.Y + r.Height; }
	operator Gdiplus::Rect() const
	{
		return Gdiplus::Rect( (INT) X1, (INT) Y1, (INT) Width(), (INT) Height() );
	}
#endif

	static TRectI MakeNull()
	{
		TRectI r;
		r.Reset();
		return r;
	}

	static TRectI ByOriginAndSize( TINT x1, TINT y1, TINT width, TINT height ) { return TRectI(x1, y1, x1 + width, y1 + height); }

	void Offset( TINT x, TINT y )
	{
		X1 += x;
		Y1 += y;
		X2 += x;
		Y2 += y;
	}

	void OffsetToOrigin()
	{
		Offset( -X1, -Y1 );
	}

	TRectI AtOrigin() const
	{
		TRectI r = *this;
		r.OffsetToOrigin();
		return r;
	}

	TRectI AtOffset( TINT x, TINT y ) const
	{
		TRectI r = *this;
		r.Offset( x, y );
		return r;
	}

	void ShiftDown( unsigned int bits )
	{
		X1 = X1 >> bits;
		Y1 = Y1 >> bits;
		X2 = X2 >> bits;
		Y2 = Y2 >> bits;
	}

	void ShiftUp( unsigned int bits )
	{
		X1 = X1 << bits;
		Y1 = Y1 << bits;
		X2 = X2 << bits;
		Y2 = Y2 << bits;
	}

	void Expand( int x, int y )
	{
		X1 -= x;
		Y1 -= y;
		X2 += x;
		Y2 += y;
	}

	TRectI ExpandedBy( int x, int y ) const
	{
		TRectI r = *this;
		r.Expand( x, y );
		return r;
	}

	void Shrink( int x, int y )
	{
		X1 += x;
		Y1 += y;
		X2 -= x;
		Y2 -= y;
	}

	TRectI ShrunkBy( int x, int y ) const
	{
		TRectI r = *this;
		r.Shrink( x, y );
		return r;
	}

	bool operator==( const TRectI& b ) const
	{
		return X1 == b.X1 && Y1 == b.Y1 && X2 == b.X2 && Y2 == b.Y2;
	}

	bool operator!=( const TRectI& b ) const { return !(*this == b); }

	TINT Width() const { return X2 - X1; }
	TINT Height() const { return Y2 - Y1; }

	INT64 Area() const { return (INT64) Width() * (INT64) Height(); }

	void Reset()
	{
		X1 = AbCore::Traits<TINT>::Max();
		Y1 = AbCore::Traits<TINT>::Max();
		X2 = AbCore::Traits<TINT>::Min();
		Y2 = AbCore::Traits<TINT>::Min();
	}

	/// Returns true if X2 > X1 && Y2 > Y1
	bool IsNaturalNonEmpty() const
	{
		return X2 > X1 && Y2 > Y1;
	}

	/// Returns true if the rectangle is 100% equal to a rectangle that has just been Reset().
	bool IsNull() const
	{
		return *this == MakeNull();
	}

	bool IsNotNullAndPositiveArea() const
	{
		return (X2 > X1) && (Y2 > Y1);
	}

	void ExpandToFit( TINT x, TINT y )
	{
		X1 = std::min(X1, x);
		Y1 = std::min(Y1, y);
		X2 = std::max(X2, x);
		Y2 = std::max(Y2, y);
	}

	void ExpandToFit( const TRectI& b )
	{
		X1 = std::min(X1, b.X1);
		Y1 = std::min(Y1, b.Y1);
		X2 = std::max(X2, b.X2);
		Y2 = std::max(Y2, b.Y2);
	}

	bool IsInsideOf( const TRectI& b ) const
	{
		return	X1 >= b.X1 && Y1 >= b.Y1 &&
						X2 <= b.X2 && Y2 <= b.Y2;
	}

	// Inclusive of borders
	bool IsInsideMe( TINT x, TINT y ) const
	{
		return	x >= X1 && x <= X2 &&
						y >= Y1 && y <= Y2;
	}

	bool PositiveUnion( const TRectI &b ) const
	{
		return	(b.X2 >= X1 && b.X1 <= X2) &&
						(b.Y2 >= Y1 && b.Y1 <= Y2);
	}

	/// Clip to another rectangle. Do not record what we discarded.
	void ClipTo( const TRectI& clipTo )
	{
		X1 = std::max(X1, clipTo.X1);
		Y1 = std::max(Y1, clipTo.Y1);
		X2 = std::min(X2, clipTo.X2);
		Y2 = std::min(Y2, clipTo.Y2);
	}

	void SetWidthPreserveCenter( TINT w )
	{
		auto c = (X1 + X2) / 2;
		X1 = c - w / 2;
		X2 = c + w / 2;
	}

	void SetHeightPreserveCenter( TINT h )
	{
		auto c = (Y1 + Y2) / 2;
		Y1 = c - h / 2;
		Y2 = c + h / 2;
	}

	void SetSizePreserveCenter( TINT w, TINT h )
	{
		SetWidthPreserveCenter( w );
		SetHeightPreserveCenter( w );
	}

	TINT MinDimension() const { return std::min(X2 - X1, Y2 - Y1); }
	TINT MaxDimension() const { return std::max(X2 - X1, Y2 - Y1); }

};



/** Clip rectangles (typically for blit operations).

@param srcClipBox	The clipping box of the source. No pixels may be fetched outside of this box.
@param dstClipBox	The clipping box of the destination. No pixels may be drawn outside of this box.
@param rsrc			The source rectangle.
@param rdst			The destination rectangle.
@param srcLevel		For power-of-2 pyramidal coordinate space setups, this is the level at which rsrc exists. srcClipBox is at level 0.
@param dstLevel		For power-of-2 pyramidal coordinate space setups, this is the level at which rdst exists. dstClipBox is at level 0.

Concerning power-of-2 levels: Level 0 is the base level. Level 1 is half the resolution of that, etc. srcClipBox and dstClipBox are
defined in level 0.

**/
template< typename TRect >
bool ClipBlitRectangles( TRect srcClipBox, TRect dstClipBox, TRect& rsrc, TRect& rdst, int srcLevel = 0, int dstLevel = 0 )
{
	// clip top/left edges
	if ( rsrc.X1 < srcClipBox.X1 ) { rdst.X1 -= rsrc.X1 - srcClipBox.X1; rsrc.X1 = srcClipBox.X1; }
	if ( rsrc.Y1 < srcClipBox.Y1 ) { rdst.Y1 -= rsrc.Y1 - srcClipBox.Y1; rsrc.Y1 = srcClipBox.Y1; }
	if ( rdst.X1 < dstClipBox.X1 ) { rsrc.X1 -= rdst.X1 - dstClipBox.X1; rdst.X1 = dstClipBox.X1; }
	if ( rdst.Y1 < dstClipBox.Y1 ) { rsrc.Y1 -= rdst.Y1 - dstClipBox.Y1; rdst.Y1 = dstClipBox.Y1; }

	// clip bottom/right edges
	rsrc.X2 = std::min(rsrc.X2, srcClipBox.X2 >> srcLevel);
	rsrc.Y2 = std::min(rsrc.Y2, srcClipBox.Y2 >> srcLevel);
	rdst.X2 = std::min(rdst.X2, dstClipBox.X2 >> dstLevel);
	rdst.Y2 = std::min(rdst.Y2, dstClipBox.Y2 >> dstLevel);

	// make width and height the lower of src/dst
	if ( rsrc.Width() > rdst.Width() )			rsrc.X2 = rsrc.X1 + rdst.Width();
	else if ( rdst.Width() > rsrc.Width() )		rdst.X2 = rdst.X1 + rsrc.Width();

	if ( rsrc.Height() > rdst.Height() )		rsrc.Y2 = rsrc.Y1 + rdst.Height();
	else if ( rdst.Height() > rsrc.Height() )	rdst.Y2 = rdst.Y1 + rsrc.Height();

	if ( rsrc.Width() <= 0 || rsrc.Height() <= 0 || rdst.Width() <= 0 || rdst.Height() <= 0 ) return false;
	return true;
}


template< typename TINT, typename TRect >
bool ClipBlitRectangles( TINT srcWidth, TINT srcHeight, TINT dstWidth, TINT dstHeight, TRect& rsrc, TRect& rdst, int srcLevel = 0, int dstLevel = 0 )
{
	return ClipBlitRectangles( TRect(0,0,srcWidth,srcHeight), TRect(0,0,dstWidth,dstHeight), rsrc, rdst, srcLevel, dstLevel );
}

typedef TRectI<INT32> Rect32;
typedef TRectI<INT64> Rect64;
