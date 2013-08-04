/******************************************************************************

Copyright (c) 1999 Advanced Micro Devices, Inc.

LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
NOT APPLY TO YOU.

AMD does not assume any responsibility for any errors which may appear in the
Materials nor any responsibility to support or update the Materials.  AMD retains
the right to make changes to its test specifications at any time, without notice.

NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
further information, software, technical information, know-how, or show-how
available to you.

So that all may benefit from your experience, please report  any  problems
or  suggestions about this software to 3dsdk.wupport@amd.com

AMD Developer Technologies, M/S 585
Advanced Micro Devices, Inc.
5900 E. Ben White Blvd.
Austin, TX 78741
3dsdk.wupport@amd.com

*******************************************************************************

QUAT_LIB.C

AMD3D 3D library code: Quaternion math

	These routines provide a portable quaternion math library.  3DNow!
	accelerated versions of these routines can be found in QUAT.ASM.

	Loosly based on the quaternion library presented by Jeff Lander.
	Adapted to 3DNow! implementation and library conventions.

	BMH:
	This is a rip from the AMD lib of course.
	I prefixed the functions with x__ to indicate x86 code.
	I changed the Matrix funcs to doubles.
	I also changed all the angles to use Radians.

	[Update May 2009 BMH. I removed the AMD 3DNow quaternion stuff.
		It relied on old D3D8 headers, that are no longer around.

******************************************************************************/
#include "pch.h"

#include <math.h>

#ifndef _WIN64
#include <amath.h>
#include <amd3dx.h>
//#include "aquat.h"
#endif

#include "Mat4.h"

#include "Quat.h"

#define HALF_PI (3.1415927f * 0.5f)

//typedef double qreal;

/*

Here is for instance one reference to basic quaternion math: 
http://www.cs.concordia.ca/~faculty/grogono/CUGL/rotation.pdf 

You can solve your specific problem like follows: 
v1=[0, 0, 1] 
v2=some random point 
cosAngle=v1 dot (v2/|v2|) 
cosHalfAngle=sqrt((1+cosAngle)/2) 
sinHalfAngle=sqrt(1-cosHalfAngle*cosHalfAngle) 
rotVector=v1 cross v2 
quat=[sinHalfAngle*rotVector/|rotVector|, cosHalfAngle] 

cosHalfAngle comes from formula: cos(x/2)=sqrt((1+cos(x))/2) because dot product always gives cos of positive angle in range [0, pi] 

sinHalfAngle comes from formula: sin(x)=sqrt(1-cos(x)^2) 

Obviously you can optimize these formulas quite a bit. 

*/

// from and to must already be normalized
void xQuatFrom2Vecf ( Quatf& r, const Vec3f& from, const Vec3f& to )
{
	//inline void QuaternionFromTwoDirs2( quaternion& quat, 
	//const vector3& UnitFrom, // assert len == 1 
	//const vector3& UnitTo){ // assert len == 1 
	//const Vec3f& from = from_arr;
	//const Vec3f& to = to_arr;

	Vec3f bisect = from + to; 
	//FIX_ME take a special cases here ( 180 degrees case) 
	if ( bisect.sizeSquared() < 1e-5 )
	{
		r.Set( 1, 0, 0, 0 );
	}
	else
	{
		bisect.normalize(); 
		Vec3f bcross;
		bcross.cross( from, bisect); // multiplied by HalfSinA 
		r.Set( bcross.x, bcross.y, bcross.z, from & bisect ); 
	}
}

void xQuatFrom2Vecd ( Quatf& r, const Vec3& from, const Vec3& to )
{
	//inline void QuaternionFromTwoDirs2( quaternion& quat, 
	//const vector3& UnitFrom, // assert len == 1 
	//const vector3& UnitTo){ // assert len == 1 
	//const Vec3& from = from_arr;
	//const Vec3& to = to_arr;

	Vec3 bisect = from + to; 
	//FIX_ME take a special cases here ( 180 degrees case) 
	if ( bisect.sizeSquared() < 1e-9 )
	{
		r.Set( 1, 0, 0, 0 );
	}
	else
	{
		bisect.normalize(); 
		Vec3 bcross;
		bcross.cross( from, bisect); // multiplied by HalfSinA 
		r.Set( bcross.x, bcross.y, bcross.z, from & bisect ); 
	}
}

