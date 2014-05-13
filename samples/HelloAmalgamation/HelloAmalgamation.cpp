#include "../../amalgamation/nuDom-amalgamation.h"

static nuSysWnd* MainWnd;

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			NUTRACE( "Hello 1\n" );
			MainWnd = nuSysWnd::CreateWithDoc();
			nuDoc* doc = MainWnd->Doc();
			doc->Root.SetText( "Hello amalgamation" );
			MainWnd->Show();
		}
		break;
	case nuMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}
