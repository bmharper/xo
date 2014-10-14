#pragma once
#ifndef DEFINED_Mat3
#define DEFINED_Mat3

#include "Vec3.h"
#include "Vec4.h"
#include "VecDef.h"

template <class FT>
class Mat3T
{
public:

	union
	{
		struct
		{
			FT	xx, xy, xz,
				yx, yy, yz,
				zx, zy, zz;
		};

		struct 
		{
			VecBase3T<FT> row[3];
		};
	};

		//double invert(Mat3T& inv);
		Mat3T transpose();
		void clear() 
		{
				row[0].x=
				row[0].y=
				row[0].z=
				
				row[1].x=
				row[1].y=
				row[1].z=
				
				row[2].x=
				row[2].y=
				row[2].z=0;
		}


		INLINE Vec3T<FT> col(int i) const {return Vec3T<FT>(row[0][i],row[1][i],row[2][i]);}


		// Constructors

		
		Mat3T() {}
		Mat3T(const Vec3T<FT>& v0, const Vec3T<FT>& v1, const Vec3T<FT>& v2);
		Mat3T(const FT m00, const FT m01, const FT m02,
				const FT m10, const FT m11, const FT m12,
				const FT m20, const FT m21, const FT m22);
		Mat3T(const FT d);
		Mat3T(const Mat3T& m);
		
		// Assignment operators
		
		Mat3T& operator  = ( const Mat3T& m );      // assignment of a Mat3T
		Mat3T& operator += ( const Mat3T& m );      // this += m
		Mat3T& operator -= ( const Mat3T& m );      // this -= m
		Mat3T& operator *= ( const FT d );      // this *= d
		Mat3T& operator /= ( const FT d );      // this /= d
		
		FT & operator ()  ( const int i, const int j){ return row[i][j]; }
		FT operator ()  ( const int i, const int j) const { return row[i][j]; }

		
		Mat3T adjoint();

		FT & m(int i, int j)  { return row[i][j]; }
		FT m(int i, int j) const  { return row[i][j]; }


		Vec3T<FT> GetViewUp();
		Vec3T<FT> GetViewNormal();
		Vec3T<FT> GetViewRight();
		
		// this = identity
		void SetIdentity();
		// this = identity
		void Identity();
		// this = transpose(this)
		void Transpose();
		// m = transpose(this)
		void GetTranspose(Mat3T & m) const;
		// this = transpose(m)
		void SetTranspose(const Mat3T & m);

		/// Returns the diagonal
		Vec3T<FT> Diagonal() const { return Vec3T<FT>( row[0].x, row[1].y, row[2].z ); }

		// this = invert(this),
		void Invert();
		// this = invert(this) for arbitrary matrix
		int InvertArbitrary();

		Mat3T Inverted() const 
		{
			Mat3T c = *this;
			c.Invert();
			return c;
		}

		void premult ( const Mat3T &M );
		void postmult ( const Mat3T &M );


		// m = inverse(this)
		void GetInverse(Mat3T & m) const;
		// m = invert(this) for arbitrary matrix
		int GetInverseArbitrary(Mat3T & m) const;
		// this = inverse(m)
		void SetInverse(const Mat3T & m);
		
		// this = inverse(m)
		int SetInverseArbitrary(const Mat3T & m);
		
		// this = 0
		void SetToZero();           // make this zero
		
		// this = rotate_x
		void SetRotateXDeg(FT degrees);
		// this = rotate_x * this
		void RotateXDeg(FT degrees, bool post = false);
		
		// this = rotate_z
		void SetRotateYDeg(FT degrees);
		// this = rotate_y * this
		void RotateYDeg(FT degrees, bool post = false);
		
		
		// this = rotate_z
		void SetRotateZDeg(FT degrees);
		// this = rotate_z * this
		void RotateZDeg(FT degrees, bool post = false);
		
		
		
		/*friend Mat3T operator ~ (const Mat3T& m);         // ~m
		friend Mat3T operator - (const Mat3T& m);         // -m
		friend Mat3T operator + (const Mat3T& m1, const Mat3T& m2);      // m1 + m2
		friend Mat3T operator - (const Mat3T& m1, const Mat3T& m2);      // m1 - m2
		friend Mat3T operator * (Mat3T& m1, Mat3T& m2);        // m1 * m2
		friend Mat3T operator * (const Mat3T& m, const FT d);      // m * d
		friend Mat3T operator * (const FT d, const Mat3T& m);      // d * m
		friend Mat3T operator / (const Mat3T& m, const FT d);      // m1 / d
		friend int operator == (const Mat3T& m1, const Mat3T& m2);      // m1 == m2 ?
		friend int operator != (const Mat3T& m1, const Mat3T& m2);      // m1 != m2 ?
		//friend ostream& operator << (ostream& s, Mat3T& m);      // output to stream
		//friend istream& operator >> (istream& s, Mat3T& m);      // input from strm.
		
		
		friend Vec3T<FT> operator * (const Mat3T& m, const Vec3T<FT>& v);      // v = m * v */



	/*
	Up
	|     Normal  
	|      /
	|    /
	|  /
	|/_________ Right

	
		
			
				
				for three points, contruct a matrix so that vpn is p1 to p2 and p3 is on the 
				Z, X plane. For Mat3T this is only the rotation part.
					
						
							
					Y          Z
					|        p2  
					|      /
					|    /
					|  /
					|/_________  X
					p1
								
						p3 is on X, Z plane some where
									
		*/
		
		// given two vectors, calculate a rotation matrix
		
