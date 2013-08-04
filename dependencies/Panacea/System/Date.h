#pragma once

#include "../Other/UTC.h"
#include "../Platform/winheaders.h"

namespace Panacea
{
namespace Sys
{

/** A small date/time class that is internally represented as UTC microseconds since 1601.
We use zero as a null date, which means that the first microsecond of 1601 is not representable. Too bad.

NOTE: Date is typedef'ed below to AbcDate. Use that going forward instead of Panacea::Sys::Date. Eventually I'll rename this so that it's only name is AbcDate.

**/
class PAPI Date
{
public:
	
	INT64 mUTC;		// Microseconds (millionth) after Midnight Jan 1 1601. Only data member.

	enum Formats
	{
		Format_Invalid = 0,
		Format_FIRST = 1,
		Format_YYYY_MM_DD = Format_FIRST,		///< 1999/12/31
		Format_DD_MM_YYYY = 2,					///< 31/12/1999
		Format_Simple = 3,						///< 17:57:01 +0200, 31 Dec 1999
		Format_YYYY_MM_DD_HH_MM_SS_MS = 4,		///< 1999-12-31 17:57:01.123456
		Format_HTTP_RFC_1123 = 5,				///< Tue, 15 Nov 1994 08:12:31 GMT    or    Sun, 06 Nov 1994 08:49:37 GMT
		Format_END,
	};

	/// Maximum year AD that we allow
	static const int MaxYear = 3000;

	static const INT64 MaxLegalVal = 3000 * (INT64) 366 * (INT64) 86400 * (INT64) 1000000;

	static const INT64 NullVal = 0;

	// Initialize to NULL
	Date();								

#ifdef _WIN32
	Date( const SYSTEMTIME& st );
#endif
	Date( const FILETIME& st );
	//Date( INT64 micro_seconds_utc );	// too ambiguous and error prone

	// Convert from nanoseconds to the units of mUTC
	static INT64 NanosecondsToInternalUnits( int64 nanoseconds ) { return nanoseconds / 1000; }

	// TODO: Rename these to something less ugly.
	// Suggestion: From1601Epoch, From1900Epoch, From1970Epoch, From2000Epoch
	static Date NEWTOR_1601( INT64 utc_1601_microseconds );
	static Date NEWTOR_1900( INT64 utc_1900_microseconds );
	static Date NEWTOR_1970( INT64 utc_1970_microseconds );
	static Date NEWTOR_2000( INT64 utc_2000_microseconds );

	static Date FromSecondsUTC_1970( double utc_1970_seconds );

	static int LocalTimezoneOffsetMinutes();

	/** Date from year, month, day of month, etc (Local time, system decides whether to use DST). Month is from 0 to 11. Day is from 1 to 31.
	This is essentially a wrapper for FromYMD()
	**/
	Date( int year, int month_0_to_11, int day_1_to_31, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );

	/// Date from year, month, day of month, etc (Local time, system decides whether to use DST). Month is from 0 to 11. Day is from 1 to 31.
	static Date FromYMD_Local( int year, int month_0_to_11, int day_1_to_31, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );

	/// Date from year, month, day of month, etc (UTC). Month is from 0 to 11. Day is from 1 to 31.
	static Date FromYMD_UTC( int year, int month_0_to_11, int day_1_to_31, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );

	/// Date from year, month, day of month, etc (UTC). Month is from 0 to 11. Day is from 1 to 31.
	static Date UTC( int year, int month_0_to_11, int day_1_to_31, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 )
	{
		return FromYMD_UTC( year, month_0_to_11, day_1_to_31, hour, minute, second, microsecond );
	}

	// Date from year and day of year (Local time, system decides whether to use DST). Jan 1st is day zero. 
	static Date FromYearDay_Local( int year, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );

	// Date from year and day of year (UTC). Jan 1st is day zero. 
	static Date FromYearDay_UTC( int year, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );

