#pragma once

#include "Vec3.h"

// See Vec2.h for a readme of what we are doing here
#ifndef DEFINED_Vec4
#define DEFINED_Vec4
#include "VecDef.h"

namespace AbCoreVec
{

template <class vreal>
class Vec4Traits
{
public:
	static const TCHAR* StringFormat() { return _T("[ %g %g %g %g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%g %g %g %g"); }
	static const char* StringAFormatBare() { return "%g %g %g %g"; }
};

template <>
class Vec4Traits<float>
{
public:
	static const TCHAR* StringFormat() { return _T("[ %.6g %.6g %.6g %.6g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%.6g %.6g %.6g %.6g"); }
	static const char* StringAFormatBare() { return "%.6g %.6g %.6g %.6g"; }
};

template <>
class Vec4Traits<double>
{
public:
	static const TCHAR* StringFormat() { return _T("[ %.10g %.10g %.10g %.10g ]"); }
	static const TCHAR* StringFormatBare() { return _T("%.10g %.10g %.10g %.10g"); }
	static const char* StringAFormatBare() { return "%.10g %.10g %.10g %.10g"; }
};


template <class vreal>
class Vec4T
{
		
public:
	
	static const int Dimensions = 4;
	typedef double vreal64;
	typedef vreal FT;

	union
	{
		vreal n[4];
		struct
		{
			vreal x,y,z,w;
		};
		struct
		{
			Vec3T<vreal> vec3;
			vreal Wdummy;
		};
		struct
		{
			vreal r,g,b,a;
		};
		//struct
		//{
		//	vreal u,v,q,r;
		//};
		struct
		{
			Vec2T<vreal> vec2;
			Vec2T<vreal> vec2_zw;
		};
	};
		
public:
		
#ifdef _MSC_VER

		Vec4T() {}

		explicit Vec4T( vreal uniform ) { x = y = z = w = uniform; }

		Vec4T(const Vec2T<vreal> & v, vreal z, vreal w) { this->x = v.x; this->y = v.y; this->z = z; this->w = w; }

		Vec4T(const Vec3T<vreal> & v, vreal w) { this->x = v.x; this->y = v.y; this->z = v.z; this->w = w; }

		Vec4T(const vreal x, const vreal y, const vreal z, const vreal w) { this->x = x; this->y = y; this->z = z; this->w = w; }
#endif

		static Vec4T Create( vreal x, vreal y, vreal z, vreal w )
		{
			Vec4T v;
			v.x = x;
			v.y = y;
			v.z = z;
			v.w = w;
			return v;
		}

		const Vec2T<FT>& AsVec2() const { return vec2; }
					Vec2T<FT>& AsVec2()			{ return vec2; }
		const Vec3T<FT>& AsVec3() const { return vec3; }
					Vec3T<FT>& AsVec3()			{ return vec3; }
		const Vec4T<FT>& AsVec4() const { return *this; }
					Vec4T<FT>& AsVec4()			{ return *this; }

		void set(vreal x, vreal y, vreal z, vreal w);
		//Vec4T(const Vec4T& v);          // copy constructor
		
		// lambda * v1 + (1 - lambda) * v2
		static Vec4T Interpolate( const Vec4T &v1, const Vec4T &v2, vreal lambda )
		{
			return lambda * v1 + (1 - lambda) * v2;
		}

		// Assignment operators
		
		Vec4T & operator += ( const Vec4T& v );     // incrementation by a Vec4T
		Vec4T & operator -= ( const Vec4T& v );     // decrementation by a Vec4T
		Vec4T & operator *= ( const vreal d );     // multiplication by a constant
		Vec4T & operator /= ( const vreal d );     // division by a constant
		vreal& operator [] ( int i) { return n[i]; };       // indexing
		const vreal&operator[](int i) const;
		vreal& operator () ( int i) { return n[i]; };       // indexing
		const vreal&operator()(int i) const;
		
		// special functions
		
		vreal size();         // length of a Vec4T
		vreal sizeSquared();  // squared length of a Vec4T
		void normalize();
		
		Vec4T normalized() const
		{
			Vec4T copy = *this;
			copy.normalize();
			return copy;
		}


		void copyTo( vreal *dst ) const
		{
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}

		
//    Vec4T& ApplyFunc(ulFuncPtrf fct);  // apply a func. to each component
		void Homogenize( void );
		
		
		// friend? who needs friends? all members are public.. so.. we're everybody's friend.

		// //friends
		
