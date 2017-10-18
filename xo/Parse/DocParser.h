#pragma once
#include "../Base/xoString.h"
#include "../Base/MemPoolsAndContainers.h"
#include "../VirtualDom/VirtualDom.h"

namespace xo {

class DomNode;

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
Carriage return (\r) is always discarded, but newline (\n) is preserved.
Examples:
	<div>  some\ntext\n </div>			"some\ntext"
	<div>some\ntext</div>				"some\ntext"
	<div>some\r\ntext</div>				"some\ntext"
	<div>some\n\rtext</div>				"some\ntext"
	<div>some   text</div>				"some   text"

*/
class XO_API DocParser {
public:
	String Parse(const char* src, DomNode* target);
	String Parse(const char* src, vdom::Node* target, xo::Pool* pool);

protected:
	xo::Pool*   Pool = nullptr;
	static bool IsWhite(int c);
	static bool IsAlpha(int c);
	static bool Eq(const char* a, const char* b, size_t bLen);
	static bool EqNoCase(const char* a, const char* b, size_t bLen);
};
} // namespace xo
