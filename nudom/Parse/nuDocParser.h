#pragma once

#include "../nuString.h"

class nuDomNode;

// Parse xml-like document format into a DOM node
// This doesn't make any special attempts to be fast
class NUAPI nuDocParser
{
public:
	nuString Parse( const char* src, nuDomNode* target );

protected:
	static bool IsWhiteNonText( int c );
	static bool IsWhiteText( int c );
	static bool IsAlpha( int c );
	static bool EqNoCase( const char* a, const char* b, intp bLen );
};