		//friend Vec4T operator - (const Vec4T& v);         // -v1
		//friend Vec4T operator + (const Vec4T& v1, const Vec4T& v2);      // v1 + v2
		//friend Vec4T operator - (const Vec4T& v, const Vec4T& v2);      // v1 - v2
		//friend Vec4T operator * (const Vec4T& v, const vreal d);      // v1 * d
		//friend Vec4T operator * (const vreal d, const Vec4T& a);      // d * v1
		
		//friend Vec4T operator * (const Mat4& m, const Vec4T& v);      // M * v
		//friend Vec4T operator * (const Vec4T& v, const Mat4& m);      // M(transpose) * v
		//
		//friend Vec4T operator * (const MatR& m, const Vec4T& v);      // M * v
		//friend Vec4T operator * (const Vec4T& v, const MatR& m);      // M(transpose) * v
		//
		//friend Vec4T operator * (const MatQ& m, const Vec4T& v);      // M * v
		//friend Vec4T operator * (const Vec4T& v, const MatQ& m);      // M(transpose) * v
		
		
		//friend vreal operator & (const Vec4T& v1, const Vec4T& v2);    // dot product
		//friend Vec4T operator * (const Vec4T& v1, const Vec4T& b);    // V1 * V2
		//friend Vec4T operator / (const Vec4T& v, const vreal d);      // v1 / 3.0
		//friend Vec4T operator % (const Vec4T& v1, const Vec4T& v2);
		//friend int operator == (const Vec4T& v1, const Vec4T& v2);      // v1 == v2 ?
		//friend int operator != (const Vec4T& v1, const Vec4T& v2);      // v1 != v2 ?
		//friend ostream& operator << (ostream& s, Vec4T& v);      // output to stream
		//friend istream& operator >> (istream& s, Vec4T& v);      // input from strm.
		
		Vec4T& operator *= (const Vec4T& v);
		Vec4T& operator /= (const Vec4T& v);
		
		// =====================================
		// Unary operators
		// =====================================
		
		//friend Vec4T operator + (const Vec4T& v);
		//friend Vec4T operator - (const Vec4T& v);
		
		
		// =====================================
		// Binary operators
		// =====================================
		
		// Addition and subtraction
		//friend Vec4T operator + (const Vec4T& v1, const Vec4T& v2);
		//friend Vec4T operator - (const Vec4T& v1, const Vec4T& v2);
		// Scalar multiplication and division
		//friend Vec4T operator * (const Vec4T& v, double s);
		//friend Vec4T operator * (double s, const Vec4T& v);
		//friend Vec4T operator * (const Vec4T& v, float s);
		//friend Vec4T operator * (float s, const Vec4T& v);
		//friend Vec4T operator / (const Vec4T& v, vreal s);
		// Memberwise multiplication and division
		//friend Vec4T operator * (const Vec4T& v1, const Vec4T& v2);
		//friend Vec4T operator / (const Vec4T& v1, const Vec4T& v2);
		
		// Vector dominance
		//friend int operator < (const Vec4T& v1, const Vec4T& v2);
		//friend int operator <= (const Vec4T& v1, const Vec4T& v2);
		
		// Bitwise equality
		//friend int operator == (const Vec4T& v1, const Vec4T& v2);
		
		
		// this *= s;
		void scale(vreal s);
		
		// this = v * s
		void scale(const Vec4T &v, vreal s);
		// this += v
		void add(const Vec4T & v);
		// this = v1 + v2
		void add(const Vec4T & v1, const Vec4T & v2);
		
		
		// min of all components
		vreal Min() const;
		
		// min abs of all components
		vreal MinAbs() const;
		// maximum of all components
		vreal Max() const;
		// max abs of components
		vreal MaxAbs() const;
		
		void Maximize( const Vec4T& rhs )
		{
				x = ( x > rhs.x ) ? x : rhs.x;
				y = ( y > rhs.y ) ? y : rhs.y;
				z = ( z > rhs.z ) ? z : rhs.z;
				w = ( w > rhs.w ) ? w : rhs.w;
		}
		void Minimize( const Vec4T& rhs )
		{
				x = ( x < rhs.x ) ? x : rhs.x;
				y = ( y < rhs.y ) ? y : rhs.y;
				z = ( z < rhs.z ) ? z : rhs.z;
				w = ( w < rhs.w ) ? w : rhs.w;
		}
		// this = -v
		void negate(const Vec4T &v);
		// this = -this
		void negate();
		// this -= v
		void sub(const Vec4T &v);
		
		// this = v1 - v2
		void sub(const Vec4T &v1, const Vec4T &v2);
		// this *= v
		void mult(const Vec4T &v);
		// this = v1 * v2
		void mult(const Vec4T &v1, const Vec4T &v2);
		//  this = v1 + lambda * v2; 
		void displace(const Vec4T &v1, const Vec4T &v2, vreal lambda); 
		
