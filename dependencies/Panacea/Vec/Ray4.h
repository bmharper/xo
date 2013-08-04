#pragma once

#include "Vec4.h"

/** A ray defined by 2 absolute points.
**/
template< typename TReal >
class Ray4T
{
public:
	typedef Vec4T<TReal> NVec4;

	Ray4T() {}

	Ray4T( const NVec4& a, const NVec4& b )
	{
		P1 = a;
		P2 = b;
	}
	/// Returns a ray which has both points equal
	static Ray4T Singular( const NVec4& a )
	{
		return Ray4T( a, a );
	}
	void Swap()							{ AbCore::Swap(P1, P2); }
	Ray4T Swapped() const				{ return Ray4T(P2, P1); }
	NVec4 PtAt( double mu ) const		{ return (1 - mu) * P1 + mu * P2; }
	NVec4 Mid() const					{ return PtAt(0.5); }
	bool IsSinglePt() const				{ return P2 == P1; }
	NVec4 Direction() const				{ return P2 - P1; }
	NVec4 DirectionNormalized() const	{ return (P2 - P1).normalized(); }
	NVec4 P1;
	NVec4 P2;
	bool operator==( const Ray4T& b ) const { return P1 == b.P1 && P2 == b.P2; }
	bool operator!=( const Ray4T& b ) const { return !(P1 == P2); }
};

typedef Ray4T<double> Ray4d;
typedef Ray4T<float>	Ray4f;
typedef Ray4d Ray4;
