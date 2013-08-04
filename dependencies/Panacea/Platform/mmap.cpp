#include "pch.h"

#ifdef _WIN32
PAPI void*	AbcMMap( size_t len, unsigned int type_AbcMMapType )
{
	int t = 0;
	switch ( type_AbcMMapType )
	{
	case 0:															t = PAGE_NOACCESS; break;
	case AbcMMapType_Read:											t = PAGE_READONLY; break;
	case AbcMMapType_Read | AbcMMapType_Write:						t = PAGE_READWRITE; break;
	case AbcMMapType_Read | AbcMMapType_Write | AbcMMapType_Exec:	t = PAGE_EXECUTE_READWRITE; break;
	default:
		return NULL;
	}
	// commit and reserve is the only thing that linux supports, so we ensure we're consistent with that
	return VirtualAlloc( NULL, len, MEM_COMMIT | MEM_RESERVE, t );
}

PAPI bool	AbcMMapFree( void* ptr, size_t len )
{
	return !!VirtualFree( ptr, 0, MEM_RELEASE );
}

typedef BOOL ( WINAPI *HeapSetInformation_proc)
	(
	PVOID HeapHandle, 
	HEAP_INFORMATION_CLASS HeapInformationClass,
	PVOID HeapInformation,
	SIZE_T HeapInformationLength
	);


PAPI bool	AbcHeapSetLowFragmentation( HANDLE heap )
{
	// Set the process heap to LFH
	HMODULE kernel = GetModuleHandle( _T("Kernel32.dll") );
	if ( kernel )
	{
		HeapSetInformation_proc hs = (HeapSetInformation_proc) GetProcAddress( kernel, "HeapSetInformation" );
		if ( hs )
		{
			// This will always fail when run under the debugger, unless you 
			// set the environment variable _NO_DEBUG_HEAP to 1
			HeapLock( heap );
			ULONG cinf = 0;
			HeapQueryInformation( heap, HeapCompatibilityInformation, &cinf, sizeof(cinf), NULL );
			cinf = 2;
			BOOL ret = hs( heap, HeapCompatibilityInformation, &cinf, sizeof(cinf) );
			HeapUnlock( heap );
			return ret != 0;
		}
	}
	else ASSERT(false);
	return false;
}

PAPI bool	AbcHeapSetProcessHeapLowFragmentation()
{
	return AbcHeapSetLowFragmentation( GetProcessHeap() );
}

#else

PAPI void*	AbcMMap( size_t len, unsigned int type_AbcMMapType )
{
	int prot = 0;
	switch ( type_AbcMMapType )
	{
	case 0:															t = PROT_NONE; break;
	case AbcMMapType_Read:											t = PROT_READ; break;
	case AbcMMapType_Read | AbcMMapType_Write:						t = PROT_READ | PROT_WRITE; break;
	case AbcMMapType_Read | AbcMMapType_Write | AbcMMapType_Exec:	t = PROT_READ | PROT_WRITE | PROT_EXEC; break;
	default:
		return NULL;
	}
	void* r = mmap( NULL, len, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
	if ( r == MAP_FAILED )
		return NULL;
	else
		return r;
}

PAPI void	AbcMMapFree( void* ptr, size_t len )
{
	return 0 == munmap( ptr, len );
}

#endif

PAPI bool	AbcMMapTest( size_t len )
{
	// Just assume that he will be able to allocate 0 bytes.
	// I don't really know what the best behaviour is here. This function is quite hackish anyway,
	// so I don't think it's better to be lenient here than strict.
	if ( len == 0 )
		return true;

	// Ideally this would do a RESERVE on Win32, but I believe the performance
	// impact will be small enough that we can just ignore this.
	void* p = AbcMMap( len, AbcMMapType_Read | AbcMMapType_Write );
	if ( p )
	{
		AbcMMapFree( p, len );
		return true;
	}
	return false;
}
