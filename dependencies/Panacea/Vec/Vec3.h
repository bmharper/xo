#pragma once

#include "Vec2.h"

// See Vec2.h for a readme of what we are doing here
#ifndef DEFINED_Vec3
#define DEFINED_Vec3
#include "VecDef.h"

template <typename vreal>
class Vec3Traits
{
public:
	static const TCHAR* StringFormat() { return _T("[ %g %g %g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%g %g %g"); }
	static const char* StringAFormatBare() { return "%g %g %g"; }
};

template <>
class Vec3Traits<float>
{
public:
	static const TCHAR* StringFormat() { return _T("[ %.6g %.6g %.6g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%.6g %.6g %.6g"); }
	static const char* StringAFormatBare() { return "%.6g %.6g %.6g"; }
};

template <>
class Vec3Traits<double>
{
public:
	static const TCHAR* StringFormat() { return _T("[ %.10g %.10g %.10g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%.10g %.10g %.10g"); }
	static const char* StringAFormatBare() { return "%.10g %.10g %.10g"; }
};

// The "Base" class has no constructor, so that it can be included inside a union
// Inside the base class, we do not expose any functions that leak our type
// For example, we cannot expose component-wise multiply, because that would
// leak VecBase3T to the outside world.
template <typename vreal>
class VecBase3T
{
public:
	static const int Dimensions = 3;
	typedef vreal FT;

	union 
	{
		struct 
		{
			vreal x,y,z;
		};
		struct 
		{
			vreal v[3];
		};
		struct
		{
			VecBase2T<vreal> vec2;
		};
		struct 
		{
			vreal n[3];
		};
	};

	static VecBase3T create(vreal _x, vreal _y, vreal _z) { VecBase3T b = {_x,_y,_z}; return b; }

	void set(const vreal _x, const vreal _y, const vreal _z) {		x = _x;		y = _y;		z = _z;	}

	// fills all with _uniform
	void set( const vreal _uniform ) 	{	x = _uniform;	y = _uniform;	z = _uniform;	}

	void scale(vreal _scale)
	{
		x *= _scale;
		y *= _scale;
		z *= _scale;
	}

	vreal size() const								{ return sqrt(x*x + y*y + z*z); }
	vreal sizeSquared() const						{ return x*x + y*y + z*z; }
	vreal size2Squared() const						{ return x*x + y*y; }
	vreal distance(const VecBase3T &b) const		{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z)); }
	vreal distance3d(const VecBase3T &b) const		{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z)); }
	vreal distance2d(const VecBase3T &b) const		{ return sqrt((b.x-x)*(b.x-x) + (b.y-y)*(b.y-y)); }
	vreal distance3dSQ(const VecBase3T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z); }
	vreal distanceSQ(const VecBase3T &b) const		{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z); }
	vreal distance2dSQ(const VecBase3T &b) const	{ return (b.x-x)*(b.x-x) + (b.y-y)*(b.y-y); }

	vreal	operator[](int i) const { return v[i]; }
	vreal&	operator[](int i)		{ return v[i]; }

	vreal	operator()(int i) const { return v[i]; }
	vreal&	operator()(int i)		{ return v[i]; }

	void cross(const VecBase3T &u, const VecBase3T &v)
	{
		x = u.y * v.z - u.z * v.y;
		y = u.z * v.x - u.x * v.z;
		z = u.x * v.y - u.y * v.x;
	}

	/// Clamps values individually
	void clamp( vreal vmin, vreal vmax )
	{
		x = CLAMP( x, vmin, vmax );
		y = CLAMP( y, vmin, vmax );
		z = CLAMP( z, vmin, vmax );
	}

	// checks that all members are not infinite or NaNs
	bool checkNaN() const
	{
		if ( _isnan(x) || _isnan(y) || _isnan(z) ) return false;
		return true;
	}

	/// Returns true if any member is a NaN.
	bool IsNan() const
	{
		return _isnan(x) || _isnan(y) || _isnan(z);
	}

	/// Only valid for VecBase3T<double>. Checks whether we won't overflow if converted to float.
	bool checkFloatOverflow() const
	{
		if (	x > FLT_MAX || x < -FLT_MAX ||
				y > FLT_MAX || y < -FLT_MAX ||
				z > FLT_MAX || z < -FLT_MAX ) return false;
		return true;
	}

	// sets all values to FLT_MIN
	void setNull()
	{
		x = FLT_MIN;
		y = FLT_MIN;
		z = FLT_MIN;
	}

	bool isNull() const
	{
		return x == FLT_MIN && y == FLT_MIN && z == FLT_MIN;
	}

	void normalize() 
	{
		vreal s = (vreal) 1.0 / sqrt(sizeSquared());
		x *= s;
		y *= s;
		z *= s;
	}

	void copyTo( vreal *dst ) const
	{
		dst[0] = x;
		dst[1] = y;
		dst[2] = z;
	}

	// only normalizes if size is not zero. Returns 0 if size() > 0
	// Sets vector to (1,0,0) if size is 0, and returns -1.
	int normalizeCheck() 
	{
		vreal s = sizeSquared();
		if (s == 0) 
		{
			x = 1;
			y = 0;
			z = 0;
			return -1;
		}
		s = (vreal) 1.0 / sqrt(s);
		x *= s;
		y *= s;
		z *= s;
		return 0;
	}

	VecBase3T operator-() const { return VecBase3T::create(-x, -y, -z); }

	// comparison operators
	bool operator==(const VecBase3T& v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=(const VecBase3T& v) const { return x != v.x || y != v.y || z != v.z; }

	vreal dot(const VecBase3T& b) const { return x * b.x + y * b.y + z * b.z; }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Duplicated inside Vec3T
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VecBase3T& operator+=(const VecBase3T &b)	{ x += b.x; y += b.y; z += b.z;						return *this; }
	VecBase3T& operator-=(const VecBase3T &b)	{ x -= b.x; y -= b.y; z -= b.z;						return *this; }
	VecBase3T& operator*=(const VecBase3T &b)	{ x *= b.x; y *= b.y; z *= b.z;						return *this; }
	VecBase3T& operator*=(const vreal d)		{ 						x *= d;	y *= d; z *= d;		return *this; }
	VecBase3T& operator/=(const vreal d)		{ vreal r = 1.0 / d;	x *= r; y *= r; z *= r;		return *this; }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

};

