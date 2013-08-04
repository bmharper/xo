#include "pch.h"

#ifndef _WIN32

// This is from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
template<typename TINT, typename TCH>
TCH* _itoaT(TINT value, TCH* result, int base)
{
	if (base < 2 || base > 36) { *result = '\0'; return result; }
	
	TCH* ptr = result, *ptr1 = result, tmp_char;
	TINT tmp_value;
	
	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );
	
	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

PAPI char* _itoa( int value, char* result, int base )
{
	return _itoaT( value, result, base );
}

PAPI wchar_t* _itow( int value, wchar_t* result, int base )
{
	return _itoaT( value, result, base );
}

PAPI char* _i64toa( int64 value, char* result, int base )
{
	return _itoaT( value, result, base );
}

PAPI wchar_t* _i64tow( int64 value, wchar_t* result, int base )
{
	return _itoaT( value, result, base );
}

PAPI int _wtoi( const wchar_t* s )
{
	return wcstol( s, NULL, 10 );
}

PAPI int64 _wtoi64( const wchar_t* s )
{
	return wcstoll( s, NULL, 10 );
}

PAPI double	_wtof( const wchar_t* s )
{
	return wcstof( s, NULL );
}


#endif