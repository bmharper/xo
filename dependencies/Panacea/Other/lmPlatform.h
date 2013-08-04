#pragma once

#include "cpudetect.h"

#ifdef _WIN32
#define DIR_SEP		'\\'
#define DIR_SEP_WC	L'\\'
#define DIR_SEP_WS	L"\\"		// Use this when intermingling with XString, or you risk doing an implicit XString(const wchar_t*) creation where the DIR_SEP char is implicitly converted to const wchar_t*
#define DIR_SEP_S	"\\"
#else
#define DIR_SEP		'/'
#define DIR_SEP_WC	L'/'
#define DIR_SEP_WS	L"/"
#define DIR_SEP_S	"/"
#endif

#ifdef _WIN32
inline bool OSAtLeastVista()
{
	return LOBYTE(LOWORD(GetVersion())) >= 6;
}

inline bool OSAtLeastWin7()
{
	DWORD v = LOWORD(GetVersion());
	return	(LOBYTE(v) == 6 && HIBYTE(v) >= 1) ||
					(LOBYTE(v) >= 7);
}
#endif

namespace AbCore
{
	PAPI void SetConsoleDimensions( int screen_width, int screen_height, int virtual_height );
	PAPI void FillStackWith( size_t bytesOfStackToFill, DWORD fillWith );
	PAPI void RedirectIOToConsole();								// Allows a /SUBSYSTEM:WINDOWS app to write to the console from which it was launched
	// NOTE: If you're looking to redirect stdout/stderr to a file, just use freopen()
}
