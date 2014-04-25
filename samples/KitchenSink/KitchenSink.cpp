#include "../../nuDom/nuDom.h"

/*
This sample was created when developing the layout concepts
*/

static nuSysWnd* MainWnd;

void InitDOM( nuDoc* doc );

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			MainWnd = nuSysWnd::CreateWithDoc();
			MainWnd->SetPosition( nuBox(2100, 60, 2100 + 1300, 60 + 800), nuSysWnd::SetPosition_Move | nuSysWnd::SetPosition_Size );
			nuDoc* doc = MainWnd->Doc();
			InitDOM( doc );
			MainWnd->Show();
		}
		break;
	case nuMainEventShutdown:
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}

void DoTwoTextRects( nuDoc* doc )
{
	if (1)
	{
		nuDomNode* div = doc->Root.AddNode( nuTagDiv );
		div->StyleParse( "width: 90px; height: 90px; background: #faa; margin: 4px" );
		div->StyleParse( "font-size: 13px" );
		div->SetText( "Ave quick brown fox jumps over the lazy dog.\nText wrap and kerning." );
	}

	if (1)
	{
		// This block has width=height=unspecified, so it gets its size from its children
		// We expect to see the green background behind this text
		nuDomNode* div = doc->Root.AddNode( nuTagDiv );
		div->StyleParse( "background: #afa8; margin: 4px" );
		div->StyleParse( "font-size: 13px" );
		div->SetText( "Parent has no size, but this text gives it size. Expect green background behind this text.\nPpPp\npPpP\n\naaa\naaapq" );
	}
}

void DoBlockMargins( nuDoc* doc )
{
	// This should produce a matrix of blocks that are perfectly separated by 8 pixels.
	// It would be 4 pixels between blocks if we collapsed margins, but we don't do that.
	if (1)
	{
		for ( int i = 0; i < 30; i++ )
		{
			nuDomNode* div = doc->Root.AddNode( nuTagDiv );
			div->StyleParse( "width: 90px; height: 90px; background: #faa8; margin: 4px; border-radius: 5px;" );
			div->StyleParse( "font-size: 13px" );
			div->SetText( fmt("\n\n   block %v", i).Z );
		}
	}

	// First block has margins, second and third blocks have no margins
	// We expect to see a 4 pixel margin around the first block, regardless of the second block's settings
	if (0)
	{
		doc->Root.AddNode( nuTagDiv )->StyleParse( "width: 90px; height: 90px; background: #faa8; margin: 4px; border-radius: 5px;" );
		doc->Root.AddNode( nuTagDiv )->StyleParse( "width: 90px; height: 90px; background: #faa8; margin: 0px; border-radius: 5px;" );
		doc->Root.AddNode( nuTagDiv )->StyleParse( "width: 90px; height: 90px; background: #faa8; margin: 0px; border-radius: 5px;" );
	}
}

void InitDOM( nuDoc* doc )
{
	nuDomNode* body = &doc->Root;
	body->StyleParse( "font-family: Segoe UI, Droid Sans" );

	//DoTwoTextRects( doc );
	DoBlockMargins( doc );

	body->OnClick( [](const nuEvent& ev) -> bool {
		nuGlobal()->EnableKerning = !nuGlobal()->EnableKerning;
		return true;
	});
}