/******************************************************************************
Routine:   add_quat
Input:     r - quaternion address
						a - quaternion address
						b - quaternion address
Output:    r.i = a.i + b.i
******************************************************************************/
void xQuatAdd (QuatT& r, const QuatT& a, const QuatT& b)
{
	r.w = a.w + b.w;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;
}


/******************************************************************************
Routine:   sub_quat
Input:     r - quaternion address
						a - quaternion address
						b - quaternion address
Output:    r.i = a.i - b.i
******************************************************************************/
void xQuatSub (QuatT& r, const QuatT& a, const QuatT& b)
{
	r.w = a.w - b.w;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;
}


/******************************************************************************
Routine:   mult_quat
Input:     r - quaternion address
						a - quaternion address
						b - quaternion address
Output:    r = a * b, using the definition of quaternion multiplication
******************************************************************************/
void xQuatMul (QuatT& r, const QuatT& a, const QuatT& b)
{
	// Do this to a temporary variable in case the output aliases an input
	QuatT tmp;
	tmp.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
	tmp.x = a.x * b.w + b.x * a.w + a.y * b.z - a.z * b.y;
	tmp.y = a.y * b.w + b.y * a.w + a.z * b.x - a.x * b.z;
	tmp.z = a.z * b.w + b.z * a.w + a.x * b.y - a.y * b.x;

	r = tmp;
}


/******************************************************************************
Routine:   norm_quat
Input:     dst  - quaternion address
						quat - quaternion address
Output:    dst  = normalize(quat) such that (x^2 + y^2 + z^2 + w^2 = 1)
******************************************************************************/
void xQuatNorm (QuatT& dst, const QuatT& a)
{
	const qreal	quatx = a.x,
							quaty = a.y,
							quatz = a.z,
							quatw = a.w;
	const qreal  magnitude = 1.0f / (qreal)sqrt((quatx * quatx) +
																							(quaty * quaty) +
																							(quatz * quatz) +
																							(quatw * quatw));

	dst.x = quatx * magnitude;
	dst.y = quaty * magnitude;
	dst.z = quatz * magnitude;
	dst.w = quatw * magnitude;
}


/******************************************************************************
Routine:   D3DMat2quat
Input:     m    - matrix (4x4) address
						quat - quaternion address
Output:    quat = quaternion from rotation matrix 'm'
******************************************************************************/
/*void D3DMat2quat (QuatT *quat, const D3DMATRIX *m)
{
		qreal   *mm = (qreal *)&m;
		qreal   tr, s;

		tr = m->_11 + m->_22 + m->_33;

		// check the diagonal
		if (tr > 0.0)
		{
				s = (qreal) sqrt (tr + 1.0f);
				quat->s = s * 0.5f;
				s = 0.5f / s;
				quat->v.x = (m->_32 - m->_23) * s;
				quat->v.y = (m->_13 - m->_31) * s;
				quat->v.z = (m->_21 - m->_12) * s;
		}
		else
		{
				if (m->_22 > m->_11 && m->_33 <= m->_22)
				{
						s = (qreal)sqrt ((m->_22 - (m->_33 + m->_11)) + 1.0f);

						quat->v.x = s * 0.5f;

						if (s != 0.0)
								s = 0.5f / s;

						quat->v.z = (m->_13 - m->_31) * s;
						quat->v.y = (m->_32 + m->_23) * s;
						quat->s   = (m->_12 + m->_21) * s;
				}
				else if ((m->_22 <= m->_11  &&  m->_33 > m->_11)  ||  (m->_33 > m->_22))
				{
						s = (qreal)sqrt ((m->_33 - (m->_11 + m->_22)) + 1.0f);

						quat->v.y = s * 0.5f;

						if (s != 0.0)
								s = 0.5f / s;

						quat->v.z = (m->_21 - m->_12) * s;
						quat->s   = (m->_13 + m->_31) * s;
						quat->v.x = (m->_23 + m->_32) * s;
				}
				else
				{
						s = (qreal)sqrt ((m->_11 - (m->_22 + m->_33)) + 1.0f);

						quat->s = s * 0.5f;

						if (s != 0.0)
								s = 0.5f / s;

						quat->v.z = (m->_32 - m->_23) * s;
						quat->v.x = (m->_21 + m->_12) * s;
						quat->v.y = (m->_31 + m->_13) * s;
				}

#if 0
				// diagonal is negative
				i = 0;
				if (m->_22 > m->_11) i = 1;
				if (m->_33 > mm[i*4+i]) i = 2;
				j = nxt[i];
				k = nxt[j];

				s = (qreal)sqrt ((mm[i*4+i] - (mm[j*4+j] + mm[k*4+k])) + 1.0f);

				q[i] = s * 0.5f;

				if (s != 0.0)
						s = 0.5f / s;

				q[3] = (mm[j*4+k] - mm[k*4+j]) * s;
				q[j] = (mm[i*4+j] + mm[j*4+i]) * s;
				q[k] = (mm[i*4+k] + mm[k*4+i]) * s;

				*quat = q;
#endif
		}
}*/


