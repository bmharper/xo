#pragma once

// This is intended to be the cousin of template< typename TINT > struct TRectI

template< typename TINT >
struct TPointI
{
	TPointI() {}
	TPointI( TINT x, TINT y ) { X = x; Y = y; }

#if defined(_WIN32)
	TPointI( SIZE s )		{ X = s.cx; Y = s.cy; }
	operator SIZE() const { SIZE s = {X,Y}; return s; }
#endif

	bool operator==( const TPointI& b ) const { return X == b.X && Y == b.Y; }
	bool operator!=( const TPointI& b ) const { return !(*this == b); }

	static int32 GetHashCode_IntPair( int x, int y )
	{
		return AbCore::_rotl( y, 16 ) | x;
	}

	static int32 GetHashCode_IntPair( int16 x, int16 y )
	{
		return ((int32) y << 16) | x;
	}

	int32 GetHashCode() const
	{
		return GetHashCode_IntPair( X, Y );
	}

	TINT X, Y;
};

typedef TPointI<int32>	Point32;
typedef TPointI<uint32> Pointu32;
typedef TPointI<int64>	Point64;
typedef TPointI<uint64> Pointu64;

