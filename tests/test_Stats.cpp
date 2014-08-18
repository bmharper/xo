#include "pch.h"

TESTFUNC(Stats)
{
	TTASSERT( sizeof(xoStyleAttrib) <= 8 );
	// Of course its OK if some of these structures rise in size, but try hard to keep them small
	TTASSERT( sizeof(xoDomEl) <= 32 );
	TTASSERT( sizeof(xoDomNode) <= 128 );
	TTASSERT( sizeof(xoDomText) <= 40 );
	printf( "sizeof(xoStyleAttrib) = %d\n", sizeof(xoStyleAttrib) );
	printf( "sizeof(xoStyle) = %d\n", sizeof(xoStyle) );
	printf( "sizeof(xoDomEl) = %d\n", sizeof(xoDomEl) );
	printf( "sizeof(xoDomNode) = %d\n", sizeof(xoDomNode) );
	printf( "sizeof(xoDomText) = %d\n", sizeof(xoDomText) );

	TTASSERT( xoPosRound(0) == 0 );
	TTASSERT( xoPosRound(127) == 0 );
	TTASSERT( xoPosRound(128) == 256 );
	TTASSERT( xoPosRound(256) == 256 );

	TTASSERT( xoPosRoundDown(0) == 0 );
	TTASSERT( xoPosRoundDown(1) == 0 );
	TTASSERT( xoPosRoundDown(255) == 0 );
	TTASSERT( xoPosRoundDown(256) == 256 );

	TTASSERT( xoPosRoundUp(0) == 0 );
	TTASSERT( xoPosRoundUp(1) == 256 );
	TTASSERT( xoPosRoundUp(255) == 256 );
	TTASSERT( xoPosRoundUp(256) == 256 );
}