/******************************************************************************
Routine:   glMat2quat
Input:     m    - matrix (4x4) address
						quat - quaternion address
Output:    quat = quaternion from rotation matrix 'm'
******************************************************************************/
void xMat2Quat (QuatT& quat, const double *m)
{
	double tr, s;

	tr = m[0] + m[5] + m[10] + 1;
	double sum_w = 1 + m[0] + m[5] + m[10];
	double sum_x = 1 + m[0] - m[5] - m[10];
	double sum_y = 1 + m[5] - m[0] - m[10];
	double sum_z = 1 + m[10] - m[0] - m[5];

	if ( sum_w > sum_x && sum_w > sum_y && sum_w > sum_z )
	{
		s = 0.5 / sqrt(sum_w);
		quat.x = (m[6] - m[9]) * s;
		quat.y = (m[8] - m[2]) * s;
		quat.z = (m[1] - m[4]) * s;
		quat.w = 0.25 / s;
	}
	else if ( sum_x > sum_y && sum_x > sum_z )
	{
		s = 0.5 / sqrt(sum_x);
		quat.x = 0.25 / s;
		quat.y = (m[1] + m[4]) * s;
		quat.z = (m[8] + m[2]) * s;
		quat.w = (m[6] - m[9]) * s;
	}
	else if ( sum_y > sum_z )
	{
		s = 0.5 / sqrt(sum_y);
		quat.x = (m[1] + m[4]) * s;
		quat.y = 0.25 / s;
		quat.z = (m[6] + m[9]) * s;
		quat.w = (m[8] - m[2]) * s;
	}
	else
	{
		s = 0.5 / sqrt(sum_z);
		quat.x = (m[8] + m[2]) * s;
		quat.y = (m[6] + m[9]) * s;
		quat.z = 0.25 / s;
		quat.w = (m[1] - m[4]) * s;
	}

	/* correct.. but above version is more stable
	// check the diagonal
	if ( tr > 1e-15 )
	{
			s = 0.5 / sqrt(tr);
			quat.x = (m[6] - m[9]) * s;
			quat.y = (m[8] - m[2]) * s;
			quat.z = (m[1] - m[4]) * s;
			quat.w = 0.25 / s;
	}
	else
	{
			if ( m[0] > m[5] && m[0] > m[10] )
			{
				s = 1.0 / ( sqrt( 1.0 + m[0] - m[5] - m[10] ) * 2 );

				quat.x = 0.25 / s;
				quat.y = (m[1] + m[4] ) * s;
				quat.z = (m[8] + m[2] ) * s;
				quat.w = (m[6] - m[9] ) * s;
			}
			else if ( m[5] > m[10] )
			{
				s = 1.0 / ( sqrt( 1.0 + m[5] - m[0] - m[10] ) * 2 );

				quat.x = (m[1] + m[4] ) * s;
				quat.y = 0.25 / s;
				quat.z = (m[6] + m[9] ) * s;
				quat.w = (m[8] - m[2] ) * s;
			}
			else
			{
				s = 1.0 / ( sqrt( 1.0 + m[10] - m[0] - m[5] ) * 2 );

				quat.x = (m[8] + m[2] ) * s;
				quat.y = (m[6] + m[9] ) * s;
				quat.z = 0.25 / s;
				quat.w = (m[1] - m[4] ) * s;
			}
	}

	*/

	//xQuatNorm( quat, quat );

	// The original AMD version seems to be incorrect

	/*tr = m[0] + m[5] + m[10];

	// check the diagonal
	if (tr > 0.0)
	{
			s = (qreal)sqrt (tr + 1.0f);
			quat.w = s * 0.5f;
			s = 0.5f / s;
			quat.x = (m[6] - m[9]) * s;
			quat.y = (m[8] - m[2]) * s;
			quat.z = (m[1] - m[4]) * s;
	}
	else
	{
			if (m[5] > m[0] && m[10] <= m[5])
			{
					s = (qreal)sqrt ((m[5] - (m[10] + m[0])) + 1.0f);

					quat.x = s * 0.5f;

					if (s != 0.0)
							s = 0.5f / s;

					quat.z = (m[8] - m[2]) * s;
					quat.y = (m[6] + m[9]) * s;
					quat.w = (m[4] + m[1]) * s;
			}
			else if ((m[5] <= m[0]  &&  m[10] > m[0])  ||  (m[10] > m[5]))
			{
					s = (qreal)sqrt ((m[10] - (m[0] + m[5])) + 1.0f);

					quat.y = s * 0.5f;

					if (s != 0.0)
							s = 0.5f / s;

					quat.z = (m[1] - m[4]) * s;
					quat.w = (m[8] + m[2]) * s;
					quat.x = (m[9] + m[6]) * s;
			}
			else
			{
					s = (qreal)sqrt ((m[0] - (m[5] + m[10])) + 1.0f);

					quat.w = s * 0.5f;

					if (s != 0.0)
							s = 0.5f / s;

					quat.z = (m[6] - m[9]) * s;
					quat.x = (m[1] + m[4]) * s;
					quat.y = (m[2] + m[8]) * s;
			}
	}*/
}


