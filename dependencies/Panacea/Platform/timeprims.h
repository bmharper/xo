#pragma once
#ifndef ABC_TIMEPRIMS_H_INCLUDED
#define ABC_TIMEPRIMS_H_INCLUDED

#include "stdint.h"
#include "winheaders.h"

inline i64 AbcFileTimeToMicroseconds(const FILETIME& ft)
{
	u64 time = ((u64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	i64 stime = (i64) time;
	return stime / 10;
}

inline double AbcFileTimeToUnixSeconds(const FILETIME& ft)
{
	const i64 days_from_1601_to_1970 = 370 * 365 - 276;
	const i64 microsecondsPerDay = 24 * 3600 * (i64) 1000000;
	i64 micro = AbcFileTimeToMicroseconds(ft);
	return (micro - (days_from_1601_to_1970 * microsecondsPerDay)) * (1.0 / 1000000.0);
}

PAPI double AbcTimeAccurateRTSeconds();

#endif // ABC_TIMEPRIMS_H_INCLUDED
