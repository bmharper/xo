#pragma once

#include "Vec4.h"

#ifndef DEFINED_Vec5
#define DEFINED_Vec5
#include "VecDef.h"

template <class FT>
class VecBase5T
{
public:

	static const int Dimensions = 5;

	union
	{
		FT n[5];
		struct
		{
			FT a,b,c,d,e;
		};
		struct
		{
			VecBase4T<FT> vec4;
			// (plus e)
		};
	};
};

template <class FT>
class Vec5T : public VecBase5T<FT>
{
public:
	using VecBase5T<FT>::a;
	using VecBase5T<FT>::b;
	using VecBase5T<FT>::c;
	using VecBase5T<FT>::d;
	using VecBase5T<FT>::e;
	using VecBase5T<FT>::vec4;
	using VecBase5T<FT>::n;

	Vec5T() {}

	Vec5T( FT _a, FT _b, FT _c, FT _d, FT _e ) 
	{
		a = _a;
		b = _b;
		c = _c;
		d = _d;
		e = _e;
	}

	Vec5T( const VecBase5T<FT> &q )
	{
		a = q.a;
		b = q.b;
		c = q.c;
		d = q.d;
		e = q.e;
	}

	/// Fill all members with scalar
	explicit Vec5T( FT scalar )
	{
		a = b = c = d = e = scalar;
	}

	Vec5T( const VecBase4T<FT>& v, FT e_ )
	{
		vec4 = v;
		e = e_;
	}

	// indexing
	FT& operator []			( int i )		{ return n[i]; }
	const FT& operator[]	( int i ) const { return n[i]; }
	FT& operator ()			( int i )		{ return n[i]; }
	const FT& operator()	( int i ) const { return n[i]; }

	bool operator==( const VecBase5T<FT>& b ) const { return a == b.a && b == b.b && c == b.c && d == b.d && e == b.e; }
	bool operator!=( const VecBase5T<FT>& b ) const { return a != b.a || b != b.b || c != b.c || d != b.d || e != b.e; }
};

typedef Vec5T<double> Vec5;
typedef Vec5T<double> Vec5d;
typedef Vec5T<float> Vec5f;

inline Vec5		ToVec5( Vec5f v ) { return Vec5( v.a, v.b, v.c, v.d, v.e ); }
inline Vec5f	ToVec5f( Vec5 v ) { return Vec5f( v.a, v.b, v.c, v.d, v.e ); }

#include "VecUndef.h"
#endif // DEFINED_Vec5