template <typename vreal>
class Vec3T : public VecBase3T<vreal>
{
public:
	typedef vreal FT;
	using VecBase3T<vreal>::x;
	using VecBase3T<vreal>::y;
	using VecBase3T<vreal>::z;
	using VecBase3T<vreal>::v;
	using VecBase3T<vreal>::vec2;
	using VecBase3T<vreal>::n;
	using VecBase3T<vreal>::normalizeCheck;

	Vec3T()													{}
	Vec3T( vreal _x, vreal _y, vreal _z )					{ x = _x; y = _y; z = _z; }
	Vec3T( const VecBase3T<vreal>& b )						{ x = b.x; y = b.y; z = b.z; }
	Vec3T( const VecBase2T<vreal>& v2, const vreal _z )		{ x = v2.x, y = v2.y, z = _z; }
	explicit Vec3T( vreal _uniform )						{ x = y = z = _uniform; }

	static Vec3T Create( vreal x, vreal y, vreal z )
	{
		Vec3T v;
		v.x = x;
		v.y = y;
		v.z = z;
		return v;
	}

	static Vec3T null()
	{
		Vec3T v;
		v.setNull();
		return v;
	}

	/// Returns a vector that is [cos(angle), sin(angle), 0]
	static Vec3T AtAngle( vreal angle ) 
	{
		return Vec3T::Create( cos(angle), sin(angle), 0 );
	}

	const Vec2T<vreal>& AsVec2() const    	{ return (const Vec2T<vreal>&) vec2; }
	      Vec2T<vreal>& AsVec2()          	{ return (Vec2T<vreal>&) vec2; }
	const Vec3T<vreal>& AsVec3() const		{ return *this; }
	      Vec3T<vreal>& AsVec3()			{ return *this; }

	Vec3T operator-() const { return Vec3T(-x, -y, -z); }

	Vec3T projectionOn3dLine(const VecBase3T<vreal> &p1, const VecBase3T<vreal> &p2) const
	{
		double u = ((*this - p1) & (p2 - p1)) / (p2 - p1).sizeSquared();
		return p1 + u*(p2-p1);
	}

