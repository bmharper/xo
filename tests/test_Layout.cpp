#include "pch.h"
#include "xoImageTester.h"

TESTFUNC(Layout)
{
	// this was consolas 11px
	xoImageTester itest;
	itest.TruthImage( "hello-world", []( xoDomNode& root ) {
		root.StyleParse( "margin: 20px;" );
		root.SetText( "hello whirled" );
	});
}
