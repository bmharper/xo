#pragma once

#include <algorithm> // for std::min/max
#include "vec2.h"
#include "vec3.h"

template <class FT>
class Bounds2T
{
public:
	union {
		struct 
		{
			Vec2T<FT> p1;
			Vec2T<FT> p2;
		};
		struct 
		{
			FT x1, y1;
			FT x2, y2;
		};
	};

	Bounds2T() 
	{
		Reset();
	}

	Bounds2T(const Vec2T<FT> &v1, const Vec2T<FT> &v2)
	{
		p1 = v1;
		p2 = v2;
	}

	Bounds2T(const FT x1, const FT y1, const FT x2, const FT y2)
	{
		p1 = Vec2T<FT>(x1,y1);
		p2 = Vec2T<FT>(x2,y2);
	}
	
	static Bounds2T Combine( const Bounds2T& a, const Bounds2T& b )
	{
		Bounds2T c = a;
		c.ExpandToFit( b );
		return c;
	}

	template< typename TVec >
	static Bounds2T BoundsOfLine( const TVec& a, const TVec& b )
	{
		Bounds2T me;
		me.ExpandToFit( a.x, a.y );
		me.ExpandToFit( b.x, b.y );
		return me;
	}

	static Bounds2T SquareAroundPoint( FT x, FT y, FT radius )
	{
		Bounds2T me( x - radius, y - radius, x + radius, y + radius );
		return me;
	}

	static Bounds2T AllEnclosing()
	{
		return Bounds2T( -AbCore::Traits<FT>::Max(), -AbCore::Traits<FT>::Max(), AbCore::Traits<FT>::Max(), AbCore::Traits<FT>::Max() );
	}

	bool IsAllEnclosing() const
	{
		return *this == AllEnclosing();
	}

	bool IsNull() const 
	{
		return (x2 < x1) || (y2 < y1);
	}

	bool IsNotNullAndPositiveArea() const
	{
		return (x2 > x1) && (y2 > y1);
	}

	bool AllFinite() const
	{
		return	AbCore::Traits<FT>::Finite(x1) &&
				AbCore::Traits<FT>::Finite(x2) &&
				AbCore::Traits<FT>::Finite(y1) &&
				AbCore::Traits<FT>::Finite(y2);
	}

	void Reset() 
	{
		p1.set( AbCore::Traits<FT>::Max(), AbCore::Traits<FT>::Max() );
		p2.set( -AbCore::Traits<FT>::Max(), -AbCore::Traits<FT>::Max() );
	}

	void Set(const Vec2T<FT> &v1, const Vec2T<FT> &v2)
	{
		p1 = v1;
		p2 = v2;
	}

	void Set(const FT x1, const FT y1, const FT x2, const FT y2)
	{
		p1 = Vec2T<FT>(x1,y1);
		p2 = Vec2T<FT>(x2,y2);
	}

	Vec2T<FT> Center() const 
	{
		return 0.5*(p1 + p2);
	}

	/** Returns one of the 4 corners of the box.
	@param i The index of the corner. The index is modded by 4 before yielding a result.
	The order of is (if the box is considered to lie in a regular cartesian system):
	- 0: Bottom Left
	- 1: Bottom Right
	- 2: Top Right
	- 3: Top Left
	**/
	Vec2T<FT> Corner( int i ) const
	{
		i = ((uint)i) & 3; // cause -1 to wrap around to 3
		switch ( i )
		{
		case 0: return Vec2T<FT>( x1, y1 );
		case 1: return Vec2T<FT>( x2, y1 );
		case 2: return Vec2T<FT>( x2, y2 );
		case 3: return Vec2T<FT>( x1, y2 );
		}
		// satisfy the compiler
		ASSERT(false);
		return Vec2T<FT>( x1, y1 );
	}

	void Corners( Vec2T<FT>* fourCorners ) const
	{
		fourCorners[0] = Corner(0);
		fourCorners[1] = Corner(1);
		fourCorners[2] = Corner(2);
		fourCorners[3] = Corner(3);
	}

	FT Width() const 
	{
		return p2.x - p1.x;
	}
	
	FT Height() const 
	{
		return p2.y - p1.y;
	}

	FT Area() const
	{
		return Width() * Height();
	}

	FT Aspect() const
	{
		return Width() / Height();
	}

	FT MaxDimension() const 
	{
		FT ww = Width();
		FT hh = Height();
		return std::max(ww, hh);
	}

	void Offset( FT x, FT y )
	{
		x1 += x;
		x2 += x;
		y1 += y;
		y2 += y;
	}

	Bounds2T OffsetBy( FT x, FT y ) const
	{
		Bounds2T b = *this;
		b.Offset( x, y );
		return b;
	}

	void Grow( FT w, FT h ) 
	{
		x1 -= w;
		x2 += w;
		y1 -= h;
		y2 += h;
	}

	/// Expands or shrinks by a positive scale factor. 1 is identity. 2 makes the rectangle twice as large. 0.5 makes it half the size.
	void ExpandBy( double factor )
	{
		ASSERT(factor >= 0);
		double h = 0.5 * (factor - 1) * (x2-x1);
		double y = 0.5 * (factor - 1) * (y2-y1);
		x1 -= h;
		x2 += h;
		y1 -= y;
		y2 += y;
	}

