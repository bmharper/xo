#include "../xo/xo.h"

/* This example demonstrates linking to xoWinMainLowLevel.cpp, and thereby utilizing
xoRunAppLowLevel. The primary difference here is that you are in control of creating
application windows, showing them, and destroying them. There is not a lot of difference
and I am tempted to remove this path altogether.
*/

static xoSysWnd* Wnd1;
static xoSysWnd* Wnd2;

void xoMain( xoMainEvent ev )
{
	switch (ev)
	{
	case xoMainEventInit:
		Wnd1 = xoSysWnd::CreateWithDoc();
		Wnd1->Doc()->Root.AddText( "Hello low level 1" );
		Wnd1->Show();
		Wnd2 = xoSysWnd::CreateWithDoc();
		Wnd2->Doc()->Root.AddText( "Hello low level 2" );
		Wnd2->Show();
		break;
	case xoMainEventShutdown:
		delete Wnd1;
		delete Wnd2;
		Wnd1 = nullptr;
		Wnd2 = nullptr;
		break;
	}
}
