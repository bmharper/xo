#include "pch.h"
#include "vectgen.h"

#ifdef _M_IX86

void aVecMul4f( float* mat, float* vec, float* res )
{
	__asm 
	{
		femms
		mov         eax, res	; result
		mov         ecx, vec	; input
		mov         edx, mat	; matrix

		movq        mm0,[ecx]	; y   |  x
		movq        mm4,[ecx+8]		; rhw |  z
		movq        mm1,mm0		; y   |  x
		pfmul       mm0,[edx]	; y * m_21 | x * m_11
		movq        mm5,mm4		; rhw |  z
		pfmul       mm4,[edx+8]		; rhw * m_41 | z * m_31
		movq        mm2,mm1		; y   |  x
		pfmul       mm1,[edx+16]	; y * m_22 | x * m_12
		movq        mm6,mm5		; rhw |  z
		pfmul       mm5,[edx+24]	; rhw * m_42 | z * m_32
		movq        mm3,mm2		; y   |  x
		pfmul       mm2,[edx+32]	; y * m_23 | x * m_13
		movq        mm7,mm6		; rhw | z
		pfmul       mm6,[edx+40]	; rhw * m_43 | z * m_33
		pfmul       mm3,[edx+48]	; y * m_24 | x * m_14
		pfacc       mm0,mm4		; rhw * m_41 + z * m_31 | y * m_21 + x * m_11 
		pfmul       mm7,[edx+56]	; rhw * m_44 | z * m_34
		pfacc       mm1,mm5		; rhw * m_42 + z * m_32 | y * m_22 + x * m_12
		pfacc       mm2,mm6		; rhw * m_43 + z * m_33 | y * m_23 + x * m_13
		pfacc       mm3,mm7		; rhw * m_44 + z * m_34 | y * m_24 | x * m_14 
		pfacc       mm0,mm1		; rhw * m_41 + z * m_31 + y * m_21 + x * m_11 | rhw * m_42 + z * m_32 + y * m_22 + x * m_12
		pfacc       mm2,mm3		; rhw * m_44 + z * m_34 + y * m_24 | x * m_14 | rhw * m_43 + z * m_33 + y * m_23 + x * m_13
		movq        [eax],mm0	; r_y   | r_x
		movq        [eax+8],mm2		; r_rhw | r_z
		femms
	}
}

#endif
