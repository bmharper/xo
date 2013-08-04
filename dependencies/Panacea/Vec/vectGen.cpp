#include "pch.h"
#include "vectgen.h"
#include "vectx86.h"
#include "vect3dnow.h"
#include "vectsse.h"
#include "quat.h"
#include "Other/cpudetect.h"
#include "Other/RandomLib.h"

//#ifndef _WIN64
//// AMD lib
//#include <aquat.h>
//#endif


VecMul4fFunc oVecMul4f;
VecMul4dFunc oVecMul4d;

Mat4Convert_f2d_Func oMat4ToDouble;
Mat4Convert_d2f_Func oMat4ToFloat;

Mat4InvertfFunc oMatInv4f;
Mat4InvertdFunc oMatInv4d;

QuatFrom2VecfFunc	oQuatFrom2Vecf;
QuatFrom2VecdFunc	oQuatFrom2Vecd;
Quat2MatfFunc 		oQuat2Matf;
Quat2MatdFunc 		oQuat2Matd;
Matd2QuatFunc		oMatd2Quat;
QuatAddFunc			oQuatAdd;
QuatSubFunc			oQuatSub;
QuatMulFunc			oQuatMul;
QuatNormFunc		oQuatNorm;


// detects CPU and inits functions accordingly
bool InitVectLibrary( bool silent )
{

	RandomInitialise( 1802, 9373 );

//#ifdef _WIN32
	lmPlatform.CRLF = true;
//#else
//	lmPlatform.CRLF = false;
//#endif

	GetCPUInfo( lmPlatform.CPU );

	// default to x86
	oVecMul4f = xVecMul4f;
	oVecMul4d = xVecMul4d;
	oMat4ToDouble = xMat4ToDouble;
	oMat4ToFloat = xMat4ToFloat;
	oMatInv4f = xMatInv4f;
	oMatInv4d = xMatInv4d;
	oQuatFrom2Vecf = (QuatFrom2VecfFunc) xQuatFrom2Vecf;
	oQuatFrom2Vecd = (QuatFrom2VecdFunc) xQuatFrom2Vecd;
	oQuat2Matf = xQuat2Matf;
	oQuat2Matd = xQuat2Matd;
	oMatd2Quat = xMat2Quat;
	oQuatAdd = xQuatAdd;
	oQuatSub = xQuatSub;
	oQuatMul = xQuatMul;
	oQuatNorm = xQuatNorm;

#if !defined(ARCH_64)
	if ( lmPlatform.CPU.SSE1 )
	{
		oMatInv4f = sMatInv4f;
	}
	if ( lmPlatform.CPU.SSE3 )
	{
		oVecMul4f = SSE3_VecMul4f;
	}
	if ( lmPlatform.CPU.AMD_3DNOW && !lmPlatform.CPU.SSE4_1 ) // presence of 4.1 is assumption here that newer AMD CPUs have fast unaligned access and slow 3DNow.
	{
		// On K8 processors, this still beats the SSE3 implementation because of unaligned memory accesses
		oVecMul4f = aVecMul4f;
		//oQuatAdd = (QuatAddFunc) _add_quat;
		//oQuatSub = (QuatSubFunc) _sub_quat;
		//oQuatMul = (QuatMulFunc) _mult_quat;
		//oQuatNorm = (QuatNormFunc) _norm_quat;
	}
#endif

	return true;
}