/******************************************************************************
Routine:   quat2D3DMat
Input:     quat - quaternion to convert
						m    - matrix (4x4) to fill
Output:    m    = rotation matrix from quat
******************************************************************************/
/*void quat2D3DMat (D3DMATRIX *m, const QuatT *quat)
{
		qreal wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

		// calculate coefficients
		x2 = quat.x + quat.x;
		y2 = quat.y + quat.y; 
		z2 = quat.z + quat.z;

		xx = quat.x * x2;   xy = quat.x * y2;   xz = quat.x * z2;
		yy = quat.y * y2;   yz = quat.y * z2;   zz = quat.z * z2;
		wx = quat.w   * x2;   wy = quat.w   * y2;   wz = quat.w   * z2;

		m->_11 = 1.0f - yy - zz;
		m->_12 = xy - wz;
		m->_13 = xz + wy;
		m->_21 = xy + wz;
		m->_22 = 1.0f - xx - zz;
		m->_23 = yz - wx;
		m->_31 = xz - wy;
		m->_32 = yz + wx;
		m->_33 = 1.0f - xx - yy;

		m->_14 =
		m->_24 =
		m->_34 =
		m->_41 =
		m->_42 =
		m->_43 = 0.0f;
		m->_44 = 1.0f;
}*/

/******************************************************************************
Routine:   quat2glMat
Input:     quat - quaternion to convert
						m    - matrix (4x4) to fill
Output:    m    = rotation matrix from quat
******************************************************************************/
void xQuat2Matf ( float *quat, float *m )
{
	qreal wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	Quatf iq(quat[1], quat[2], quat[3], quat[0]);

	// calculate coefficients
	x2 = iq.x + iq.x;
	y2 = iq.y + iq.y; 
	z2 = iq.z + iq.z;

	xx = iq.x * x2;   xy = iq.x * y2;   xz = iq.x * z2;
	yy = iq.y * y2;   yz = iq.y * z2;   zz = iq.z * z2;
	wx = iq.w * x2;   wy = iq.w * y2;   wz = iq.w * z2;

	m[0] = 1.0f - yy - zz;
	m[1] = xy + wz;
	m[2] = xz - wy;

	m[4] = xy - wz;
	m[5] = 1.0f - xx - zz;
	m[6] = yz + wx;

	m[8] = xz + wy;
	m[9] = yz - wx;
	m[10] = 1.0f - xx - yy;

	m[3] =
	m[7] =
	m[11] =
	m[12] =
	m[13] =
	m[14] = 0.0f;
	m[15] = 1.0f;
}

