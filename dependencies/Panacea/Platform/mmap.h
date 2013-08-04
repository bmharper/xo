#pragma once

/*
This file contains functions dealing with:
	* Memory mapping and virtual address space
	* Heap management
*/

#ifdef _WIN32
#include <sal.h>

// Note: I filed an MSFT connect issue (#683247).
// On September 4 2011, they said it would be fixed in the next VS release.
#if _MSC_VER > 0x1000
#pragma message( "You should be able to get rid of AbcMapViewOfFile now" )
#endif

#pragma warning(push)
#pragma warning(disable:6540)	// The use of attribute annotations on this function will invalidate all of its existing __declspec annotations
_Ret_opt_bytecap_(dwNumberOfBytesToMap) __out_data_source(FILE) inline void* AbcMapViewOfFile(
		__in HANDLE hFileMappingObject,
		__in DWORD dwDesiredAccess,
		__in DWORD dwFileOffsetHigh,
		__in DWORD dwFileOffsetLow,
		__in SIZE_T dwNumberOfBytesToMap
)
{
	return MapViewOfFile( hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap );
}
#pragma warning(pop)
#endif

enum AbcMMapType
{
	AbcMMapType_Read = 1,
	AbcMMapType_Write = 2,
	AbcMMapType_Exec = 4,
};

PAPI void*	AbcMMap( size_t len, unsigned int type_AbcMMapType );
PAPI bool	AbcMMapFree( void* ptr, size_t len );		// the 'len' parameter is required by munmap, but not by Windows
PAPI bool	AbcMMapTest( size_t len );					// Cheap and dirty way to see if you have virtual address space available. Does an AbcMMap/AbcMMapFree pair.

#ifdef _WIN32
PAPI bool	AbcHeapSetLowFragmentation( HANDLE heap );				// Set the specified heap to be a low fragmentation heap.
PAPI bool	AbcHeapSetProcessHeapLowFragmentation();				// Set the process' main heap to be LFH
#endif