	Vec3T normalized() const
	{
		Vec3T copy = *this;
		copy.normalize();
		return copy;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Duplicated inside VecBase3T
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Vec3T& operator+=(const Vec3T &b)	{ x += b.x; y += b.y; z += b.z;						return *this; }
	Vec3T& operator-=(const Vec3T &b)	{ x -= b.x; y -= b.y; z -= b.z;						return *this; }
	Vec3T& operator*=(const Vec3T &b)	{ x *= b.x; y *= b.y; z *= b.z;						return *this; }
	Vec3T& operator*=(const vreal d)	{ 						x *= d;	y *= d; z *= d;		return *this; }
	Vec3T& operator/=(const vreal d)	{ vreal r = 1.0 / d;	x *= r; y *= r; z *= r;		return *this; }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// some swizzlers... i doubt these are used much
	Vec2T<vreal>	xx() const		{ return Vec2T<vreal>(x, x); }
	Vec2T<vreal>	yy() const		{ return Vec2T<vreal>(y, y); }
	Vec2T<vreal>	xy() const		{ return Vec2T<vreal>(x, y); }
	Vec2T<vreal>	yx() const		{ return Vec2T<vreal>(y, x); }
	Vec3T			xxx() const		{ return Vec3T::Create(x, x, x); }
	Vec3T			yyy() const		{ return Vec3T::Create(y, y, y); }
	Vec3T			zzz() const		{ return Vec3T::Create(z, z, z); }
	Vec3T			zyx() const		{ return Vec3T::Create(z, y, x); }

	Vec3T        operator^(const VecBase3T<vreal>& v) const;
	int MakeOrthonormalBasis(VecBase3T<vreal> & base1, VecBase3T<vreal> & base2);

	int ToStringABare( char* buff, size_t buffChars ) const
	{
		return sprintf_s( buff, buffChars, Vec3Traits<vreal>::StringAFormatBare(), x, y, z );
	}

	#ifndef NO_XSTRING
		/// Writes "[ %.8g %.8g %.8g ]"
		XString ToString() const
		{
			XString s;
			s.Format( Vec3Traits<vreal>::StringFormat(), x, y, z );
			return s;
		}

		/// Writes "%lf %lf %lf"
		XString ToStringBare() const
		{
			XString s;
			s.Format( Vec3Traits<vreal>::StringFormatBare(), x, y, z );
			return s;
		}

		XString ToString( int significant_digits, bool bare = false ) const
		{
			XString f, s;
			if ( bare ) f.Format( _T("%%.%dg %%.%dg %%.%dg"), significant_digits, significant_digits, significant_digits );
			else		f.Format( _T("[ %%.%dg %%.%dg %%.%dg ]"), significant_digits, significant_digits, significant_digits );
			s.Format( f, x, y, z );
			return s;
		}

		/// Parses "[ x y z ]", "x y z", "x,y,z"
		bool Parse( const XString& str )
		{
			double a, b, c;
#ifdef LM_VS2005_SECURE
			if ( _stscanf_s( str, _T("[ %lf %lf %lf ]"), &a, &b, &c ) != 3 )
			{
				if ( _stscanf_s( str, _T("%lf %lf %lf"), &a, &b, &c ) != 3 )
				{
					if ( _stscanf_s( str, _T("%lf, %lf, %lf"), &a, &b, &c ) != 3 )
					{
						return false;
					}
				}
			}
#else
			if ( _stscanf( str, _T("[ %lf %lf %lf ]"), &a, &b, &c ) != 3 )
			{
				if ( _stscanf( str, _T("%lf %lf %lf"), &a, &b, &c ) != 3 )
				{
					if ( _stscanf( str, _T("%lf, %lf, %lf"), &a, &b, &c ) != 3 )
					{
						return false;
					}
				}
			}
#endif
			x = (vreal) a;
			y = (vreal) b;
			z = (vreal) c;
			
			return true;
		}

		static Vec3T FromString( const XString& str )
		{
			Vec3T v;
			v.Parse( str );
			return v;
		}

	#endif 

};


template <class vreal> INLINE bool
operator < (const VecBase3T<vreal>& v1, const VecBase3T<vreal>& v2)
{
		return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z;
}

template <class vreal> INLINE bool
operator <= (const VecBase3T<vreal>& v1, const VecBase3T<vreal>& v2)
{
		return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z;
}


template <class vreal> INLINE bool
operator > (const VecBase3T<vreal>& v1, const VecBase3T<vreal>& v2)
{
		return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z;
}

template <class vreal> INLINE bool
operator >= (const VecBase3T<vreal>& v1, const VecBase3T<vreal>& v2)
{
		return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z;
}

template<typename vreal> inline Vec3T<vreal> operator*(vreal s, const VecBase3T<vreal> &v)							{ return Vec3T<vreal>(v.x * s, v.y * s, v.z * s); }
template<typename vreal> inline Vec3T<vreal> operator*(const VecBase3T<vreal> &v, vreal s)							{ return Vec3T<vreal>(v.x * s, v.y * s, v.z * s); }
template<typename vreal> inline Vec3T<vreal> operator*(const VecBase3T<vreal> &a, const VecBase3T<vreal> &b)		{ return Vec3T<vreal>(a.x * b.x, a.y * b.y, a.z * b.z); }
template<typename vreal> inline Vec3T<vreal> operator/(const VecBase3T<vreal> &a, const VecBase3T<vreal> &b)		{ return Vec3T<vreal>(a.x / b.x, a.y / b.y, a.z / b.z); }
template<typename vreal> inline Vec3T<vreal> operator/(const VecBase3T<vreal> &a, vreal s)							{ vreal r = (vreal) 1.0 / s; return Vec3T<vreal>(a.x * r, a.y * r, a.z * r); }
template<typename vreal> inline Vec3T<vreal> operator+(const VecBase3T<vreal>& a, const VecBase3T<vreal>& b)		{ return Vec3T<vreal>(a.x + b.x, a.y + b.y, a.z + b.z ); }
template<typename vreal> inline Vec3T<vreal> operator-(const VecBase3T<vreal>& a, const VecBase3T<vreal>& b)		{ return Vec3T<vreal>(a.x - b.x, a.y - b.y, a.z - b.z ); }
template<typename vreal> inline vreal        dot(const VecBase3T<vreal>& a, const VecBase3T<vreal>& b)				{ return a.x * b.x + a.y * b.y + a.z * b.z; }
template<typename vreal> inline vreal        operator&(const VecBase3T<vreal>& a, const VecBase3T<vreal>& b)		{ return dot(a,b); }
template<typename vreal> inline Vec3T<vreal> normalize(const VecBase3T<vreal>& a)									{ Vec3T<vreal> copy = a; copy.normalize(); return copy; }
template<typename vreal> inline vreal        length(const VecBase3T<vreal>& a) 										{ return a.size(); }
template<typename vreal> inline vreal        lengthSQ(const VecBase3T<vreal>& a) 									{ return a.sizeSquared(); }

// cross
template<typename vreal> inline
Vec3T<vreal> Vec3T<vreal>::operator^(const VecBase3T<vreal>& v) const
{
		return Vec3T<vreal>::Create( n[1]*v[2] - v[1]*n[2],
									-n[0]*v[2] + v[0]*n[2],
									 n[0]*v[1] - v[0]*n[1] );
}


template <class vreal>
INLINE int Vec3T<vreal>::MakeOrthonormalBasis(VecBase3T<vreal> & base1, VecBase3T<vreal> & base2)
{

	if (base1.normalizeCheck())
		return -1;

	if (base2.normalizeCheck())
		return -1;

	*this = base1 ^ base2;

	// should be already normalized
	if (normalizeCheck())
		return -1;

	base1 = base2 ^ *this;

	// this too, should be already normalized
	if (base1.normalizeCheck())
		return -1;

	return 0;
}

typedef Vec3T<double> Vec3d;
typedef Vec3T<float> Vec3f;
typedef Vec3d Vec3;

#ifdef OHASH_DEFINED
namespace ohash
{
	template< class vreal >
	class ohashfunc_Vec3T
	{
	public:
		static ohash::hashkey_t gethashcode( const VecBase3T<vreal>& elem )
		{
			vreal sum = elem.x + elem.y + elem.z;
			ohash::hashkey_t* hard = (ohash::hashkey_t*) &sum;
			return *hard;
		}
	};

	typedef ohashfunc_Vec3T<double> ohashfunc_Vec3;
	typedef ohashfunc_Vec3T<double> ohashfunc_Vec3d;
	typedef ohashfunc_Vec3T<float> ohashfunc_Vec3f;
}
#endif

inline Vec3		ToVec3( Vec3f v ) { return Vec3( v.x, v.y, v.z ); }
inline Vec3f	ToVec3f( Vec3 v ) { return Vec3f( (float) v.x, (float) v.y, (float) v.z ); }

#include "VecUndef.h"
#endif // DEFINED_Vec3

