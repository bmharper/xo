#pragma once

#include "Vec3.h"
#include "Vec4.h"
#include "Mat3.h"

#ifndef DEFINED_Mat4
#define DEFINED_Mat4
#include "VecDef.h"

template <class FT>
class Mat4T
{
public:
	VecBase4T<FT> row[4];

	Mat4T(void)
	{
		Identity();
	}

	// Don't initialize to identity
	Mat4T( int uninit )
	{
	}

	Mat4T(	FT a0, FT b0, FT c0, FT d0,
			FT a1, FT b1, FT c1, FT d1,
			FT a2, FT b2, FT c2, FT d2,
			FT a3, FT b3, FT c3, FT d3)
	{
		XX = a0;
		XY = b0;
		XZ = c0;
		XW = d0;

		YX = a1;
		YY = b1;
		YZ = c1;
		YW = d1;

		ZX = a2;
		ZY = b2;
		ZZ = c2;
		ZW = d2;

		WX = a3;
		WY = b3;
		WZ = c3;
		WW = d3;
	}


	FT&	m(int i, int j)			{ return row[i][j]; }
	FT	m(int i, int j) const	{ return row[i][j]; }

	FT Determinant() const;

	/// Returns the diagonal
	Vec4T<FT> Diagonal() const { return Vec4T<FT>( row[0].x, row[1].y, row[2].z, row[3].w ); }

	Mat4T Transposed(void) const {
		Mat4T t;
		t.XX = XX;
		t.YX = XY;
		t.ZX = XZ;
		t.WX = XW;
					
		t.XY = YX;
		t.YY = YY;
		t.ZY = YZ;
		t.WY = YW;
					
		t.XZ = ZX;
		t.YZ = ZY;
		t.ZZ = ZZ;
		t.WZ = ZW;
					
		t.XW = WX;
		t.YW = WY;
		t.ZW = WZ;
		t.WW = WW;
		return t;
	} 

	Mat4T Inverted() const {
		Mat4T mr;
		mr = *this;
		mr.Invert();
		return mr;
	}

	void Zero() 
	{
		XX = 0;
		XY = 0;
		XZ = 0;
		XW = 0;
		
		YX = 0;
		YY = 0;
		YZ = 0;
		YW = 0;
		
		ZX = 0;
		ZY = 0;
		ZZ = 0;
		ZW = 0;
		
		WX = 0;
		WY = 0;
		WZ = 0;
		WW = 0;
	}

	void Identity()
	{
		XX = 1;
		XY = 0;
		XZ = 0;
		XW = 0;
		
		YX = 0;
		YY = 1;
		YZ = 0;
		YW = 0;
		
		ZX = 0;
		ZY = 0;
		ZZ = 1;
		ZW = 0;
		
		WX = 0;
		WY = 0;
		WZ = 0;
		WW = 1;
	}


	Mat4T operator*(const FT d) const
	{
		Mat4T b;
		b.XX = XX * d;
		b.XY = XY * d;
		b.XZ = XZ * d;
		b.XW = XW * d;
		
		b.YX = YX * d;
		b.YY = YY * d;
		b.YZ = YZ * d;
		b.YW = YW * d;
		
		b.ZX = ZX * d;
		b.ZY = ZY * d;
		b.ZZ = ZZ * d;
		b.ZW = ZW * d;
		
		b.WX = WX * d;
		b.WY = WY * d;
		b.WZ = WZ * d;
		b.WW = WW * d;
		
		return b;
	}

	void Translate( const Vec3T<FT>& vec, bool post = true ) 
	{
		Translate(vec.x, vec.y, vec.z, post);
	}

	void Translate( double x, double y, double z, bool post = true ) 
	{
		Mat4T tm;
		tm.XW = x;
		tm.YW = y;
		tm.ZW = z;
		if (post)
			*this = (*this) * tm;
		else
			*this = tm * (*this);
	}

	void Scale( const Vec3T<FT>& vec, bool post = true ) 
	{
		Scale(vec.x, vec.y, vec.z, post);
	}

	// equivalent to a glScale3d
	void Scale( FT x, FT y, FT z, bool post = true ) 
	{
		Mat4T m;
		m.XX = x;
		m.YY = y;
		m.ZZ = z;
		if (post)
			*this = (*this) * m;
		else
			*this = m * (*this);
	}

