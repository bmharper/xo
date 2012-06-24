#pragma once

#include "VecPrim.h"

#ifndef DEFINED_Vec2
#define DEFINED_Vec2
#include "VecDef.h"

/*
A note about our hideous namespace hack:
C++ is stupid in the sense that if one defines any non-trivial constructor, then
one no longer has a default constructor for that object. Consequently, you define
your default constructor as an empty thing. The ensuing chaos is that you can't
place this class inside a union!
So what we do is we make a static function called Vec2(double x, double y),
which returns a Vec2. In order to avoid name collisions, we need to place
the Vec class in it's own namespace.

MSVC doesn't mind trivial constructors in anonymous structs, so we can still constructors
there.
*/

namespace AbCoreVec
{

template <class vreal>
class Vec2Traits
{
public:
	static const TCHAR* StringFormat() { return _T("[ %g %g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%g %g"); }
	static const char* StringAFormatBare() { return "%g %g"; }
};

template <>
class Vec2Traits<float>
{
public:
	static const TCHAR* StringFormat() { return _T("[ %.6g %.6g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%.6g %.6g"); }
	static const char* StringAFormatBare() { return "%.6g %.6g"; }
};

template <>
class Vec2Traits<double>
{
public:
	static const TCHAR* StringFormat() { return _T("[ %.10g %.10g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%.10g %.10g"); }
	static const char* StringAFormatBare() { return "%.10g %.10g"; }
};

template <class vreal>
class Vec2T
{
public:

	static const int Dimensions = 2;
	typedef vreal FT;

	vreal x, y;

#ifdef _MSC_VER
	Vec2T() {}
	Vec2T( vreal _x, vreal _y ) { x = _x; y = _y; }
#endif

	/*Vec2T(const vreal *_v) 
	{
		x = _v[0];
		y = _v[1];
	}*/

	static Vec2T Create( vreal x, vreal y )
	{
		Vec2T v;
		v.x = x;
		v.y = y;
		return v;
	}

	void set(const vreal _x, const vreal _y) 
	{
		x = _x;
		y = _y;
	}
	/*void set(const vreal *_v) 
	{
		x = _v[0];
		y = _v[1];
	}*/

	/// Returns a vector that is [cos(angle), sin(angle)]
	static Vec2T AtAngle( vreal angle ) 
	{
		return Vec2T( cos(angle), sin(angle) );
	}

	// I don't know how to do this better.
	const Vec2T& AsVec2() const { return *this; }
				Vec2T& AsVec2()			{ return *this; }

	vreal operator&(const Vec2T &b) const {
		return x*b.x + y*b.y;
	}
	vreal dot(const Vec2T &b) const {
		return x*b.x + y*b.y;
	}

	Vec2T operator*(const vreal d) const {
		return Vec2T(x*d, y*d);
	}
	Vec2T operator*(const Vec2T &b) const {
		return Vec2T(x*b.x, y*b.y);
	}
	Vec2T operator+(const Vec2T &b) const {
		return Vec2T(x+b.x, y+b.y);
	}
	Vec2T operator-(const Vec2T &b) const {
		return Vec2T(x-b.x, y-b.y);
	}
	Vec2T operator/(const Vec2T &b) const {
		return Vec2T(x/b.x, y/b.y);
	}
	vreal sizeSquared() const {
		return x*x + y*y;
	}
	vreal size() const 
	{
		return sqrt(x*x + y*y);
	}
	
	vreal distance(const Vec2T &b) const	{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }
	vreal distance2d(const Vec2T &b) const	{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }
	vreal distance3d(const Vec2T &b) const	{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }
	vreal distanceSQ(const Vec2T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y); }

