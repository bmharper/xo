#pragma once
#ifndef ABC_PROCESS_H_INCLUDED
#define ABC_PROCESS_H_INCLUDED

#include "../Containers/podvec.h"

#ifdef _WIN32
typedef DWORD					AbcProcessID;
typedef HANDLE					AbcForkedProcessHandle;
#else
typedef pid_t					AbcProcessID;
typedef FILE*					AbcForkedProcessHandle;
#endif

struct AbcProcessStatistics
{
	uint64	UserCpuTimeMicroseconds;
	uint64	KernelCpuTimeMicroseconds;
	uint64	WorkingSetBytes;			// On linux, this is the same as MaxWorkingSetBytes
	uint64	MaxWorkingSetBytes;
	uint64	AddressSpaceCommittedBytes;	// I don't know how to get this number for linux, so it is always zero on linux.
};

PAPI bool				AbcProcessCreate( const char* cmd, AbcForkedProcessHandle* handle, AbcProcessID* pid );
PAPI bool				AbcProcessWait( AbcForkedProcessHandle handle, int* exitCode );
PAPI void 				AbcProcessCloseHandle( AbcForkedProcessHandle handle );
PAPI AbcProcessID		AbcProcessGetPID();
PAPI void				AbcProcessGetPath( char* path, size_t maxPath );		// Return full path of executing process, for example "c:\programs\notepad.exe"
#ifdef XSTRING_DEFINED
PAPI XString			AbcProcessGetPath();									// Return full path of executing process, for example "c:\programs\notepad.exe"
#endif
PAPI void				AbcProcessesEnum( podvec<AbcProcessID>& pids );
PAPI void				AbcProcessGetStatistics( AbcProcessStatistics& stats );
PAPI uint64				AbcProcessWorkingSetBytes();							// wrapper around AbcProcessGetStatistics. On linux, this is the same as AbcProcessMaxWorkingSetBytes().
PAPI uint64				AbcProcessMaxWorkingSetBytes();							// wrapper around AbcProcessGetStatistics
PAPI uint64				AbcProcessVirtualCommittedBytes();						// wrapper around AbcProcessGetStatistics. Only supported on Win32. Returns 0 on any other OS.

#endif // ABC_PROCESS_H_INCLUDED