		int CalcRotationMatrixFromUR(Vec3T<FT> vup, Vec3T<FT> vr); // up right
		int CalcRotationMatrixFromRN(Vec3T<FT> vr, Vec3T<FT> vpn); // right normal
		int CalcRotationMatrixFromNU(Vec3T<FT> vpn, Vec3T<FT> vup); // normal up
		
		// given the threee vectorsm normal, up and right
		// calc the rotation matrix
		void CalcRotationMatrixFromAllVectors(const Vec3T<FT> & vpn, const Vec3T<FT> &vup, const Vec3T<FT> &vr);
		// given three points in space, calc the rotation matrix
		int CalcRotationMatrixFromPoints(const Vec3T<FT> & p1, const Vec3T<FT> & p2, const Vec3T<FT> & p3);
		
		FT det() const;
		
		void xform(const Vec3T<FT> &v, Vec3T<FT> &xv) const; // (this)(v) => xv; 
		// v & xv must be distinct
		void xform(Vec3T<FT> &v) const;                  // (this)(v) => v
		
		void invXform(const Vec3T<FT> &v, Vec3T<FT> &xv) const;
		void invXform(Vec3T<FT> &v) const;
		
		
		Mat3T(const Vec3T<FT> &diag, const Vec3T<FT> &sym) {set(diag, sym);}
		// make a symmetric matrix, given the diagonal and symmetric
		// (off-diagonal) elements in canonical order
		
		//void set(const Quat &q);
		void set(const Mat3T& m);
		void set(const Vec3T<FT> &diag, const Vec3T<FT> &sym);
		
		void xpose(const Mat3T &M);                   // M^T      
		void xpose();                                // this^T
		void symmetrize(const Mat3T &M);              // M + M^T
		void symmetrize();                           // this + this^T
		
		//double  invert(const Mat3T &M);                  // M^-1      
		//double  invert();                               // this^-1
		
		void negate(const Mat3T &M);                  // -M
		void negate();                               // -this
		void add(const Mat3T &M, const Mat3T &N);      // M + N
		void add(const Mat3T &M);                     // this + M
		void sub(const Mat3T &M, const Mat3T &N);      // M - N
		void sub(const Mat3T &M);                     // this - M
		void scale(const Mat3T &M, FT s);           // s * M
		void scale(FT s);                          // s * this
		void mult(const Mat3T &M, const Mat3T &N);     // M * N     
		void mult(const Mat3T &M, bool  post = false);                 // M * this  
		//void postmult(const Mat3T &M);                // this * M  
		
		// for reading a symmetric matrix
		Vec3T<FT> diag() const {return Vec3T<FT>(XX, YY, ZZ);}
		Vec3T<FT> sym()  const {return Vec3T<FT>(YZ, ZX, XY);}
		
		// set matrix to the skew symmetric matrix corresponding to 'v X'
		void setSkew(const Vec3T<FT> &v);
		
		// for reading rows
		const Vec3T<FT> &xrow() const {return *((Vec3T<FT> *) &XX);}
		const Vec3T<FT> &yrow() const {return *((Vec3T<FT> *) &YX);}
		const Vec3T<FT> &zrow() const {return *((Vec3T<FT> *) &ZX);}
		// for writing to rows
		Vec3T<FT> &xrow()  {return *((Vec3T<FT> *) &XX);}
		Vec3T<FT> &yrow()  {return *((Vec3T<FT> *) &YX);}
		Vec3T<FT> &zrow()  {return *((Vec3T<FT> *) &ZX);}
		
		// for reading columns
		Vec3T<FT> xcol() const {return Vec3T<FT>(XX, YX, ZX);}
		Vec3T<FT> ycol() const {return Vec3T<FT>(XY, YY, ZY);}
		Vec3T<FT> zcol() const {return Vec3T<FT>(XZ, YZ, ZZ);}
		// for writing to columns
		void setXcol(const Vec3T<FT> &v);
		void setYcol(const Vec3T<FT> &v);
		void setZcol(const Vec3T<FT> &v);


		// functions for homogenous 2d coords
		void Translate( FT x, FT y, bool post = true );
		void Scale( FT x, FT y, bool post = true );
		void Rotate( FT angle, bool post = true );

		//ostream& print(ostream &os) const;
};


//INLINE ostream &operator<<(ostream &os, const Mat3T &M)  {return M.print(os);}
//#include "ulCommon.h"
//#include "ulVec4.h"
//#include "ulQuat.h"

#include <ctype.h>


/****************************************************************
*				                                                *
*	    Mat3T member functions		                              *
*				                                                *
****************************************************************/



template <class FT> INLINE
Vec3T<FT> operator * (const Mat3T<FT>& m, const Vec3T<FT>& v)
{
	return Vec3T<FT>(	m.row[0] & v,
										m.row[1] & v,
										m.row[2] & v  );
}


template <class FT> INLINE
void ulInvertMatrix(const Mat3T<FT> & a, Mat3T<FT> & b)  
{
	b(0, 0) = a(0, 0);
	b(1, 0) = a(0, 1);
	b(2, 0) = a(0, 2);
									
	b(0, 1) = a(1, 0);
	b(1, 1) = a(1, 1);
	b(2, 1) = a(1, 2);
									
	b(0, 2) = a(2, 0);
	b(1, 2) = a(2, 1);
	b(2, 2) = a(2, 2);
}

template <class FT> INLINE
void ulInvertMatrix(Mat3T<FT> & a)
{
	Mat3T<FT> b;
	ulInvertMatrix(a, b);
	a = b;

}



