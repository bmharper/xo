#include "pch.h"

#ifdef _WIN32

// Allows reentry by operating system into message-dispatched code
void PumpWindowMessages( HWND wnd )
{
	MSG msg;
	while (PeekMessage(&msg, wnd, 0, 0, PM_REMOVE))   
	{
		TranslateMessage(&msg);		// Translates virtual key codes
		DispatchMessage(&msg);		// Dispatches message to window
	}
}

#else

void PumpWindowMessages()
{
}

#endif
