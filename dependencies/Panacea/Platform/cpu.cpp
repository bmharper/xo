#include "pch.h"
#include "cpu.h"

#ifdef _WIN32
PAPI void			AbcMachineInformationGet( AbcMachineInformation& info )
{
	memset( &info, 0, sizeof(info) );
	
	SYSTEM_INFO inf;
	GetSystemInfo( &inf );
	info.CPUCount = inf.dwNumberOfProcessors;

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(stat);
	if ( GlobalMemoryStatusEx( &stat ) == TRUE )
		info.PhysicalMemory = stat.ullTotalPhys;
}
#else
PAPI void			AbcMachineInformationGet( AbcMachineInformation& info )
{
	memset( &info, 0, sizeof(info) );
	info.CPUCount = sysconf( _SC_NPROCESSORS_ONLN );
	uint64 ps = sysconf( _SC_PAGESIZE );
	uint64 pn = sysconf( _SC_PHYS_PAGES );
	info.PhysicalMemory = ps * pn;
}
#endif

PAPI uint64			AbcPhysicalMemory()
{
	AbcMachineInformation inf;
	AbcMachineInformationGet( inf );
	return inf.PhysicalMemory;
}
