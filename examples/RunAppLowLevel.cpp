#include "../xo/xo.h"

/* This example demonstrates linking to xoWinMainLowLevel.cpp, and thereby utilizing
xoRunAppLowLevel. The primary difference here is that you are in control of creating
application windows, showing them, and destroying them.
*/

static xo::SysWnd* Wnd1;
static xo::SysWnd* Wnd2;

void xoMain(xo::MainEvent ev)
{
	switch (ev)
	{
	case xo::MainEventInit:
		Wnd1 = xo::SysWnd::CreateWithDoc();
		Wnd1->Doc()->Root.AddText("Hello low level 1");
		Wnd1->Show();
		Wnd2 = xo::SysWnd::CreateWithDoc();
		Wnd2->Doc()->Root.AddText("Hello low level 2");
		Wnd2->Show();
		break;
	case xo::MainEventShutdown:
		delete Wnd1;
		delete Wnd2;
		Wnd1 = nullptr;
		Wnd2 = nullptr;
		break;
	}
}