	static Date FromYMD_Internal( bool utc, int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );
	static Date FromYearDay( bool utc, int year, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0 );

	static Date FromFILETIMEi64( int64 t )
	{
		FILETIME ft;
		ft.dwHighDateTime = t >> 32;
		ft.dwLowDateTime = t & 32;
		return Date( ft );
	}

	static Date FromTM( bool utc, const tm& t );

	/** Returns true if the year is a leap year. 
	Takes century and quad-century adjustments into account.
	**/
	static bool IsLeapYear( int year )
	{
		return (year % 4 == 0) && !( (year % 100 == 0) && (year % 400 != 0) );
	}

	/// Returns number of days in year, taking leap years into account.
	static int DaysOfYear( int year )
	{
		return IsLeapYear( year ) ? 366 : 365;
	}

	/// Returns the month (from 0 to 11) given the day and year. Leap years are considered. Returns -1 if day is invalid.
	static int MonthFromDay( int year, int day );

	/** Returns the month (from 0 to 11) given the day and year. 
	Leap years are considered. 
	Returns -1 if day is invalid.
	Day will be adjusted to the day of the month (from 1 to 31)
	**/
	static int MonthFromDay( int year, int* day );

	/** Returns the year from the day. Day is measured in days since 1st January 2000, which is day zero.
	This function is lazy and slow, and only works up until 2500. The year returned is in AD years.
	@param dayOfYear If not null, will be assigned the value of the day within the year in which it falls.
	**/
	static int YearFromDaySince2000( int day, int* dayOfYear = NULL );

	/** Returns a date given the number of days since 1st Jan 2000 (UTC midnight).
	This function is lazy and slow, and only works up until 2500. The year returned is in AD years.
	**/
	static Date DateFromDaySince2000( int day )
	{
		int year, day_of_year;
		year = YearFromDaySince2000( day, &day_of_year );
		if ( year < 0 ) return Date();
		return FromYearDay_UTC( year, day_of_year );
	}

	AbCore::DateTime	GetDetails( bool utc ) const;
	bool				GetDetails( bool utc, AbCore::DateTime& dt ) const;
	void				ToTM( bool utc, tm& t ) const;

	/** Returns the number of days that have passed since Jan 1st 2000. 
	Returns -1 for dates behind 2000.
	This function, like YearFromDaySince2000, is iterative, lazy, and slow.
	**/
	int DaysSince2000() const;

	/// Fills the structure with the current time.
	static Date Now();

	/// Fills the structure with the current time + n seconds.
	static Date SecondsFromNow( double seconds ) { Date d = Now(); d.AddSeconds(seconds); return d; }

	/// Fills the structure with the current time + n minutes.
	static Date MinutesFromNow( double minutes ) { Date d = Now(); d.AddMinutes(minutes); return d; }

	/// Fills the structure with the current time + n hours.
	static Date HoursFromNow( double hours ) { Date d = Now(); d.AddHours(hours); return d; }

	/// Fills the structure with the current time + n days.
	static Date DaysFromNow( double days ) { Date d = Now(); d.AddDays(days); return d; }

	/// Return month name from month. Month is from 0 to 11.
	static XString MonthName( int m );

	/// Copies the desired first n characters of the month name to the specified buffer, adding a null terminator. Month is from 0 to 11.
	static void MonthName( int m, char* buff, int fillChars );

	/// Copies the desired first n characters of the month name to the specified buffer, adding a null terminator. Month is from 0 to 11.
	static void MonthName( int m, wchar_t* buff, int fillChars );

	/// Return month integer from month name. Integer is from 0 to 11, or -1 for error.
	static int MonthFromName( int n, const char* name );
	static int MonthFromName( int n, const wchar_t* name );

	/// Returns the signed number of days between the two dates (computed as a - b).
	static double DaysApart( const Date& a, const Date& b );

	/// Returns the signed number of hours between the two dates (computed as a - b).
	static double HoursApart( const Date& a, const Date& b );

