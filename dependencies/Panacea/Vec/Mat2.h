#pragma once
#ifndef DEFINED_Mat2
#define DEFINED_Mat2

#include "Vec2.h"
#include "VecDef.h"

template <class FT>
class Mat2T
{
public:

	union
	{
		struct
		{
			FT	xx, xy,
				yx, yy;
		};

		struct 
		{
			VecBase2T<FT> row[2];
		};
	};

	Mat2T() {}

	void Identity()
	{
		row[0] = Vec2(1,0);
		row[1] = Vec2(0,1);
	}
	
	bool IsIdentity() const
	{
		Mat2T m;
		m.Identity();
		return *this == m;
	}

	bool Invertable() const
	{
		return Det() != 0;
	}

	/// Determinant
	double Det() const
	{
		return xx * yy - xy * yx;
	}

	bool Invert()
	{
		double r = Det();
		if ( r == 0 ) return false;
		r = 1.0 / r;
		Mat2T me;
		me.xx = r * yy;
		me.xy = r * -xy;
		me.yx = r * -yx;
		me.yy = r * xx;
		*this = me;
		return true;
	}

	Mat2T Inverted() const
	{
		Mat2T inv = *this;
		inv.Invert();
		return inv;
	}

	Mat2T& operator*=( double v )
	{
		row[0].scale(v);
		row[1].scale(v);
		return *this;
	}

	bool operator==( const Mat2T& b ) const
	{
		return memcmp(this, &b, sizeof(b)) == 0;
	}
	bool operator!=( const Mat2T& b ) const { return !(*this == b); }

};

template< typename FT >
Vec2T<FT> operator*( const Mat2T<FT>& m, const Vec2T<FT>& v )
{
	return Vec2T<FT>( m.row[0].dot(v), m.row[1].dot(v) );
}
		
typedef Mat2T<double> Mat2;
typedef Mat2T<double> Mat2d;
typedef Mat2T<float> Mat2f;

#include "VecUndef.h"
#endif // DEFINED_Mat2
