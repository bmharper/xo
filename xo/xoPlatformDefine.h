#pragma once

enum xoPlatform
{
	xoPlatform_WinDesktop		= 1,
	xoPlatform_Android			= 2,
	xoPlatform_LinuxDesktop		= 4,
	xoPlatform_All				= 1 | 2 | 4,
};

#if defined(_WIN32)
	#define XO_PLATFORM_ANDROID			0
	#define XO_PLATFORM_WIN_DESKTOP		1
	#define XO_PLATFORM_LINUX_DESKTOP	0
	#define XO_PLATFORM					xoPlatform_WinDesktop
	#define XOTRACE_WRITE				OutputDebugStringA
#elif defined(ANDROID)
	#define XO_PLATFORM_ANDROID			1
	#define XO_PLATFORM_WIN_DESKTOP		0
	#define XO_PLATFORM_LINUX_DESKTOP	0
	#define XO_PLATFORM					xoPlatform_Android
	#define XOTRACE_WRITE(msg)			__android_log_write(ANDROID_LOG_INFO, "xo", msg)
	#define XOASSERT(x)					(void) ((x) || (__android_log_print(ANDROID_LOG_ERROR, "xo", "assertion failed %s:%d %s", __FILE__, __LINE__, #x), AbcPanicHere(), 0))
#elif defined(__linux__)
	#define XO_PLATFORM_ANDROID			0
	#define XO_PLATFORM_WIN_DESKTOP		0
	#define XO_PLATFORM_LINUX_DESKTOP	1
	#define XO_PLATFORM					xoPlatform_LinuxDesktop
	#define XOTRACE_WRITE(msg)			fputs(msg, stderr)
#else
	#ifdef _MSC_VER
		#pragma error( "Unknown xo platform" )
	#else
		#error Unknown xo platform
	#endif
#endif

typedef unsigned char	byte;
typedef unsigned char	u8;
typedef unsigned char	uint8;
typedef unsigned int	u32;
typedef unsigned int	uint32;
typedef int				int32;
typedef unsigned short	uint16;

#ifndef BIT
	#define BIT(x) (1 << (x))
#endif

// XOASSERT executes in ALL BUILDS (not just debug).
#ifndef XOASSERT
	#define XOASSERT(x)			AbcAssert(x)
#endif

#ifdef _DEBUG
	#define XOVERIFY(x)			XOASSERT(x)
	#define XOASSERTDEBUG(x)	XOASSERT(x)
#else
	#define XOVERIFY(x)			((void)(x))
	#define XOASSERTDEBUG(x)	((void)0)
#endif

#define XOCHECKALLOC(x)		XOASSERT((x) != NULL)
#define XOPANIC(msg)		AbcPanic(msg)
#define XOTODO				XOPANIC("not yet implemented") 
#define XOTODO_STATIC		static_assert(false, "Implement me");

// We need to define this early on, because things such as gl.h will include stdint.h
#if !XO_PLATFORM_WIN_DESKTOP
	// Necessary on some linux stdint.h (esp Android), to get INT32_MAX etc.
	#ifndef __STDC_LIMIT_MACROS
	#define __STDC_LIMIT_MACROS
	#endif
#endif
