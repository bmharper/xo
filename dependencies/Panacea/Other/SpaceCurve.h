#pragma once

#include "lmDefs.h"

/*
Benchmark Core i7, 24 bits of precision:

32-bit
------
					Sierpinski: 666 clocks
						Hilbert: 207 clocks
							Morton: 330 clocks

x64
---

					Sierpinski: 599 clocks
						Hilbert: 170 clocks
							Morton: 120 clocks
*/

PAPI u64 SierpinskiIndex( u32 max_xy, u32 x, u32 y );
PAPI u64 HilbertIndex( u32 bits, u32 x, u32 y );

template<typename TResult, u32 bits>
TResult MortonOrder( u32 x, u32 y )
{
	TResult r = 0;
	u32 bit = 1;
	u32 shift = 0;
	for ( u32 b = bits; b != 0; b-- )
	{
		r = r | (((TResult)x & bit) << shift);
		shift++;
		r = r | (((TResult)y & bit) << shift);
		bit <<= 1;
	}
	return r;
}
