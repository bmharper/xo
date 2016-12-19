#include "pch.h"
#include "OS_Time.h"

#ifdef _WIN32
static int32_t CachedPerformanceFrequency_Set;
static double  CachedPerformanceFrequency_Inv;
#endif

namespace xo {

#ifdef _WIN32
void SleepMS(int milliseconds) {
	Sleep(milliseconds);
}
double TimeAccurateSeconds() {
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
void SleepMS(int milliseconds) {
	int64    nano = milliseconds * (int64) 1000;
	timespec t;
	t.tv_nsec = nano % 1000000000;
	t.tv_sec  = (nano - t.tv_nsec) / 1000000000;
	nanosleep(&t, NULL);
}
double TimeAccurateSeconds() {
	timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
}

#endif
}