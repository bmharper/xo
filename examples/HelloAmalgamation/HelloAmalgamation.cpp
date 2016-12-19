#include "../../amalgamation/xo-amalgamation.h"

void xoMain(xoSysWnd* wnd)
{
	Trace("Hello 1\n");
	wnd->Doc()->Root.SetText("Hello amalgamation");
}