template <class FT> INLINE
int ulInvertMatrixArbitrary(const Mat3T<FT> &a, Mat3T<FT> & b)  
{
	const FT EPS = (FT) 1e-12;

	double fDetInv = a(0, 0) * ( a(1, 1) * a(2, 2) - a(1, 2) * a(2, 1) ) -
		a(0, 1) * ( a(1, 0) * a(2, 2) - a(1, 2) * a(2, 0) ) +
		a(0, 2) * ( a(1, 0) * a(2, 1) - a(1, 1) * a(2, 0) );
	

	if (fabs(fDetInv) < EPS)
	{
		b.SetIdentity();
		return -1;
	}
	fDetInv = 1.0 /fDetInv;
	b(0, 0) =  fDetInv * ( a(1, 1) * a(2, 2) - a(1, 2) * a(2, 1) );
	b(0, 1) = -fDetInv * ( a(0, 1) * a(2, 2) - a(0, 2) * a(2, 1) );
	b(0, 2) =  fDetInv * ( a(0, 1) * a(1, 2) - a(0, 2) * a(1, 1) );
	
	b(1, 0) = -fDetInv * ( a(1, 0) * a(2, 2) - a(1, 2) * a(2, 0) );
	b(1, 1) =  fDetInv * ( a(0, 0) * a(2, 2) - a(0, 2) * a(2, 0) );
	b(1, 2) = -fDetInv * ( a(0, 0) * a(1, 2) - a(0, 2) * a(1, 0) );
	
	b(2, 0) =  fDetInv * ( a(1, 0) * a(2, 1) - a(1, 1) * a(2, 0) );
	b(2, 1) = -fDetInv * ( a(0, 0) * a(2, 1) - a(0, 1) * a(2, 0) );
	b(2, 2) =  fDetInv * ( a(0, 0) * a(1, 1) - a(0, 1) * a(1, 0) );
	
	return 0;
}

template <class FT> INLINE
int ulInvertMatrixArbitrary(Mat3T<FT> & a)
{
	int res;
	Mat3T<FT> b;
	res = ulInvertMatrixArbitrary(a, b);
	a = b;
	return res;
}


template <class FT> INLINE
Mat3T<FT>::Mat3T(const Vec3T<FT>& v0, const Vec3T<FT>& v1, const Vec3T<FT>& v2)
{ 
	m(0, 0) = v0(0);
	m(0, 1) = v0(1);
	m(0, 2) = v0(2);
							
	m(1, 0) = v1(0);
	m(1, 1) = v1(1);
	m(1, 2) = v1(2);
							
	m(2, 0) = v2(0);
	m(2, 1) = v2(1);
	m(2, 2) = v2(2);

}

template <class FT> INLINE
Mat3T<FT>::Mat3T(const FT d)
{ 
	m(0, 0) = d;
	m(0, 1) = d;
	m(0, 2) = d;
		
	m(1, 0) = d;
	m(1, 1) = d;
	m(1, 2) = d;
		
	m(2, 0) = d;
	m(2, 1) = d;
	m(2, 2) = d;
}

template <class FT> INLINE
Mat3T<FT>::Mat3T(const Mat3T<FT>& n)
{ 
	m(0, 0) = n(0, 0);
	m(0, 1) = n(0, 1);
	m(0, 2) = n(0, 2);
		
	m(1, 0) = n(1, 0);
	m(1, 1) = n(1, 1);
	m(1, 2) = n(1, 2);
		
	m(2, 0) = n(2, 0);
	m(2, 1) = n(2, 1);
	m(2, 2) = n(2, 2);
}


// ASSIGNMENT OPERATORS

template <class FT> INLINE
Mat3T<FT>::Mat3T( const FT m00,  const FT m01,  const FT m02,
						const FT m10,  const FT m11,  const FT m12,
						const FT m20,  const FT m21,  const FT m22)
{                                            
	m(0, 0) = m00;
	m(0, 1) = m01;
	m(0, 2) = m02;
		
	m(1, 0) = m10;
	m(1, 1) = m11;
	m(1, 2) = m12;
		
	m(2, 0) = m20;
	m(2, 1) = m21;
	m(2, 2) = m22;
}


template <class FT> INLINE
Mat3T<FT>& Mat3T<FT>::operator = ( const Mat3T<FT>& n )
{
	m(0, 0) = n(0, 0);
	m(0, 1) = n(0, 1);
	m(0, 2) = n(0, 2);
		
	m(1, 0) = n(1, 0);
	m(1, 1) = n(1, 1);
	m(1, 2) = n(1, 2);
		
	m(2, 0) = n(2, 0);
	m(2, 1) = n(2, 1);
	m(2, 2) = n(2, 2);

	
	return *this; 
}

template <class FT> INLINE
Mat3T<FT>& Mat3T<FT>::operator += ( const Mat3T<FT>& n )
{
	m(0, 0) += n(0, 0);
	m(0, 1) += n(0, 1);
	m(0, 2) += n(0, 2);
		
	m(1, 0) += n(1, 0);
	m(1, 1) += n(1, 1);
	m(1, 2) += n(1, 2);
		
	m(2, 0) += n(2, 0);
	m(2, 1) += n(2, 1);
	m(2, 2) += n(2, 2);

	return *this; 
}

