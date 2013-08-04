#include "nuDom.h"
#include "nuProcessor.h"

// This file should be compiled and linked into your exe

#if NU_WIN_DESKTOP

// This is your "main" function, which you define in your own code
void nuMain( nuMainEvent ev );

static int __cdecl CrtAllocHook( int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine );

#pragma warning(disable: 28251) // Inconsistent annotation for 'WinMain': this instance has no annotations. See c:\program files (x86)\windows kits\8.0\include\um\winbase.h(2188). 

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	//NUTRACE("sizeof(nuDomEl) = %d\n", (int) sizeof(nuDomEl));
	_CrtSetAllocHook( CrtAllocHook );
	nuInitialize();
	nuMain( nuMainEventInit );
	nuRunWin32MessageLoop();
	nuMain( nuMainEventShutdown );
	nuShutdown();
	_CrtDumpMemoryLeaks();
	return 0;
}

static int __cdecl CrtAllocHook( int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine )
{
	if ( request > 300 && size == 128 )
		int abc = 123;
	return TRUE;
}

#endif
