#pragma once

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
PAPI void				AbcProcessGetPath( wchar_t* path, size_t maxPath );		// Return full path of executing process, for example "c:\programs\notepad.exe"
#ifdef XSTRING_DEFINED
PAPI XString			AbcProcessGetPath();									// Return full path of executing process, for example "c:\programs\notepad.exe"
#endif
PAPI void				AbcProcessesEnum( podvec<AbcProcessID>& pids );
PAPI void				AbcProcessGetStatistics( AbcProcessStatistics& stats );
PAPI uint64				AbcProcessWorkingSetBytes();							// wrapper around AbcProcessGetStatistics. On linux, this is the same as AbcProcessMaxWorkingSetBytes().
PAPI uint64				AbcProcessMaxWorkingSetBytes();							// wrapper around AbcProcessGetStatistics
PAPI uint64				AbcProcessVirtualCommittedBytes();						// wrapper around AbcProcessGetStatistics. Only supported on Win32. Returns 0 on any other OS.