	/// Returns the signed number of hours between the two dates (computed as a - b).
	static double MinutesApart( const Date& a, const Date& b );

	/// Returns the signed number of seconds between the two dates (computed as a - b).
	static double SecondsApart( const Date& a, const Date& b );

	/// Returns this time in seconds minus the parameter
	double SecondsSince( const Date& b ) const { return SecondsApart( *this, b ); }

	/// Return month name.
	XString MonthName( bool utc = false ) const { return MonthName(GetDetails(utc).Month); }

	/// Output as the system default short date
	XString FormatSysShortDate( bool utc = false ) const;

	/// Output as the system default long date
	XString FormatSysLongDate( bool utc = false ) const;

	/// Output as the system default time
	XString FormatSysTime( bool utc = false ) const;

	// Formats using strftime.
	XString Format( const wchar_t* format, bool utc = false ) const;

	// Formats using strftime.
	XStringA FormatA( const char* format, bool utc = false ) const;

	/// Number of characters used by Simple formatting.
	static const int SimpleTypeLength = 27;

	/** Output as "17:57:01 +0200, 29 Jan 2003".
	Intended for easy and consistent parsing for computers and humanoids alike.
	The +0230 is for +0230 GMT minutes. There is always a +hhmm or -hhmm.
	
	@param time_zone_offset If not the default value of -10000, then this is a manual
	specification of the time zone minutes bias to use. +2 * 60 would mean the time zone is 2 hours 
	behind (east of) grennwhich. The default value causes the current system's time zone
	to be used.
	**/
	XString FormatSimple( int time_zone_offset_minutes = -10000 ) const;

	void FormatSimple( char* str, int time_zone_offset_minutes = -10000 ) const;

	/// Output as "12:32 AM, 03 January 2003"
	XString FormatTimeDate( bool utc = false ) const;

	/// Output as "YYYY-MM-DD HH:MM:SS"
	XStringA FormatSqlDateTime( bool utc = false ) const;

	/// Output as "Sun, 06 Nov 1994 08:49:37 GMT" (30 characters including terminator)
	void FormatHttp( char* buf ) const;

	/// Output as "Sun, 06 Nov 1994 08:49:37 GMT" (30 characters including terminator)
	XStringA FormatHttp() const;

	/// Output as "12:32 AM"
	XString FormatTime( bool utc = false ) const;

	/// Output as "12:32:59 AM"
	XString FormatTimeSeconds( bool utc = false ) const;

	/// Output as "15 December 2003"
	XString FormatDate( bool utc = false ) const;

	/// Output as "2008-12-31"
	XString FormatDate_YYYY_MM_DD( bool utc = false ) const;

	void Format( char* str, Formats f, bool utc, int time_zone_offset_minutes = -10000 ) const;

	// Returns 3d 23h 10m 60.00s. Absolute value of time difference is used. Hours or minutes are omitted if zero.
	XStringA FormatAbsIntervalDHMS( const Date& absDifferenceWith ) const;

	/// Output as "UTCM70:micro_seconds_utc"
	XString ToString() const;

	/// Parse from "UTCM70:micro_seconds_utc". Will also accept Simple formatting.
	static Date ParseString( const XString& str );

	/// Try and parse any date string. Local Time assumed. Will also accept Simple formatting.
	static bool ParseStringTry( const XString& str, Date& result, bool utc = false, const Date& beginExpect = Date(), const Date& endExpect = Date() );

	static Formats DetectFormat( XString str );

	/// Parse from FormatSimple(), and only from that. Upon error returns a null date.
	template< typename TCH >
	static Date TParseSimple( const TCH* str );

	static Date ParseSimple( const char* str );
	static Date ParseSimple( const wchar_t* str );

	static Date ParseHttp( const char* str );