template <class FT> INLINE
Mat3T<FT>& Mat3T<FT>::operator -= ( const Mat3T<FT>& n )
{ 
	m(0, 0) -= n(0, 0);
	m(0, 1) -= n(0, 1);
	m(0, 2) -= n(0, 2);
		
	m(1, 0) -= n(1, 0);
	m(1, 1) -= n(1, 1);
	m(1, 2) -= n(1, 2);
		
	m(2, 0) -= n(2, 0);
	m(2, 1) -= n(2, 1);
	m(2, 2) -= n(2, 2);

	return *this; 
}

template <class FT> INLINE
Mat3T<FT>& Mat3T<FT>::operator *= ( const FT d )
{
	m(0, 0) *= d;
	m(0, 1) *= d;
	m(0, 2) *= d;
		
	m(1, 0) *= d;
	m(1, 1) *= d;
	m(1, 2) *= d;
		
	m(2, 0) *= d;
	m(2, 1) *= d;
	m(2, 2) *= d;
	
	return *this; 
}

template <class FT> INLINE
Mat3T<FT>& Mat3T<FT>::operator /= ( const FT d )
{ 
	FT u = 1/d;
	m(0, 0) *= u;
	m(0, 1) *= u;
	m(0, 2) *= u;
		
	m(1, 0) *= u;
	m(1, 1) *= u;
	m(1, 2) *= u;
		
	m(2, 0) *= u;
	m(2, 1) *= u;
	m(2, 2) *= u;
	

	return *this;
}





template <class FT> INLINE
void Mat3T<FT>::Transpose() 
{
	FT tmp;

	tmp = XY;
	XY = YX;
	YX = tmp;

	tmp = YZ;
	YZ = ZY;
	ZY = tmp;

	tmp = ZX;
	ZX = XZ;
	XZ = tmp;
}





template <class FT> INLINE
Mat3T<FT> operator - (const Mat3T<FT>& a)
{

	Mat3T<FT> b;
	b(0, 0) = -a(0, 0);
	b(0, 1) = -a(0, 1);
	b(0, 2) = -a(0, 2);

	
	b(1, 0) = -a(1, 0);
	b(1, 1) = -a(1, 1);
	b(1, 2) = -a(1, 2);

	
	b(2, 0) = -a(2, 0);
	b(2, 1) = -a(2, 1);
	b(2, 2) = -a(2, 2);

	return b;
}

template <class FT> INLINE
Mat3T<FT> operator + (const Mat3T<FT>& a, const Mat3T<FT>& b)
{ 
	Mat3T<FT> c;

	c(0, 0) = a(0, 0) + b(0, 0);
	c(0, 1) = a(0, 1) + b(0, 1);
	c(0, 2) = a(0, 2) + b(0, 2);
	
	c(1, 0) = a(1, 0) + b(1, 0);
	c(1, 1) = a(1, 1) + b(1, 1);
	c(1, 2) = a(1, 2) + b(1, 2);
	
	c(2, 0) = a(2, 0) + b(2, 0);
	c(2, 1) = a(2, 1) + b(2, 1);
	c(2, 2) = a(2, 2) + b(2, 2);

	return c;
}

template <class FT> INLINE
Mat3T<FT> operator - (const Mat3T<FT>& a, const Mat3T<FT>& b)
{ 
	Mat3T<FT> c;

	c(0, 0) = a(0, 0) - b(0, 0);
	c(0, 1) = a(0, 1) - b(0, 1);
	c(0, 2) = a(0, 2) - b(0, 2);
	
	c(1, 0) = a(1, 0) - b(1, 0);
	c(1, 1) = a(1, 1) - b(1, 1);
	c(1, 2) = a(1, 2) - b(1, 2);
	
	c(2, 0) = a(2, 0) - b(2, 0);
	c(2, 1) = a(2, 1) - b(2, 1);
	c(2, 2) = a(2, 2) - b(2, 2);
	return c;
}


template <class FT> INLINE
Mat3T<FT> operator * (const Mat3T<FT>& a, const FT d)
{ 
	Mat3T<FT> b;

	b(0, 0) = a(0, 0) * d;
	b(0, 1) = a(0, 1) * d;
	b(0, 2) = a(0, 2) * d;

	b(1, 0) = a(1, 0) * d;
	b(1, 1) = a(1, 1) * d;
	b(1, 2) = a(1, 2) * d;

	b(2, 0) = a(2, 0) * d;
	b(2, 1) = a(2, 1) * d;
	b(2, 2) = a(2, 2) * d;

	
	return b;
}


template <class FT> INLINE
Mat3T<FT> operator / (const Mat3T<FT>& a, const FT d)
{
	Mat3T<FT> b;
	double u = 1.0 / d;

	b(0, 0) = a(0, 0) * u;
	b(0, 1) = a(0, 1) * u;
	b(0, 2) = a(0, 2) * u;

	b(1, 0) = a(1, 0) * u;
	b(1, 1) = a(1, 1) * u;
	b(1, 2) = a(1, 2) * u;

	b(2, 0) = a(2, 0) * u;
	b(2, 1) = a(2, 1) * u;
	b(2, 2) = a(2, 2) * u;

	return b;
}

template <class FT> INLINE
int operator == (const Mat3T<FT>& a, const Mat3T<FT>& b)
{
	return (
		b(0, 0) == a(0, 0) &&
		b(0, 1) == a(0, 1) &&
		b(0, 2) == a(0, 2) &&
		
		b(1, 0) == a(1, 0) &&
		b(1, 1) == a(1, 1) &&
		b(1, 2) == a(1, 2) &&
		
		b(2, 0) == a(2, 0) &&
		b(2, 1) == a(2, 1) &&
		b(2, 2) == a(2, 2));

}

