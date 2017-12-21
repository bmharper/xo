#include "pch.h"
#include "CPU.h"

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

namespace xo {

XO_API int GetNumberOfCores() {
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif defined(__APPLE__)
	int    count;
	size_t count_len = sizeof(count);
	sysctlbyname("hw.logicalcpu", &count, &count_len, NULL, 0);
	return count;
#else
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
} // namespace xo