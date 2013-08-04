#include "pch.h"
#include "SpaceCurve.h"

PAPI u64 SierpinskiIndex( u32 max_xy, u32 x, u32 y )
{
	u32 index = max_xy;
	u64 res = 0;

	if ( x > y )
	{
		res++;
		x = max_xy - x;
		y = max_xy - y;
	}

	while ( index > 0 )
	{
		res += res;
		if ( x + y > max_xy )
		{
			res++;
			auto oldx = x;
			x = max_xy - y;
			y = oldx;
		}

		x += x;
		y += y;
		res += res;

		if ( y > max_xy )
		{
			res++;
			auto oldx = x;
			x = y - max_xy;
			y = max_xy - oldx;
		}
		index = index >> 1;
	}

	return res;
}

PAPI u64 HilbertIndex( u32 bits, u32 x, u32 y )
{
	u64 res = 0;
	x <<= 1;					// by shifting one up first, we don't have to worry about the final shift, getting the LSB Y bit into bit position 2.
	y <<= 1;
	u32 bitx = bits;
	u32 bity = bits - 1;

	/*
	Position on Curve
	-----------------

	0      3
	|      |
	|      |
	1------2
	

	Types of Curves
	---------------

	0   |_|
	
	1   --+
		--+

	2   +--
		+--
	
	3   +-+
		| |
	*/

	u32 current = 0;
	u8 score[4][4] = {
		{1, 2, 0, 3},
		{3, 2, 0, 1},
		{1, 0, 2, 3},
		{3, 0, 2, 1},
	};
	u8 transition[4][4] = {
		{0, 0, 1, 2},
		{3, 1, 0, 1},
		{2, 3, 2, 0},
		{1, 2, 3, 3},
	};

	while ( bitx )
	{
		u32 v = ((x >> bitx) & 1) | ((y >> bity) & 2);
		res = (res << 2) | score[current][v];
		current = transition[current][v];
		bitx--;
		bity--;
	}
	return res;
}
