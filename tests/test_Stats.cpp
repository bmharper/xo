#include "pch.h"

TESTFUNC(Stats)
{
	TTASSERT(sizeof(xo::StyleAttrib) <= 8);
	// Of course its OK if some of these structures rise in size, but try hard to keep them small
	TTASSERT(sizeof(xo::DomEl) <= 40);
	TTASSERT(sizeof(xo::DomNode) <= 136);
	TTASSERT(sizeof(xo::DomText) <= 48);
	printf("sizeof(xo::StyleAttrib) = %d\n", (int) sizeof(xo::StyleAttrib));
	printf("sizeof(xo::Style) = %d\n", (int) sizeof(xo::Style));
	printf("sizeof(xo::DomEl) = %d\n", (int) sizeof(xo::DomEl));
	printf("sizeof(xo::DomNode) = %d\n", (int) sizeof(xo::DomNode));
	printf("sizeof(xo::DomText) = %d\n", (int) sizeof(xo::DomText));

	TTASSERT(xo::PosRound(0) == 0);
	TTASSERT(xo::PosRound(127) == 0);
	TTASSERT(xo::PosRound(128) == 256);
	TTASSERT(xo::PosRound(256) == 256);

	TTASSERT(xo::PosRoundDown(0) == 0);
	TTASSERT(xo::PosRoundDown(1) == 0);
	TTASSERT(xo::PosRoundDown(255) == 0);
	TTASSERT(xo::PosRoundDown(256) == 256);

	TTASSERT(xo::PosRoundUp(0) == 0);
	TTASSERT(xo::PosRoundUp(1) == 256);
	TTASSERT(xo::PosRoundUp(255) == 256);
	TTASSERT(xo::PosRoundUp(256) == 256);
}
