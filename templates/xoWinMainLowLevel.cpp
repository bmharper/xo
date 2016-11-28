///////////////////////////////////////////////////////////////////////////////////////////////////
// Keep this file in sync with xoWinMainLowLevel.cpp
// This file should be compiled and linked into your exe. Alternatively, just #include "xoWinMainLowLevel.cpp"
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef XO_AMALGAMATION
#include "../amalgamation/xo-amalgamation.h"
#else
#include "../xo/xo.h"
#include "../xo/xoDocGroup.h"
#endif

// This is your "main" function, which you define in your own code
void xoMain(xoMainEvent ev);

#if XO_PLATFORM_WIN_DESKTOP

#pragma warning(disable: 28251) // Inconsistent annotation for 'WinMain': this instance has no annotations. See c:\program files (x86)\windows kits\8.0\include\um\winbase.h(2188). 

static int __cdecl CrtAllocHook(int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine)
{
	if (size == 12582943)
		int abc = 123;
	return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	_CrtSetAllocHook(CrtAllocHook);
	xoRunAppLowLevel(xoMain);
	_CrtDumpMemoryLeaks();
	return 0;
}

#elif XO_PLATFORM_LINUX_DESKTOP

int main(int argc, char** argv)
{
	xoRunAppLowLevel(xoMain);
	return 0;
}

#else
XOTODO_STATIC;
#endif
