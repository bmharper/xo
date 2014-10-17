#include "pch.h"
#include "cpu.h"

#ifndef _WIN32
#include <unistd.h>	// Added for Android
#endif

#ifdef _WIN32
PAPI void			AbcMachineInformationGet( AbcMachineInformation& info )
{
	memset( &info, 0, sizeof(info) );
	
	SYSTEM_INFO inf;
	GetSystemInfo( &inf );
	info.LogicalCoreCount = inf.dwNumberOfProcessors;
	info.PhysicalCoreCount = inf.dwNumberOfProcessors;

	SYSTEM_LOGICAL_PROCESSOR_INFORMATION log[256];
	DWORD logSize = sizeof(log);
	if ( GetLogicalProcessorInformation( log, &logSize ) )
	{
		info.PhysicalCoreCount = 0;
		for ( uint i = 0; i < logSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++ )
		{
			if ( log[i].Relationship == RelationProcessorCore )
				info.PhysicalCoreCount++;
		}
	}

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(stat);
	if ( GlobalMemoryStatusEx( &stat ) == TRUE )
		info.PhysicalMemory = stat.ullTotalPhys;
}
#else
PAPI void			AbcMachineInformationGet( AbcMachineInformation& info )
{
	memset( &info, 0, sizeof(info) );
	info.LogicalCoreCount = sysconf( _SC_NPROCESSORS_ONLN );
	info.PhysicalCoreCount = sysconf( _SC_NPROCESSORS_ONLN );
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
