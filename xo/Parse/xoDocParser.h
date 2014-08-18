#pragma once

#include "../xoString.h"

class xoDomNode;

/* Parse xml-like document format into a DOM node.
This doesn't make any special attempts to be fast.

Example:

<div class='class1 class2' style='font-size: 12px'>
	Hello &lt; World!
</div>

The class and style strings can be quoted with single or double quotes.
If you're embedding these strings inside C++ code, it's obviously nicer using single quotes.

The only escaped character sequences are these:
	&gt;   >
	&lt;   <
	&amp;  &

Whitespace:
On either side of a text element, all whitespace, including newlines and carriage returns, is ignored.
In the example above, the text element will be "Hello < World!"
Within a text element, whitespace is not condensed.
Carriage return is always discarded.
Examples:
	<div>  some\ntext\n </div>			"some\ntext"
	<div>some\ntext</div>				"some\ntext"
	<div>some\r\ntext</div>				"some\ntext"
	<div>some\n\rtext</div>				"some\ntext"
	<div>some   text</div>				"some   text"

*/
class XOAPI xoDocParser
{
public:
	xoString Parse( const char* src, xoDomNode* target );

protected:
	static bool IsWhite( int c );
	static bool IsAlpha( int c );
	static bool EqNoCase( const char* a, const char* b, intp bLen );
};
