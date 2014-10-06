#ifdef XO_AMALGAMATION
#include "../amalgamation/xo-amalgamation.h"
#else
#include "xo.h"
#include "xoDocGroup.h"
#endif

// This file should be compiled and linked into your exe

// This is your "main" function, which you define in your own code
void xoMain( xoMainEvent ev );

#if XO_PLATFORM_WIN_DESKTOP

static int __cdecl CrtAllocHook( int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine );

#pragma warning(disable: 28251) // Inconsistent annotation for 'WinMain': this instance has no annotations. See c:\program files (x86)\windows kits\8.0\include\um\winbase.h(2188). 

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	//XOTRACE("sizeof(xoDomEl) = %d\n", (int) sizeof(xoDomEl));
	_CrtSetAllocHook( CrtAllocHook );
	xoInitialize();
	xoMain( xoMainEventInit );
	xoRunWin32MessageLoop();
	xoMain( xoMainEventShutdown );
	xoShutdown();
	_CrtDumpMemoryLeaks();
	return 0;
}

static int __cdecl CrtAllocHook( int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine )
{
	if ( size == 168 )
		int abc = 123;
	return TRUE;
}

#elif XO_PLATFORM_LINUX_DESKTOP

int main( int argc, char** argv )
{
	xoInitialize();
	xoMain( xoMainEventInit );
	xoRunXMessageLoop();
	xoMain( xoMainEventShutdown );
	xoShutdown();
	return 0;
}

#else
XOTODO_STATIC;
#endif
