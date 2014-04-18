#include "../../nuDom/nuDom.h"

/*
This sample was created when developing the layout concepts
*/

static nuSysWnd* MainWnd;

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			MainWnd = nuSysWnd::CreateWithDoc();
			MainWnd->SetPosition( nuBox(2000, 30, 2000 + 1500, 30 + 1000), nuSysWnd::SetPosition_Move | nuSysWnd::SetPosition_Size );
			nuDoc* doc = MainWnd->Doc();

			nuDomNode* div = doc->Root.AddNode( nuTagDiv );
			div->StyleParse( fmt("width: 90px; height: 90px; display: inline; background: #faa; margin: 4px;").Z );
			div->SetText( "The quick brown fox jumps over the lazy dog" );

			MainWnd->Show();
		}
		break;
	case nuMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}
