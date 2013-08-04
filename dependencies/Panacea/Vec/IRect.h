#pragma once

namespace AbCore
{

template< typename TINT >
struct IRect
{
	IRect() {}
	IRect( TINT x1, TINT y1, TINT x2, TINT y2 ) { X1 = x1; Y1 = y1; X2 = x2; Y2 = y2; }

	void Reset()
	{
		X1 = Traits<TINT>::Max();
		Y1 = Traits<TINT>::Max();
		X2 = Traits<TINT>::Min();
		Y2 = Traits<TINT>::Min();
	}

	void Offset( TINT x, TINT y )
	{
		X1 += x;
		Y1 += y;
		X2 += x;
		Y2 += y;
	}

	bool operator==( const IRect& b ) const
	{
		return X1 == b.X1 && Y1 == b.Y1 && X2 == b.X2 && Y2 == b.Y2;
	}

	bool operator!=( const IRect& b ) const { return !(*this == b); }

	void ExpandToFit( TINT x, TINT y )
	{
		X1 = min( X1, x );
		Y1 = min( X1, y );
		X2 = max( X2, x );
		Y2 = max( Y2, y );
	}

	bool IsInsideOf( const IRect& b ) const
	{
		return	X1 >= b.X1 && Y1 >= b.Y1 &&
						X2 <= b.X2 && Y2 <= b.Y2;
	}

	bool PositiveUnion( const IRect& b ) const
	{
		return	(b.X2 >= X1 && b.X1 <= X2) &&
						(b.Y2 >= Y1 && b.Y1 <= Y2);
	}

	TINT X1, Y1, X2, Y2;
};

typedef IRect<INT32> IRect32;
typedef IRect<INT64> IRect64;

}