	vreal distance2dSQ(const Vec2T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y); }
	vreal distance3dSQ(const Vec2T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y); }

	Vec2T& operator-=(const Vec2T &b) 
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}
	Vec2T& operator+=(const Vec2T &b) 
	{
		x += b.x;
		y += b.y;
		return *this;
	}
	Vec2T& operator*=(const Vec2T &b) 
	{
		x *= b.x;
		y *= b.y;
		return *this;
	}
	Vec2T& operator/=(const Vec2T &b) 
	{
		x /= b.x;
		y /= b.y;
		return *this;
	}
	Vec2T& operator/=(const vreal d) 
	{
		double r = 1.0 / d;
		x *= r;
		y *= r;
		return *this;
	}
	Vec2T& operator*=(float s) 
	{
		x *= s;
		y *= s;
		return *this;
	}
	Vec2T& operator*=(double s) 
	{
		x *= s;
		y *= s;
		return *this;
	}

	void normalize()
	{
		double r = 1.0 / sqrt(x * x + y * y);
		x *= r;
		y *= r;
	}

	Vec2T normalized() const
	{
		Vec2T copy = *this;
		copy.normalize();
		return copy;
	}

	// makes sure all members are finite
	bool checkNaN() const
	{
		if ( !_finite(x) || !_finite(y) ) return false;
		return true;
	}

	/// Only valid for Vec2T<double>. Checks whether we won't overflow if converted to float.
	bool checkFloatOverflow() const
	{
		if (	x > FLT_MAX || x < -FLT_MAX ||
					y > FLT_MAX || y < -FLT_MAX ) return false;
		return true;
	}

	void copyTo( vreal *dst ) const
	{
		dst[0] = x;
		dst[1] = y;
	}

	// unary
	Vec2T operator-() const  {  return Vec2T(-x, -y);  }


	// comparision operators
	bool operator==(const Vec2T& v) const
	{
		return x == v.x && y == v.y;
	}
	bool operator!=(const Vec2T& v) const 
	{
		return x != v.x || y != v.y;
	}

	//friend bool operator < (const Vec2T& v1, const Vec2T& v2);
	//friend bool operator <= (const Vec2T& v1, const Vec2T& v2);
	//friend bool operator > (const Vec2T& v1, const Vec2T& v2);
	//friend bool operator >= (const Vec2T& v1, const Vec2T& v2);
	//friend Vec2T operator+(const Vec2T &a, const Vec2T &b);
	//friend Vec2T operator*(const float d, const Vec2T &v);
	//friend Vec2T operator*(const double d, const Vec2T &v);
	//friend Vec2T operator/(const Vec2T &v, const vreal d);
	//friend Vec2T operator/(const vreal d, const Vec2T &v);

	/// Returns the result of sprintf
	int ToStringABare( char* buff, size_t buffChars ) const
	{
		return sprintf_s( buff, buffChars, Vec2Traits<vreal>::StringAFormatBare(), x, y );
	}

	#ifndef NO_XSTRING
		/// Writes "[ %g %g ]"
		XString ToString() const
		{
			XString s;
			s.Format( Vec2Traits<vreal>::StringFormat(), x, y );
			return s;
		}

		/// Writes "%g %g"
		XString ToStringBare() const
		{
			XString s;
			s.Format( Vec2Traits<vreal>::StringFormatBare(), x, y );
			return s;
		}

		/// Writes "[ %g %g ]" or "%g %g"
		XString ToString( int significant_digits, bool bare = false ) const
		{
			XString f, s;
			if ( bare ) f.Format( _T("%%.%dg %%.%dg"), significant_digits, significant_digits );
			else		f.Format( _T("[ %%.%dg %%.%dg ]"), significant_digits, significant_digits );
			s.Format( f, x, y );
			return s;
		}

		/// Parses "[ x y ]", "x y", "x,y"
		bool Parse( const XString& str )
		{
			double a, b;
#ifdef LM_VS2005_SECURE
			if ( _stscanf_s( str, _T("[ %lf %lf ]"), &a, &b ) != 2 )
			{
				if ( _stscanf_s( str, _T("%lf %lf"), &a, &b ) != 2 )
				{
					if ( _stscanf_s( str, _T("%lf, %lf"), &a, &b ) != 2 )
					{
						return false;
					}
				}
			}
#else
			if ( _stscanf( str, _T("[ %lf %lf ]"), &a, &b ) != 2 )
			{
				if ( _stscanf( str, _T("%lf %lf"), &a, &b ) != 2 )
				{
					if ( _stscanf( str, _T("%lf, %lf"), &a, &b ) != 2 )
					{
						return false;
					}
				}
			}
#endif
			x = (vreal) a;
			y = (vreal) b;
			
			return true;
		}

		static Vec2T FromString( const XString& str )
		{
			Vec2T v;
			v.Parse( str );
			return v;
		}

	#endif 

};

template <class vreal> INLINE bool
operator <= (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
		return v1.x <= v2.x && v1.y <= v2.y;
}

template <class vreal> INLINE bool
operator < (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
		return v1.x < v2.x && v1.y < v2.y;
}


template <class vreal> INLINE bool
operator > (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
		return v1.x > v2.x && v1.y > v2.y;
}

template <class vreal> INLINE bool
operator >= (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
		return v1.x >= v2.x && v1.y >= v2.y;
}

template <class vreal> INLINE Vec2T<vreal>
operator*(const float d, const Vec2T<vreal> &v) 
{
	return Vec2T<vreal>(v.x*d, v.y*d);
}

template <class vreal> INLINE Vec2T<vreal>
operator*(const double d, const Vec2T<vreal> &v) 
{
	return Vec2T<vreal>(v.x*d, v.y*d);
}

template <class vreal> INLINE Vec2T<vreal>
operator/(const Vec2T<vreal> &v, const vreal d)
{
	vreal rec = 1.0 / d;
	return Vec2T<vreal>(v.x * rec, v.y * rec);
}

template <class vreal> INLINE Vec2T<vreal>
operator/(const vreal d, const Vec2T<vreal> &v)
{
	return Vec2T<vreal>( d / v.x, d / v.y );
}

template <class vreal> INLINE Vec2T<vreal>
operator/(const Vec2T<vreal> &a, const Vec2T<vreal> &b)
{
	return Vec2T<vreal>( a.x / b.x, a.y / b.y );
}

template <class vreal> INLINE vreal
dot (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
		return v1.x*v2.x + v1.y*v2.y;
}

/*template <class vreal> INLINE Vec2T<vreal> operator+(const Vec2T<vreal> &a, const Vec2T<vreal> &b)
{
	return Vec2T<vreal>(a.x + b.x, a.y + b.y);
}*/

template <class vreal> INLINE Vec2T<vreal>
normalize( const Vec2T<vreal>& a )
{
	return a.normalized();
}

template <class vreal> INLINE vreal
length( const Vec2T<vreal>& a )
{
	return a.size();
}

template <class vreal> INLINE vreal
lengthSQ( const Vec2T<vreal>& a )
{
	return a.sizeSquared();
}

typedef Vec2T< double > Vec2d;
typedef Vec2T< float > Vec2f;
typedef Vec2d Vec2;

} // namespace AbCoreVec

// pollution.
using namespace AbCoreVec;

#ifdef DVECT_DEFINED
typedef dvect< Vec2f > Vec2fVect;
typedef dvect< Vec2d > Vec2dVect;
#endif

inline Vec2		ToVec2( vec2 v )  { return Vec2::Create( v.x, v.y ); }
inline Vec2		ToVec2( Vec2f v ) { return Vec2::Create( v.x, v.y ); }
inline Vec2f	ToVec2f( Vec2 v ) { return Vec2f::Create( (float) v.x, (float) v.y ); }

#include "VecUndef.h"
#endif // DEFINED_Vec2
