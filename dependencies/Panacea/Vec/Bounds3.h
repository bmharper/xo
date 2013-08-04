#pragma once

#include "vec3.h"
#include "Bounds2.h"

template <class FT>
class Bounds3T
{
public:
	union {
		struct 
		{
			Vec3T<FT> p1;
			Vec3T<FT> p2;
		};
		struct 
		{
			FT x1, y1, z1;
			FT x2, y2, z2;
		};
	};

	Bounds3T() 
	{
		Reset();
	}

	Bounds3T(const Vec3T<FT> &v1, const Vec3T<FT> &v2)
	{
		p1 = v1;
		p2 = v2;
	}

	Bounds3T( FT x1, FT y1, FT z1, FT x2, FT y2, FT z2 )
	{
		this->x1 = x1;
		this->y1 = y1;
		this->z1 = z1;
		this->x2 = x2;
		this->y2 = y2;
		this->z2 = z2;
	}
	
	explicit Bounds3T( const Bounds2T<FT>& b )
	{
		x1 = b.x1;
		y1 = b.y1;
		z1 = -AbCore::Traits<FT>::Max();
		x2 = b.x2;
		y2 = b.y2;
		z2 = AbCore::Traits<FT>::Max();
	}

	Bounds2T<FT> ToBounds2() const
	{
		return Bounds2T<FT>( x1, y1, x2, y2 );
	}

	void Reset() 
	{
		p1.set( AbCore::Traits<FT>::Max(), AbCore::Traits<FT>::Max(), AbCore::Traits<FT>::Max() );
		p2.set( -AbCore::Traits<FT>::Max(), -AbCore::Traits<FT>::Max(), -AbCore::Traits<FT>::Max() );
	}
	
	bool IsNull() const 
	{
		return (x2 < x1) || (y2 < y1) || (z2 < z1);
	}

	bool IsNotNullAndPositiveVolume() const
	{
		return (x2 > x1) && (y2 > y1) && (z2 > z1);
	}

	void Set(const Vec3T<FT> &v1, const Vec3T<FT> &v2)
	{
		p1 = v1;
		p2 = v2;
	}

	Vec3T<FT> Center() const 
	{
		return 0.5*(p1 + p2);
	}

	FT Width() const 
	{
		return p2.x - p1.x;
	}

	FT Height() const 
	{
		return p2.y - p1.y;
	}

	FT Depth() const 
	{
		return p2.z - p1.z;
	}

	FT MaxDimension() const 
	{
		FT ww = Width();
		FT hh = Height();
		FT dd = Depth();
		return max(max(ww, hh), max(ww, dd));
	}

	FT MaxDimension2D() const 
	{
		FT ww = Width();
		FT hh = Height();
		return max(ww, hh);
	}

	void Offset( FT x, FT y, FT z )
	{
		x1 += x;
		x2 += x;
		y1 += y;
		y2 += y;
		z1 += z;
		z2 += z;
	}

	void Grow( FT w, FT h, FT d ) 
	{
		x1 -= w;
		x2 += w;
		y1 -= h;
		y2 += h;
		z1 -= d;
		z2 += d;
	}

	void Expand( FT factor )
	{
		factor = (factor - 1) * 0.5;
		FT u = p2.x - p1.x;
		FT v = p2.y - p1.y;
		FT w = p2.z - p1.z;
		p1.x -= u * factor;
		p2.x += u * factor;
		p1.y -= v * factor;
		p2.y += v * factor;
		p1.z -= w * factor;
		p2.z += w * factor;
	}

	void ExpandToFit( FT x, FT y, FT z )
	{
		p1.x = std::min(p1.x, x);
		p1.y = std::min(p1.y, y);
		p1.z = std::min(p1.z, z);
		p2.x = std::max(p2.x, x);
		p2.y = std::max(p2.y, y);
		p2.z = std::max(p2.z, z);
	}

	void ExpandToFit( const Vec3f &v ) 
	{
		p1.x = std::min(p1.x, (FT) v.x);
		p1.y = std::min(p1.y, (FT) v.y);
		p1.z = std::min(p1.z, (FT) v.z);
		p2.x = std::max(p2.x, (FT) v.x);
		p2.y = std::max(p2.y, (FT) v.y);
		p2.z = std::max(p2.z, (FT) v.z);
	}

	void ExpandToFit( const Vec3d &v ) 
	{
		p1.x = std::min(p1.x, (FT) v.x);
		p1.y = std::min(p1.y, (FT) v.y);
		p1.z = std::min(p1.z, (FT) v.z);
		p2.x = std::max(p2.x, (FT) v.x);
		p2.y = std::max(p2.y, (FT) v.y);
		p2.z = std::max(p2.z, (FT) v.z);
	}

	void ExpandToFit(const Bounds3T &b) 
	{
		ExpandToFitX(b);
		ExpandToFitY(b);
		ExpandToFitZ(b);
	}

	void ExpandToFitX(const Bounds3T &b) 
	{
		p1.x = std::min(b.p1.x, p1.x);
		p2.x = std::max(b.p2.x, p2.x);
	}

	void ExpandToFitY(const Bounds3T &b) 
	{
		p1.y = std::min(b.p1.y, p1.y);
		p2.y = std::max(b.p2.y, p2.y);
	}

	void ExpandToFitZ(const Bounds3T &b) 
	{
		p1.z = std::min(b.p1.z, p1.z);
		p2.z = std::max(b.p2.z, p2.z);
	}


	bool PositiveUnion(const Bounds3T &b) const
	{
		return	(b.x2 >= x1 && b.x1 <= x2) &&
						(b.y2 >= y1 && b.y1 <= y2) &&
						(b.z2 >= z1 && b.z1 <= z2);
	}

	template< typename TBounds >
	bool PositiveUnion2D( const TBounds& b ) const
	{
		return	(b.x2 >= x1 && b.x1 <= x2) &&
						(b.y2 >= y1 && b.y1 <= y2);
	}

	bool IsInsideMe(const Vec3T<FT>& v) const
	{
		return v >= p1 && v <= p2;
	}

	bool IsInsideOf( const Bounds3T &b ) const
	{
		return p1 >= b.p1 && p2 <= b.p2;
	}

	template< typename TBounds >
	bool IsInsideOf2D( const TBounds &b ) const
	{
		return	x1 >= b.x1 && y1 >= b.y1 &&
						x2 <= b.x2 && y2 <= b.y2;
	}

	bool operator==( const Bounds3T& b ) const { return memcmp(this, &b, sizeof(*this)) == 0; }
	bool operator!=( const Bounds3T& b ) const { return !(*this == b); }

	Bounds3T operator+( const Vec3& v ) const
	{
		return Bounds3T(	x1 + v.x, y1 + v.y, z1 + v.z, 
											x2 + v.x, y2 + v.y, z2 + v.z );
	}

	Bounds3T operator-( const Vec3& v ) const
	{
		return Bounds3T(	x1 - v.x, y1 - v.y, z1 - v.z, 
											x2 - v.x, y2 - v.y, z2 - v.z );
	}

	Bounds3T operator+( const Vec3f& v ) const
	{
		return Bounds3T(	x1 + v.x, y1 + v.y, z1 + v.z, 
											x2 + v.x, y2 + v.y, z2 + v.z );
	}

	Bounds3T operator-( const Vec3f& v ) const
	{
		return Bounds3T(	x1 - v.x, y1 - v.y, z1 - v.z, 
											x2 - v.x, y2 - v.y, z2 - v.z );
	}

};

typedef Bounds3T<double> Bounds3;
typedef Bounds3T<float> Bounds3f;
