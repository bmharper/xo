#include "pch.h"
#include "Asserts.h"

namespace xo {

XO_API void XO_NORETURN Die(const char* file, int line, const char* msg)
{
#if XO_PLATFORM_ANDROID
	__android_log_print(ANDROID_LOG_ERROR, "xo", "assertion failed %s:%d %s", file, line, msg)
#endif
	fprintf(stdout, "Program is self-terminating at %s:%d. (%s)\n", file, line, msg);
	fprintf(stderr, "Program is self-terminating at %s:%d. (%s)\n", file, line, msg);
	fflush(stdout);
	fflush(stderr);
	XO_DEBUG_BREAK();
	*((int*)0) = 1;
}

}