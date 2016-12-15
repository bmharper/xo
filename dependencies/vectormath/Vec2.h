#pragma once
#ifndef DEFINED_Vec2
#define DEFINED_Vec2

/*
Why do we have VecBase2T and Vec2T ?
VecBase2T exists so that we can include it inside unions. Thus, it defines no constructors.
Users of these classes should not need to know about VecBase2T. They should simply use Vec2T.
Unless, of course, they need to include these things inside unions. In that case the facade breaks
down.
*/

#include "VecPrim.h"

#include "VecDef.h"

template <typename vreal>
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

// The "Base" class has no constructor, so that it can be included inside a union
// Inside the base class, we do not expose any functions that leak our type
// For example, we cannot expose component-wise multiply, because that would
// leak VecBase2T to the outside world.
template <typename vreal>
class VecBase2T
{
public:
	static const int Dimensions = 2;
	typedef vreal FT;

	vreal x, y;

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Mutating operations
	/////////////////////////////////////////////////////////////////////////////////////////////

	void set(const vreal _x, const vreal _y)
	{
		x = _x;
		y = _y;
	}

	void normalize()
	{
		FT r = 1.0f / sqrt(x * x + y * y);
		x *= r;
		y *= r;
	}

	void normalizeIfNotZero()
	{
		FT lenSq = x * x + y * y;
		if (lenSq != 0)
		{
			FT r = 1.0 / sqrt(lenSq);
			x *= r;
			y *= r;
		}
	}

	void scale(vreal _scale)
	{
		x *= _scale;
		y *= _scale;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// const operations
	/////////////////////////////////////////////////////////////////////////////////////////////

	vreal operator&(const VecBase2T &b) const {
		return x*b.x + y*b.y;
	}
	vreal dot(const VecBase2T &b) const {
		return x*b.x + y*b.y;
	}

	// comparison operators
	bool operator==(const VecBase2T& v) const
	{
		return x == v.x && y == v.y;
	}
	bool operator!=(const VecBase2T& v) const
	{
		return x != v.x || y != v.y;
	}

	vreal sizeSquared() const {
		return x*x + y*y;
	}
	vreal size() const {
		return sqrt(x*x + y*y);
	}

	vreal distance(const VecBase2T &b) const	{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }
	vreal distance2d(const VecBase2T &b) const	{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }
	vreal distance3d(const VecBase2T &b) const	{ return sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y)); }
	vreal distanceSQ(const VecBase2T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y); }

	vreal distance2dSQ(const VecBase2T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y); }
	vreal distance3dSQ(const VecBase2T &b) const	{ return (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y); }

	// makes sure all members are not NaN
	bool checkNaN() const
	{
		if (vec_IsNaN(x) || vec_IsNaN(y))
			return false;
		return true;
	}

	/// Only valid for Vec2T<double>. Checks whether we won't overflow if converted to float.
	bool checkFloatOverflow() const
	{
		if (x > FLT_MAX || x < -FLT_MAX ||
				y > FLT_MAX || y < -FLT_MAX) return false;
		return true;
	}

	void copyTo(vreal *dst) const
	{
		dst[0] = x;
		dst[1] = y;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Duplicated inside Vec2T
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VecBase2T& operator-=(const VecBase2T &b)	{ x -= b.x; y -= b.y; return *this; }
	VecBase2T& operator+=(const VecBase2T &b)	{ x += b.x; y += b.y; return *this; }
	VecBase2T& operator*=(const VecBase2T &b)	{ x *= b.x; y *= b.y; return *this; }
	VecBase2T& operator/=(const VecBase2T &b)	{ x /= b.x; y /= b.y; return *this; }
	VecBase2T& operator/=(vreal s)				{ vreal r = 1.0 / s; 	x *= r; y *= r;	return *this; }
	VecBase2T& operator*=(vreal s)				{						x *= s; y *= s; return *this; }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

};

template <typename vreal>
class Vec2T : public VecBase2T<vreal>
{
public:
	typedef vreal FT;
	using VecBase2T<vreal>::x;
	using VecBase2T<vreal>::y;

	Vec2T()													{}
	Vec2T(vreal _x, vreal _y)								{ x = _x; y = _y; }
	Vec2T(const VecBase2T<vreal>& b)						{ x = b.x; y = b.y; }

	static Vec2T Create(vreal x, vreal y)
	{
		Vec2T v;
		v.x = x;
		v.y = y;
		return v;
	}

	/// Returns a vector that is [cos(angle), sin(angle)]
	static Vec2T AtAngle(vreal angle)
	{
		return Vec2T::Create(cos(angle), sin(angle));
	}

