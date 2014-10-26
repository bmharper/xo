#include "pch.h"
#include "xoImageTester.h"

TESTFUNC(Layout)
{
	xoImageTester itest;
	itest.TruthImage( "hello-world", []( xoDomNode& root ) {
		root.StyleParse( "margin: 20px;" );
		//root.SetText( "hello whirled" );
	});
}
