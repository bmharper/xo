#include "pch.h"
#include "nuImageTester.h"

TESTFUNC(Layout)
{
	nuImageTester itest;
	itest.TruthImage( "hello-world", []( nuDomEl& root ) {
		root.StyleParse( "margin: 20px;" );
		root.SetText( "hello whirled" );
	});
	/*

	nuSysWnd* wnd = nuSysWnd::CreateWithDoc();

	wnd->SetPosition( nuBox(0, 0, 256, 256), nuSysWnd::SetPosition_Size );
	wnd->Show();
	nuDomEl& root = wnd->Doc()->Root;
	root.StyleParse( "margin: 20px;" );
	root.SetText( "hello whirled" );

	itest.CreateTruthImage( *wnd->DocGroup, "hello-world" );

	delete wnd;
	*/
}
