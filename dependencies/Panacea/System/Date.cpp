#include "pch.h"
#include "Date.h"
#include "../strings/fmt.h"
#include <stdlib.h>
#include <time.h>

using namespace AbCore;

namespace Panacea
{
namespace Sys
{

static volatile int tzsetCalled = 0;

Date::Date()
{
	mUTC = NullVal;
}

Date Date::NEWTOR_1601( INT64 utc_1601 )
{
	Date d;
	if ( utc_1601 == NullVal || utc_1601 < 0 || utc_1601 > MaxLegalVal )
		d.mUTC = NullVal;
	else
		d.mUTC = utc_1601;
	return d;
}

Date Date::NEWTOR_1900( INT64 utc_1900_microseconds )
{
	if ( utc_1900_microseconds == 0 || utc_1900_microseconds == NullVal )	return NullDate();
	else																	return Date::NEWTOR_1601( UTC_1900_to_1601(utc_1900_microseconds) );
}

Date Date::NEWTOR_1970( INT64 utc_1970_microseconds )
{
	if ( utc_1970_microseconds == 0 || utc_1970_microseconds == NullVal )	return NullDate();
	else																	return Date::NEWTOR_1601( UTC_1970_to_1601(utc_1970_microseconds) );
}

Date Date::NEWTOR_2000( INT64 utc_2000_microseconds )
{
	if ( utc_2000_microseconds == 0 || utc_2000_microseconds == NullVal )	return NullDate();
	else																	return Date::NEWTOR_1601( UTC_2000_to_1601(utc_2000_microseconds) );
}

Date Date::FromSecondsUTC_1970( double utc_1970_seconds )
{
	if ( utc_1970_seconds == 0 )	return NullDate();
	return							Date::NEWTOR_1970( utc_1970_seconds * 1000000.0 );
}

Date::Date( const FILETIME& st )
{
	UINT64 tt = ((UINT64) st.dwHighDateTime << 32) | (UINT64) st.dwLowDateTime;
	tt /= 10; // to microseconds
	//tt -= ((UINT64) (370 * 365 - 276) * 24 * 3600 * 1000000);	// from 1601 to 1970
	*this = NEWTOR_1601( tt );
}

#ifdef _WIN32
Date::Date( const SYSTEMTIME& st )
{
	FILETIME ft;
	SystemTimeToFileTime( &st, &ft );
	*this = Date( ft );
}

SYSTEMTIME Date::GetSYSTEMTIME( bool utc ) const
{
	DateTime dt = GetDetails(utc);
	SYSTEMTIME st;
	st.wYear = dt.Year;
	st.wMonth = dt.Month + 1;
	st.wDayOfWeek = -1;
	st.wDay = dt.Day;
	st.wHour = dt.Hour;
	st.wMinute = dt.Min;
	st.wSecond = dt.Sec;
	st.wMilliseconds = dt.Microsecond / 1000;
	return st;
}

Date::operator SYSTEMTIME() const
{
	return GetSYSTEMTIME(true);
}
#endif

Date::Date( int year, int month, int day, int hour, int minute, int second, int microsecond )
{
	*this = FromYMD_Local( year, month, day, hour, minute, second, microsecond );
}

Date Date::FromYMD_Internal( bool utc, int year, int month, int day, int hour, int minute, int second, int microsecond )
{
	ASSERT( month >= 0 && month <= 11 );
	ASSERT( day >= 1 && day <= 31 );
	if ( utc )
	{
		INT64 v = AbCore::UTC_Encode( year, month, day, hour, minute, second, microsecond );
		return NEWTOR_1601( v );
	}
	else
	{
		struct tm tv;
		tv.tm_year = year - 1900;
		tv.tm_mon = month;
		tv.tm_mday = day;
		tv.tm_hour = hour;
		tv.tm_min = minute;
		tv.tm_sec = second;
		tv.tm_wday = -1;
		tv.tm_yday = -1;
		tv.tm_isdst = -1;	// -1 = let system decide on whether to us daylight savings time
#ifdef _WIN32
		__time64_t val = 0;
		val = _mktime64( &tv );
#else
		time_t val = 0;
		val = mktime( &tv );
#endif
		return NEWTOR_1970( val * 1000000 + microsecond );
	}
}

Date Date::FromYMD_Local( int year, int month, int day, int hour, int minute, int second, int microsecond )
{
	return FromYMD_Internal( false, year, month, day, hour, minute, second, microsecond );
}

Date Date::FromYMD_UTC( int year, int month, int day, int hour, int minute, int second, int microsecond )
{
	return FromYMD_Internal( true, year, month, day, hour, minute, second, microsecond );
}

Date Date::FromYearDay( bool utc, int year, int day, int hour, int minute, int second, int microsecond )
{
	// NOTE: The local thing here is obviously wrong, since you're computing the day of the month before doing the
	// time zone adjustment.
	// mktime64 does not support yday, so we have to do it ourselves.
	int month = MonthFromDay( year, &day );
	return FromYMD_Internal( utc, year, month, day, hour, minute, second, microsecond );
}

Date Date::FromYearDay_Local( int year, int day, int hour, int minute, int second, int microsecond )
{
	return FromYearDay( false, year, day, hour, minute, second, microsecond );
}

Date Date::FromYearDay_UTC( int year, int day, int hour, int minute, int second, int microsecond )
{
	return FromYearDay( true, year, day, hour, minute, second, microsecond );
}

Date Date::Now()
{
	// According to a DDJ article from 2003, GetSystemTimeAsFileTime is 400 times faster than GetSystemTime.
	// Things have definitely changed since then. I'm not sure that Windows has a monotonic clock that is more
	// accurate than this, with good universal performance.
#ifdef _WIN32
	FILETIME fTime;
	GetSystemTimeAsFileTime( &fTime );
	return Date( fTime );
#else
	// CLOCK_MONOTONIC_COARSE is very close to the Windows GetSystemTimeAsFileTime.
	// I think that Date() should have two functions - one to retrieve an accurate
	// date and one to retrieve the tick-based coarse date.
	timespec t;
	clock_gettime( CLOCK_MONOTONIC_COARSE, &t );
	double seconds = t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
	return Date::FromSecondsUTC_1970( seconds );
#endif
}

INT64 Date::ToFileTimei64() const
{
	INT64 tt = IsNull() ? 0 : mUTC;
	//tt += ((UINT64) (370 * 365 - 276) * 24 * 3600);
	//tt *= 10000000; // to 100-nanosecond
	//tt += ((UINT64) (370 * 365 - 276) * 24 * 3600 * 1000000);
	//tt += ((UINT64) (370 * 365 - 276) * 24 * 3600 * 1000000);
	tt *= 10; // to 100-nanosecond
	return tt;
}

FILETIME Date::ToFileTime() const
{
	INT64 tt = ToFileTimei64();
	FILETIME ft;
	ft.dwHighDateTime = (UINT32) (tt >> 32);
	ft.dwLowDateTime = (UINT32) (tt);
	return ft;
}

AbCore::DateTime Date::GetDetails( bool utc ) const
{
	DateTime dt;
	if ( utc )	UTC_Decode( mUTC, dt );
	else		UTC_Decode_Local( mUTC, dt );
	return dt;
}

bool Date::GetDetails( bool utc, AbCore::DateTime& dt ) const
{
	if ( utc )	{ UTC_Decode( mUTC, dt ); return true; }
	else		return UTC_Decode_Local( mUTC, dt );
}

Date Date::FromTM( bool utc, const tm& t )
{
	return FromYMD_Internal( utc, t.tm_year + 1900, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec );
}

void Date::ToTM( bool utc, tm& t ) const
{
	auto dt = GetDetails(utc);
	int mday = dt.YearDay;
	MonthFromDay( dt.Year, &mday );
	t.tm_year = dt.Year - 1900;
	t.tm_yday = dt.YearDay;
	t.tm_mon = dt.Month;
	t.tm_mday = mday;
	t.tm_wday = -1;
	t.tm_isdst = -1;
	t.tm_hour = dt.Hour;
	t.tm_min = dt.Min;
	t.tm_sec = dt.Sec;
}

int Date::YearFromDaySince2000( int day, int* dayOfYear )
{
	int year = 0;
	for ( year = 0; year < 500; year++ )
	{
		bool leap = IsLeapYear( year + 2000 );
		int yearDays = leap ? 366 : 365;
		if ( day < yearDays ) 
		{
			if ( dayOfYear != NULL ) *dayOfYear = day;
			return year + 2000;
		}
		day -= yearDays;
	}
	if ( dayOfYear != NULL ) *dayOfYear = -1;
	return -1;
}

int Date::DaysSince2000() const
{
	int year = Year() - 2000;
	if ( year < 0 ) return -1;
	int day = DayOfYear();
	for ( int i = 0; i < year; i++ )
		day += DaysOfYear( i );
	return day;
}

int Date::MonthFromDay( int year, int day )
{
	return MonthFromDayInternal( year, day );
}

int Date::MonthFromDay( int year, int* day )
{
	int m = MonthFromDayInternal( year, *day );
	// keep things consistent and make days from 1 - 31.
	(*day)++;
	return m;
}

int Date::MonthFromDayInternal( int year, int& day )
{
	bool leap = IsLeapYear( year );
	int feb = leap ? 28 : 27;
	if ( day >= 0 && day <= 30 )	return 0;	day -= 31;	// january
	if ( day >= 0 && day <= feb ) return 1;		day -= feb + 1;	// february
	if ( day >= 0 && day <= 30 ) return 2;		day -= 31;	// march
	if ( day >= 0 && day <= 29 ) return 3;		day -= 30;	// april
	if ( day >= 0 && day <= 30 ) return 4;		day -= 31;	// may
	if ( day >= 0 && day <= 29 ) return 5;		day -= 30;	// june
	if ( day >= 0 && day <= 30 ) return 6;		day -= 31;	// julius
	if ( day >= 0 && day <= 30 ) return 7;		day -= 31;	// augustine
	if ( day >= 0 && day <= 29 ) return 8;		day -= 30;	// septem
	if ( day >= 0 && day <= 30 ) return 9;		day -= 31;	// octem
	if ( day >= 0 && day <= 29 ) return 10;		day -= 30;	// novem
	if ( day >= 0 && day <= 30 ) return 11;		day -= 31;	// decem
	return -1;
}

Date& Date::operator+=( INT64 microSeconds )
{
	mUTC += microSeconds;
	return *this;
}

Date& Date::operator-=( INT64 microSeconds )
{
	mUTC -= microSeconds;
	return *this;
}

double Date::DaysApart( const Date& a, const Date& b )
{
	return ((double) a.mUTC - b.mUTC) / (3600.0 * 24.0 * 1000000.0);
}

double Date::HoursApart( const Date& a, const Date& b )
{
	return ((double) a.mUTC - b.mUTC) / (3600.0 * 1000000.0);
}

double Date::MinutesApart( const Date& a, const Date& b )
{
	return ((double) a.mUTC - b.mUTC) / (60.0 * 1000000.0);
}

double Date::SecondsApart( const Date& a, const Date& b )
{
	return ((double) a.mUTC - b.mUTC) / (1000000.0);
}

Date Date::ParseString( const XString& str )
{
	if ( str.IsEmpty() ) return NullDate();

	if ( str.Length() == SimpleTypeLength )
	{
		Date d = ParseSimple( str );
		if ( !d.IsNull() ) return d;
	}

	INT64 v = 0;
	INT64 nmul = 0;
	XString n;
	bool is1970 = false;

	if ( str.UpCase().Left(6) == _T("UTC70:") )
	{
		is1970 = true;
		n = str.Right( str.Length() - 6 );
		nmul = 1000000;
	}
	else if ( str.UpCase().Left(7) == _T("UTCM70:") )
	{
		is1970 = true;
		n = str.Right( str.Length() - 7 );
		nmul = 1;
	}
	else if ( str.UpCase().Left(8) == _T("UTC1610:") )
	{
		is1970 = false;
		n = str.Right( str.Length() - 8 );
		nmul = 1000000;
	}
	else if ( str.UpCase().Left(9) == _T("UTCM1610:") )
	{
		is1970 = false;
		n = str.Right( str.Length() - 9 );
		nmul = 1;
	}
	else
	{
		n = str;
		nmul = 1;
	}

	if ( n.Length() > 0 )
	{
		v = nmul * _wtoi64( n );
	}

	if ( is1970 )	return NEWTOR_1970( v );
	else			return NEWTOR_1601( v );
}

XStringA Date::FormatAbsIntervalDHMS( const Date& absDifferenceWith ) const
{
	double secs = fabs(SecondsApart( *this, absDifferenceWith ));
	int32 hundreds = (secs - floor(secs)) * 100;
	hundreds = min(hundreds, 99);
	int64 isecs = (int64) secs;
	int32 secsonly = isecs % 60;
	int64 mins = (isecs - secsonly) / 60;
	int32 minsonly = mins % 60;
	int64 hours = (mins - minsonly) / 60;
	int32 hoursonly = hours % 24;
	int64 days = (hours - hoursonly) / 24;
	if ( days > 0 )			return fmt( "%vd %vh %vm %vs", days, hoursonly, minsonly, secsonly );
	else if ( hours > 0 )	return fmt( "%vh %vm %vs", hoursonly, minsonly, secsonly );
	else if ( mins > 0 )	return fmt( "%vm %v.%02vs", minsonly, secsonly, hundreds );
	else					return fmt( "%v.%02vs", secsonly, hundreds );
}

XString Date::ToString() const
{
	return XString::FromFormat( _T("UTCM70:%I64d"), UTC_1601_to_1970(mUTC) );
}

// Returns false if not all characters were between '0' .. '9'
template< typename TCH >
bool ParseIntN( int& res, const TCH* c, int nChars )
{
	res = 0;
	int pos = 1;
	for ( c = c + nChars - 1; nChars > 0; nChars--, c-- )
	{
		uint v = *c - '0';
		if ( v <= 9 )
			res += v * pos;
		else
			return false;
		pos *= 10;
	}
	return true;
}

bool ValidMD( int month, int day )
{
	return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

bool ValidHMS_12( int hour, int min, int sec )
{
	return hour >= 0 && hour <= 12 && min >= 0 && min < 60 && sec >= 0 && sec < 60;
}

bool ValidHMS_24( int hour, int min, int sec )
{
	return hour >= 0 && hour < 24 && min >= 0 && min < 60 && sec >= 0 && sec < 60;
}

template< typename TCH >
bool ParseAMPM( bool& isPM, const TCH* c )
{
	if ( c[1] == 'M' || c[1] == 'm' )
	{
		if ( c[0] == 'a' || c[0] == 'A' ) { isPM = false; return true; }
		if ( c[0] == 'p' || c[0] == 'P' ) { isPM = true; return true; }
	}
	return false;
}

template< typename TCH >
bool ParseTimeTry( int& hour, int& min, int& sec, const TCH* c, int nChars )
{
	// 11 chars: 01:24:38 PM
	//  8 chars: 01:24 PM
	//  7 chars: 1:24 PM
	//  8 chars: 13:24:38
	bool isPM = false;
	int good = -1;
	if ( nChars >= 7 )
	{
		int offset = 0;
		int hlen = 2;
		if ( !isalnum(c[1]) )
		{
			// 1:20 PM
			hlen = 1;
			offset = -1;
		}

		if (	ParseIntN(hour, c, hlen) &&
				ParseIntN(min, c + 3 + offset, 2) )
		{
			if ( nChars == 8 )
			{
				if ( ParseAMPM(isPM, c + 6 + offset) && ValidHMS_12(hour, min, 0) )			good = 1;		// 01:24 PM or 1:24 PM 
				else if ( ParseIntN(sec, c + 6 + offset, 2) && ValidHMS_24(hour, min, sec) ) good = 1;		// 13:24:38
				else good = 0;
			}
			else if ( nChars == 11 )
			{
				if ( ParseIntN(sec, c + 6, 2) && ParseAMPM(isPM, c + 9) && ValidHMS_12(hour, min, sec) ) good = 1; // 01:24:38 PM
				else good = 0;
			}
			else good = 0;
		}
		else good = 0;
	}
	else good = 0;
	ASSERT( good != -1 );
	if ( isPM ) hour += 12;
	return good == 1;
}

template< typename TCH >
bool ParseDateTry( int& year, int& month, int& day, const TCH* c, int nChars )
{
	if ( nChars == 10 )
	{
		// 2009-01-02
		if (	ParseIntN( year, c, 4 ) &&
				ParseIntN( month, c + 5, 2 ) &&
				ParseIntN( day, c + 8, 2 ) &&
				ValidMD( month, day ) )
		{
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This code is HIDEOUSly too much and long. Use a DSL for this, such as scanf.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Eat whitespace
template< typename TCH >
void EatWS( const TCH* c, int& start, int& remain )
{
	while ( remain > 0 && c[start] == ',' || c[start] == ' ' ) { start++; remain--; }
}

// Find two separators
template< typename TCH >
bool FindSeps( const TCH* c, int len, int start, int& s1, int& s2 )
{
	s1 = -1;
	s2 = -1;
	for ( int i = start; i < len; i++ )
	{
		if ( !isalnum(c[i]) )
		{
			if ( s1 == -1 ) s1 = i;
			else
			{
				s2 = i;
				return true;
			}
		}
	}
	return false;
}

Date::Formats Date::DetectFormat( XString str )
{
	str.Trim();
	int year, month, day;
	const wchar_t* s = str;

	if ( str.Length() == 10 )
	{
		if ( ParseDateTry(year, month, day, s, str.Length()) ) return Format_YYYY_MM_DD;		// 2009-01-02
	}

	return Format_Invalid;
}

bool Date::ParseStringTry( const XString& str, Date& result, bool utc, const Date& beginExpect, const Date& endExpect )
{
	int len = str.Length();
	if ( len == SimpleTypeLength )
	{
		result = ParseSimple( str );
		if ( !result.IsNull() ) return true;
	}

	int day, month, year, hour, min, sec;
	const wchar_t* s = str;

	if ( len == 10 )
	{
		// 2009-01-02
		if ( ParseDateTry(year, month, day, s, 10) )
		{
			result = Date::FromYMD_Internal( utc, year, month - 1, day );
			return true;
		}
	}

	if ( len < 15 ) return false;

	// A: 01:01 AM, 05 january 2005
	// B: 2009/May/11 01:24:38 PM
	bool haveTime = false;
	bool haveDate = false;
	int dateStart = 0;
	int timeStart = 0;

	for ( int pass = 0; pass < 2; pass++ )
	{
		if ( !haveTime )
		{
			int remain = len - timeStart;
			EatWS( s, timeStart, remain );

			//if ( ParseTimeTry(hour, min, sec, s + timeStart, remain) )		{ haveTime = true; dateStart = 11; }
			if ( remain >= 11 && ParseTimeTry(hour, min, sec, s + timeStart, 11) )		{ haveTime = true; dateStart = 11; }
			else if ( remain >= 8 && ParseTimeTry(hour, min, sec, s + timeStart, 8) )	{ haveTime = true; dateStart = 8; }
		}

		if ( !haveDate )
		{
			int remain = len - dateStart;
			EatWS( s, dateStart, remain );

			int sep1, sep2;
			if ( remain >= 11 )
			{
				if ( FindSeps(s, len, dateStart, sep1, sep2) )
				{
					int monlen = sep2 - sep1 - 1;
					month = MonthFromName( monlen, s + sep1 + 1 );
					if ( month != -1 )
					{
						bool ok = false;
						int end = -1;
						if ( sep1 - dateStart == 4 && len - sep2 > 2 )
						{
							// 2009/May/01
							ok =	ParseIntN( year, s + dateStart, 4 ) &&
										ParseIntN( day, s + sep2 + 1, 2 );
							end = sep2 + 4;
						}
						else if ( (sep1 - dateStart == 2 || sep1 - dateStart == 1) && len - sep2 > 4 )
						{
							// 06 January 2009
							// 6 January 2009
							int daylen = sep1 - dateStart;
							ok =	ParseIntN( day, s + dateStart, daylen ) &&
										ParseIntN( year, s + sep2 + 1, 4 );
							end = sep2 + 6;
						}
						if ( ok && ValidMD(month + 1, day) )
						{
							haveDate = true;
							timeStart = end;
						}
					}
				}
			}
		}
	}

	if ( haveDate && haveTime )
	{
		result = Date::FromYMD_Internal( utc, year, month, day, hour, min, sec );
		return true;
	}
	return false;

	/*
	XString up = str.UpCase();

	// this is only made to match "01:01 AM, 05 january 2005"
	// it assumes that the time is local time.

	// should really split date and time sections and do them independently
	XString first5 = up.Left( 5 );
	XString first9 = up.Left( 9 );
	
	// whitespace-split
	dvect<XString> wss;
	Split< TCHAR, dvect<XString> >( up, ' ', wss, true );

	if ( first5.Count( ':' ) == 1 )
	{
		int colpos = first5.Find( ':' );
		int hour = ParseInt( first5.Left( colpos ) );
		int minute = ParseInt( first5.Mid( colpos + 1, 2 ) );
		if (	hour >= 0 && hour < 24 && minute >= 0 && minute < 60 )
		{
			int amPos = first9.Find( _T("AM") );
			int pmPos = first9.Find( _T("PM") );
			if ( pmPos > 0 ) 
			{
				ASSERT( hour <= 12 );
				hour += 12;
			}
			if ( wss.size() >= 4 )
			{
				int day = ParseInt( wss[ wss.size() - 3 ] );
				int month = MonthFromName( wss[ wss.size() - 2 ] );
				int year = ParseInt( wss[ wss.size() - 1 ] );
				if ( day >= 1 && day <= 31 && month >= 0 && month < 12 )
				{
					result = Date::FromYMD_Internal( utc, year, month, day, hour, minute );
					return true;
				}
			}
		}
	}
	*/

	return false;
}

Date Date::Parse_DD_MM_YYYY( const char* str, bool utc )		{ return TParse_DD_MM_YYYY( str, utc ); }
Date Date::Parse_DD_MM_YYYY( const wchar_t* str, bool utc )		{ return TParse_DD_MM_YYYY( str, utc ); }

Date Date::Parse_YYYY_MM_DD( const char* str, bool utc )		{ return TParse_YYYY_MM_DD( str, utc ); }
Date Date::Parse_YYYY_MM_DD( const wchar_t* str, bool utc )		{ return TParse_YYYY_MM_DD( str, utc ); }

Date Date::Parse_YYYY_MM_DD_HH_MM_SS_MS( const char* str, bool utc )		{ return TParse_YYYY_MM_DD_HH_MM_SS_MS( str, utc ); }
Date Date::Parse_YYYY_MM_DD_HH_MM_SS_MS( const wchar_t* str, bool utc )		{ return TParse_YYYY_MM_DD_HH_MM_SS_MS( str, utc ); }

template< typename TCH >
Date Date::TParse_DD_MM_YYYY( const TCH* str, bool utc )
{
	int d = (str[0] - '0') * 10 + (str[1] - '0');
	int m = (str[3] - '0') * 10 + (str[4] - '0');
	int y = (str[6] - '0') * 1000 + (str[7] - '0') * 100 + (str[8] - '0') * 10 + (str[9] - '0');
	if ( d < 1 || d > 31 || m < 1 || m > 12 || y < 0 )
		return Date();
	else
		return Date::FromYMD_Internal( utc, y, m - 1, d );
}

template< typename TCH >
void Date::TParseYYYY_MM_DD( const TCH* str, int& y, int& m, int& d )
{
	d = (str[8] - '0') * 10 + (str[9] - '0');
	m = (str[5] - '0') * 10 + (str[6] - '0');
	y = (str[0] - '0') * 1000 + (str[1] - '0') * 100 + (str[2] - '0') * 10 + (str[3] - '0');
}

template< typename TCH >
void Date::TParseHH_MM_SS_MS( const TCH* str, int& h, int& m, int& s, int& microsec )
{
	microsec = 0;
	h = (str[0] - '0') * 10 + (str[1] - '0');
	m = (str[3] - '0') * 10 + (str[4] - '0');
	s = (str[6] - '0') * 10 + (str[7] - '0');
	if ( str[8] == '.' )
	{
		// 01:34:67.9012345678
		int i;
		for ( i = 9; i < 9 + 6; i++ )
		{
			if ( str[i] < '0' || str[i] > '9' ) break;
			microsec = microsec * 10 + str[i] - '0';
		}
		for ( ; i < 9 + 6; i++ )
			microsec = 10 * microsec;
	}
}

template< typename TCH >
Date Date::TParse_YYYY_MM_DD( const TCH* str, bool utc )
{
	int y,m,d;
	TParseYYYY_MM_DD( str, y, m, d );
	if ( d < 1 || d > 31 || m < 1 || m > 12 || y < 0 )
		return Date();
	else
		return Date::FromYMD_Internal( utc, y, m - 1, d );
}

template< typename TCH >
Date Date::TParse_YYYY_MM_DD_HH_MM_SS_MS( const TCH* str, bool utc )
{
	int y,m,d, h,min,s,ms;
	TParseYYYY_MM_DD( str, y, m, d );
	TParseHH_MM_SS_MS( str + 11, h, min, s, ms );
	if ( d < 1 || d > 31 || m < 1 || m > 12 || y < 0 )
		return Date();
	else
		return Date::FromYMD_Internal( utc, y, m - 1, d, h, min, s, ms );
}

template< typename TCH >
int TMonthFromName( int n, const TCH* name )
{
	/*

	m = {'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'}
	all = {}
	for k,v in pairs(m) do
		v = v:upper()
		x = v:byte(1) * 65536 + v:byte(2) * 256 + v:byte(3)
		all[#all + 1] = string.format('case 0x%X: return %d;', x, k - 1)
	end

	table.sort(all)
	for k,v in pairs(all) do print( v ) end

	*/
	if ( n < 3 ) return -1;
	uint v = (toupper(name[0]) << 16) | (toupper(name[1]) << 8) | toupper(name[2]);

	switch ( v )
	{
	case 0x415052: return 3;
	case 0x415547: return 7;
	case 0x444543: return 11;
	case 0x464542: return 1;
	case 0x4A414E: return 0;
	case 0x4A554C: return 6;
	case 0x4A554E: return 5;
	case 0x4D4152: return 2;
	case 0x4D4159: return 4;
	case 0x4E4F56: return 10;
	case 0x4F4354: return 9;
	case 0x534550: return 8;
	}
	
	return -1;
}

int Date::MonthFromName( int n, const char* name )
{
	return TMonthFromName( n, name );
}

int Date::MonthFromName( int n, const wchar_t* name )
{
	return TMonthFromName( n, name );
}

//int Date::MonthFromName( const XString& name )
//{
//	XString up = name.UpCase();
//	for ( int i = 0; i < 12; i++ )
//	{
//		// require 3 characters to disambiguate julian month names
//		if ( MonthName(i).UpCase().MatchCount( up ) >= 3 )
//		return i;
//	}
//	return -1;
//}

// disable deprecation warnings.
#pragma warning( push )
#pragma warning( disable: 4996 )
void Date::MonthName( int m, char* buff, int fillChars )
{
	switch ( m )
	{
	case 0: strncpy( buff, "January", fillChars ); break;
	case 1: strncpy( buff, "February", fillChars ); break;
	case 2: strncpy( buff, "March", fillChars ); break;
	case 3: strncpy( buff, "April", fillChars ); break;
	case 4: strncpy( buff, "May", fillChars ); break;
	case 5: strncpy( buff, "June", fillChars ); break;
	case 6: strncpy( buff, "July", fillChars ); break;
	case 7: strncpy( buff, "August", fillChars ); break;
	case 8: strncpy( buff, "September", fillChars ); break;
	case 9: strncpy( buff, "October", fillChars ); break;
	case 10: strncpy( buff, "November", fillChars ); break;
	case 11: strncpy( buff, "December", fillChars ); break;
	default: strncpy( buff, "XXXXXXXXX", fillChars ); break;
	}
	buff[fillChars] = 0;
}
void Date::MonthName( int m, wchar_t* buff, int fillChars )
{
	switch ( m )
	{
	case 0: wcsncpy( buff, L"January", fillChars ); break;
	case 1: wcsncpy( buff, L"February", fillChars ); break;
	case 2: wcsncpy( buff, L"March", fillChars ); break;
	case 3: wcsncpy( buff, L"April", fillChars ); break;
	case 4: wcsncpy( buff, L"May", fillChars ); break;
	case 5: wcsncpy( buff, L"June", fillChars ); break;
	case 6: wcsncpy( buff, L"July", fillChars ); break;
	case 7: wcsncpy( buff, L"August", fillChars ); break;
	case 8: wcsncpy( buff, L"September", fillChars ); break;
	case 9: wcsncpy( buff, L"October", fillChars ); break;
	case 10: wcsncpy( buff, L"November", fillChars ); break;
	case 11: wcsncpy( buff, L"December", fillChars ); break;
	default: wcsncpy( buff, L"XXXXXXXXX", fillChars ); break;
	}
	buff[fillChars] = 0;
}
#pragma warning( pop )

XString Date::MonthName( int m )
{
	switch ( m )
	{
	case 0: return "January";
	case 1: return "February";
	case 2: return "March";
	case 3: return "April";
	case 4: return "May";
	case 5: return "June";
	case 6: return "July";
	case 7: return "August";
	case 8: return "September";
	case 9: return "October";
	case 10: return "November";
	case 11: return "December";
	default: return "Invalid Month";
	}
}

XString Date::FormatTime( bool utc ) const
{
	if ( IsNull() ) return "Never";
	DateTime dt = GetDetails( utc );
	XString str;
	bool pm = dt.Hour >= 12;
	int hour = dt.Hour % 12;
	if ( pm && hour == 0 ) hour = 12;
	str += XString::FromFormat( _T("%d:%02d "), hour, dt.Min );
	str += pm ? _T("PM") : _T("AM");
	return str;
}

XStringA Date::FormatSqlDateTime( bool utc ) const
{
	if ( IsNull() ) return "0000-00-00 00:00:00";
	DateTime dt = GetDetails( utc );
	return XStringA::FromFormat( "%04d-%02d-%02d %02d:%02d:%02d", 
		(int) dt.Year,
		(int) dt.Month + 1,
		(int) dt.Day,
		(int) dt.Hour,
		(int) dt.Min,
		(int) dt.Sec );
}

XString Date::FormatTimeSeconds( bool utc ) const
{
	if ( IsNull() ) return "Never";
	DateTime dt = GetDetails( utc );
	XString str;
	bool pm = dt.Hour >= 12;
	int hour = dt.Hour % 12;
	if ( pm && hour == 0 ) hour = 12;
	str += XString::FromFormat( _T("%d:%02d:%02d "), hour, (int) dt.Min, (int) dt.Sec );
	str += pm ? _T("PM") : _T("AM");
	return str;
}

XString Date::FormatDate( bool utc ) const
{
	if ( IsNull() ) return "Never";
	DateTime dt = GetDetails( utc );
	XString str;
	str += XString::FromFormat( _T("%d "), (int) dt.Day );
	str += MonthName( dt.Month ) + _T(" ");
	str += XString::FromFormat( _T("%d"), dt.Year );
	return str;
}

XString Date::FormatTimeDate( bool utc ) const
{
	if ( IsNull() ) return "Never";
	XString time = FormatTime(utc);
	XString date = FormatDate(utc);
	return time + _T(", ") + date;
}

XString Date::FormatDate_YYYY_MM_DD( bool utc ) const
{
	DateTime dt = GetDetails( utc );
	return XString::FromFormat( L"%04d-%02d-%02d", dt.Year, dt.Month + 1, dt.Day );
}

#define BUFLEN 256

XStringA Date::FormatA( const char* format, bool utc ) const
{
	char buf[BUFLEN];
	tm tv;
	ToTM( utc, tv );
	if ( strftime( buf, arraysize(buf), format, &tv ) == 0 )
		return "DATE-FORMAT-OVERFLOW";
	buf[BUFLEN - 1] = 0;
	return XStringA( buf );
}

XStringW Date::Format( const wchar_t* format, bool utc ) const
{
	return FormatA( XString(format).ToUtf8(), utc ).ToWideFromUtf8();
}

XString Date::FormatSysShortDate( bool utc ) const
{
	if ( IsNull() ) return "Never";
#ifdef _WIN32
	SYSTEMTIME st = GetSYSTEMTIME(utc);
	TCHAR ds[BUFLEN] = L"";
	GetDateFormat( LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, ds, BUFLEN-1 );
	return ds;
#else
	return Format( L"%x", utc );
#endif
}

XString Date::FormatSysLongDate( bool utc ) const
{
	if ( IsNull() ) return "Never";
#ifdef _WIN32
	SYSTEMTIME st = GetSYSTEMTIME(utc);
	TCHAR ds[BUFLEN] = L"";
	GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, ds, BUFLEN-1 );
	return ds;
#else
	return Format( L"%c", utc );
#endif
}

XString Date::FormatSysTime( bool utc ) const
{
	if ( IsNull() ) return "Never";
#ifdef _WIN32
	SYSTEMTIME st = GetSYSTEMTIME(utc);
	TCHAR ds[BUFLEN] = L"";
	GetTimeFormat( LOCALE_USER_DEFAULT, 0, &st, NULL, ds, BUFLEN-1 );
	return ds;
#else
	return Format( L"%X", utc );
#endif
}

XString Date::FormatSimple( int time_zone_offset ) const
{
	char buf[SimpleTypeLength + 1];
	FormatSimple( buf, time_zone_offset );
	return buf;
}

int Date::LocalTimezoneOffsetMinutes()
{
#ifdef _WIN32
	TIME_ZONE_INFORMATION zone;
	GetTimeZoneInformation( &zone );
	// Bias = seconds west of UTC
	return zone.Bias;
#else
	// tzset() is safe to call from multiple threads, which is why we use this lazy initialization system.
	if ( tzsetCalled == 0 )
	{
		tzset();
		tzsetCalled = 1;
	}
	// timezone = seconds west of UTC
	return ((int) timezone) / 60;
#endif
}

void Date::FormatSimple( char* str, int time_zone_offset ) const
{
	// "17:57:01 +0200, 29 Jan 2003"
	if ( time_zone_offset == -10000 )
		time_zone_offset = LocalTimezoneOffsetMinutes();
	else
		time_zone_offset = -time_zone_offset;

	INT64 copy = IsNull() ? mUTC : UTC_AddMinutes( mUTC, -time_zone_offset );

	DateTime dt;
	UTC_Decode( copy, dt );

	int bias_h = abs( time_zone_offset / 60 );
	int bias_m = abs( time_zone_offset % 60 );

	memcpy( str, "99:99:99 +9999, 99 xxx 9999", 28 );

	char mon[4];
	MonthName( dt.Month, mon, 3 );

	sprintf( str, "%02d:%02d:%02d %c%02d%02d, %02d %s %04d",
		(int) dt.Hour,
		(int) dt.Min,
		(int) dt.Sec,
		time_zone_offset <= 0 ? '+' : '-',
		bias_h,
		bias_m,
		(int) dt.Day,
		mon,
		(int) dt.Year );

	str[SimpleTypeLength] = 0;
}

Date Date::ParseSimple( const char* str )
{
	return TParseSimple(str);
}

Date Date::ParseSimple( const wchar_t* str )
{
	return TParseSimple(str);
}

template< typename TCH > int TAtoI( const TCH* str ) { return 0; }
template<> int TAtoI( const char* str )		{ return atoi(str); }
template<> int TAtoI( const wchar_t* str )	{ return _wtoi(str); }

template< typename TCH, int len > int AtoILen( const TCH* str )
{
	// 123
	int v = 0;
	for ( int i = 0; i < len; i++ )
	{
		v *= 10;
		v += str[i] - '0';
		//v += str[len - 1 - i] - '0';
	}
	return v;
}


template< typename TCH >
Date Date::TParseSimple( const TCH* str )
{
	// "17:57:01 +0200, 29 Jan 2003"
	int len = (int) TStrLen(str);
	if ( len != SimpleTypeLength ) return Date::NullDate();
	const TCH* all = str;
	TCH ho[3], mi[3], se[3], uth[3], utm[3], da[3], mon[4], year[5];
	const size_t TS = sizeof(TCH);
	memcpy( ho, all + 0, 2*TS );		ho[2] = 0;
	memcpy( mi, all + 3, 2*TS );		mi[2] = 0;
	memcpy( se, all + 6, 2*TS );		se[2] = 0;
	
	TCH ups = all[9]; 
	if ( ups != '+' && ups != '-' ) return Date::NullDate();
	bool time_positive = ups == '+';

	memcpy( uth, all + 10, 2*TS );		uth[2] = 0;
	memcpy( utm, all + 12, 2*TS );		utm[2] = 0;
	memcpy( da, all + 16, 2*TS );		da[2] = 0;
	memcpy( mon, all + 19, 3*TS );		mon[3] = 0;
	memcpy( year, all + 23, 4*TS );		year[4] = 0;
	XString_MakeUpCase( mon );
	int imon = 0;
	switch ( mon[0] )
	{
	case 'J':
		if ( mon[1] == 'A' ) imon = 0;			// jan
		else if ( mon[2] == 'N' ) imon = 5;		// june
		else if ( mon[2] == 'L' ) imon = 6;		// july
		break;
	case 'F': imon = 1; break;				// feb
	case 'M':
		if ( mon[2] == 'R' ) imon = 2;			// march
		else if ( mon[2] == 'Y' ) imon = 4;		// may
		break;
	case 'A': 
		if ( mon[2] == 'R' ) imon = 3;			// april
		else if ( mon[2] == 'G' ) imon = 7;		// august
		break;
	case 'S': imon = 8; break;				// sep
	case 'O': imon = 9; break;				// oct
	case 'N': imon = 10; break;				// nov
	case 'D': imon = 11; break;				// dec
	default: return NullDate();
	}

	int iyear = TAtoI( year );
	int ida = TAtoI( da );
	int iutm = TAtoI( utm );
	int iuth = TAtoI( uth );
	int ise = TAtoI( se );
	int imi = TAtoI( mi );
	int iho = TAtoI( ho );

	if ( ida < 1 || ida > 31 ) return NullDate();
	if ( ise < 0 || ise > 59 ) return NullDate();

	int bias = iuth * 60 + iutm;
	if ( !time_positive ) bias *= -1;
	Date d = Date::FromYMD_Internal( true, iyear, imon, ida, iho, imi, ise, 0 );
	d -= (INT64) bias * 60 * 1000 * 1000;
	return d;
}

void Date::Format( char* str, Formats f, bool utc, int time_zone_offset_minutes ) const
{
	DateTime dt = GetDetails( utc );
	switch ( f )
	{
	case Format_YYYY_MM_DD: 				sprintf( str, "%04u-%02d-%02u", dt.Year, dt.Month + 1, dt.Day ); break;
	case Format_DD_MM_YYYY: 				sprintf( str, "%02u-%02d-%04u", dt.Day, dt.Month + 1, dt.Year ); break;
	case Format_YYYY_MM_DD_HH_MM_SS_MS:		sprintf( str, "%04u-%02d-%02u %02u:%02u:%02u.%06u", dt.Year, dt.Month + 1, dt.Day, dt.Hour, dt.Min, dt.Sec, dt.Microsecond ); break;
	case Format_Simple:						FormatSimple( str, time_zone_offset_minutes ); break;
	case Format_HTTP_RFC_1123:				ASSERT(utc); FormatHttp( str ); break;
	default: ASSERT(false); return;
	}
}

Date Date::Parse( Formats f, const char* str, bool utc )
{
	switch ( f )
	{
	case Format_DD_MM_YYYY: 				return Parse_DD_MM_YYYY(str, utc);
	case Format_YYYY_MM_DD: 				return Parse_YYYY_MM_DD(str, utc);
	case Format_YYYY_MM_DD_HH_MM_SS_MS:		return Parse_YYYY_MM_DD_HH_MM_SS_MS(str, utc);
	case Format_Simple:						return ParseSimple(str);
	case Format_HTTP_RFC_1123:				return ParseHttp(str);
	default: ASSERT(false); return Date();
	}
}

const char* WeekDayTable[] = {
	"Sunday",
	"Monday",
	"Tueday",
	"Wedneday",
	"Thursday",
	"Friday",
	"Saturday",
};

Date Date::ParseHttp( const char* str )
{
	// 0         10        20
	// 01234567890123456789012345678
	// Sun, 06 Nov 1994 08:49:37 GMT
	if ( str[0] == 0 )
		return Date();
	int dmon = AtoILen<char, 2>( str + 5 );
	int year = AtoILen<char, 4>( str + 12 );
	int h = AtoILen<char, 2>( str + 17 );
	int m = AtoILen<char, 2>( str + 20 );
	int s = AtoILen<char, 2>( str + 23 );
	int mon = MonthFromName( 3, str + 8 );
	ASSERT( str[26] == 'G' && str[27] == 'M' && str[28] == 'T' );
	return Date::FromYMD_UTC( year, mon, dmon, h, m, s );
}

void Date::FormatHttp( char* buf ) const
{
	DateTime dt = GetDetails( true );
	
	// Sun, 06 Nov 1994 08:49:37 GMT
	char day[4], mon[4];
	memcpy( day, WeekDayTable[dt.WeekDay], 3 );
	MonthName( dt.Month, mon, 3 );
	day[3] = 0;
	mon[3] = 0;
	sprintf( buf, "%s, %02u %s %04u %02u:%02u:%02u GMT", day, dt.Day, mon, dt.Year, dt.Hour, dt.Min, dt.Sec );
}

XStringA Date::FormatHttp() const
{
	char buf[30];
	FormatHttp( buf );
	return buf;
}

bool Date::MatchFormat( Formats f, const char* str, bool utc )
{
	return !Parse( f, str, utc ).IsNull();
}

Date::Formats Date::MatchFormatFirst( const char* str, bool utc )
{
	for ( int i = Format_FIRST; i < Format_END; i++ )
	{
		if ( MatchFormat( (Formats) i, str, utc ) )
			return (Formats) i;
	}
	return Format_Invalid;
}


}
}