template <class FT> INLINE
int operator != (const Mat3T<FT>& a, const Mat3T<FT>& b)
{ return !(a == b); }

/*template <class FT> INLINE
ostream& operator << (ostream& s, Mat3T<FT>& a)
{ 
	return s << a(0, 0) << " " << a(0, 1) << " " << a(0, 2) <<'\n' \
		<< a(1, 0) << " " << a(1, 1) << " " << a(1, 2) <<'\n'
		<< a(2, 0) << " " << a(2, 1) << " " << a(2, 2) <<'\n';

}*/


template <class FT> INLINE
void Mat3T<FT>::Identity()
{
	SetIdentity();
}

template <class FT> INLINE
void Mat3T<FT>::SetIdentity()
{
	m(0, 0) = 1;
	m(0, 1) = 0;
	m(0, 2) = 0;
		
	m(1, 0) = 0;
	m(1, 1) = 1;
	m(1, 2) = 0;
		
	m(2, 0) = 0;
	m(2, 1) = 0;
	m(2, 2) = 1;

}


template <class FT> INLINE
void Mat3T<FT>::SetToZero()
{
	m(0, 0) = 0;
	m(0, 1) = 0;
	m(0, 2) = 0;

	m(1, 0) = 0;
	m(1, 1) = 0;
	m(1, 2) = 0;


	m(2, 0) = 0;
	m(2, 1) = 0;
	m(2, 2) = 0;

}



template <class FT> INLINE
void Mat3T<FT>::Invert()
{
	ulInvertMatrixArbitrary(*this);
	
}



template <class FT> INLINE
void Mat3T<FT>::GetInverse(Mat3T<FT> & a) const
{
	ulInvertMatrix(*this, a);
}

template <class FT> INLINE
void Mat3T<FT>::SetInverse(const Mat3T<FT> & a)
{
	ulInvertMatrix(a, *this);
}

template <class FT> INLINE
int Mat3T<FT>::SetInverseArbitrary(const Mat3T<FT> & a)
{
	return ulInvertMatrixArbitrary(a, *this);
}






template <class FT> INLINE
Mat3T<FT> operator ~(const Vec3T<FT>& a)
{
	return Mat3T<FT>(   0, -a.z,  a.y,  
							a.z,    0, -a.x, 
							-a.y,  a.x,    0); 

}



template <class FT> INLINE
Mat3T<FT> operator * (Mat3T<FT>& a, Mat3T<FT>& b) 
{
	Mat3T<FT> c;
	int i,j,k;

	c.SetToZero();

	for( i=0; i<3; i++ ) 
		for( j=0; j<3; j++ ) 
			for( k=0; k<3; k++ ) 
				c(i, j) += a(k, j) * b(i, k); 
			
	return c;
}


template <class FT> INLINE
void Mat3T<FT>::SetRotateXDeg(FT angle)
{
		
	FT sin_angle, cos_angle;

	SetIdentity();

	sin_angle = sinf (angle);
	cos_angle = cosf (angle);
	sin_angle = sinf (angle);
	cos_angle = cosf (angle);

	m(Y, Y) = cos_angle;
	m(Y, Z) = sin_angle;
	m(Z, Y) = -sin_angle;
	m(Z, Z) = cos_angle;
}

template <class FT> INLINE
void Mat3T<FT>::RotateXDeg(FT angle, bool post)
{
		
	Mat3T<FT> Rx;
	Rx.SetRotateXDeg(angle);

	if (post)
		*this = *this * Rx;
	else
		*this = Rx * *this;
}


template <class FT> INLINE
void Mat3T<FT>::SetRotateYDeg(FT angle)
{
	FT sin_angle, cos_angle;

	SetIdentity();
	sin_angle = sinf (angle);
	cos_angle = cosf (angle);
	m(X, X) = cos_angle;
	m(X, Z) = -sin_angle;
	m(Z, X) = sin_angle;
	m(Z, Z) = cos_angle;
}



template <class FT> INLINE
void Mat3T<FT>::RotateYDeg(FT angle, bool post)
{
	Mat3T<FT> Ry;
	Ry.SetRotateYDeg(angle);
	if (post)
		*this = *this * Ry;
	else
		*this = Ry * *this;
}



template <class FT> INLINE
void Mat3T<FT>::SetRotateZDeg(FT angle)
{
	FT sin_angle, cos_angle;

	SetIdentity();

	sin_angle = sinf (angle);
	cos_angle = cosf (angle);
	m(X, X) = cos_angle;
	m(X, Y) = sin_angle;
	m(Y, X) = -sin_angle;
	m(Y, Y) = cos_angle;
}


template <class FT> INLINE
void Mat3T<FT>::RotateZDeg(FT angle, bool post)
{

	Mat3T<FT> Rz;
	Rz.SetRotateZDeg(angle);

	if (post)
		*this = *this * Rz;
	else
		*this = Rz * *this; 
}






template <class FT> INLINE
void Mat3T<FT>::GetTranspose(Mat3T<FT> & a) const 
{

	a(0, 0) = m(0, 0);
	a(1, 0) = m(0, 1);
	a(2, 0) = m(0, 2);
								
	a(0, 1) = m(1, 0);
	a(1, 1) = m(1, 1);
	a(2, 1) = m(1, 2);
									
	a(0, 2) = m(2, 0);
	a(1, 2) = m(2, 1);
	a(2, 2) = m(2, 2);
}