		//  this += lambda * v; 
		void displace(const Vec4T &v, vreal lambda);
		
	
		vreal distance3d(const Vec4T &b) const		{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z)); }
		vreal distance2d(const Vec4T &b) const		{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }

		vreal distance3dSQ(const Vec4T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z); }
		vreal distance2dSQ(const Vec4T &b) const	{ return (b.x-x)*(b.x-x) + (b.y-y)*(b.y-y); }

		// mag(this - other)
		vreal distance(const Vec4T &b) const 
		{
			return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z) + (w-b.w)*(w-b.w));
		}

		// mag(this - other) ^2
		vreal distanceSQ(const Vec4T &b) const 
		{
			return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z) + (w-b.w)*(w-b.w);
		}

		/// Clamps values individually
		void clamp( vreal vmin, vreal vmax );
		
		bool checkNaN() const;

		/// Only valid for Vec4T<double>. Checks whether we won't overflow if converted to float.
		bool checkFloatOverflow() const
		{
			if (	x > FLT_MAX || x < -FLT_MAX ||
						y > FLT_MAX || y < -FLT_MAX ||
						z > FLT_MAX || z < -FLT_MAX ||
						w > FLT_MAX || w < -FLT_MAX ) return false;
			return true;
		}

		//friend ostream& operator << (ostream& s, Vec4T& v);      // output to stream
		//friend istream& operator >> (istream& s, Vec4T& v);      // input from strm.
		//ostream &print(ostream &os) const;
	
		int ToStringABare( char* buff, size_t buffChars ) const
		{
			return sprintf_s( buff, buffChars, Vec4Traits<vreal>::StringAFormatBare(), x, y, z, w );
		}

	#ifdef XSTRING_DEFINED
		/// Writes "[ %g %g %g %g ]"
		XString ToString() const
		{
			XString s;
			s.Format( Vec4Traits<vreal>::StringFormat(), x, y, z, w );
			return s;
		}

		/// Writes "%g %g %g %g"
		XString ToStringBare() const
		{
			XString s;
			s.Format( Vec4Traits<vreal>::StringFormatBare(), x, y, z, w );
			return s;
		}

		XString ToString( int significant_digits, bool bare = false ) const
		{
			XString f, s;
			if ( bare ) f.Format( _T("%%.%dg %%.%dg %%.%dg %%.%dg"), significant_digits, significant_digits, significant_digits, significant_digits );
			else		f.Format( _T("[ %%.%dg %%.%dg %%.%dg %%.%dg ]"), significant_digits, significant_digits, significant_digits, significant_digits );
			s.Format( f, x, y, z, w );
			return s;
		}

		/// Parses "[ x y z w ]", "z y z w", "x,y,z,w"
		bool Parse( const XString& str )
		{
			double a, b, c, d;
#ifdef LM_VS2005_SECURE
			if ( _stscanf_s( str, _T("[ %lf %lf %lf %lf ]"), &a, &b, &c, &d ) != 4 )
			{
				if ( _stscanf_s( str, _T("%lf %lf %lf %lf"), &a, &b, &c, &d ) != 4 )
				{
					if ( _stscanf_s( str, _T("%lf, %lf, %lf, %lf"), &a, &b, &c ) != 4 )
					{
						return false;
					}
				}
			}
#else
			if ( _stscanf( str, "[ %lf %lf %lf %lf ]", &a, &b, &c, &d ) != 4 )
			{
				if ( _stscanf( str, "%lf %lf %lf %lf", &a, &b, &c, &d ) != 4 )
				{
					if ( _stscanf( str, "%lf, %lf, %lf, %lf", &a, &b, &c ) != 4 )
					{
						return false;
					}
				}
			}
#endif

			x = (vreal) a;
			y = (vreal) b;
			z = (vreal) c;
			w = (vreal) d;
			
			return true;
		}

		static Vec4T FromString( const XString& str )
		{
			Vec4T v;
			v.Parse( str );
			return v;
		}

	#endif
		
};


template <class vreal> INLINE
void Vec4T<vreal>::clamp( vreal vmin, vreal vmax )
{
	x = CLAMP(x, vmin, vmax);
	y = CLAMP(y, vmin, vmax);
	z = CLAMP(z, vmin, vmax);
	w = CLAMP(w, vmin, vmax);
}


template <class vreal> INLINE void Vec4T<vreal>::Homogenize( void )
{
		double u = 1.0 / w;
		x = (vreal)(x*u);
		y = (vreal)(y*u);
		z = (vreal)(z*u);
}


