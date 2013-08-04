#include "pch.h"
#include "debuglib.h"
#include "../Strings/strings.h"
#include "process.h"
#include <signal.h>

PAPI XString AbcDebug_ReadAppValueStr( LPCSTR name )
{
#ifdef _WIN32
	XString file = ChangeExtension(AbcProcessGetPath(), "DebugParams");
	XString key = name;
	wchar_t wval[1024];
	wval[0] = 0;
	GetPrivateProfileString( L"debug", key, NULL, wval, 1023, file );
	return wval;
#else
	return "";
#endif
}

PAPI bool AbcDebug_ReadAppValue( LPCSTR name, double& v )
{
	XString s = AbcDebug_ReadAppValueStr(name);
	if ( s == "" ) return false;
	v = ParseFloat( s );
	return true;
}

PAPI bool AbcDebug_ReadAppValue( LPCSTR name, float& v )
{
	double dbl;
	if ( AbcDebug_ReadAppValue( name, dbl ) ) { v = dbl; return true; }
	return false;
}

PAPI XString AbcSysErrorMsg( DWORD err )
{ 
#ifdef _WIN32
	wchar_t szBuf[1024];
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR) &lpMsgBuf,
		0, NULL );

	_swprintf( szBuf, L"(%u) %s", err, (const wchar_t*) lpMsgBuf );
	LocalFree( lpMsgBuf );

	// chop off trailing carriage returns
	XString r = szBuf;
	while ( r.Length() > 0 && (r.EndsWith(10) || r.EndsWith(13)) )
		r.Chop();

	return r;
#else
	char buf[1024];
	sprintf( "System error %u", err );
	buf[arraysize(buf) - 1] = 0;
	return buf;
#endif
}

PAPI XString AbcSysLastErrorMsg()
{
#ifdef _WIN32
	return AbcSysErrorMsg( GetLastError() );
#else
	return "Last system error (unknown)";
#endif
}

PAPI void AbcOutputDebugString( LPCSTR str )
{
#ifdef _WIN32
	OutputDebugStringA( str );
#else
	fputs( str, stdout );
#endif
}


PAPI void AbcDebugBreak()
{
#ifdef _WIN32
	__debugbreak();
#else
	raise(SIGTRAP);
#endif
}
