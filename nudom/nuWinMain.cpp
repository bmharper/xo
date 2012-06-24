#include "nuDom.h"

// This file should be compiled and linked into your exe

#if NU_WIN_DESKTOP

void nuMain( nuMainEvent ev );

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	nuInitialize();

	nuMain( nuMainEventInit );

	MSG msg;
	while ( GetMessage(&msg, NULL, 0, 0) )
	{
		//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	nuMain( nuMainEventShutdown );

	nuShutdown();
	return 0;
}

#endif
