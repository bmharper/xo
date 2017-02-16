#include "pch.h"

TESTFUNC(Parser)
{
	{
		xo::Doc d(nullptr);
		TTASSERT(d.Parse(
			"<div>"
			"	<div>  text   </div>"
			"</div>"
			) == "");

		// Pure whitespace is always stripped.
		// This is solely in order to allow one to write a DOM string like the one above.
		xo::DomNode* div1;
		xo::DomNode* div2;
		TTASSERT((div1 = (xo::DomNode*) d.Root.ChildByIndex(0)) != nullptr);
		TTASSERT(div1->ChildCount() == 1);

		TTASSERT((div2 = (xo::DomNode*) div1->ChildByIndex(0)) != nullptr);
		TTASSERT(div2->ChildCount() == 1);
		TTASSERT(xo::String(div2->GetText()) == "  text   ");
	}

}
