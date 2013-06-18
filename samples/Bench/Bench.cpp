#include "../../nuDom/nuDom.h"

static nuSysWnd* MainWnd;

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			MainWnd = nuSysWnd::CreateWithDoc();
			nuDoc* doc = MainWnd->Doc();

			int nx = 100;
			int ny = 100;
			for ( int y = 0; y < ny; y++ )
			{
				nuDomEl* line = doc->Root.AddChild( nuTagDiv );
				line->StyleParse( "width: 100%; height: 1%; margin: 1px; display: block;" );
				for ( int x = 0; x < nx; x++ )
				{
					nuDomEl* div = line->AddChild( nuTagDiv );
					div->StyleParse( "width: 1%; height: 100%; border-radius: 2px; margin: 1px; display: inline;" );
					div->StyleParsef( "background: #%02x%02x%02x%02x", (uint8) (x * 5), 0, 0, 255 );
				}
			}

			MainWnd->Show();
		}
		break;
	case nuMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}
