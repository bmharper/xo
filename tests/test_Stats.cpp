#include "pch.h"

TESTFUNC(Stats)
{
	TTASSERT( sizeof(nuStyleAttrib) <= 8 );
	// Of course its OK if some of these structures rise in size, but try hard to keep them small
	TTASSERT( sizeof(nuDomEl) <= 32 );
	TTASSERT( sizeof(nuDomNode) <= 128 );
	TTASSERT( sizeof(nuDomText) <= 40 );
	printf( "sizeof(nuStyleAttrib) = %d\n", sizeof(nuStyleAttrib) );
	printf( "sizeof(nuStyle) = %d\n", sizeof(nuStyle) );
	printf( "sizeof(nuDomEl) = %d\n", sizeof(nuDomEl) );
	printf( "sizeof(nuDomNode) = %d\n", sizeof(nuDomNode) );
	printf( "sizeof(nuDomText) = %d\n", sizeof(nuDomText) );

	TTASSERT( nuPosRound(0) == 0 );
	TTASSERT( nuPosRound(127) == 0 );
	TTASSERT( nuPosRound(128) == 256 );
	TTASSERT( nuPosRound(256) == 256 );

	TTASSERT( nuPosRoundDown(0) == 0 );
	TTASSERT( nuPosRoundDown(1) == 0 );
	TTASSERT( nuPosRoundDown(255) == 0 );
	TTASSERT( nuPosRoundDown(256) == 256 );

	TTASSERT( nuPosRoundUp(0) == 0 );
	TTASSERT( nuPosRoundUp(1) == 256 );
	TTASSERT( nuPosRoundUp(255) == 256 );
	TTASSERT( nuPosRoundUp(256) == 256 );
}
