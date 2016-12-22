// The .cpp file that includes this, may not define any tests of its own.
// This is a linker issue that I haven't solved (nor put any effort into solving).

// Define storage for the list of tests
TT_TestList TT_TESTS_ALL;

#define _CRT_SECURE_NO_WARNINGS 1

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4530) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#pragma warning(disable: 4996) // CRT safety
#pragma warning(disable: 28020) // Issue in the Windows 10 SDK 10240
#endif

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <tchar.h>
#include <windows.h>
#include <process.h>
#include <signal.h>
#else
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <stdarg.h>
#endif

#include <time.h>
#include <string>
#include <algorithm>

#define TT_UNIVERSAL_FUNC

#include "TinyLib.cpp"
#include "TinyAssert.cpp"
#include "TinyMaster.cpp"
#include "StackWalker.cpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