	// equivalent to glRotated(), except RADIANS
	void Rotate( FT angle, FT x, FT y, FT z, bool post = true )
	{
		Vec3 v(x,y,z);
		v.normalize();
		x = v.x;
		y = v.y;
		z = v.z;
		Mat4T r;
		FT c = cos(angle);
		FT s = sin(angle);
		FT cm1 = 1 - c;
		// row 0
		r.XX = x*x*cm1 + c;
		r.XY = x*y*cm1 - z*s;
		r.XZ = x*z*cm1 + y*s;
		r.XW = 0;
		// row 1
		r.YX = y*x*cm1 + z*s;
		r.YY = y*y*cm1 + c;
		r.YZ = y*z*cm1 - x*s;
		r.YW = 0;
		// row 2
		r.ZX = z*x*cm1 - y*s;
		r.ZY = z*y*cm1 + x*s;
		r.ZZ = z*z*cm1 + c;
		r.ZW = 0;
		// row 3
		r.WX = 0;
		r.WY = 0;
		r.WZ = 0;
		r.WW = 1;
		if (post)
			*this = (*this) * r;
		else
			*this = r * (*this);
	}



	void Invert() 
	{
		/************************************************************
		*
		* input:
		* mat - pointer to array of 16 floats (source matrix)
		* output:
		* dst - pointer to array of 16 floats (invert matrix)
		*
		*************************************************************/
		//void Invert2( float *mat, float *dst)
		//Streaming SIMD Extensions - Inverse of 4x4 Matrix
		//7
		//{
		FT dst[16];
		FT tmp[12]; /* temp array for pairs */
		FT src[16]; /* array of transpose source matrix */
		FT det; /* determinant */
		/* transpose matrix */
		/*for ( int i = 0; i < 4; i++) {
			src[i] = row[i].x;
			src[i + 4] = row[i].y;
			src[i + 8] = row[i].z;
			src[i + 12] = row[i].w;
		}*/
		// transpose
		src[0] = XX;
		src[1] = YX;
		src[2] = ZX;
		src[3] = WX;

		src[4] = XY;
		src[5] = YY;
		src[6] = ZY;
		src[7] = WY;

		src[8] = XZ;
		src[9] = YZ;
		src[10] = ZZ;
		src[11] = WZ;

		src[12] = XW;
		src[13] = YW;
		src[14] = ZW;
		src[15] = WW;

		/* calculate pairs for first 8 elements (cofactors) */
		tmp[0] = src[10] * src[15];
		tmp[1] = src[11] * src[14];
		tmp[2] = src[9] * src[15];
		tmp[3] = src[11] * src[13];
		tmp[4] = src[9] * src[14];
		tmp[5] = src[10] * src[13];
		tmp[6] = src[8] * src[15];
		tmp[7] = src[11] * src[12];
		tmp[8] = src[8] * src[14];
		tmp[9] = src[10] * src[12];
		tmp[10] = src[8] * src[13];
		tmp[11] = src[9] * src[12];
		/* calculate first 8 elements (cofactors) */
		dst[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
		dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
		dst[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
		dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
		dst[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
		dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
		dst[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
		dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
		dst[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
		dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
		dst[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
		dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
		dst[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
		dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
		dst[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
		dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
		/* calculate pairs for second 8 elements (cofactors) */
		tmp[0] = src[2]*src[7];
		tmp[1] = src[3]*src[6];
		tmp[2] = src[1]*src[7];
		tmp[3] = src[3]*src[5];
		tmp[4] = src[1]*src[6];
		tmp[5] = src[2]*src[5];
		tmp[6] = src[0]*src[7];
		tmp[7] = src[3]*src[4];
		tmp[8] = src[0]*src[6];
		tmp[9] = src[2]*src[4];
		tmp[10] = src[0]*src[5];
		tmp[11] = src[1]*src[4];
		/* calculate second 8 elements (cofactors) */
		dst[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
		dst[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
		dst[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
		dst[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
		dst[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
		dst[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
		dst[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
		dst[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
		dst[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
		dst[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
		dst[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
		dst[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
		dst[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
		dst[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
		dst[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
		dst[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
		/* calculate determinant */
		det=src[0]*dst[0]+src[1]*dst[1]+src[2]*dst[2]+src[3]*dst[3];
		/* calculate matrix inverse */
		det = 1/det;
		for ( int j = 0; j < 16; j++) {
			dst[j] *= det;
		}
		memcpy((void*) this, dst, 16*sizeof(FT));
	}

#ifndef NO_XSTRING
	XString ToString() const
	{
		XString s;
		s.Format(_T("[ %f %f %f %f;\n  %f %f %f %f;\n  %f %f %f %f;\n  %f %f %f %f ]"), 
							XX, XY, XZ, XW,
							YX, YY, YZ, YW,
							ZX, ZY, ZZ, ZW,
							WX, WY, WZ, WW );
		return s;
	}
#endif

	/*friend double Difference( const Mat4T<FT>& a, const Mat4T<FT>& b );

	friend Mat4T operator - (const Mat4T& a);         // -m1
	friend Mat4T operator + (const Mat4T& a, const Mat4T& b);		// m1 + m2
	friend Mat4T operator - (const Mat4T& a, const Mat4T& b);		// m1 - m2
	friend Mat4T operator * (const Mat4T& b, const Mat4T& a);		// m1 * m2
	friend Mat4T operator * (const Mat4T& a, const FT d);			// m1 * d
	friend Mat4T operator * (const FT d, const Mat4T& a);			// d * m1
	friend Mat4T operator / (const Mat4T& a, const FT d);			// m1 / d
	friend int operator == (const Mat4T& a, const Mat4T& b);		// m1 == m2 ?
	friend int operator != (const Mat4T& a, const Mat4T& b);		// m1 != m2 ?
	//friend Vec4T<FT> operator * ( const Vec4T<FT>& v, const Mat4T& a );    // v * m1
	//friend Vec4T<FT> operator * ( const Mat4T& a, const Vec4T<FT>& v );    // m1 * v */

};

template <class FT>
INLINE FT Mat4T<FT>::Determinant() const
{
	FT sub1 = YY * (ZZ * WW - ZW * WZ) -
						YZ * (ZY * WW - ZW * WY) +
						YW * (ZY * WZ - ZZ * WY);
	
	FT sub2 = YX * (ZZ * WW - ZW * WZ) -
						YZ * (ZX * WW - ZW * WX) +
						YW * (ZX * WZ - ZZ * WX);
	
	FT sub3 = YX * (ZY * WW - ZW * WY) -
						YY * (ZX * WW - ZW * WX) +
						YW * (ZX * WY - ZY * WX);
		
	FT sub4 = YX * (ZY * WZ - ZZ * WY) -
						YY * (ZX * WZ - ZZ * WX) +
						YZ * (ZX * WY - ZY * WX);
	FT det = XX * sub1 - XY * sub2 + XZ * sub3 - XW * sub4;
	return det;
}

template <class FT>
INLINE Mat4T<FT> operator + (const Mat4T<FT>& a, const Mat4T<FT>& b)
{ 
	Mat4T<FT> c;
	
	c.XX = a.XX + b.XX;
	c.XY = a.XY + b.XY;
	c.XZ = a.XZ + b.XZ;
	c.XW = a.XW + b.XW;
	
	c.YX = a.YX + b.YX;
	c.YY = a.YY + b.YY;
	c.YZ = a.YZ + b.YZ;
	c.YW = a.YW + b.YW;
	
	c.ZX = a.ZX + b.ZX;
	c.ZY = a.ZY + b.ZY;
	c.ZZ = a.ZZ + b.ZZ;
	c.ZW = a.ZW + b.ZW;
	
	c.WX = a.WX + b.WX;
	c.WY = a.WY + b.WY;
	c.WZ = a.WZ + b.WZ;
	c.WW = a.WW + b.WW;
	
	return c; 
}


template <class FT>
INLINE Mat4T<FT> operator - (const Mat4T<FT>& a, const Mat4T<FT>& b)
{ 
		Mat4T<FT> c;
		
		c.XX = a.XX - b.XX;
		c.XY = a.XY - b.XY;
		c.XZ = a.XZ - b.XZ;
		c.XW = a.XW - b.XW;
		
		c.YX = a.YX - b.YX;
		c.YY = a.YY - b.YY;
		c.YZ = a.YZ - b.YZ;
		c.YW = a.YW - b.YW;
		
		c.ZX = a.ZX - b.ZX;
		c.ZY = a.ZY - b.ZY;
		c.ZZ = a.ZZ - b.ZZ;
		c.ZW = a.ZW - b.ZW;
		
		c.WX = a.WX - b.WX;
		c.WY = a.WY - b.WY;
		c.WZ = a.WZ - b.WZ;
		c.WW = a.WW - b.WW;
		
		return c;
}


template <class FT>
INLINE Mat4T<FT> operator * (const Mat4T<FT>& b, const Mat4T<FT>& a) 
{
	Mat4T<FT> c;
	
	c.XX = a.XX * b.XX + a.YX * b.XY + a.ZX * b.XZ + a.WX * b.XW;
	c.XY = a.XY * b.XX + a.YY * b.XY + a.ZY * b.XZ + a.WY * b.XW;
	c.XZ = a.XZ * b.XX + a.YZ * b.XY + a.ZZ * b.XZ + a.WZ * b.XW;
	c.XW = a.XW * b.XX + a.YW * b.XY + a.ZW * b.XZ + a.WW * b.XW;
	
	c.YX = a.XX * b.YX + a.YX * b.YY + a.ZX * b.YZ + a.WX * b.YW;
	c.YY = a.XY * b.YX + a.YY * b.YY + a.ZY * b.YZ + a.WY * b.YW;
	c.YZ = a.XZ * b.YX + a.YZ * b.YY + a.ZZ * b.YZ + a.WZ * b.YW;
	c.YW = a.XW * b.YX + a.YW * b.YY + a.ZW * b.YZ + a.WW * b.YW;
	
	c.ZX = a.XX * b.ZX + a.YX * b.ZY + a.ZX * b.ZZ + a.WX * b.ZW;
	c.ZY = a.XY * b.ZX + a.YY * b.ZY + a.ZY * b.ZZ + a.WY * b.ZW;
	c.ZZ = a.XZ * b.ZX + a.YZ * b.ZY + a.ZZ * b.ZZ + a.WZ * b.ZW;
	c.ZW = a.XW * b.ZX + a.YW * b.ZY + a.ZW * b.ZZ + a.WW * b.ZW;

	c.WX = a.XX * b.WX + a.YX * b.WY + a.ZX * b.WZ + a.WX * b.WW;
	c.WY = a.XY * b.WX + a.YY * b.WY + a.ZY * b.WZ + a.WY * b.WW;
	c.WZ = a.XZ * b.WX + a.YZ * b.WY + a.ZZ * b.WZ + a.WZ * b.WW;
	c.WW = a.XW * b.WX + a.YW * b.WY + a.ZW * b.WZ + a.WW * b.WW;                                                                  
	
	return c;
}



template <class FT>
INLINE Mat4T<FT> operator * (const Mat4T<FT>& a, const FT d)
{ 
	Mat4T<FT> b;
	b.XX = a.XX * d;
	b.XY = a.XY * d;
	b.XZ = a.XZ * d;
	b.XW = a.XW * d;

	b.YX = a.YX * d;
	b.YY = a.YY * d;
	b.YZ = a.YZ * d;
	b.YW = a.YW * d;

	b.ZX = a.ZX * d;
	b.ZY = a.ZY * d;
	b.ZZ = a.ZZ * d;
	b.ZW = a.ZW * d;

	b.WX = a.WX * d;
	b.WY = a.WY * d;
	b.WZ = a.WZ * d;
	b.WW = a.WW * d;

	return b;
}

template <class FT>
INLINE Mat4T<FT> operator * (const FT d, const Mat4T<FT>& a)
{ return a*d; }


template <class FT>
INLINE Vec3T<FT> operator * (const Vec3T<FT>& v3, const Mat4T<FT>& m4)
{
	// multiply v3 as though it were v4 with w = 1
	Vec4T<FT> v4;
	Vec3T<FT> ret;
	v4.x = v3.x;
	v4.y = v3.y;
	v4.z = v3.z;
	v4.w = 1.0f;
	v4 = v4 * m4;
	v4.Homogenize();
	
	ret.x = v4.x;
	ret.y = v4.y;
	ret.z = v4.z;
	return( ret );
}

template <class FT>
INLINE Vec3T<FT> operator * (const Mat4T<FT>& m4, const Vec3T<FT>& v3)
{
	// multiply v3 as though it were v4 with w = 1
	Vec4T<FT> v4;
	Vec3T<FT> ret;
	v4.x = v3.x;
	v4.y = v3.y;
	v4.z = v3.z;
	v4.w = 1.0f;
	v4 = m4 * v4;
	v4.Homogenize();
	
	ret.x = v4.x;
	ret.y = v4.y;
	ret.z = v4.z;
	return( ret );
}

template <class FT>
INLINE Vec4T<FT> operator * ( const Vec4T<FT>& v, const Mat4T<FT>& a )
{
	return Vec4T<FT> (
		v.x * a.XX + v.y * a.YX + v.z * a.ZX + v.w * a.WX,
		v.x * a.XY + v.y * a.YY + v.z * a.ZY + v.w * a.WY,
		v.x * a.XZ + v.y * a.YZ + v.z * a.ZZ + v.w * a.WZ,
		v.x * a.XW + v.y * a.YW + v.z * a.ZW + v.w * a.WW );
}

template <class FT>
INLINE Vec4T<FT> operator * ( const Mat4T<FT>& a, const Vec4T<FT>& v )
{
	return Vec4T<FT> (
		v.x * a.XX + v.y * a.XY + v.z * a.XZ + v.w * a.XW,
		v.x * a.YX + v.y * a.YY + v.z * a.YZ + v.w * a.YW,
		v.x * a.ZX + v.y * a.ZY + v.z * a.ZZ + v.w * a.ZW,
		v.x * a.WX + v.y * a.WY + v.z * a.WZ + v.w * a.WW );
}



template <class FT>
INLINE Mat4T<FT> operator / (const Mat4T<FT>& a, const FT d)
{
	Mat4T<FT> b;
	FT u = 1 / d;
	b.XX = a.XX * u;
	b.XY = a.XY * u;
	b.XZ = a.XZ * u;
	b.XW = a.XW * u;

	b.YX = a.YX * u;
	b.YY = a.YY * u;
	b.YZ = a.YZ * u;
	b.YW = a.YW * u;

	b.ZX = a.ZX * u;
	b.ZY = a.ZY * u;
	b.ZZ = a.ZZ * u;
	b.ZW = a.ZW * u;

	b.WX = a.WX * u;
	b.WY = a.WY * u;
	b.WZ = a.WZ * u;
	b.WW = a.WW * u;

	return b;
}

template <class FT>
INLINE bool operator == (const Mat4T<FT>& a, const Mat4T<FT>& b)
{
	// I don't care about the IEEE == semantics
	return memcmp( &a, &b, sizeof(a) ) == 0;
	/*
	return (
			(b.XX == a.XX) &&
			(b.XY == a.XY) &&
			(b.XZ == a.XZ) &&
			(b.XW == a.XW) &&
			
			(b.YX == a.YX) &&
			(b.YY == a.YY) &&
			(b.YZ == a.YZ) &&
			(b.YW == a.YW) &&
			
			(b.ZX == a.ZX) &&
			(b.ZY == a.ZY) &&
			(b.ZZ == a.ZZ) &&
			(b.ZW == a.ZW) &&
			
			(b.WX == a.WX) &&
			(b.WY == a.WY) &&
			(b.WZ == a.WZ) &&
			(b.WW == a.WW));
			*/
}

template <class FT>
INLINE bool operator != (const Mat4T<FT>& a, const Mat4T<FT>& b)
{ return !(a == b); }

// sum of component absolute differences
template <class FT>
double Difference( const Mat4T<FT>& a, const Mat4T<FT>& b )
{
	double d = 0;
	FT *ap = (FT*) &a;
	FT *bp = (FT*) &b;
	for (int i = 0; i < 16; i++)
		d += fabs(ap[i] - bp[i]);
	return d;
}


typedef Mat4T<float> Mat4f;
typedef Mat4T<double> Mat4d;
typedef Mat4T<double> Mat4;

#include "VecUndef.h"
#endif // DEFINED_Mat4