/******************************************************************************
Routine:   quat2glMat
Input:     quat - quaternion to convert
						m    - matrix (4x4) to fill
Output:    m    = rotation matrix from quat
******************************************************************************/
void xQuat2Matd ( float *quat, double *m )
{
	/*
	float W = quat[0];
	float X = quat[1];
	float Y = quat[2];
	float Z = quat[3];

	float xx      = X * X;
	float xy      = X * Y;
	float xz      = X * Z;
	float xw      = X * W;
	float yy      = Y * Y;
	float yz      = Y * Z;
	float yw      = Y * W;
	float zz      = Z * Z;
	float zw      = Z * W;
	m[0]  = 1 - 2 * ( yy + zz );
	m[4]  =     2 * ( xy - zw );
	m[8]  =     2 * ( xz + yw );
	m[1]  =     2 * ( xy + zw );
	m[5]  = 1 - 2 * ( xx + zz );
	m[9]  =     2 * ( yz - xw );
	m[2]  =     2 * ( xz - yw );
	m[6]  =     2 * ( yz + xw );
	m[10] = 1 - 2 * ( xx + yy );
	m[3]  = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
	m[15] = 1;
	*/

	qreal wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	Quatf iq(quat[1], quat[2], quat[3], quat[0]);

	// calculate coefficients
	x2 = iq.x + iq.x;
	y2 = iq.y + iq.y; 
	z2 = iq.z + iq.z;

	xx = iq.x * x2;   xy = iq.x * y2;   xz = iq.x * z2;
	yy = iq.y * y2;   yz = iq.y * z2;   zz = iq.z * z2;
	wx = iq.w * x2;   wy = iq.w * y2;   wz = iq.w * z2;

	m[0] = 1.0f - yy - zz;
	m[1] = xy + wz;
	m[2] = xz - wy;

	m[4] = xy - wz;
	m[5] = 1.0f - xx - zz;
	m[6] = yz + wx;

	m[8] = xz + wy;
	m[9] = yz - wx;
	m[10] = 1.0f - xx - yy;

	m[3] =
	m[7] =
	m[11] =
	m[12] =
	m[13] =
	m[14] = 0.0f;
	m[15] = 1.0f; 
}


// force instantiation of both quat types
/*void xQuat2MatLINK()
{
	float mfloat[16];
	double mdouble[16];
	QuatT q;
	xQuat2Mat( mfloat, q );
	xQuat2Mat( mdouble, q );
}*/
//typedef xQuat2MatT<float> xQuat2Matf;


/******************************************************************************
Routine:   euler2quat
Input:     rot  - qreal address, <pitch, yaw, roll> angles
						quat - quaternion address
Output:    quat = rotation specified by 'rot'
******************************************************************************/
void xEuler2Quat (QuatT& quat, const qreal *rot)
{
	qreal csx[2],csy[2],csz[2],cc,cs,sc,ss, cy, sy;

	// Convert angles to radians/2, construct the quat axes
	if (rot[0] == 0.0f)
	{
			csx[0] = 1.0f;
			csx[1] = 0.0f;
	}
	else
	{
			const qreal deg = rot[0] * 0.5;
			csx[0] = (qreal)cosf (deg);
			csx[1] = (qreal)sinf (deg);
	}

	if (rot[2] == 0.0f)
	{
			cc = csx[0];
			ss = 0.0f;
			cs = 0.0f;
			sc = csx[1];
	}
	else
	{
			const qreal deg = rot[2] * 0.5;
			csz[0] = (qreal)cosf (deg);
			csz[1] = (qreal)sinf (deg);
			cc = csx[0] * csz[0];
			ss = csx[1] * csz[1];
			cs = csx[0] * csz[1];
			sc = csx[1] * csz[0];
	}

	if (rot[1] == 0.0f)
	{
			quat.x = sc;
			quat.y = ss;
			quat.z = cs;
			quat.w   = cc;
	}
	else
	{
			const qreal deg = rot[1] * 0.5;
			cy = csy[0] = (qreal)cosf (deg);
			sy = csy[1] = (qreal)sinf (deg);
			quat.x = (cy * sc) - (sy * cs);
			quat.y = (cy * ss) + (sy * cc);
			quat.z = (cy * cs) - (sy * sc);
			quat.w   = (cy * cc) + (sy * ss);
	}

	// should be normal, if sin and cos are accurate enough
}


