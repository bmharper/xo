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
#define NUASSERT(x)			AbcAssert(x)

#ifdef _DEBUG
	#define NUVERIFY(x)			NUASSERT(x)
	#define NUASSERTDEBUG(x)	NUASSERT(x)
#else
	#define NUVERIFY(x)			((void)(x))
	#define NUASSERTDEBUG(x)	((void)0)
#endif

void* nuMallocOrDie( size_t bytes );

#define NUCHECKALLOC(x)		NUASSERT((x) != NULL)
#define NUPANIC(msg)		AbcPanic(msg)
#define NUTODO				NUPANIC("not yet implemented") 

#define NU_LAMBDA 1

#ifdef _WIN32
	//#define NU_LAMBDA 1
	#define NU_WIN_DESKTOP 1
	#define NUTRACE_WRITE OutputDebugStringA
#else
	#define NU_WIN_DESKTOP 0
#endif

#ifdef _WIN32
	#define NU_ANDROID 0
#else
	#ifdef ANDROID
		//#define NU_LAMBDA 0
		#define NU_ANDROID 1
		#define NUTRACE_WRITE(msg) __android_log_write(ANDROID_LOG_INFO, "nudom", msg)
	#else
		#define NU_ANDROID 0
		#define NUTRACE_WRITE(msg) ((void)0)
	#endif
#endif
