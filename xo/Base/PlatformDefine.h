#pragma once
namespace xo {
enum Platform {
	Platform_WinDesktop   = 1,
	Platform_Android      = 2,
	Platform_LinuxDesktop = 4,
	Platform_OSX          = 8,
	Platform_All          = 1 | 2 | 4 | 8,
};
}

#if defined(_WIN32)
#define XO_PLATFORM_ANDROID 0
#define XO_PLATFORM_WIN_DESKTOP 1
#define XO_PLATFORM_LINUX_DESKTOP 0
#define XO_PLATFORM_OSX 0
#define XO_PLATFORM Platform_WinDesktop
#define XO_TRACE_WRITE OutputDebugStringA
#define XO_ANALYSIS_ASSUME(x) __analysis_assume(x)
#define XO_NORETURN __declspec(noreturn)
#define XO_DEBUG_BREAK() __debugbreak()
#elif defined(ANDROID)
#define XO_PLATFORM_ANDROID 1
#define XO_PLATFORM_WIN_DESKTOP 0
#define XO_PLATFORM_LINUX_DESKTOP 0
#define XO_PLATFORM_OSX 0
#define XO_PLATFORM Platform_Android
#define XO_TRACE_WRITE(msg) __android_log_write(ANDROID_LOG_INFO, "xo", msg)
#define XO_ANALYSIS_ASSUME(x) ((void) 0)
#define XO_NORETURN __attribute__((noreturn)) __attribute__((analyzer_noreturn))
#define XO_DEBUG_BREAK() __builtin_trap()
#elif defined(__linux__)
#define XO_PLATFORM_ANDROID 0
#define XO_PLATFORM_WIN_DESKTOP 0
#define XO_PLATFORM_LINUX_DESKTOP 1
#define XO_PLATFORM_OSX 0
#define XO_PLATFORM Platform_LinuxDesktop
#define XO_TRACE_WRITE(msg) fputs(msg, stderr)
#define XO_ANALYSIS_ASSUME(x) ((void) 0)
#define XO_NORETURN __attribute__((noreturn)) __attribute__((analyzer_noreturn))
#define XO_DEBUG_BREAK() __builtin_trap()
#elif defined(__APPLE__)
#define XO_PLATFORM_ANDROID 0
#define XO_PLATFORM_WIN_DESKTOP 0
#define XO_PLATFORM_LINUX_DESKTOP 0
#define XO_PLATFORM_OSX 1
#define XO_PLATFORM Platform_OSX
#define XO_TRACE_WRITE(msg) fputs(msg, stderr)
#define XO_ANALYSIS_ASSUME(x) ((void) 0)
#define XO_NORETURN __attribute__((noreturn)) __attribute__((analyzer_noreturn))
#define XO_DEBUG_BREAK() __builtin_trap()
#else
#ifdef _MSC_VER
#pragma error("Unknown xo platform")
#else
#error Unknown xo platform
#endif
#endif

#if XO_PLATFORM_WIN_DESKTOP
#define _UNICODE
#define UNICODE
#endif

// We need to define this early on, because things such as gl.h will include stdint.h
#if !XO_PLATFORM_WIN_DESKTOP
// Necessary on some linux stdint.h (esp Android), to get INT32_MAX etc.
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#endif

#if XO_PLATFORM_WIN_DESKTOP
#ifndef XO_API
#define XO_API __declspec(dllexport)
#else
#define XO_API __declspec(dllimport)
#endif
#else
#define XO_API
#endif
