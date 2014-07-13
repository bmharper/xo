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
			// DO NOT COMMIT MainWnd->SetPosition( nuBox(2100, 60, 2100 + 1300, 60 + 800), /*nuSysWnd::SetPosition_Move |*/ nuSysWnd::SetPosition_Size );
			MainWnd->SetPosition( nuBox(2100, 60, 2100 + 800, 60 + 400), nuSysWnd::SetPosition_Move | nuSysWnd::SetPosition_Size );
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

void DoBaselineAlignment( nuDoc* doc )
{
	// Text DOM elements cannot define styles on themselves - they MUST inherit all of their styling from
	// their parents. Therefore, if you want different font styles on the same line, then you must wrap
	// those different styles in DOM Nodes. This example demonstrates that the "baseline" position is
	// preserved when entering those DOM Nodes. If this were not true, then the baselines of the different
	// sized text elements would not line up.

	auto root = &doc->Root;
	root->StyleParse( "padding: 10px;" );
	root->StyleParse( "background: #ddd" );
	//root->StyleParse( "text-align-vertical: top" );

	if (1)
	{
		auto txt1 = root->AddNode( nuTagDiv );
		txt1->StyleParse( "font-size: 38px; font-family: Microsoft Sans Serif; background: #fff0f0" );
		txt1->SetText( "H" );
		
		auto txt2 = root->AddNode( nuTagDiv );
		txt2->StyleParse( "font-size: 13px; font-family: Microsoft Sans Serif; background: #f0fff0" );
		txt2->SetText( "ello." );
		
		auto txt3 = root->AddNode( nuTagDiv );
		txt3->StyleParse( "font-size: 18px; font-family: Times New Roman; background: #f0f0ff" );
		txt3->SetText( " More times at a smaller size." );
	}

	// ramp of 'e' characters from 8 to 30 pixels
	if (1)
	{
		root->StyleParse( "font-family: Segoe UI; background: #fff" );
		
		for ( int size = 8; size < 30; size++ )
		{
			auto txt = root->AddNode( nuTagDiv );
			txt->StyleParse( fmt("font-size: %dpx; background: #e0e0e0", size).Z );
			txt->SetText( "e" );
		}
	}
}

void DoBaselineAlignment_rev2( nuDoc* doc )
{
	// This creates 2 blocks in a single line.
	// The first block has 14px text, and the second block has 12px text.
	// The first block has text centered inside it. The second block's text is aligned to the baseline of the first.
	auto root = &doc->Root;
	/*
		<div width: 30, height:30, sizing:margin-box, background:#ddd, margin:0 2 0 2>
		  <lab center:center, height:10>
			Hello
		  <lab>
		</div>
		<div width: 20, height:30, sizing:margin-box, background:#bbb, margin:0 2 0 2>
		  <lab baseline:baseline, height:7>
			world
		  <lab>
		</div>
		<edit baseline:baseline, height:7, background:#fff, border: solid 1 #00c, width: 12em>
		  edit me
		<edit>
	*/

	// This demo proves several things:
	// * The first Lab object has no height defined, yet it can center itself
	// * The second Div object has no height defined, yet it can align itself to the bottom of its parent

	nuString e;
	int v = 4;
	if ( v == 1 )
	{
		// only 1 deep
		e = root->Parse( 
			"<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
			"	<lab style='hcenter: hcenter; vcenter: vcenter; background: #faa; width: 80ep; height: 50ep'/>"
			"</div>"
			);
	}
	else if ( v == 2 )
	{
		// inner element's size comes from text impostor
		e = root->Parse(
			"<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
			"	<lab style='hcenter: hcenter; vcenter: vcenter; background: #faa;'>"
			"		<div style='width: 80ep; height: 50ep; background: #eee'/>"
			"	</lab>"
			"</div>"
			);
	}
	else if ( v == 3 )
	{
		// inner element's size comes from text
		e = root->Parse(
			"<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
			"	<lab style='hcenter: hcenter; vcenter: vcenter; background: #faa;'>"
			"		Hello\nWorld!"
			"	</lab>"
			"</div>"
			);
	}
	else if ( v == 4 )
	{
		// two text elements beneath each other, centered horizontally
		// txtContainer needs two passes over its children, so that it can center them after it knows its own width.
		auto div1 = root->AddNode( nuTagDiv );
		div1->StyleParse( "width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep" );
		auto txtContainer = div1->AddNode( nuTagDiv );
		txtContainer->StyleParse( "hcenter: hcenter; vcenter: vcenter; background: #faa;" );
		auto lab1 = txtContainer->AddNode( nuTagLab );
		//txtContainer->AddBreak();
		lab1->StyleParse( "hcenter: hcenter; background: #afa; break: after" );
		lab1->SetText( "Hello" );
		auto lab2 = txtContainer->AddNode( nuTagLab );
		lab2->StyleParse( "hcenter: hcenter; background: #afa;" );
		lab2->SetText( "World!" );
	}

	NUASSERT(e == "");
	// 2nd div, that aligns to baseline
	/*
	auto div2 = root->AddNode( nuTagDiv );
	div2->StyleParse( "width: 150ep; height: 20ep; bottom: bottom; box-sizing: margin; background: #eee; margin: 0 2ep 0 2ep" );
	auto div2_lab = div2->AddNode( nuTagLab );
	div2_lab->StyleParse( "baseline: baseline; font-size: 12ep; background: #dfd" );
	div2_lab->StyleParse( "width: 130ep; height: 12ep;" ); // hack for lack of supporting text
	// hack for lack of supporting text
	div2_lab->AddNode( nuTagDiv )->StyleParse( "width: 130ep; height: 12ep; background: #fff" );
	//div2_lab->SetText( "world" );
	*/
}

