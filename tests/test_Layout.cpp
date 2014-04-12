#include "pch.h"
#include "nuImageTester.h"

TESTFUNC(Layout)
{
	// this was consolas 11px
	nuImageTester itest;
	itest.TruthImage( "hello-world", []( nuDomEl& root ) {
		root.StyleParse( "margin: 20px;" );
		root.SetText( "hello whirled" );
	});
}
