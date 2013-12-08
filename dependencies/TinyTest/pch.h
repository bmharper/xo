#pragma once

#define _CRT_SECURE_NO_WARNINGS 1

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
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

#define PAPI
#define FMT_STRING          std::string
#define FMT_STRING_BUF(s)   (s).c_str()
#define FMT_STRING_LEN(s)   (s).length()
//#include <Panacea/coredefs.h>
//#include <Panacea/Strings/XString.h>
//#include <Panacea/Platform/err.h>
#include <Panacea/Containers/podvec.h>
//#include <Panacea/Platform/stdint.h>
//#include <Panacea/Platform/winheaders.h>
#include <Panacea/Platform/syncprims.h>
//#include <Panacea/Platform/process.h>
#include <Panacea/Platform/thread.h>
#include <Panacea/Platform/cpu.h>
//#include <Panacea/Platform/timeprims.h>
//#include <Panacea/Strings/strings.h>
//#include <Panacea/Strings/wildcard.h>
#include <Panacea/Strings/fmt.h>
//#include <Panacea/IO/Path.h>
//#include <Panacea/Other/profile.h>
//#include <Panacea/Windows/Win.h>
