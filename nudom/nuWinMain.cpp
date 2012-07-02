#include "nuDom.h"
#include "nuProcessor.h"

// This file should be compiled and linked into your exe

#if NU_WIN_DESKTOP

void nuMain( nuMainEvent ev );

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	nuInitialize();
	nuMain( nuMainEventInit );
	nuRunWin32MessageLoop();
	nuMain( nuMainEventShutdown );
	nuShutdown();
	return 0;
}

#endif
