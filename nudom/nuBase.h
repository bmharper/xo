#pragma once

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 1
	#include "../dependencies/biggle.h"
	#ifdef _DEBUG
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
	#include <windows.h>
	#include <mmsystem.h>
#else
	typedef const wchar_t* LPCTSTR;
	typedef const char* LPCSTR;
	#ifdef ANDROID
		// Android
		#include <jni.h>
		#include <android/log.h>
	#endif
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#include <sys/atomics.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <math.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <string>
#include <algorithm>
#include <limits>
#include <functional>

#if PROJECT_NUDOM
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

#ifndef ASSERT
	#define TEMP_ASSERT
	#ifdef _DEBUG
		#define ASSERT(condition) (void)0
	#else
		#define ASSERT(condition) assert(condition)
	#endif
#endif

#include "../dependencies/Panacea/Platform/coredefs.h"
#include "../dependencies/Panacea/Platform/stdint.h"
#include "../dependencies/Panacea/Platform/cpu.h"
#include "../dependencies/Panacea/Platform/err.h"
#include "../dependencies/Panacea/Platform/syncprims.h"
#include "../dependencies/Panacea/Platform/timeprims.h"
#include "../dependencies/Panacea/Platform/thread.h"
#include "../dependencies/Panacea/Other/StackAllocators.h"
#include "../dependencies/Panacea/Containers/pvect.h"
#include "../dependencies/Panacea/Containers/podvec.h"
#include "../dependencies/Panacea/Containers/queue.h"
#include "../dependencies/Panacea/Bits/BitMap.h"
#include "../dependencies/Panacea/fhash/fhashtable.h"

#include "nuString.h"
#define FMT_STRING nuString
#define FMT_STRING_BUF(s)	(s.Z)
#define FMT_STRING_LEN(s)	(s.Len)
#include "../dependencies/Panacea/Strings/fmt.h"

#ifdef TEMP_ASSERT
	#undef TEMP_ASSERT
	#undef ASSERT
#endif
