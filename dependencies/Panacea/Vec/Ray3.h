#pragma once

#include "Vec3.h"

/** A ray defined by 2 absolute points.
**/
template< typename TReal >
class Ray3T
{
public:
	typedef Vec3T<TReal> NVec3;

	Ray3T() {}

	Ray3T( const NVec3& a, const NVec3& b )
	{
		P1 = a;
		P2 = b;
	}
	/// Returns a ray which has both points equal
	static Ray3T Singular( const NVec3& a )
	{
		return Ray3T( a, a );
	}
	/// Returns the ray (x, y, -1) to (x, y, +1)
	template< typename TVec >
	static Ray3T Through2D( const TVec& a )
	{
		return Ray3T( NVec3(a.x, a.y, -1), NVec3(a.x, a.y, 1) );
	}
	void Swap()							{ AbCore::Swap(P1, P2); }
	Ray3T Swapped() const				{ return Ray3T(P2, P1); }
	NVec3 PtAt( double mu ) const		{ return (1 - mu) * P1 + mu * P2; }
	NVec3 Mid() const					{ return PtAt(0.5); }
	bool IsSinglePt() const				{ return P2 == P1; }
	NVec3 Direction() const				{ return P2 - P1; }
	NVec3 DirectionNormalized() const	{ return (P2 - P1).normalized(); }
	NVec3 P1;
	NVec3 P2;
	bool operator==( const Ray3T& b ) const { return P1 == b.P1 && P2 == b.P2; }
	bool operator!=( const Ray3T& b ) const { return !(P1 == P2); }
};

typedef Ray3T<double> Ray3d;
typedef Ray3T<float>	Ray3f;
typedef Ray3d Ray3;