/******************************************************************************
Routine:   euler2quat2
Input:     rot  - qreal address, <pitch, yaw, roll> angles
						quat - quaternion address
Output:    quat = rotation specified by 'rot'
Comment:   construct a quaternion by applying the given euler angles
						in X-Y-Z order (pitch-yaw-roll). Less efficient than euler2quat(),
						but more easily modified for different rotation orders.
						Note that euler2quat() was derived by manually inlining the
						mult_quat calls, factoring out zero terms, and working some
						basic (but extensive) algebra on the results.  If you find that
						you need a converter that does not use XYZ order, it might be
						worth taking the time to craft a custom version using a similar
						technique.
******************************************************************************/
void xEuler2Quat2 (QuatT& quat, const qreal *rot)
{
	QuatT qx, qy, qz, qf;
	qreal   deg;

	// Convert angles to radians (and half-angles), and compute partial quats
	deg = rot[0] * 0.5f;
	qx.x = (qreal)sinf (deg);
	qx.y = 0.0f; 
	qx.z = 0.0f; 
	qx.w   = (qreal)cosf (deg);

	deg = rot[1] * 0.5f;
	qy.x = 0.0f; 
	qy.y = (qreal)sinf (deg);
	qy.z = 0.0f;
	qy.w   = (qreal)cosf (deg);

	deg = rot[2] * 0.5f;
	qz.x = 0.0f;
	qz.y = 0.0f;
	qz.z = (qreal)sinf (deg);
	qz.w   = (qreal)cosf (deg);

	xQuatMul (qf, qy, qx);
	xQuatMul (quat, qz, qf);

	// should be normal, if sin/cos and quat_mult are accurate enough
}


/******************************************************************************
Routine:   quat2axis_angle
Input:     quat      - quaternion address
						axisAngle - qreal[4] <x,y,z,angle>
Output:    axisAngle representation of 'quat'
******************************************************************************/
void xQuat2Axis_Angle (qreal *axisAngle, const QuatT& quat)
{
	qreal cosA = quat.w;
	qreal sinA = sqrt( 1 - cosA * cosA );
	if ( fabs( sinA ) < 0.00005 ) sinA = 1;
	qreal scale = 1.0 / sinA;
	axisAngle[0] = quat.x * scale;
	axisAngle[1] = quat.y * scale;
	axisAngle[2] = quat.z * scale;
	axisAngle[3] = acos( cosA ) * 2;
	/*
		const qreal tw = (qreal)acos (quat.w);
		const qreal scale = (qreal)(1.0 / sin (tw));

		axisAngle[3] = tw * 2.0;
		axisAngle[0] = quat.x * scale;
		axisAngle[1] = quat.y * scale;
		axisAngle[2] = quat.z * scale;
		*/
}


/******************************************************************************
Routine:   _axis_angle2quat
Input:     axisAngle - axis address (1x4 of <x,y,z,radians>)
						quat      - quaternion address
Output:    quat      = rotation around given axis
******************************************************************************/
void xAxis_Angle2Quat (QuatT& quat, const qreal *axisAngle)
{
	//const qreal deg = axisAngle[3] / (2.0f * RAD2DEG);
	const qreal deg = axisAngle[3] * 0.5;
	const qreal cs = (qreal)cosf (deg);
	quat.w = (qreal)sinf (deg);
	quat.x = cs * axisAngle[0];
	quat.y = cs * axisAngle[1];
	quat.z = cs * axisAngle[2];
}


