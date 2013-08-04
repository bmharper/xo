#pragma once

namespace AbCore
{
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////
	// Time encoding using by avdb.
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////

	/*
	Our time units are 1/20000 of a second
	Number of seconds in a day: 3600 * 24 = 86400
	86400 * 20000 = 1,728,000,000, which is well beneath _I32_MAX = 2^31 - 1
	*/
	#define ABCORE_TIME_SCALE 20000

	static const INT32 NullTime = INT32MIN;

	/** Decodes a standard Time value from INT32 into seconds since midnight.
	@return The time in seconds since midnight, or -1 if the time is null.
	**/
	inline double DecodeTime( INT32 v )
	{
		if ( v < 0 ) return -1;
		return v / (double) ABCORE_TIME_SCALE;
	}

	/** Format time in HH:MM:SS.
	The character buffer provided must have space for 8 characters. We do not write a null terminator.
	**/
	void PAPI FormatTimeHMS( INT32 v, char* buf );
	void PAPI FormatTimeHMS( INT32 v, wchar_t* buf );

	inline void DecodeTime( INT32 v, int& hour, int& minute, int& second, int& microsecond )
	{
		if ( v < 0 )
		{
			hour = -1;
			minute = -1;
			second = -1;
			microsecond = -1;
		}
		else
		{
			int absS = v / ABCORE_TIME_SCALE;
			int absM = v / (ABCORE_TIME_SCALE * 60);
			int absH = v / (ABCORE_TIME_SCALE * 3600);
			hour = absH;
			minute = absM % 60;
			second = absS % 60;
			microsecond = (v % ABCORE_TIME_SCALE) * 1000 / (ABCORE_TIME_SCALE / 1000);
		}
	}

	/** Encodes a standard Time value from seconds since midnight into an INT32 value.
	@param seconds_since_midnight Seconds elapsed since midnight. Any negative value is treated as null.
	@return The time as an INT32 value.
	**/
	inline INT32 EncodeTime( double seconds_since_midnight )
	{
		if ( seconds_since_midnight < 0 ) return NullTime;
		return seconds_since_midnight * ABCORE_TIME_SCALE;
	}

	inline INT32 EncodeTime( int hour, int minute, int second, int microsecond = 0 )
	{
		INT32 v = (hour * 3600 + minute * 60 + second) * ABCORE_TIME_SCALE;
		v += (microsecond * (ABCORE_TIME_SCALE/1000)) / 1000;
		return v;
	}


	struct DateTime
	{
		UINT16	Year;			///< Absolute year [1601 - 3000]
		UINT16	YearDay;		///< Day of year [0 - 365] (January 1st = 0)
		UINT8	Month;			///< Month from [0 - 11]
		UINT8	Day;			///< Day of Month [1 - 31]
		UINT8	Hour;			///< Hour [0 - 23]
		UINT8	Min;			///< Minute [0 - 59]
		UINT8	Sec;			///< Second [0 - 59]
		UINT8	WeekDay;		///< Day of week [0 - 6] (Sunday = 0)
		UINT32	Microsecond;	///< Microsecond [0 - 999999]
	};

	static const INT64 NullDate = INT64MIN;

	inline void ClearDateTime( DateTime& dt )
	{
		memset( &dt, 0, sizeof(dt) );
	}

	inline bool IsLeapYear( UINT32 year )
	{
		return (year % 4 == 0) && !( (year % 100 == 0) && (year % 400 != 0) );
	}

	static const INT64 UTC_DaysAt_1601 = 0;
	static const INT64 UTC_DaysAt_1900 = 300 * 365 - 276;
	static const INT64 UTC_DaysAt_1970 = 370 * 365 - 276;
	static const INT64 UTC_DaysAt_2000 = 400 * 365 - 269;
	static const INT64 UTC_MicrosecondsPerDay = 24 * 3600 * (INT64) 1000000;

	inline INT64 UTC_1601_to_1970( INT64 v )
	{
		return v - (UTC_DaysAt_1970 * UTC_MicrosecondsPerDay);
	}

	inline INT64 UTC_1970_to_1601( INT64 v )
	{
		return v + (UTC_DaysAt_1970 * UTC_MicrosecondsPerDay);
	}

	inline INT64 UTC_1601_to_1900( INT64 v )
	{
		return v - (UTC_DaysAt_1900 * AbCore::UTC_MicrosecondsPerDay);
	}

	inline INT64 UTC_1900_to_1601( INT64 v )
	{
		return v + (UTC_DaysAt_1900 * AbCore::UTC_MicrosecondsPerDay);
	}

	inline INT64 UTC_2000_to_1601( INT64 v )
	{
		return v + (UTC_DaysAt_2000 * UTC_MicrosecondsPerDay);
	}

	inline INT64 UTC_1601_to_2000( INT64 v )
	{
		return v - (UTC_DaysAt_2000 * UTC_MicrosecondsPerDay);
	}

	/// Useful for adjusting for a time zone offset
	inline INT64 UTC_AddMinutes( INT64 v, int min )
	{
		return v + (INT64) min * (INT64) 60 * (INT64) 1000000;
	}

	/** Decode a UTC date that that is specified in microseconds since Midnight 1st January 1601.
	At microsecond accuracy, and at a year being measured as 365.25 days, we can represent dates
	from 1601 until the year circa 290000. Since we don't pretend to cater for astrophysics, this is fine.
	**/
	void PAPI UTC_Decode( INT64 utc_ms_1601, DateTime& dt );

	/** Decode a UTC date into local time, using the local time zone
	of this computer. 
	**/
	bool PAPI UTC_Decode_Local( INT64 utc_ms_1601, DateTime& dt );

	inline void UTC_Decode( INT64 utc_ms_1601, int& year, int& month_0_to_11, int& day_1_to_31, int* hour = NULL, int* min = NULL, int* sec = NULL, int* microsecond = NULL )
	{
		DateTime dt;
		UTC_Decode( utc_ms_1601, dt );
		year = dt.Year;
		month_0_to_11 = dt.Month;
		day_1_to_31 = dt.Day;
		if ( hour ) *hour = dt.Hour;
		if ( min ) *min = dt.Min;
		if ( sec ) *sec = dt.Sec;
		if ( microsecond ) *microsecond = dt.Microsecond;
	}

	/** Encode a UTC date.
	@return UTC seconds elapsed since Midnight 1st January 1601.
	**/
	INT64 PAPI UTC_Encode( const DateTime& dt );

	inline INT64 UTC_Encode( UINT16 year, BYTE month_0_to_11, BYTE day_1_to_31, BYTE hour = 0, BYTE min = 0, BYTE sec = 0, UINT32 microsecond = 0 )
	{
		DateTime dt;
		dt.Year = year;
		dt.Month = month_0_to_11;
		dt.Day = day_1_to_31;
		dt.Hour = hour;
		dt.Min = min;
		dt.Sec = sec;
		dt.Microsecond = microsecond;
		return UTC_Encode( dt );
	}

}
