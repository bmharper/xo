#pragma once

// Necessary on some linux stdint.h, to get INT32_MAX etc.
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "coredefs.h"

#ifdef _WIN32
	// This block here is necessary to quiet macro redefinition warnings from including stdint.h and intsafe.h.
	// Issue https://connect.microsoft.com/VisualStudio/feedback/details/621653/including-stdint-after-intsafe-generates-warnings
	#pragma warning( push )
	#pragma warning( disable: 4005 ) // macro redef.
	#include <limits.h>
	#include <stdint.h>
	#include <intsafe.h>
	#pragma warning( pop )
#else
	#include <limits.h>
	#include <stdint.h>
#endif

typedef signed char		i8;			// preferred (i8)
typedef signed char		int8;
typedef signed char		int8_t;
typedef unsigned char	u8;			// preferred (u8)
typedef unsigned char	uint8;
typedef unsigned char	uint8_t;
typedef unsigned char	byte;		
typedef int int32;					// preferred (int or int32) -- see also intr/intp below
typedef int INT32;
typedef int int32_t;
typedef short i16;					// preferred (i16)
typedef short int16_t;
typedef short int16;
typedef unsigned int u32;			// preferred (u32) -- see also intr/intp below
typedef unsigned int uint;
typedef unsigned int UINT;
typedef unsigned int uint32;
typedef unsigned int UINT32;
typedef unsigned int uint32_t;
typedef unsigned short u16;			// preferred (u16)
typedef unsigned short uint16;
typedef unsigned short uint16_t;


#ifdef _MSC_VER
typedef __int64 i64;				// preferred (i64) -- see also intr/intp below
typedef __int64 int64;
typedef __int64 INT64;
typedef unsigned __int64 u64;		// preferred (u64) -- see also intr/intp below
typedef unsigned __int64 uint64;
typedef unsigned __int64 uint64_t;
typedef unsigned __int64 UINT64;
#else
typedef int64_t		i64;
typedef int64_t		int64;
typedef int64_t		INT64;
typedef uint64_t	u64;
typedef uint64_t	uint64;
typedef uint64_t	UINT64;
#endif

// These are suggestions by Charles Bloom:
// intr: size of register
// intp: size of pointer
// intp and intr are the same on Windows x86/x64, but not true for all architectures.
// Here we assume that pointer and register sizes are always the same.
#if ARCH_64
typedef int64 intr;
typedef int64 intp;
typedef uint64 uintr;
typedef uint64 uintp;
#else
typedef int32 intr;
typedef int32 intp;
typedef unsigned int uintr;
typedef unsigned int uintp;
#endif

typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;

typedef UINT8 BYTE;
typedef INT16 SHORT;
typedef UINT16 WORD;

#ifndef _WIN32
//typedef unsigned long DWORD;
typedef UINT32 DWORD;
#endif

// Don't use these. Instead, use %lld and %llu. Those work for all platforms. Note that the 'd' and 'u' are essential.
// HOWEVER, if you need to scanf a 64-bit hex string, then you have to use PRIx64
#ifdef _WIN32
//#define PRIu64 "I64u"
//#define PRId64 "I64d"
#	ifndef PRIu64
#		define PRIu64 ((void) static_assert(false, "Use %llu instead"))
#	endif
#	ifndef PRIu64
#		define PRId64 ((void) static_assert(false, "Use %lld instead"))
#	endif
#	ifndef PRIX64
#		define  PRIX64  "I64X"
#		define wPRIX64 L"I64X"
#	endif
	typedef  unsigned __int64 PRIX64_t;
#else
#	ifndef PRIX64
#		define  PRIX64  "llX"
#		define wPRIX64 L"llX"
#	endif
	typedef  long long unsigned int PRIX64_t;
#endif


#ifdef _WIN32
	#define INTMIN		_I32_MIN
	#define INTMAX		_I32_MAX
	#define INT8MIN		_I8_MIN
	#define INT8MAX		_I8_MAX
	#define INT16MIN	_I16_MIN
	#define INT16MAX	_I16_MAX
	#define INT32MIN	_I32_MIN
	#define INT32MAX	_I32_MAX
	#define INT64MIN	_I64_MIN
	#define INT64MAX	_I64_MAX
	#define UINT8MAX	_UI8_MAX
	#define UINT16MAX	_UI16_MAX
	#define UINT32MAX	_UI32_MAX
	#define UINT64MAX	_UI64_MAX
#else
	#ifdef INT32_MIN
		#define INTMIN		INT32_MIN
		#define INTMAX		INT32_MAX
		#define INT8MIN		INT8_MIN
		#define INT8MAX		INT8_MAX
		#define INT16MIN	INT16_MIN
		#define INT16MAX	INT16_MAX
		#define INT32MIN	INT32_MIN
		#define INT32MAX	INT32_MAX
		#define INT64MIN	INT64_MIN
		#define INT64MAX	INT64_MAX
		#define UINT8MAX	UINT8_MAX
		#define UINT16MAX	UINT16_MAX
		#define UINT32MAX	UINT32_MAX
		#define UINT64MAX	UINT64_MAX
	#else
		#define INT32MAX	2147483647L
	#endif
#endif