void DoTwoTextRects( nuDoc* doc )
{
	if (1)
	{
		nuDomNode* div = doc->Root.AddNode( nuTagDiv );
		div->StyleParse( "width: 90px; height: 90px; background: #faa; margin: 4px" );
		div->StyleParse( "font-size: 13px" );
		//div->StyleParse( "text-align-vertical: top" );
		div->SetText( "Ave quick brown fox jumps over the lazy dog.\nText wrap and kerning. >>" );
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
	doc->Root.StyleParse( "padding: 4px" );

	// This should produce a matrix of blocks that are perfectly separated by 8 pixels.
	// It would be 4 pixels between blocks if we collapsed margins, but we don't do that.
	if (1)
	{
		for ( int i = 0; i < 20; i++ )
		{
			nuDomNode* div = doc->Root.AddNode( nuTagDiv );
			div->StyleParse( "width: 150px; height: 80px; background: #faa8; margin: 4px; border-radius: 5px;" );
			div->StyleParse( "font-size: 13px" );
			div->SetText( fmt("  block %v", i).Z );
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

void DoLongText( nuDoc* doc )
{
	auto div = doc->Root.AddNode( nuTagDiv );
	div->StyleParse( "padding: 10px; width: 500px; font-family: Times New Roman; font-size: 19px; color: #333;" );
	div->SetText(
		"It is an ancient Mariner,\n"
		"And he stoppeth one of three.\n"
		"'By thy long grey beard and glittering eye,\n"
		"Now wherefore stopp'st thou me?\n"
		"\n"
		"The Bridegroom's doors are opened wide,\n"
		"And I am next of kin;\n"
		"The guests are met, the feast is set:\n"
		"May'st hear the merry din.'\n"
		"\n"
		"He holds him with his skinny hand,\n"
		"'There was a ship,' quoth he.\n"
		"'Hold off! unhand me, grey-beard loon!'\n"
		"Eftsoons his hand dropt he.\n"
		);
}

void DoBackupSettings( nuDoc* doc )
{
	nuDomNode* root = &doc->Root;
	doc->ClassParse( "bg-light", "background: #eee; padding: 8ep" );
	doc->ClassParse( "bg-dark", "background: #ddd; padding: 8ep" );
	doc->ClassParse( "button", "background: #fdd; padding: 8ep 1ep 8ep 1ep" );

	nuDomNode* label = root->AddNode( nuTagDiv );
	label->AddClass( "bg-light" );
	label->SetText( "Backup from" );

	nuDomNode* txt = root->AddNode( nuTagDiv );
	txt->AddClass( "bg-dark" );
	txt->SetText( "this is a text box" );
	
	nuDomNode* btn = root->AddNode( nuTagDiv );
	btn->AddClass( "button" );
	btn->SetText( "Browse..." );
}

void InitDOM( nuDoc* doc )
{
	nuDomNode* body = &doc->Root;
	body->StyleParse( "font-family: Segoe UI, Droid Sans" );

	//DoBaselineAlignment( doc );
	DoBaselineAlignment_rev2( doc );
	//DoTwoTextRects( doc );
	//DoBlockMargins( doc );
	//DoLongText( doc );
	//DoBackupSettings( doc );

	body->OnClick( [](const nuEvent& ev) -> bool {
		nuGlobal()->EnableKerning = !nuGlobal()->EnableKerning;
		return true;
	});
}