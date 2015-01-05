#include "pch.h"

TESTFUNC(Parser)
{
	{
		xoDoc d;
		TTASSERT(d.Parse(
			"<div>"
			"	<div>  text   </div>"
			"</div>"
			) == "");

		// Pure whitespace is always stripped.
		// This is solely in order to allow one to write a DOM string like the one above.
		xoDomNode* div1;
		xoDomNode* div2;
		TTASSERT((div1 = (xoDomNode*) d.Root.ChildByIndex(0)) != nullptr);
		TTASSERT(div1->ChildCount() == 1);

		TTASSERT((div2 = (xoDomNode*) div1->ChildByIndex(0)) != nullptr);
		TTASSERT(div2->ChildCount() == 1);
		TTASSERT(xoString(div2->GetText()) == "  text   ");
	}

}
