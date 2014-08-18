#include "../../xoDom/xo.h"

static xoSysWnd* MainWnd;

void xoMain( xoMainEvent ev )
{
	switch ( ev )
	{
	case xoMainEventInit:
		{
			MainWnd = xoSysWnd::CreateWithDoc();
			xoDoc* doc = MainWnd->Doc();

			int nx = 100;
			int ny = 100;
			for ( int y = 0; y < ny; y++ )
			{
				xoDomEl* line = doc->Root.AddChild( xoTagDiv );
				line->StyleParse( "width: 100%; height: 1%; margin: 1px; display: block;" );
				for ( int x = 0; x < nx; x++ )
				{
					xoDomEl* div = line->AddChild( xoTagDiv );
					div->StyleParse( "width: 1%; height: 100%; border-radius: 2px; margin: 1px; display: inline;" );
					div->StyleParsef( "background: #%02x%02x%02x%02x", (uint8) (x * 5), 0, 0, 255 );
				}
			}

			MainWnd->Show();
		}
		break;
	case xoMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}
