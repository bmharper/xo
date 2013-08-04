#include "pch.h"
#include <stdio.h>
#include <string.h>
#include "strings_os.h"

template<typename CHAR_A, typename CHAR_B, bool CASE_SENSE>
static int t_strcmp_x_y( const CHAR_A* a, const CHAR_B* b, int max_chars )
{
	for ( int i = 0; true; i++ )
	{
		if ( (uint) i >= (uint) max_chars )
			return 0;
		if ( a[i] == 0 )
		{
			if ( b[i] == 0 )	return 0;
			else				return -1;
		}
		else if ( b[i] == 0 )
			return 1;
		
		int x = a[i];
		int y = b[i];
		if ( !CASE_SENSE && x >= 'A' && x <= 'Z' ) x += 'a' - 'A';
		if ( !CASE_SENSE && y >= 'A' && y <= 'Z' ) y += 'a' - 'A';
		int delta = x - y;
		if ( delta != 0 )
			return delta > 0 ? 1 : -1;
	}
}

PAPI int strcmp_u16_wchar	( const uint16* a, const wchar_t* b, int max_chars )	{ return t_strcmp_x_y<uint16, wchar_t, true>	( a, b, max_chars ); }
PAPI int stricmp_u16_wchar	( const uint16* a, const wchar_t* b, int max_chars )	{ return t_strcmp_x_y<uint16, wchar_t, false>	( a, b, max_chars ); }
PAPI int strcmp_u16_u16		( const uint16* a, const uint16* b, int max_chars )		{ return t_strcmp_x_y<uint16, uint16, true>		( a, b, max_chars ); }
PAPI int stricmp_u16_u16	( const uint16* a, const uint16* b, int max_chars )		{ return t_strcmp_x_y<uint16, uint16, false>	( a, b, max_chars ); }