	/// Parse from DD-MM-YYYY
	static Date Parse_DD_MM_YYYY( const char* str, bool utc = false );
	static Date Parse_DD_MM_YYYY( const wchar_t* str, bool utc = false );

	/// Parse from YYYY-MM-DD
	static Date Parse_YYYY_MM_DD( const char* str, bool utc = false );
	static Date Parse_YYYY_MM_DD( const wchar_t* str, bool utc = false );

	/// Parse from YYYY-MM-DD HH:MM:SS.MS
	static Date Parse_YYYY_MM_DD_HH_MM_SS_MS( const char* str, bool utc = false );
	static Date Parse_YYYY_MM_DD_HH_MM_SS_MS( const wchar_t* str, bool utc = false );

	static Date Parse( Formats f, const char* str, bool utc );
	static bool MatchFormat( Formats f, const char* str, bool utc );
	static Formats MatchFormatFirst( const char* str, bool utc );

	/// Return date as FILETIME
	FILETIME ToFileTime() const;

	/// Return date as FILETIME, combined into a 64-bit integer.
	INT64 ToFileTimei64() const;

	/// Adds the specified number of nanoseconds to the date
	void AddNanoseconds( int64 nano ) { *this += NanosecondsToInternalUnits(nano); }

	/// Adds the specified number of seconds to the date
	void AddSeconds( double seconds ) { *this += seconds * 1000000; }

	/// Adds the specified number of minutes to the date
	void AddMinutes( double minutes ) { *this += minutes * 60.0 * 1000000.0; }

	/// Adds the specified number of hours to the date
	void AddHours( double hours ) { *this += hours * 3600.0 * 1000000.0; }

	/// Adds the specified number of days to the date
	void AddDays( double days ) { *this += days * 24.0 * 3600.0 * 1000000.0; }

	/// Returns a date with the specified number of seconds added
	Date PlusSeconds( double seconds ) const { Date d = *this; d.AddSeconds(seconds); return d; }

	/// Returns a date with the specified number of minutes added
	Date PlusMinutes( double minutes ) const { Date d = *this; d.AddMinutes(minutes); return d; }

	/// Returns a date with the specified number of hours added
	Date PlusHours( double hours ) const { Date d = *this; d.AddHours(hours); return d; }

	/// Returns a date with the specified number of days added
	Date PlusDays( double days ) const { Date d = *this; d.AddDays(days); return d; }

	Date& operator-=( INT64 microSeconds );
	Date& operator+=( INT64 microSeconds );

	bool operator==( const Date& b ) const { return mUTC == b.mUTC; }

	bool operator!=( const Date& b ) const { return mUTC != b.mUTC; }

	bool operator<( const Date& b ) const { return mUTC < b.mUTC; }
	bool operator>( const Date& b ) const { return mUTC > b.mUTC; }
	bool operator<=( const Date& b ) const { return mUTC <= b.mUTC; }
	bool operator>=( const Date& b ) const { return mUTC >= b.mUTC; }

#ifdef _WIN32
	/// Returns a SYSTEMTIME in either UTC or Local time.
	SYSTEMTIME GetSYSTEMTIME( bool utc ) const;

	/// Returns a SYSTEMTIME in UTC.
	operator SYSTEMTIME() const;
#endif

	/// Default construction is 'null'.
	bool IsNull() const { return mUTC == NullVal; }

	/// Returns a null date
	static Date NullDate() { return NEWTOR_1601(NullVal); }

	/// Seconds after midnight jan 1 1601.
	double SecondsUTC_1601() const { return mUTC / 1000000.0; }

	/// Seconds after midnight jan 1 1970.
	double SecondsUTC_1970() const { return AbCore::UTC_1601_to_1970(mUTC) / 1000000.0; }

	/// Seconds after midnight jan 1 2000.
	double SecondsUTC_2000() const { return AbCore::UTC_1601_to_2000(mUTC) / 1000000.0; }

