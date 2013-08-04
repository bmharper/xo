#pragma once

#ifdef _WIN32
// Beware of re-entrant code
void PAPI PumpWindowMessages( HWND wnd = NULL );
#else
void PAPI PumpWindowMessages();
#endif