template <class FT> INLINE
void Mat3T<FT>::SetTranspose(const Mat3T<FT> & a) 
{

	m(0, 0) = a(0, 0);
	m(1, 0) = a(0, 1);
	m(2, 0) = a(0, 2);
									
	m(0, 1) = a(1, 0);
	m(1, 1) = a(1, 1);
	m(2, 1) = a(1, 2);
									
	m(0, 2) = a(2, 0);
	m(1, 2) = a(2, 1);
	m(2, 2) = a(2, 2);
}                  



// p1 at 0
// p2 on z
// p3 x,y
/*
	left handed,
	z into plane
*/

template <class FT> INLINE
int Mat3T<FT>::CalcRotationMatrixFromPoints(const Vec3T<FT> & p1, const Vec3T<FT> & p2, const Vec3T<FT> & p3)
{

	Vec3T<FT> vpn, vup, vr;

	vpn[X] = p2[X] - p1[X];
	vpn[Y] = p2[Y] - p1[Y];
	vpn[Z] = p2[Z] - p1[Z];

	vr[X] = p3[X] - p1[X];
	vr[Y] = p3[Y] - p1[Y];
	vr[Z] = p3[Z] - p1[Z];


	return CalcRotationMatrixFromRN(vr, vpn);
}



template <class FT> INLINE
int Mat3T<FT>::CalcRotationMatrixFromRN(Vec3T<FT> vr, Vec3T<FT> vpn)
{
	int res;

	Vec3T<FT> vup;

	res = vup.MakeOrthonormalBasis(vr, vpn);
	if (res)
		return res;

	CalcRotationMatrixFromAllVectors(vpn, vup, vr);

	return 0;
}


template <class FT> INLINE
int Mat3T<FT>::CalcRotationMatrixFromUR(Vec3T<FT> vup, Vec3T<FT> vr)
{
	int res;
	Vec3T<FT> vpn;

	res = vpn.MakeOrthonormalBasis(vup, vr);

	CalcRotationMatrixFromAllVectors(vpn, vup, vr);
	return 0;

}

template <class FT> INLINE
int Mat3T<FT>::CalcRotationMatrixFromNU(Vec3T<FT> vpn, Vec3T<FT> vup)
{
	int res;
	Vec3T<FT> vr;

	res = vr.MakeOrthonormalBasis(vpn, vup);

	CalcRotationMatrixFromAllVectors(vpn, vup, vr);
	return 0;

}





template <class FT> INLINE
void Mat3T<FT>::CalcRotationMatrixFromAllVectors(const Vec3T<FT> & vpn, const Vec3T<FT> & vup,
																						const Vec3T<FT> & vr)
{
	m(X, UL_R) = vr[X];
	m(Y, UL_R) = vr[Y];
	m(Z, UL_R) = vr[Z];

	m(X, UL_U) = vup[X];
	m(Y, UL_U) = vup[Y];
	m(Z, UL_U) = vup[Z];

	m(X, UL_N) = vpn[X];
	m(Y, UL_N) = vpn[Y];
	m(Z, UL_N) = vpn[Z];

}




template <class FT> INLINE
void Mat3T<FT>::xform(const Vec3T<FT> &v, Vec3T<FT> &xv) const
{
	xv.x = XX * v.x + XY * v.y + XZ * v.z;
	xv.y = YX * v.x + YY * v.y + YZ * v.z;
	xv.z = ZX * v.x + ZY * v.y + ZZ * v.z;
}


template <class FT> INLINE
void Mat3T<FT>::xform(Vec3T<FT> &v) const
{
	FT ox, oy;

	ox = v.x; oy= v.y;
	v.x = XX * ox + XY * oy + XZ * v.z;
	v.y = YX * ox + YY * oy + YZ * v.z;
	v.z = ZX * ox + ZY * oy + ZZ * v.z;
}


template <class FT> INLINE
void Mat3T<FT>::invXform(const Vec3T<FT> &v, Vec3T<FT> &xv) const
{
	xv.x = XX * v.x + YX * v.y + ZX * v.z;
	xv.y = XY * v.x + YY * v.y + ZY * v.z;
	xv.z = XZ * v.x + YZ * v.y + ZZ * v.z;
}


template <class FT> INLINE
void Mat3T<FT>::invXform(Vec3T<FT> &v) const
{
	FT ox, oy;

	ox = v.x; oy= v.y;
	v.x = XX * ox + YX * oy + ZX * v.z;
	v.y = XY * ox + YY * oy + ZY * v.z;
	v.z = XZ * ox + YZ * oy + ZZ * v.z;
}



/*template <class FT> INLINE
void Mat3T<FT>::set(const Quat &q)
{
	XX = 2.0f * (q.s * q.s + q.i * q.i - 0.5f);
	YY = 2.0f * (q.s * q.s + q.j * q.j - 0.5f);
	ZZ = 2.0f * (q.s * q.s + q.k * q.k - 0.5f);

	XY = 2.0f * (q.j * q.i - q.k * q.s);
	YX = 2.0f * (q.i * q.j + q.k * q.s);


	YZ = 2.0f * (q.k * q.j - q.i * q.s);
	ZY = 2.0f * (q.j * q.k + q.i * q.s);

	ZX = 2.0f * (q.i * q.k - q.j * q.s);
	XZ = 2.0f * (q.k * q.i + q.j * q.s);
}*/



