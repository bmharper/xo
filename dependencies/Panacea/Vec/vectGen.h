#pragma once

#ifdef _WIN32
#define VECT_ALIGN(var, alignment) __declspec(align(alignment)) var
#else
#define VECT_ALIGN(var, alignment) var __attribute__((aligned(alignment)))
#endif

// vertex transform
typedef void (*VecMul4fFunc) ( float* mat, float* vec, float* res );
typedef void (*VecMul4dFunc) ( double* mat, double* vec, double* res );

// matrix float conversion
typedef void (*Mat4Convert_f2d_Func) ( float* src, double* dst );
typedef void (*Mat4Convert_d2f_Func) ( double* src, float* dst );

// matrix inverse
typedef void (*Mat4InvertfFunc) ( float* mat );
typedef void (*Mat4InvertdFunc) ( double* mat );

// quaternion
typedef void (*QuatFrom2VecfFunc) ( float* quat, float* v3from, float* v3to );
typedef void (*QuatFrom2VecdFunc) ( float* quat, double* v3from, double* v3to );
typedef void (*Quat2MatfFunc) ( float* quat, float* mat );
typedef void (*Quat2MatdFunc) ( float* quat, double* mat );
typedef void (*Matd2QuatFunc) ( Quatf& quat, const double *m );
typedef void (*QuatAddFunc) ( Quatf& r, const Quatf& a, const Quatf& b );
typedef void (*QuatSubFunc) ( Quatf& r, const Quatf& a, const Quatf& b );
typedef void (*QuatMulFunc) ( Quatf& r, const Quatf& a, const Quatf& b );
typedef void (*QuatNormFunc) ( Quatf& r, const Quatf& a );


// Storage for these externs is in vectGen.cpp

// vertex transform
extern PAPI VecMul4fFunc oVecMul4f;
extern PAPI VecMul4dFunc oVecMul4d;

// matrix float conversion
extern PAPI Mat4Convert_f2d_Func oMat4ToDouble;
extern PAPI Mat4Convert_d2f_Func oMat4ToFloat;

// matrix inverse
extern PAPI Mat4InvertfFunc oMatInv4f;
extern PAPI Mat4InvertdFunc oMatInv4d;

// quaternion
extern PAPI QuatFrom2VecfFunc	oQuatFrom2Vecf;
extern PAPI QuatFrom2VecdFunc	oQuatFrom2Vecd;
extern PAPI Quat2MatfFunc		oQuat2Matf;
extern PAPI Quat2MatdFunc		oQuat2Matd;
extern PAPI Matd2QuatFunc		oMatd2Quat;
extern PAPI QuatAddFunc			oQuatAdd;
extern PAPI QuatSubFunc			oQuatSub;
extern PAPI QuatMulFunc			oQuatMul;
extern PAPI QuatNormFunc			oQuatNorm;

// MANDATORY. 
bool PAPI InitVectLibrary( bool silent );

// some inline wrappers to make things a bit cleaner
// The overloaded versions may be a bit slower because of the temp variable
// they allocate. This shouldn't be an issue to a good optimizer but I never know.

//////////////////////////////////////////////////////////////////////////////
// vector transform

// specific
INLINE void zVecMul4f( const Mat4f& m, const Vec4f& vec, Vec4f& res )
{
	oVecMul4f( (float*) &m, (float*) vec.n, (float*) res.n );
}
// overloaded
INLINE Vec4f zMul( const Mat4f& m, const Vec4f& vec )
{
	VECT_ALIGN(Vec4f res, 16);
	oVecMul4f( (float*) &m, (float*) vec.n, (float*) res.n );
	return res;
}

// specific
INLINE void zVecMul4d( const Mat4d& m, const Vec4d& vec, Vec4d& res )
{
	// Allow this to get inlined
	//oVecMul4d( (double*) &m, (double*) vec.n, (double*) res.n );
	res = m * vec;
}
// overloaded
INLINE Vec4d zMul( const Mat4d& m, const Vec4d& vec )
{
	// Allow this to get inlined
	return m * vec;
}


//////////////////////////////////////////////////////////////////////////////
// matrix conversion
INLINE void zConvert2Double( const Mat4f& src, Mat4& dst )
{
	oMat4ToDouble( (float*) &src, (double*) &dst );
}

INLINE void zConvert2Float( const Mat4& src, Mat4f& dst )
{
	oMat4ToFloat( (double*) &src, (float*) &dst );
}

//////////////////////////////////////////////////////////////////////////////
// matrix inverse

// specific
INLINE void zMatInv4f( Mat4f& m )
{
	oMatInv4f( (float*) &m );
}
// overloaded
INLINE void zInvert( Mat4f& m )
{
	oMatInv4f( (float*) &m );
}

// specific
INLINE void zMatInv4d( Mat4d& m )
{
	oMatInv4d( (double*) &m );
}
// overloaded
INLINE void zInvert( Mat4d& m )
{
	oMatInv4d( (double*) &m );
}

//////////////////////////////////////////////////////////////////////////////
// quaternion

// Quaternion to get from 1 normalized vector to another.
INLINE Quatf zQuatFrom2Vec( const Vec3f& from_normalized, const Vec3f& to_normalized )
{
	Quatf q;
	oQuatFrom2Vecf( (float*) &q, (float*) &from_normalized, (float*) &to_normalized );
	return q;
}

// Quaternion to get from 1 normalized vector to another.
INLINE Quatf zQuatFrom2Vec( const Vec3& from_normalized, const Vec3& to_normalized )
{
	Quatf q;
	oQuatFrom2Vecd( (float*) &q, (double*) &from_normalized, (double*) &to_normalized );
	return q;
}

// specific
INLINE void zQuat2Matf( const Quatf& quat, Mat4f& m )
{
	oQuat2Matf( (float*) &quat, (float*) &m );
}
// overloaded
INLINE void zQuat2Mat( const Quatf& quat, Mat4f& m )
{
	oQuat2Matf( (float*) &quat, (float*) &m );
}

// specific
INLINE void zQuat2Matd( const Quatf& quat, Mat4d& m )
{
	oQuat2Matd( (float*) &quat, (double*) &m );
}
// overloaded
INLINE void zQuat2Mat( const Quatf& quat, Mat4d& m )
{
	oQuat2Matd( (float*) &quat, (double*) &m );
}

// overloaded
INLINE void zMat2Quat( const Mat4d& m, Quatf& quat )
{
	oMatd2Quat( quat, (double*) &m );
}


// quat algebra
INLINE Quatf zQuatAdd( const Quatf& a, const Quatf& b )
{
	Quatf r;
	oQuatAdd( r, a, b );
	return r;
}

INLINE Quatf zQuatSub( const Quatf& a, const Quatf& b )
{
	Quatf r;
	oQuatSub( r, a, b );
	return r;
}

INLINE Quatf zQuatMul( const Quatf& a, const Quatf& b )
{
	Quatf r;
	oQuatMul( r, a, b );
	return r;
}

INLINE void zQuatNorm( Quatf& q )
{
	Quatf r;
	oQuatNorm( r, q );
	q = r;
}