	/// Microseconds (millionth) after midnight jan 1 2000.
	INT64 MicroSecondsUTC_2000() const				{ return AbCore::UTC_1601_to_2000(mUTC); }

	/// Microseconds (millionth) after midnight jan 1 1970.
	INT64 MicroSecondsUTC_1970() const				{ return AbCore::UTC_1601_to_1970(mUTC); }

	/// Microseconds (millionth) after midnight jan 1 1601. Use this to determine relativity (ie to compare dates).
	INT64 MicroSecondsUTC_1601() const				{ return mUTC; }

	/// Returns
	/// *this < b    -1
	/// *this = b     0
	/// *this > b    +1
	int Compare( const Date& b ) const
	{
		if ( mUTC < b.mUTC ) return -1;
		if ( mUTC > b.mUTC ) return 1;
		return 0;
	}

	static int Compare( const Date& a, const Date& b )
	{
		return a.Compare( b );
	}

	/** Set microseconds.
	@sa MicroSecondsUTC_1970
	**/
	void SetMicroSecondsUTC_1970( INT64 v )						{ mUTC = AbCore::UTC_1970_to_1601(v); }

	/** Set microseconds.
	@sa MicroSecondsUTC_1601
	**/
	void SetMicroSecondsUTC_1601( INT64 v )						{ mUTC = v; }

	/// Years AD
	int Year( bool utc = false ) const							{ return GetDetails(utc).Year; }

	/// Month (0 - 11, January = 0)
	int Month( bool utc = false ) const							{ return GetDetails(utc).Month; }

	/// Day of month (1 - 31)
	int DayOfMonth( bool utc = false ) const					{ return GetDetails(utc).Day; }

	/// Day of week (0 - 6, Sunday = 6)
	int DayOfWeek( bool utc = false ) const						{ return GetDetails(utc).WeekDay; }

	/// Day of year (0 - 365, January 1st = 0)
	int DayOfYear( bool utc = false ) const						{ return GetDetails(utc).YearDay; }

	/// Seconds after midnight
	double SecondsAfterMidnight( bool utc = false ) const
	{
		AbCore::DateTime dt = GetDetails(utc);
		return dt.Hour * 3600 + dt.Min * 60 + dt.Sec + ((mUTC % 1000000) / 1000000.0);
	}

	/// Microseconds after the second
	int MicrosecondsAfterSecond() const							{ return mUTC % 1000000; }

	/// Seconds after the minute
	int SecondsAfterMinute( bool utc = false ) const			{ return GetDetails(utc).Sec; }

	/// Minutes after the hour
	int MinutesAfterHour( bool utc = false ) const				{ return GetDetails(utc).Min; }

	/// Hours after midnight
	int HoursAfterMidnight( bool utc = false ) const			{ return GetDetails(utc).Hour; }

protected:

	static int MonthFromDayInternal( int year, int& day );

	template< typename TCH >	static void TParseYYYY_MM_DD( const TCH* str, int& y, int& m, int& d );
	template< typename TCH >	static void TParseHH_MM_SS_MS( const TCH* str, int& h, int& m, int& s, int& microsec );
	template< typename TCH >	static Date TParse_DD_MM_YYYY( const TCH* str, bool utc = false );
	template< typename TCH >	static Date TParse_YYYY_MM_DD( const TCH* str, bool utc = false );
	template< typename TCH >	static Date TParse_YYYY_MM_DD_HH_MM_SS_MS( const TCH* str, bool utc = false );

	XString FormatStrFTime( const char* format, bool utc = false ) const;

};

static Date operator+( const Date& d, INT64 microSeconds )
{
	return Date::NEWTOR_1601( d.MicroSecondsUTC_1601() + microSeconds );
}

static Date operator-( const Date& d, INT64 microSeconds )
{
	return Date::NEWTOR_1601( d.MicroSecondsUTC_1601() - microSeconds );
}

}
}

typedef Panacea::Sys::Date AbcDate;
