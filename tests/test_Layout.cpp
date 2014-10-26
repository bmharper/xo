#include "pch.h"
#include "xoImageTester.h"

// It would be good to take these tests outside of C++ source code,
// and embed them in some kind of html-like document. You would need
// a GUI program to help you vet failing tests (ie yes, the new image
// is now correct; or no, the old image is correct).

TESTFUNC(Layout_BodyMargin)
{
	xoImageTester::DoTruthImage( "body-margin", []( xoDomNode& root ) {
		root.StyleParse( "margin: 20px;" );
	});
}