template <class FT> INLINE
void Mat3T<FT>::mult(const Mat3T<FT> &M, const Mat3T<FT> &N)
{
	XX = M.XX * N.XX + M.XY * N.YX + M.XZ * N.ZX;
	XY = M.XX * N.XY + M.XY * N.YY + M.XZ * N.ZY;
	XZ = M.XX * N.XZ + M.XY * N.YZ + M.XZ * N.ZZ;
	YX = M.YX * N.XX + M.YY * N.YX + M.YZ * N.ZX;
	YY = M.YX * N.XY + M.YY * N.YY + M.YZ * N.ZY;
	YZ = M.YX * N.XZ + M.YY * N.YZ + M.YZ * N.ZZ;
	ZX = M.ZX * N.XX + M.ZY * N.YX + M.ZZ * N.ZX;
	ZY = M.ZX * N.XY + M.ZY * N.YY + M.ZZ * N.ZY;
	ZZ = M.ZX * N.XZ + M.ZY * N.YZ + M.ZZ * N.ZZ;
}


template <class FT> INLINE
void Mat3T<FT>::mult(const Mat3T<FT> &M, bool post)
{
		
		FT oxy, oyz, ozx, oyx, ozy, oxz;
		if (post == false)
		{
				oxy = XY; oyx = YX; oyz = YZ; ozy = ZY; ozx = ZX; oxz = XZ;
				
				XY = M.XX * oxy + M.XY * YY  + M.XZ * ozy;
				XZ = M.XX * oxz + M.XY * oyz + M.XZ * ZZ;
				YX = M.YX * XX  + M.YY * oyx + M.YZ * ozx;
				YZ = M.YX * oxz + M.YY * oyz + M.YZ * ZZ;
				ZX = M.ZX * XX  + M.ZY * oyx + M.ZZ * ozx;
				ZY = M.ZX * oxy + M.ZY * YY  + M.ZZ * ozy;
				
				XX = M.XX * XX  + M.XY * oyx + M.XZ * ozx;
				YY = M.YX * oxy + M.YY * YY  + M.YZ * ozy;
				ZZ = M.ZX * oxz + M.ZY * oyz + M.ZZ * ZZ;
		}
		else
		{
				
				//FT oxy, oyz, ozx, oyx, ozy, oxz;
				
				oxy = XY; oyx = YX; oyz = YZ; ozy = ZY; ozx = ZX; oxz = XZ;
				
				XY = XX *  M.XY + oxy * M.YY + oxz * M.ZY;
				XZ = XX *  M.XZ + oxy * M.YZ + oxz * M.ZZ;
				YX = oyx * M.XX + YY  * M.YX + oyz * M.ZX;
				YZ = oyx * M.XZ + YY  * M.YZ + oyz * M.ZZ;
				ZX = ozx * M.XX + ozy * M.YX + ZZ  * M.ZX;
				ZY = ozx * M.XY + ozy * M.YY + ZZ  * M.ZY;
				
				XX = XX  * M.XX + oxy * M.YX + oxz * M.ZX;
				YY = oyx * M.XY + YY  * M.YY + oyz * M.ZY;
				ZZ = ozx * M.XZ + ozy * M.YZ + ZZ  * M.ZZ;
		}
}



template <class FT> INLINE
void Mat3T<FT>::xpose(const Mat3T<FT> &M)
{
	XX = M.XX;
	XY = M.YX;
	XZ = M.ZX;

	YX = M.XY;
	YY = M.YY;
	YZ = M.ZY;

	ZX = M.XZ;
	ZY = M.YZ;
	ZZ = M.ZZ;
}


template <class FT> INLINE
void Mat3T<FT>::xpose()
{
	FT tmp;

	tmp = XY;
	XY = YX;
	YX = tmp;

	tmp = YZ;
	YZ = ZY;
	ZY = tmp;

	tmp = ZX;
	ZX = XZ;
	XZ = tmp;
}




template <class FT> INLINE
void Mat3T<FT>::set(const Vec3T<FT> &diag, const Vec3T<FT> &sym)
{
	XX = diag.x;
	YY = diag.y;
	ZZ = diag.z;
	YZ = ZY = sym.x;
	ZX = XZ = sym.y;
	XY = YX = sym.z;
}

template <class FT> INLINE
void Mat3T<FT>::setXcol(const Vec3T<FT> &v)
{
	XX = v.x;
	YX = v.y;
	ZX = v.z;
}


template <class FT> INLINE
void Mat3T<FT>::setYcol(const Vec3T<FT> &v)
{
	XY = v.x;
	YY = v.y;
	ZY = v.z;
}


template <class FT> INLINE
void Mat3T<FT>::setZcol(const Vec3T<FT> &v)
{
	XZ = v.x;
	YZ = v.y;
	ZZ = v.z;
}


template <class FT> INLINE
void Mat3T<FT>::setSkew(const Vec3T<FT> &v)
{
	XX = YY = ZZ = 0.0;
	ZY =  v.x;
	YZ = -v.x;
	XZ =  v.y;
	ZX = -v.y;
	YX =  v.z;
	XY = -v.z;
}


template <class FT> INLINE
FT Mat3T<FT>::det() const
{
	return  XX * (YY * ZZ - YZ * ZY)
				+ XY * (YZ * ZX - YX * ZZ)
				+ XZ * (YX * ZY - YY * ZX);
}



/*template <class FT> INLINE
ostream& Mat3T<FT>::print(ostream &os) const
{
	int oldFlags = os.setf(ios::showpos);
	os << '[' << XX << ' ' << XY << ' ' << XZ << ')' << endl;
	os << '[' << YX << ' ' << YY << ' ' << YZ << ')' << endl;
	os << '[' << ZX << ' ' << ZY << ' ' << ZZ << ')' << endl;
	os.flags(oldFlags);
	return os;
}*/







