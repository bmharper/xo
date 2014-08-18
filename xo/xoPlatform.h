#pragma once

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

// This executes in ALL BUILDS (not just debug).
#define XOASSERT(x)			AbcAssert(x)

#ifdef _DEBUG
	#define XOVERIFY(x)			XOASSERT(x)
	#define XOASSERTDEBUG(x)	XOASSERT(x)
#else
	#define XOVERIFY(x)			((void)(x))
	#define XOASSERTDEBUG(x)	((void)0)
#endif

void*		xoMallocOrDie( size_t bytes );
void*		xoReallocOrDie( void* buf, size_t bytes );
xoString	xoCacheDir();

#define XOCHECKALLOC(x)		XOASSERT((x) != NULL)
#define XOPANIC(msg)		AbcPanic(msg)
#define XOTODO				XOPANIC("not yet implemented") 
#define XOTODO_STATIC		static_assert(false, "Implement me");

enum xoPlatform
{
	xoPlatform_WinDesktop		= 1,
	xoPlatform_Android			= 2,
	xoPlatform_LinuxDesktop		= 4,
	xoPlatform_All			= 1 | 2 | 4,
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
#elif defined(__linux__)
	#define XO_PLATFORM_ANDROID			0
	#define XO_PLATFORM_WIN_DESKTOP		0
	#define XO_PLATFORM_LINUX_DESKTOP	1
	#define XO_PLATFORM					xoPlatform_LinuxDesktop
	#define XOTRACE_WRITE(msg)			fputs(msg, stderr)
#else
	#ifdef _MSC_VER
		#pragma error( "Unknown xoDom platform" )
	#else
		#error Unknown xoDom platform
	#endif
#endif
