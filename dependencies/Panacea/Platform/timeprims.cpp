#include "pch.h"
#include "timeprims.h"

#ifdef _WIN32
static int32	CachedPerformanceFrequency_Set;
static double	CachedPerformanceFrequency_Inv;
#endif

#ifdef _WIN32
PAPI double AbcTimeAccurateRTSeconds()
{
	if ( CachedPerformanceFrequency_Set == 0 )
	{
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency( &tmp );
		CachedPerformanceFrequency_Inv = 1.0 / (double) tmp.QuadPart;
		MemoryBarrier();
		CachedPerformanceFrequency_Set = 1;
	}
	LARGE_INTEGER t;
	QueryPerformanceCounter( &t );
	return t.QuadPart * CachedPerformanceFrequency_Inv;
}
#else
PAPI double AbcTimeAccurateRTSeconds()
{
	timespec t;
	clock_gettime( CLOCK_MONOTONIC, &t );
	return t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
}
#endif
