#include "pch.h"
#include "OS_Time.h"

#ifdef _WIN32
static int32_t CachedPerformanceFrequency_Set;
static double  CachedPerformanceFrequency_Inv;
#endif

namespace xo {

#ifdef _WIN32
XO_API void SleepMS(int milliseconds) {
	Sleep(milliseconds);
}
XO_API double TimeAccurateSeconds() {
	if (CachedPerformanceFrequency_Set == 0) {
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		CachedPerformanceFrequency_Inv = 1.0 / (double) tmp.QuadPart;
		MemoryBarrier();
		CachedPerformanceFrequency_Set = 1;
	}
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return t.QuadPart * CachedPerformanceFrequency_Inv;
}
#else
XO_API void SleepMS(int milliseconds) {
	int64_t    nano = milliseconds * (int64_t) 1000;
	timespec t;
	t.tv_nsec = nano % 1000000000;
	t.tv_sec  = (nano - t.tv_nsec) / 1000000000;
	nanosleep(&t, NULL);
}
XO_API double TimeAccurateSeconds() {
	timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
}

#endif

XO_API int64_t MilliTicks() {
	return (int64_t)(TimeAccurateSeconds() * 1e3);
}

XO_API int64_t NanoTicks() {
	return (int64_t)(TimeAccurateSeconds() * 1e9);
}
}