	const Vec2T& AsVec2() const { return *this; }
	Vec2T& AsVec2()       { return *this; }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Duplicated inside VecBase2T
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Vec2T& operator-=(const Vec2T &b)		{ x -= b.x; y -= b.y; return *this; }
	Vec2T& operator+=(const Vec2T &b)		{ x += b.x; y += b.y; return *this; }
	Vec2T& operator*=(const Vec2T &b)		{ x *= b.x; y *= b.y; return *this; }
	Vec2T& operator/=(const Vec2T &b)		{ x /= b.x; y /= b.y; return *this; }
	Vec2T& operator/=(vreal s)				{ vreal r = 1.0 / s; 	x *= r; y *= r;	return *this; }
	Vec2T& operator*=(vreal s)				{						x *= s; y *= s; return *this; }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Vec2T normalized() const
	{
		Vec2T copy = *this;
		copy.normalize();
		return copy;
	}

	Vec2T normalizedIfNotZero() const
	{
		Vec2T copy = *this;
		copy.normalizeIfNotZero();
		return copy;
	}

	// unary
	Vec2T operator-() const  {  return Vec2T(-x, -y);  }

	/// Returns the result of sprintf
	int ToStringABare(char* buff, size_t buffChars) const
	{
#ifdef _WIN32
		return sprintf_s(buff, buffChars, Vec2Traits<vreal>::StringAFormatBare(), x, y);
#else
		return snprintf(buff, buffChars, Vec2Traits<vreal>::StringAFormatBare(), x, y);
#endif
	}

};

template<typename vreal> inline Vec2T<vreal> operator*(const VecBase2T<vreal> &a, vreal s)							{ return Vec2T<vreal>(a.x * s,   a.y * s); }
template<typename vreal> inline Vec2T<vreal> operator*(vreal s, const VecBase2T<vreal> &a)							{ return Vec2T<vreal>(a.x * s,   a.y * s); }
template<typename vreal> inline Vec2T<vreal> operator*(const VecBase2T<vreal> &a, const VecBase2T<vreal> &b)		{ return Vec2T<vreal>(a.x * b.x, a.y * b.y); }
template<typename vreal> inline Vec2T<vreal> operator+(const VecBase2T<vreal> &a, const VecBase2T<vreal> &b)		{ return Vec2T<vreal>(a.x + b.x, a.y + b.y); }
template<typename vreal> inline Vec2T<vreal> operator-(const VecBase2T<vreal> &a, const VecBase2T<vreal> &b)		{ return Vec2T<vreal>(a.x - b.x, a.y - b.y); }
template<typename vreal> inline Vec2T<vreal> operator/(const VecBase2T<vreal> &a, const VecBase2T<vreal> &b)		{ return Vec2T<vreal>(a.x / b.x, a.y / b.y); }
template<typename vreal> inline Vec2T<vreal> operator/(const vreal s, const Vec2T<vreal> &b)						{ return Vec2T<vreal>(s   / b.x, s   / b.y); }
template<typename vreal> inline vreal        dot(const VecBase2T<vreal>& a, const VecBase2T<vreal>& b)				{ return a.x * b.x + a.y * b.y; }
template<typename vreal> inline Vec2T<vreal> operator/(const VecBase2T<vreal> &a, const vreal s)					{ vreal rec = (vreal) 1.0 / s; return Vec2T<vreal>(a.x * rec, a.y * rec); }
template<typename vreal> inline Vec2T<vreal> normalize(const VecBase2T<vreal>& a)									{ Vec2T<vreal> copy = a; copy.normalized(); return copy; }
template<typename vreal> inline vreal        length(const VecBase2T<vreal>& a)										{ return a.size(); }
template<typename vreal> inline vreal        lengthSQ(const VecBase2T<vreal>& a)									{ return a.sizeSquared(); }

template <class vreal>  bool
operator <= (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
	return v1.x <= v2.x && v1.y <= v2.y;
}

template <class vreal>  bool
operator < (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
	return v1.x < v2.x && v1.y < v2.y;
}


template <class vreal>  bool
operator > (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
	return v1.x > v2.x && v1.y > v2.y;
}

template <class vreal>  bool
operator >= (const Vec2T<vreal>& v1, const Vec2T<vreal>& v2)
{
	return v1.x >= v2.x && v1.y >= v2.y;
}

typedef Vec2T<double> Vec2d;
typedef Vec2T<float> Vec2f;
typedef Vec2d Vec2;

#ifdef DVECT_DEFINED
typedef dvect< Vec2f > Vec2fVect;
typedef dvect< Vec2d > Vec2dVect;
#endif

inline Vec2		ToVec2(vec2 v)  { return Vec2::Create(v.x, v.y); }
inline Vec2		ToVec2(Vec2f v) { return Vec2::Create(v.x, v.y); }
inline Vec2f	ToVec2f(Vec2 v) { return Vec2f::Create((float) v.x, (float) v.y); }

#include "VecUndef.h"
#endif // DEFINED_Vec2
