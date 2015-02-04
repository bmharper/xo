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

// These purposefully do not pass by reference, because of this: http://randomascii.wordpress.com/2013/11/24/stdmin-causing-three-times-slowdown-on-vc/
template<typename T>	T xoClamp(T v, T vmin, T vmax)		{ return (v < vmin) ? vmin : (v > vmax) ? vmax : v; }
template<typename T>	T xoMin(T a, T b)					{ return a < b ? a : b; }
template<typename T>	T xoMax(T a, T b)					{ return a < b ? b : a; }
