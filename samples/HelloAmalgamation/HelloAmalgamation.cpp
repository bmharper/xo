#include "../../amalgamation/xoDom-amalgamation.h"

static xoSysWnd* MainWnd;

void xoMain( xoMainEvent ev )
{
	switch ( ev )
	{
	case xoMainEventInit:
		{
			XOTRACE( "Hello 1\n" );
			MainWnd = xoSysWnd::CreateWithDoc();
			xoDoc* doc = MainWnd->Doc();
			doc->Root.SetText( "Hello amalgamation" );
			MainWnd->Show();
		}
		break;
	case xoMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}
