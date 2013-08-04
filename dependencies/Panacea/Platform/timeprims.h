#pragma once

#include "../Platform/stdint.h"

#ifdef _WIN32
inline i64 AbcTimeAsFileTime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	return ((u64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}
inline i64 AbcFileTimeToMicroseconds( const FILETIME& ft )
{
	u64 time = ((u64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	i64 stime = (i64) time;
	return stime / 10;
}
#endif

// Return time suitable for 120hz

#ifdef _WIN32
inline double AbcTimeAccurateRTSeconds()
{
	LARGE_INTEGER t, f;
	QueryPerformanceCounter( &t );
	QueryPerformanceFrequency( &f );
	return t.QuadPart / (double) f.QuadPart;
}
#else
inline double AbcTimeAccurateRTSeconds()
{
	timespec t;
	clock_gettime( CLOCK_MONOTONIC, &t );
	return t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
}
#endif