	/// Expands or shrinks around the center so that our width is w and our height is h.
	void SizeAroundCenter( FT w, FT h ) 
	{
		double cx = 0.5 * (x1 + x2);
		double cy = 0.5 * (y1 + y2);
		w = w * 0.5;
		h = h * 0.5;
		x1 = cx - w;
		x2 = cx + w;
		y1 = cy - h;
		y2 = cy + h;
	}

	// Flips x1,x2 if x2 < x1. Does the same for y1,y2
	void Normalize()
	{
		if ( x2 < x1 ) std::swap( x1, x2 );
		if ( y2 < y1 ) std::swap( y1, y2 );
	}

	void ExpandToFit( FT x, FT y )
	{
		p1.x = std::min(p1.x, x);
		p1.y = std::min(p1.y, y);
		p2.x = std::max(p2.x, x);
		p2.y = std::max(p2.y, y);
	}

	void ExpandToFit(const Vec2d &v) 
	{
		p1.x = std::min(p1.x, (FT) v.x);
		p1.y = std::min(p1.y, (FT) v.y);
		p2.x = std::max(p2.x, (FT) v.x);
		p2.y = std::max(p2.y, (FT) v.y);
	}

	void ExpandToFit(const Vec2f &v) 
	{
		p1.x = std::min(p1.x, (FT) v.x);
		p1.y = std::min(p1.y, (FT) v.y);
		p2.x = std::max(p2.x, (FT) v.x);
		p2.y = std::max(p2.y, (FT) v.y);
	}

	void ExpandToFit(const Vec3d &v) 
	{
		p1.x = std::min(p1.x, (FT) v.x);
		p1.y = std::min(p1.y, (FT) v.y);
		p2.x = std::max(p2.x, (FT) v.x);
		p2.y = std::max(p2.y, (FT) v.y);
	}

	void ExpandToFit(const Vec3f &v) 
	{
		p1.x = std::min(p1.x, (FT) v.x);
		p1.y = std::min(p1.y, (FT) v.y);
		p2.x = std::max(p2.x, (FT) v.x);
		p2.y = std::max(p2.y, (FT) v.y);
	}

	void ExpandToFit(const Bounds2T &b) 
	{
		p1.x = std::min(b.p1.x, p1.x);
		p1.y = std::min(b.p1.y, p1.y);
		p2.x = std::max(b.p2.x, p2.x);
		p2.y = std::max(b.p2.y, p2.y);
	}

	/// This is true if the boxes overlap or touch, within a given epsilon. A positive epsilon will cause nearby-but-not-touching boxes to yield a true result.
	bool PositiveUnionEps(const Bounds2T &b, FT epsilon) const
	{
		return	(b.x2 + epsilon >= x1 && b.x1 <= x2 + epsilon) &&
				(b.y2 + epsilon >= y1 && b.y1 <= y2 + epsilon);
	}

	/// This is true if the boxes overlap or touch.
	bool PositiveUnion(const Bounds2T &b) const
	{
		return	b.x2 >= x1 && b.x1 <= x2 &&
				b.y2 >= y1 && b.y1 <= y2;
	}

	/// This is true only if the boxes overlap. Merely touching is not enough.
	bool PositiveUnionOverlap(const Bounds2T &b) const
	{
		return	b.x2 > x1 && b.x1 < x2 &&
				b.y2 > y1 && b.y1 < y2;
	}

	/// Boxes must overlap in at least one dimension. They may touch in the other.
	bool PositiveUnionOverlapAnyDim(const Bounds2T &b) const
	{
		bool overlapX = (b.x2 > x1 && b.x1 < x2);
		bool overlapY = (b.y2 > y1 && b.y1 < y2);
		bool touchX = (b.x2 >= x1 && b.x1 <= x2);
		bool touchY = (b.y2 >= y1 && b.y1 <= y2);
		return (overlapX && touchY) || (overlapY && touchX);
	}

	/// Clip to another rectangle.
	void ClipTo( const Bounds2T& clipTo )
	{
		x1 = std::max(x1, clipTo.x1);
		y1 = std::max(y1, clipTo.y1);
		x2 = std::min(x2, clipTo.x2);
		y2 = std::min(y2, clipTo.y2);
	}

	bool IsInsideMe( const Bounds2T& b ) const
	{
		return	b.x1 >= x1 && b.y1 >= y1 &&
				b.x2 <= x2 && b.y2 <= y2;
	}

	bool IsInsideMe(const Vec2d& v) const
	{
		return	v.x >= p1.x && v.x <= p2.x &&
				v.y >= p1.y && v.y <= p2.y;
	}

	bool IsInsideMe(const Vec2f& v) const
	{
		return	v.x >= p1.x && v.x <= p2.x &&
				v.y >= p1.y && v.y <= p2.y;
	}

	bool IsInsideOf(const Bounds2T &b) const
	{
		return b.IsInsideMe( *this );
	}

	bool IsInsideOf2D(const Bounds2T &b) const
	{
		return b.IsInsideMe( *this );
	}

	bool operator==( const Bounds2T& b ) const
	{
		return p1 == b.p1 && p2 == b.p2;
	}

	bool operator!=( const Bounds2T& b ) const
	{
		return !(*this == b);
	}
};


typedef Bounds2T<double> Bounds2;
typedef Bounds2T<float> Bounds2f;
