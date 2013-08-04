#pragma once

#include "../Other/lmTypes.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Mat4.h"

// we don't generally need doubles for rotations
struct PAPI Quatd
{
	double w, x, y, z;

	void Set( double _x, double _y, double _z, double _w )
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
};

class PAPI Quatf
{
public:
	// order is important. this is same as D3D Quaternion. required for amd libs.
	float w, x, y, z;

	Quatf( )
	{
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	}

	Quatf( float _x, float _y, float _z, float _w )
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	Quatf( float angle2d )
	{
		x = 0;
		y = 0;
		z = sinf( angle2d * 0.5 );
		w = -cosf( angle2d * 0.5 );
	}

	void Set( float _x, float _y, float _z, float _w )
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	/// Returns sqrt(x^2 + y^2 + z^2 + w^2)
	float Magnitude() const
	{
		return sqrt(x*x + y*y + z*z + w*w);
	}

	/// Returns true if the quaternion is (0,0,0,1) or (0,0,0,-1)
	bool IsIdentity() const
	{
		return (x == 0 && y == 0 && z == 0) && (w == 1 || w == -1);
	}

	/// Intelligent equals, that takes into account that -b = b.
	bool EqualsQuat( const Quatf& b ) const
	{
		return (x == b.x) && (y == b.y) && (z == b.z) && (w == b.w || w == -b.w);
	}

	/// Binary equality
	bool operator==( const Quatf& b ) const
	{
		return (x == b.x) && (y == b.y) && (z == b.z) && (w == b.w);
	}

	/// Binary inequality
	bool operator!=( const Quatf& b ) const
	{
		return (x != b.x) || (y != b.y) || (z != b.z) || (w != b.w);
	}
};


/*
void xQuatAdd (Quatd& r, const Quatd& a, const Quatd& b);
void xQuatSub (Quatd& r, const Quatd& a, const Quatd& b);
void xQuatMul (Quatd& r, const Quatd& a, const Quatd& b);
void xQuatNorm (Quatd& dst, const Quatd& a);
void xMat2Quat (Quatd& quat, const real *m);
void xQuat2Mat (real *m, const Quatd& quat);
void xEuler2Quat (Quatd& quat, const real *rot);
void xEuler2Quat2 (Quatd& quat, const real *rot);
void xQuat2Axis_Angle (real *axisAngle, const Quatd& quat);
void xAxis_Angle2Quat (Quatd& quat, const real *axisAngle);
void xSlerpQuat (Quatd& result,
								const Quatd& quat1, const Quatd& quat2,
								real slerp);
void xTransQuat ( Vec3T<real> &result, const Vec3T<real> &v, const Quatd& q); */

typedef Quatf QuatT;
typedef float qreal;

void PAPI xQuatFrom2Vecf (Quatf& r, const Vec3f& from, const Vec3f& to);
void PAPI xQuatFrom2Vecd (Quatf& r, const Vec3& from, const Vec3& to);
void PAPI xQuatAdd (Quatf& r, const Quatf& a, const Quatf& b);
void PAPI xQuatSub (Quatf& r, const Quatf& a, const Quatf& b);
void PAPI xQuatMul (Quatf& r, const Quatf& a, const Quatf& b);
void PAPI xQuatNorm (Quatf& dst, const Quatf& a);
void PAPI xMat2Quat (Quatf& quat, const double *m);

void PAPI xQuat2Matf ( float* quat, float *m );
void PAPI xQuat2Matd ( float* quat, double *m );

void PAPI xEuler2Quat (Quatf& quat, const float *rot);
void PAPI xEuler2Quat2 (Quatf& quat, const float *rot);
void PAPI xQuat2Axis_Angle (float *axisAngle, const Quatf& quat);
void PAPI xAxis_Angle2Quat (Quatf& quat, const float *axisAngle);
void PAPI xSlerpQuat (Quatf& result,
																	const Quatf& quat1, const Quatf& quat2,
																	float slerp);
void PAPI xTransQuat ( Vec3T<float> &result, const Vec3T<float> &v, const Quatf& q);












