#include "../../nuDom/nuDom.h"

static nuSysWnd* MainWnd;

bool touch( const nuEvent& ev );

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			NUTRACE( "Hello 1" );
			MainWnd = nuSysWnd::CreateWithDoc();
			nuDoc* doc = MainWnd->Doc();
	
			NUTRACE( "Hello 2" );

			for ( int i = 0; i < 4; i++ )
			{
				nuDomEl* div = new nuDomEl();
				div->Tag = nuTagDiv;
				div->Style.Parse( "width: 100px; height: 100px; border-radius: 15px; display: inline;" );
				div->Style.Parse( "margin: 3px;" );
				doc->Root.AddChild( div );
			}
			doc->Root.ChildByIndex(0)->Style.Parse( "background: #e00e" );
			doc->Root.ChildByIndex(1)->Style.Parse( "background: #0e0e" );
			doc->Root.ChildByIndex(2)->Style.Parse( "background: #00ee" );

			NUTRACE( "Hello 3" );

			nuDomEl* greybox = doc->Root.ChildByIndex(3);
			greybox->Style.Parse( "background: #aaaa; position: absolute; left: 100px; top: 100px;" );
			//greybox->OnTouch( touch );
			doc->Root.OnMouseMove( touch, greybox );
			doc->Root.OnTouch( touch, greybox );

			NUTRACE( "Hello 4" );

			MainWnd->Show();

			NUTRACE( "Hello 5" );
		}
		break;
	case nuMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}

bool touch( const nuEvent& ev )
{
	nuDomEl* greybox = (nuDomEl*) ev.Context;
	greybox->Style.Parsef( "left: %fpx; top: %fpx;", ev.Points[0].x - 50.0, ev.Points[0].y - 50.0 );
	return true;
}
