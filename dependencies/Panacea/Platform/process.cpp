#include "pch.h"
#include "process.h"
#include "timeprims.h"
#include "../Strings/ConvertUTF.h"

#ifdef _WIN32
#include <Psapi.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef _WIN32

#if XSTRING_DEFINED
PAPI bool			AbcProcessCreate( const char* cmd, AbcForkedProcessHandle* handle, AbcProcessID* pid )
{
	XStringW c = cmd;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);
	if ( !CreateProcess( NULL, c.GetRawBuffer(), NULL, NULL, false, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi ) )
		return false;
	CloseHandle( pi.hThread );
	if ( handle )
		*handle = pi.hProcess;
	else
		CloseHandle( pi.hProcess );
	if ( pid )
		*pid = pi.dwProcessId;
	return true;
}
#endif
PAPI bool			AbcProcessWait( AbcForkedProcessHandle handle, int* exitCode )
{
	if ( WaitForSingleObject( handle, INFINITE ) == WAIT_OBJECT_0 )
	{
		if ( exitCode )
		{
			DWORD code = 0;
			GetExitCodeProcess( handle, &code );
			*exitCode = code;
		}
		return true;
	}
	return false;
}
PAPI void 			AbcProcessCloseHandle( AbcForkedProcessHandle handle )
{
	CloseHandle( handle );
}
PAPI AbcProcessID	AbcProcessGetPID()
{
	return GetCurrentProcessId();
}
PAPI void			AbcProcessGetPath( char* path, size_t maxPath )
{
	wchar_t* tmp = new wchar_t[maxPath];
	GetModuleFileNameW( NULL, tmp, (DWORD) maxPath );
	tmp[maxPath - 1] = 0;
	ConvertWideToUTF8( tmp, -1, path, maxPath );
	delete[] tmp;
}
#if XSTRING_DEFINED
PAPI XString		AbcProcessGetPath()
{
	wchar_t p[MAX_PATH];
	GetModuleFileNameW( NULL, p, (DWORD) arraysize(p) );
	p[MAX_PATH - 1] = 0;
	return p;
}
#endif
PAPI void			AbcProcessesEnum( podvec<AbcProcessID>& pids )
{
	DWORD id_static[1024];
	DWORD* id = id_static;
	DWORD id_size = sizeof(id_static);
	DWORD id_used = 0;
	while ( true )
	{
		id_used = 0;
		EnumProcesses( id, id_size, &id_used );
		if ( id_used == id_size )
		{
			// likely need more space
			if ( id == id_static )
				id = (DWORD*) AbcMallocOrDie( id_size );
			else
				id = (DWORD*) AbcReallocOrDie( id, id_size );
		}
		else
		{
			break;
		}
	}
	pids.resize_uninitialized( id_used / sizeof(DWORD) );
	memcpy( &pids[0], id, id_used );
	if ( id != id_static )
		free(id);
}
PAPI void			AbcProcessGetStatistics( AbcProcessStatistics& stats )
{
	memset( &stats, 0, sizeof(stats) );
	PROCESS_MEMORY_COUNTERS_EX p;
	GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*) &p, sizeof(p) );
	stats.WorkingSetBytes = p.WorkingSetSize;
	stats.MaxWorkingSetBytes = p.PeakWorkingSetSize;

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(stat);
	if ( GlobalMemoryStatusEx( &stat ) == TRUE )
	{
		uint64 tot = stat.ullTotalVirtual;
		uint64 avail = stat.ullAvailVirtual;
		stats.AddressSpaceCommittedBytes = (size_t) (tot - avail);
	}

	FILETIME create, exit, kernel, user;
	GetProcessTimes( GetCurrentProcess(), &create, &exit, &kernel, &user );
	
	stats.KernelCpuTimeMicroseconds = AbcFileTimeToMicroseconds( kernel );
	stats.UserCpuTimeMicroseconds = AbcFileTimeToMicroseconds( user );
}

#else
// linux

PAPI bool			AbcProcessCreate( const char* cmd, AbcForkedProcessHandle* handle, AbcProcessID* pid )
{
	AbcAssert(false);
}
PAPI bool			AbcProcessWait( AbcForkedProcessHandle handle, int* exitCode )
{
	AbcAssert(false);
}
PAPI void			AbcProcessCloseHandle( AbcForkedProcessHandle handle )
{
	AbcAssert(false);
}
PAPI AbcProcessID	AbcProcessGetPID()
{
	return getpid();
}
PAPI void			AbcProcessGetPath( char* path, size_t maxPath )
{
	AbcAssert(false); // untested code
	if ( maxPath == 0 )
		return;
	path[0] = 0;
    size_t r = readlink( "/proc/self/exe", path, maxPath - 1 );
	if ( r == -1 )
		return;
	path[r] = 0;
}
PAPI void			AbcProcessesEnum( podvec<AbcProcessID>& pids )
{
	AbcAssert(false);
}
PAPI void			AbcProcessGetStatistics( AbcProcessStatistics& stats )
{
	memset( &stats, 0, sizeof(stats) );
	rusage r;
	getrusage( RUSAGE_SELF, &r );
	stats.MaxWorkingSetBytes = (u64) r.ru_maxrss * 1024;
	stats.WorkingSetBytes = stats.MaxWorkingSetBytes;
	stats.AddressSpaceCommittedBytes = 0;
	stats.UserCpuTimeMicroseconds = r.ru_utime.tv_sec * 1000000 + r.ru_utime.tv_usec;
	stats.KernelCpuTimeMicroseconds = r.ru_stime.tv_sec * 1000000 + r.ru_stime.tv_usec;

}
#endif

PAPI uint64			AbcProcessWorkingSetBytes()
{
	AbcProcessStatistics stats;
	AbcProcessGetStatistics( stats );
	return stats.WorkingSetBytes;
}

PAPI uint64			AbcProcessMaxWorkingSetBytes()
{
	AbcProcessStatistics stats;
	AbcProcessGetStatistics( stats );
	return stats.MaxWorkingSetBytes;
}

PAPI uint64			AbcProcessVirtualCommittedBytes()
{
	AbcProcessStatistics stats;
	AbcProcessGetStatistics( stats );
	return stats.AddressSpaceCommittedBytes;
}