#define DELTA   0.0001      // DIFFERENCE AT WHICH TO LERP INSTEAD OF SLERP
/******************************************************************************
Routine:   _slerp_quat
Input:     quat1  - quaternion address
						quat2  - quaternion address
						slerp  - qreal interp factor (0.0 = quat1, 1.0 = quat2)
						result - quaternion address
Output:    result = quaternion spherically interpolate between q1 and q2
******************************************************************************/
void xSlerpQuat (QuatT& result,
								const QuatT& quat1, const QuatT& quat2,
								qreal slerp)
{
		double omega, cosom, isinom;
		qreal scale0, scale1;
		qreal q2x, q2y, q2z, q2w;

		// DOT the quats to get the cosine of the angle between them
		cosom = quat1.x * quat2.x +
						quat1.y * quat2.y +
						quat1.z * quat2.z +
						quat1.w * quat2.w;

		// Two special cases:
		// Quats are exactly opposite, within DELTA?
		if (cosom > DELTA - 1.0)
		{
				// make sure they are different enough to avoid a divide by 0
				if (cosom < 1.0 - DELTA)
				{
						// SLERP away
						omega = acos (cosom);
						isinom = 1.0 / sinf (omega);
						scale0 = (qreal)(sinf ((1.0 - slerp) * omega) * isinom);
						scale1 = (qreal)(sinf (slerp * omega) * isinom);
				}
				else
				{
						// LERP is good enough at this distance
						scale0 = 1.0f - slerp;
						scale1 = slerp;
				}

				q2x = quat2.x * scale1;
				q2y = quat2.y * scale1;
				q2z = quat2.z * scale1;
				q2w = quat2.w   * scale1;
		}
		else
		{
				// SLERP towards a perpendicular quat
				// Set slerp parameters
				scale0 = (qreal)sinf ((1.0f - slerp) * HALF_PI);
				scale1 = (qreal)sinf (slerp * HALF_PI);

				q2x = -quat2.y * scale1;
				q2y =  quat2.x * scale1;
				q2z = -quat2.w * scale1;
				q2w =  quat2.z * scale1;
		}

		// Compute the result
		result.x = scale0 * quat1.x + q2x;
		result.y = scale0 * quat1.y + q2y;
		result.z = scale0 * quat1.z + q2z;
		result.w = scale0 * quat1.w   + q2w;
}


/******************************************************************************
Routine:   _trans_quat
Input:     result - vector 3x1 address
						q      - quaternion address
						v      - vector 3x1 address
Output:    d      = 'a' rotated with 'b'
Comments:  Note that this is equivalent to using quat2mat to make a rotation
						matrix, and then multiplying the vector by the matrix.  This form
						is more compact, and equally efficient when only transforming a
						single vector.  For other cases, it is advisable to construct
						a rotation matrix.
******************************************************************************/
void xTransQuat ( Vec3T<qreal> &result, const Vec3T<qreal> &v, const QuatT& q)
{
	// result = av + bq + c(q.v CROSS v)
	// where
	//  a = q.w^2 - (q.v DOT q.v)
	//  b = 2 * (q.v DOT v)
	//  c = 2q.w
	qreal   w = q.w;   // just a convenience name
	qreal   a = w * w - (q.x * q.x + q.y * q.y + q.z * q.z);
	qreal   b = 2.0f * (q.x * v.x   + q.y * v.y   + q.z * v.z);
	qreal   c = 2.0f * w;

	// Must store this, because result may alias v
	qreal cross[3]; // q.v CROSS v
	cross[0] = q.y * v.z - q.z * v.y;
	cross[1] = q.z * v.x - q.x * v.z;
	cross[2] = q.x * v.y - q.y * v.x;

	result.x = a * v.x + b * q.x + c * cross[0];
	result.y = a * v.y + b * q.y + c * cross[1];
	result.z = a * v.z + b * q.z + c * cross[2];
}


// eof - quat_lib.c
