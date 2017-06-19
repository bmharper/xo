///////////////////////////////////////////////////////////////////////////////////////////////////
// Keep this file in sync with xoWinMainLowLevel.cpp
// This file should be compiled and linked into your exe. Alternatively, just #include "xoWinMain.cpp"
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef XO_AMALGAMATION
#include "../amalgamation/xo-amalgamation.h"
#else
#include "../xo/xo.h"
#endif

// This is your "main" function, which you define in your own code
void xoMain(xo::SysWnd* wnd);

#if XO_PLATFORM_WIN_DESKTOP

#pragma warning(disable: 28251) // Inconsistent annotation for 'WinMain': this instance has no annotations. See c:\program files (x86)\windows kits\8.0\include\um\winbase.h(2188). 

static int __cdecl CrtAllocHook(int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine)
{
	if (request == 1825)
		int adsdssd = 2332;
	return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	_CrtMemState m1;
	_CrtMemCheckpoint(&m1);

	_CrtSetAllocHook(CrtAllocHook);

	xo::RunApp(xoMain);
	
	_CrtMemState m2;
	_CrtMemCheckpoint(&m2);
	
	_CrtMemState mdiff;
	_CrtMemDifference(&mdiff, &m1, &m2);

	// OpenCV

	//_CrtDumpMemoryLeaks();
	return 0;
}

// This is useful if you want a console application, which you'll typically only do during some stage
// of debugging, and you really want stdout/stderr.
// If you set the linker flag /SUBSYSTEM:CONSOLE, then you get a console application.
//int main(int argc, char** argv)
//{
//	_CrtSetAllocHook(CrtAllocHook);
//	xo::RunApp(xoMain);
//	_CrtDumpMemoryLeaks();
//	return 0;
//}

#elif XO_PLATFORM_LINUX_DESKTOP

int main(int argc, char** argv)
{
	xo::RunApp(xoMain);
	return 0;
}

#else
XO_TODO_STATIC;
#endif