/*Mat3T<FT> operator * (const FT d, const Mat3T<FT>& a)
{ 
} */



template <class FT> INLINE
Vec3T<FT> Mat3T<FT>::GetViewNormal()
{
	return Vec3T<FT>(XZ, YZ, ZZ);
}
template <class FT> INLINE
Vec3T<FT> Mat3T<FT>::GetViewUp()
{
	return Vec3T<FT>(XY, YY, ZY);
}
template <class FT> INLINE
Vec3T<FT> Mat3T<FT>::GetViewRight()
{
	return Vec3T<FT>(XX, YX, ZX);
}


template <class FT> INLINE
Mat3T<FT> Mat3T<FT>::adjoint()
{
		return Mat3T<FT>(row[1]^row[2],
		row[2]^row[0],
		row[0]^row[1]);
}

template <class FT> INLINE
Mat3T<FT> Mat3T<FT>::transpose()
{
		return Mat3T<FT>(col(0), col(1), col(2));
}

/*template <class FT> INLINE
double Mat3T<FT>::invert(Mat3T<FT>& inv)
{
		Mat3T<FT> A = adjoint();
		double d = A.row[0] & row[0];

		if( d==0.0 )
			return 0.0;

		inv = A.transpose() / d;
		return d;
}*/

// invert self
/*template <class FT> INLINE
double Mat3T<FT>::invert()
{
		Mat3T<FT> inv;
		double d;
		d = invert(inv);
		
		m(0, 0) = inv(0, 0);
		m(0, 1) = inv(0, 1);
		m(0, 2) = inv(0, 2);
										
		m(1, 0) = inv(1, 0);
		m(1, 1) = inv(1, 1);
		m(1, 2) = inv(1, 2);
										
										
		m(2, 0) = inv(2, 0);
		m(2, 1) = inv(2, 1);
		m(2, 2) = inv(2, 2);
		
		return d;

}*/



template <class FT> INLINE
void Mat3T<FT>::postmult(const Mat3T<FT> &M)
{
	FT oxy, oyz, ozx, oyx, ozy, oxz;

	oxy = XY; oyx = YX; oyz = YZ; ozy = ZY; ozx = ZX; oxz = XZ;

	XY = XX *  M.XY + oxy * M.YY + oxz * M.ZY;
	XZ = XX *  M.XZ + oxy * M.YZ + oxz * M.ZZ;
	YX = oyx * M.XX + YY  * M.YX + oyz * M.ZX;
	YZ = oyx * M.XZ + YY  * M.YZ + oyz * M.ZZ;
	ZX = ozx * M.XX + ozy * M.YX + ZZ  * M.ZX;
	ZY = ozx * M.XY + ozy * M.YY + ZZ  * M.ZY;

	XX = XX  * M.XX + oxy * M.YX + oxz * M.ZX;
	YY = oyx * M.XY + YY  * M.YY + oyz * M.ZY;
	ZZ = ozx * M.XZ + ozy * M.YZ + ZZ  * M.ZZ;
}


template <class FT> INLINE
void Mat3T<FT>::premult(const Mat3T<FT> &M)
{
	FT oxy, oyz, ozx, oyx, ozy, oxz;

	oxy = XY; 
	oyx = YX; 
	oyz = YZ; 
	ozy = ZY; 
	ozx = ZX; 
	oxz = XZ;

	XY = M.XX * oxy + M.XY * YY  + M.XZ * ozy;
	XZ = M.XX * oxz + M.XY * oyz + M.XZ * ZZ;
	YX = M.YX * XX  + M.YY * oyx + M.YZ * ozx;
	YZ = M.YX * oxz + M.YY * oyz + M.YZ * ZZ;
	ZX = M.ZX * XX  + M.ZY * oyx + M.ZZ * ozx;
	ZY = M.ZX * oxy + M.ZY * YY  + M.ZZ * ozy;

	XX = M.XX * XX  + M.XY * oyx + M.XZ * ozx;
	YY = M.YX * oxy + M.YY * YY  + M.YZ * ozy;
	ZZ = M.ZX * oxz + M.ZY * oyz + M.ZZ * ZZ;
}


// equivalent to a glTranslate
template <class FT> INLINE
void Mat3T<FT>::Translate(FT x, FT y, bool post) 
{
	Mat3T<FT> tm;
	tm.SetIdentity();
	tm.m(0, 2) = x;
	tm.m(1, 2) = y;

	if (post)	*this = (*this) * tm;
	else		*this = tm * (*this);
}

// equivalent to a glScale3d
template <class FT> INLINE 
void Mat3T<FT>::Scale(FT x, FT y, bool post) 
{
	Mat3T<FT> sm;
	sm.SetIdentity();
	sm.m(0, 0) = x;
	sm.m(1, 1) = y;

	if (post)	*this = (*this) * sm;
	else		*this = sm * (*this);
}

// angle in radians
template <class FT> INLINE 
void Mat3T<FT>::Rotate( FT angle, bool post )
{
	Mat3T<FT> r;
	r.SetIdentity();
	FT cosa = cosf(angle);
	FT sina = sinf(angle);
	r.m(0, 0) = cosa;
	r.m(0, 1) = -sina;
	r.m(1, 0) = sina;
	r.m(1, 1) = cosa;

	if (post)	*this = (*this) * r;
	else		*this = r * (*this);
}


typedef Mat3T<double> Mat3;
typedef Mat3T<double> Mat3d;
typedef Mat3T<float> Mat3f;

#include "VecUndef.h"
#endif // DEFINED_Mat3
