#include "../../nuDom/nuDom.h"

static nuSysWnd* MainWnd;

bool touch( const nuEvent& ev );

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			NUTRACE( "Hello 1\n" );
			MainWnd = nuSysWnd::CreateWithDoc();
			nuDoc* doc = MainWnd->Doc();
	
			NUTRACE( "Hello 2\n" );

			for ( int i = 0; i < 4; i++ )
			{
				nuDomEl* div = doc->Root.AddChild( nuTagDiv );
				div->StyleParse( "width: 90px; height: 90px; border-radius: 15px; display: inline;" );
				div->StyleParse( "margin: 3px;" );
			}

			// block with text inside it
			nuDomEl* txtBox = doc->Root.AddChild( nuTagDiv );
			txtBox->StyleParse( "width: 90px; height: 90px; border-radius: 15px; background: #f0f0f0; margin: 3px; position: absolute;" );
			//txtBox->SetText( "from ethnic minorities" );
			txtBox->SetText( "| The quick brown fox JUMPS |" );
			//txtBox->SetText( "fox" );

			//doc->Root.ChildByIndex(0)->StyleParse( "background: #e00e" );
			//doc->Root.ChildByIndex(1)->StyleParse( "background: #0e0e" );
			//doc->Root.ChildByIndex(2)->StyleParse( "background: #00ee" );
			doc->Root.ChildByIndex(0)->StyleParse( "background: #ff000040" );
			doc->Root.ChildByIndex(1)->StyleParse( "background: #00e500ff" );
			doc->Root.ChildByIndex(2)->StyleParse( "background: #0000e5ff" );

			NUTRACE( "Hello 3\n" );

			nuDomEl* greybox = doc->Root.ChildByIndex(3);
			greybox->StyleParse( "background: #aaaa; position: absolute; left: 90px; top: 90px;" );

#if NU_LAMBDA
			auto onMoveOrTouch = [greybox, txtBox](const nuEvent& ev) -> bool {
				greybox->StyleParsef( "left: %fpx; top: %fpx;", ev.Points[0].x - 45.0, ev.Points[0].y - 45.0 );
				txtBox->StyleParsef( "left: %fpx; top: %fpx;", ev.Points[0].x * 0.01 + 45.0, ev.Points[0].y * 0.01 + 150.0 );
				return true;
			};
			doc->Root.OnMouseMove( onMoveOrTouch );
			doc->Root.OnTouch( onMoveOrTouch );
#else
			doc->Root.OnMouseMove( touch, greybox );
			doc->Root.OnTouch( touch, greybox );
#endif

			NUTRACE( "Hello 4\n" );

			MainWnd->Show();

			NUTRACE( "Hello 5\n" );
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
	greybox->StyleParsef( "left: %fpx; top: %fpx;", ev.Points[0].x - 50.0, ev.Points[0].y - 50.0 );
	return true;
}