/*
template <class vreal> INLINE
Vec4T<vreal>::Vec4T(vreal scalar)
{
	x = scalar;
	y = scalar;
	z = scalar;
	w = scalar;
}

template <class vreal> INLINE
Vec4T<vreal>::Vec4T(vreal _x, vreal _y, vreal _z, vreal _w)
{
		x = _x; 
		y = _y; 
		z = _z;
		w = _w;
}

template <class vreal> INLINE
Vec4T<vreal>::Vec4T(const Vec2T<vreal> & v, vreal _z, vreal _w)
{
		x = v.x; 
		y = v.y; 
		z = _z;
		w = _w;
}

template <class vreal> INLINE
Vec4T<vreal>::Vec4T(const Vec3T<vreal> & v, vreal _w)
{
		x = v.x; 
		y = v.y; 
		z = v.z;
		w = _w;
}


// copy contructor
template <class vreal> INLINE
Vec4T<vreal>::Vec4T(const Vec4T<vreal> & v)
{
		x = v.x; 
		y = v.y; 
		z = v.z;
		w = v.w;
} 
*/



// =====================================
// Access grants
// =====================================

template <class vreal> INLINE const vreal& Vec4T<vreal>::operator[](int i) const
{
		return n[i];
}

template <class vreal> INLINE
int operator != (const Vec4T<vreal>& a, const Vec4T<vreal>& b)
{ return !(a == b); }


template <class vreal> INLINE
vreal Vec4T<vreal>::size()
{
		//return (vreal)sqrt(GetMagnitudeSquared()); 
		return sqrt(sizeSquared()); 
}

template <class vreal> INLINE
vreal Vec4T<vreal>::sizeSquared()
{ 
	return x*x + y*y + z*z + w*w; 
}



// =====================================
// Assignment operators
// =====================================

template <class vreal> INLINE Vec4T<vreal>&
Vec4T<vreal>::operator += (const Vec4T<vreal>& v)
{
		x += v.x;   
		y += v.y;   
		z += v.z;
		w += v.w;
		return *this;
}

template <class vreal> INLINE Vec4T<vreal>&
Vec4T<vreal>::operator -= (const Vec4T<vreal>& v)
{
		x -= v.x;   
		y -= v.y;   
		z -= v.z;
		w -= v.w;
		return *this;
}

template <class vreal> INLINE Vec4T<vreal>&
Vec4T<vreal>::operator *= (const Vec4T<vreal>& v)
{
		x *= v.x;   
		y *= v.y;   
		z *= v.z;
		w *= v.w;
		return *this;
}

template <class vreal> INLINE Vec4T<vreal>&
Vec4T<vreal>::operator /= (const Vec4T<vreal>& v)
{
		x /= v.x;   
		y /= v.y;   
		z /= v.z;
		w /= v.w;
		return *this;
}

template <class vreal> INLINE Vec4T<vreal>&
Vec4T<vreal>::operator *= (vreal s)
{
		x *= s;   
		y *= s;   
		z *= s;
		w *= s; 
		return *this;
}

template <class vreal> INLINE Vec4T<vreal>&
Vec4T<vreal>::operator /= (vreal d)
{
		vreal64 u = 1.0 / d;
		x = (vreal)(x*u);   
		y = (vreal)(y*u);   
		z = (vreal)(z*u);
		w = (vreal)(w*u);
		return *this;
}

template <class vreal> INLINE Vec4T<vreal>
operator + (const Vec4T<vreal>& v)
{
		return v;
}

template <class vreal> INLINE Vec4T<vreal>
operator - (const Vec4T<vreal>& v)
{
		return Vec4T<vreal>(-v.x, -v.y, -v.z, -v.w);
}

template <class vreal> INLINE Vec4T<vreal>
operator + (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return Vec4T<vreal>(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z, v1.w+v2.w);
}

template <class vreal> INLINE Vec4T<vreal>
operator - (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return Vec4T<vreal>(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z, v1.w-v2.w);
}

template <class vreal> INLINE vreal
operator & (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
}

template <class vreal> INLINE Vec4T<vreal>
operator * (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return Vec4T<vreal>(v1.x*v2.x, v1.y*v2.y, v1.z*v2.z, v1.w*v2.w);
}

template <class vreal> INLINE Vec4T<vreal>
operator / (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return Vec4T<vreal>(v1.x/v2.x, v1.y/v2.y, v1.z/v2.z, v1.w/v2.w);
}

template <class vreal> INLINE int
operator < (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z && v1.w < v2.w;
}

template <class vreal> INLINE int
operator <= (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z && v1.w <= v2.w;
}

