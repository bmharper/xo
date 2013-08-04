#include "pch.h"
#include "UTC.h"
#include <time.h>

namespace AbCore
{

static volatile int tzsetCalled = 0;

template< typename TCH >
void TFormatTimeHMS( INT32 v, TCH* buf )
{
	int hour, min, sec, ms;
	DecodeTime( v, hour, min, sec, ms );
	char tbuf[9];
	sprintf( tbuf, "%02d:%02d:%02d", hour, min, sec );
	for ( int i = 0; i < 8; i++ ) buf[i] = tbuf[i];
}

void PAPI FormatTimeHMS( INT32 v, char* buf )
{
	TFormatTimeHMS( v, buf );
}

void PAPI FormatTimeHMS( INT32 v, wchar_t* buf )
{
	TFormatTimeHMS( v, buf );
}

/** Decode UTC seconds into [year, seconds in year]
**/
template< typename T >
void DecodeYearSecond( T utc_s_1601, UINT32& year, UINT32& secs_in_year )
{
	T secs = utc_s_1601;
	UINT32 y = 1601;

	/* A precomputed year:day table
	Number of Days between Jan 1 1601 and start of year, Number of Seconds
	2300	255304	22058265600
	2200	218780	18902592000
	2100	182256	15746918400
	2050	163994	14169081600
	2030	156689	13537929600
	2020	153036	13222310400
	2010	149384	12906777600
	2000	145731	12591158400
	1990	142079	12275625600
	1980	138426	11960006400
	1970	134774	11644473600
	1950	127469	11013321600
	1900	109207	9435484800
	1800	72683 	6279811200
	1700	36159 	3124137600
	1601	0		0
	*/


	if ( secs >= 22058265600 )		{ y = 2300; secs -= 22058265600; }
	else if ( secs >= 18902592000 )	{ y = 2200; secs -= 18902592000; }
	else if ( secs >= 15746918400 )	{ y = 2100; secs -= 15746918400; }
	else if ( secs >= 14169081600 )	{ y = 2050; secs -= 14169081600; }
	else if ( secs >= 13537929600 )	{ y = 2030; secs -= 13537929600; }
	else if ( secs >= 13222310400 )	{ y = 2020; secs -= 13222310400; }
	else if ( secs >= 12906777600 )	{ y = 2010; secs -= 12906777600; }
	else if ( secs >= 12591158400 )	{ y = 2000; secs -= 12591158400; }
	else if ( secs >= 12275625600 )	{ y = 1990; secs -= 12275625600; }
	else if ( secs >= 11960006400 )	{ y = 1980; secs -= 11960006400; }
	else if ( secs >= 11644473600 )	{ y = 1970; secs -= 11644473600; }
	else if ( secs >= 11013321600 )	{ y = 1950; secs -= 11013321600; }
	else if ( secs >= 9435484800 )	{ y = 1900; secs -= 9435484800; }
	else if ( secs >= 6279811200 )	{ y = 1800; secs -= 6279811200; }
	else if ( secs >= 3124137600 )	{ y = 1700; secs -= 3124137600; }

	while ( true )
	{
		T ys = IsLeapYear(y) ? 366 * 86400 : 365 * 86400;
		if ( secs < ys ) break;
		secs -= ys;
		y++;
	}

	year = y;
	secs_in_year = secs;
}

bool PAPI UTC_Decode_Local( INT64 utc_ms_1601, DateTime& dt )
{
	FILETIME utc, loc;
	utc.dwHighDateTime = ((u64) utc_ms_1601 * 10) >> 32;
	utc.dwLowDateTime = ((u64) utc_ms_1601 * 10) & 0xFFFFFFFF;
	
#ifdef _WIN32
	FileTimeToLocalFileTime( &utc, &loc );
	UINT64 t = (((u64) loc.dwHighDateTime << 32) | loc.dwLowDateTime) / 10;
#else
	// tzset() is safe to call from multiple threads, which is why we use this lazy initialization system.
	if ( tzsetCalled == 0 )
	{
		tzset();
		tzsetCalled = 1;
	}
	// timezone = seconds west of UTC
	UINT64 t = (((u64) loc.dwHighDateTime << 32) | loc.dwLowDateTime) / 10;
	t += (int64) timezone * (int64) 1000000;
#endif
	// testing code, in the absence of being able to test this for real on a linux build
	//UINT64 ttest = (((u64) utc.dwHighDateTime << 32) | utc.dwLowDateTime) / 10;
	//ttest += (int64) (-7200) * (int64) -1000000;
	//UINT64 delta = ttest - t;

	UTC_Decode( t, dt );

	return true;
}

// 0 = Sunday
int dayofweek(int y, int m, int d)
{
	static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	y -= m < 3;
	return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

void PAPI UTC_Decode( INT64 utc_ms_1601, DateTime& dt )
{
	UINT32 y = -1;
	UINT32 ysec = -1;

	if ( utc_ms_1601 == NullDate )
	{
		memset( &dt, 0, sizeof(dt) );
		return;
	}

	DecodeYearSecond<INT64>( utc_ms_1601 / 1000000, y, ysec );

	UINT32 yday = ysec / 86400;
	UINT32 day;
	UINT32 mon;

	if ( IsLeapYear(y) )
	{
		if ( yday < 31 )		{ mon = 0; day = yday; }
		else if ( yday < 60 )	{ mon = 1; day = yday - 31; }
		else if ( yday < 91 )	{ mon = 2; day = yday - 60; }
		else if ( yday < 121 )	{ mon = 3; day = yday - 91; }
		else if ( yday < 152 )	{ mon = 4; day = yday - 121; }
		else if ( yday < 182 )	{ mon = 5; day = yday - 152; }
		else if ( yday < 213 )	{ mon = 6; day = yday - 182; }
		else if ( yday < 244 )	{ mon = 7; day = yday - 213; }
		else if ( yday < 274 )	{ mon = 8; day = yday - 244; }
		else if ( yday < 305 )	{ mon = 9; day = yday - 274; }
		else if ( yday < 335 )	{ mon = 10; day = yday - 305; }
		else					{ mon = 11; day = yday - 335; }
	}
	else
	{
		if ( yday < 31 )		{ mon = 0; day = yday; }
		else if ( yday < 59 )	{ mon = 1; day = yday - 31; }
		else if ( yday < 90 )	{ mon = 2; day = yday - 59; }
		else if ( yday < 120 )	{ mon = 3; day = yday - 90; }
		else if ( yday < 151 )	{ mon = 4; day = yday - 120; }
		else if ( yday < 181 )	{ mon = 5; day = yday - 151; }
		else if ( yday < 212 )	{ mon = 6; day = yday - 181; }
		else if ( yday < 243 )	{ mon = 7; day = yday - 212; }
		else if ( yday < 273 )	{ mon = 8; day = yday - 243; }
		else if ( yday < 304 )	{ mon = 9; day = yday - 273; }
		else if ( yday < 334 )	{ mon = 10; day = yday - 304; }
		else					{ mon = 11; day = yday - 334; }
	}

	UINT32 dsec = ysec % 86400;
	UINT32 ahour = dsec / 3600;
	UINT32 amin = (dsec / 60) % 60;
	UINT32 asec = dsec % 60;
	UINT32 amsec = utc_ms_1601 % 1000000;

	dt.Year = y;
	dt.YearDay = yday;
	dt.Month = mon;
	dt.Day = day + 1;
	dt.Hour = ahour;
	dt.Min = amin;
	dt.Sec = asec;
	dt.WeekDay = dayofweek(y, mon + 1, day + 1);
	dt.Microsecond = amsec;
}

INT64 PAPI UTC_Encode( const DateTime& dt )
{
	if ( dt.Year > 3000 || dt.Month > 11 ) return NullDate;
	INT64 v = 0;
	UINT32 y = 1601;

	if ( dt.Year > 2300 )		{ y = 2300; v = 22058265600; }
	else if ( dt.Year > 2200 )	{ y = 2200; v = 18902592000; }
	else if ( dt.Year > 2100 )	{ y = 2100; v = 15746918400; }
	else if ( dt.Year > 2050 )	{ y = 2050; v = 14169081600; }
	else if ( dt.Year > 2030 )	{ y = 2030; v = 13537929600; }
	else if ( dt.Year > 2020 )	{ y = 2020; v = 13222310400; }
	else if ( dt.Year > 2010 )	{ y = 2010; v = 12906777600; }
	else if ( dt.Year > 2000 )	{ y = 2000; v = 12591158400; }
	else if ( dt.Year > 1990 )	{ y = 1990; v = 12275625600; }
	else if ( dt.Year > 1980 )	{ y = 1980; v = 11960006400; }
	else if ( dt.Year > 1970 )	{ y = 1970; v = 11644473600; }
	else if ( dt.Year > 1950 )	{ y = 1950; v = 11013321600; }
	else if ( dt.Year > 1900 )	{ y = 1900; v = 9435484800; }
	else if ( dt.Year > 1800 )	{ y = 1800; v = 6279811200; }
	else if ( dt.Year > 1700 )	{ y = 1700; v = 3124137600; }

	for ( ; y < dt.Year; y++ )
		v += IsLeapYear( y ) ? 366 * 86400 : 365 * 86400;

	UINT32 yday = 0;

	if ( IsLeapYear(dt.Year) )
	{
		UINT32 mtab[12] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
		yday = mtab[dt.Month];
	}
	else
	{
		UINT32 mtab[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
		yday = mtab[dt.Month];
	}

	yday += dt.Day - 1;

	v += yday * 86400 + (UINT32) dt.Hour * 3600 + (UINT32) dt.Min * 60 + (UINT32) dt.Sec;
	v *= 1000000;
	v += dt.Microsecond;

	return v;
}

}
