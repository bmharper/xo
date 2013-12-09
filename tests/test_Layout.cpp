#include "pch.h"
#include "nuImageTester.h"

TESTFUNC(Layout)
{
	nuImageTester itest;
	itest.TruthImage( "hello-world", []( nuDomEl& root ) {
		root.StyleParse( "margin: 20px;" );
		root.SetText( "hello whirled" );
	});
}