// Hardcode mixing of floats/doubles
// double *
template <class vreal> INLINE Vec4T<vreal>
operator * (const Vec4T<vreal>& v, double s)
{
		return Vec4T<vreal>(s*v.x, s*v.y, s*v.z, s * v.w);
}

template <class vreal> INLINE Vec4T<vreal>
operator * (double s, const Vec4T<vreal>& v)
{
		return Vec4T<vreal>(s*v.x, s*v.y, s*v.z, s*v.w);
}

// float *
template <class vreal> INLINE Vec4T<vreal>
operator * (const Vec4T<vreal>& v, float s)
{
		return Vec4T<vreal>(s*v.x, s*v.y, s*v.z, s * v.w);
}

template <class vreal> INLINE Vec4T<vreal>
operator * (float s, const Vec4T<vreal>& v)
{
		return Vec4T<vreal>(s*v.x, s*v.y, s*v.z, s*v.w);
}


/*template <class vreal> INLINE Vec4T<vreal>
operator * (const Vec4T<vreal>& v, vreal s)
{
		return Vec4T<vreal>(s*v.x, s*v.y, s*v.z, s * v.w);
}

template <class vreal> INLINE Vec4T<vreal>
operator * (vreal s, const Vec4T<vreal>& v)
{
		return Vec4T<vreal>(s*v.x, s*v.y, s*v.z, s*v.w);
}*/


template <class vreal> INLINE Vec4T<vreal>
operator / (const Vec4T<vreal>& v, vreal s)
{
		return Vec4T<vreal>(v.x/s, v.y/s, v.z/s, v.w/s);
}

template <class vreal> INLINE bool
operator == (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return v1.x==v2.x && v1.y==v2.y && v1.z == v2.z && v1.w == v2.w;
}

// Vec4T<vreal>
template <class vreal> INLINE 
void Vec4T<vreal>::normalize() 
{
	double s = 1.0 / sqrt(x*x + y*y + z*z + w*w);
	x *= s;
	y *= s;
	z *= s;
	w *= s;
}






/*
ostream& operator << (ostream& s, Vec4T<vreal>& v)
{ return s << "| " << v.x << ' ' << v.y << ' ' << v.z << ' '
	<< v.w << " |"; }

istream& operator >> (istream& s, Vec4T<vreal>& v) 
{
	Vec4T<vreal>	v_tmp;
	char	c = ' ';
	
	while (isspace(c))
		s >> c;
	// The vectors can be formatted either as x y z w or | x y z w |
	if (c == '|') {
		s >> v_tmp[X] >> v_tmp[Y] >> v_tmp[Z] >> v_tmp[W];
		while (s >> c && isspace(c)) ;
		//if (c != '|')
		//  s.set(_bad); DHR
	}
	else {
		s.putback(c);
		s >> v_tmp[X] >> v_tmp[Y] >> v_tmp[Z] >> v_tmp[W];
	}
	if (s)
		v = v_tmp;
	return s;
}


	*/


//INLINE Vec4T<vreal> ulProduct(const Vec4T<vreal>& a, const Vec4T<vreal>& b)
//{
//	return Vec4T<vreal>(a.x * b.x, a.y * b.y, a.z * b.z,
//			a.w * b.w);
//}



template <class vreal> INLINE bool Vec4T<vreal>::checkNaN() const
{
	if ( !_finite(x) || !_finite(y) || !_finite(z) || !_finite(w)) return false;
	return true;
}


template <class vreal> INLINE void Vec4T<vreal>::set(vreal _x, vreal _y, vreal _z, vreal _w)
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}

template <class vreal> INLINE vreal
dot (const Vec4T<vreal>& v1, const Vec4T<vreal>& v2)
{
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
}

template <class vreal> INLINE Vec4T<vreal>
normalize( const Vec4T<vreal>& a )
{
	return a.normalized();
}

template <class vreal> INLINE vreal
length( const Vec4T<vreal>& a )
{
	return a.size();
}

template <class vreal> INLINE vreal
lengthSQ( const Vec4T<vreal>& a )
{
	return a.sizeSquared();
}

typedef Vec4T<double> Vec4d;
typedef Vec4T<float> Vec4f;
typedef Vec4d Vec4;

} // namespace AbCoreVec

// pollution.
using namespace AbCoreVec;

inline Vec4		ToVec4( Vec4f v ) { return AbCoreVec::Vec4::Create( v.x, v.y, v.z, v.w ); }
inline Vec4f	ToVec4f( Vec4 v ) { return AbCoreVec::Vec4f::Create( (float) v.x, (float) v.y, (float) v.z, (float) v.w ); }

#include "VecUndef.h"
#endif // DEFINED_Vec4
