#pragma once

#ifndef _WIN32

#include "stdint.h"
#include <stdlib.h>

typedef struct tagRECT
{
    int    left;
    int    top;
    int    right;
    int    bottom;
} RECT;

#ifndef GUID_DEFINED
#define GUID_DEFINED 1
typedef struct _GUID {
	unsigned long  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[ 8 ];
} GUID;
#endif


typedef const char*		LPCSTR;
typedef const wchar_t*	LPCWSTR;

// Note: it looks like PATH_MAX on linux is 4096
#define MAX_PATH (255)

PAPI	char*		_itoa( int value, char* result, int base );
PAPI	wchar_t*	_itow( int value, wchar_t* result, int base );
PAPI	char*		_i64toa( int64 value, char* result, int base );
PAPI	wchar_t*	_i64tow( int64 value, wchar_t* result, int base );
PAPI	int			_wtoi( const wchar_t* s );
PAPI	int64		_wtoi64( const wchar_t* s );
inline	int			_atoi64( const char* s )		{ return atoll(s); }
PAPI	double		_wtof( const wchar_t* s );

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#define _FILETIME_

#endif



#ifndef _AFX
class CRect : public RECT
{
public:
	CRect() { left = right = top = bottom = 0; }
	CRect( int l, int t, int r, int b ) { left = l; top = t; right = r; bottom = b; }
	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
};
#endif
