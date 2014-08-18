#pragma once

#ifndef _WIN32
	typedef const wchar_t* LPCTSTR;
	typedef const char* LPCSTR;
#endif

#if PROJECT_XO
	#ifdef _WIN32
		#define PAPI __declspec(dllexport)
	#else
		#define PAPI
	#endif
#else
	#ifdef _WIN32
		#define PAPI __declspec(dllimport)
	#else
		#define PAPI
	#endif
#endif

#define NO_XSTRING

#ifndef INLINE
	#define INLINE inline
#endif

#ifndef _TCHAR_DEFINED
	typedef char TCHAR;
#endif

#ifndef _INC_TCHAR
	//#define _T(x) L ## x
	#define _T(x) x
#endif
