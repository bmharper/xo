#include "pch.h"

TESTFUNC(Stats)
{
	TTASSERT( sizeof(nuStyleAttrib) <= 8 );
	TTASSERT( sizeof(nuDomEl) <= 136 );
	printf( "sizeof(nuStyleAttrib) = %d\n", sizeof(nuStyleAttrib) );
	printf( "sizeof(nuStyle) = %d\n", sizeof(nuStyle) );
	printf( "sizeof(nuDomEl) = %d\n", sizeof(nuDomEl) );
}
