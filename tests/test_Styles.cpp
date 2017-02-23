#include "pch.h"

static xo::StyleAttrib AttribAtPos(int pos) {
	xo::StyleAttrib a;
	a.SetSize((xo::StyleCategories)(pos + 1), xo::Size::Pixels((float) pos));
	return a;
}

static bool EQUALS(const xo::StyleAttrib& a, const xo::StyleAttrib& b) {
	if (a.Category != b.Category)
		return false;
	if (a.SubType != b.SubType)
		return false;
	if (a.Flags != b.Flags)
		return false;
	if (a.ValU32 != b.ValU32)
		return false;
	return true;
}

TESTFUNC(StyleVars) {
	// Run all sub-tests twice. The first time, we do not expand variables inside style classes.
	// This is the behaviour you'd have if you were using StyleResolveOnceOff inside "user" code.
	// On the second pass, we bake the style variables into the style classes, by calling 
	// ExpandVerbatimVariables. This is a performance enhancement that is done before rendering,
	// on the renderer's copy of the document, so that every time a class is referenced, we don't
	// need to expand it's variables.
	for (int expandEarly = 0; expandEarly < 2; expandEarly++) {
		{
			xo::Doc doc(nullptr);

			// Notice the nested variables here
			doc.ParseStyleSheet(R"(
				$dark-color2 = #ccc;
				$px1 = 1px;
				$px1234 = 1px 2px 3px 4px;
				$dark-border = 1px 2px 3px 4px $dark-color2;
				$double-border-1 = $px1 $dark-color2;
				$double-border-2 = $px1234 $dark-color2;
				thing1 {
					border: $dark-border;
					padding: 2px;
				}
				thing2 {
					border: $double-border-1;
				}
				thing3 {
					border: $double-border-2;
				}
				thing4 {
					border: $nothing 9px #fed;
				}
			)");
			TTASSERT(EQ(doc.StyleVar("$dark-color2"), "#ccc"));
			TTASSERT(EQ(doc.StyleVar("$dark-border"), "1px 2px 3px 4px $dark-color2"));

			doc.Root.ParseAppend("<div class='thing1'></div>");
			doc.Root.ParseAppend("<div class='thing2'></div>");
			doc.Root.ParseAppend("<div class='thing3'></div>");
			doc.Root.ParseAppend("<div class='thing4'></div>");

			auto thing1 = doc.ClassStyles.GetOrCreate("thing1");
			TTASSERT(thing1);

			// Verify sanity of 'thing' class - this is before variable bake
			const auto& attribs = thing1->Default.Attribs;
			TTASSERT(attribs.size() == 5);
			TTASSERT(attribs[0].Category == xo::CatGenBorder && attribs[0].Flags == xo::StyleAttrib::FlagVerbatim);
			TTASSERT(attribs[1].Category == xo::CatPadding_Left && attribs[1].GetSize().Val == 2);

			if (expandEarly)
				doc.ClassStyles.ExpandVerbatimVariables(&doc);

			{
				// thing1
				xo::StyleResolveOnceOff res((const xo::DomNode*) doc.Root.ChildByIndex(0));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Left).GetColor() == xo::Color(0xcc, 0xcc, 0xcc, 255));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Bottom).GetColor() == xo::Color(0xcc, 0xcc, 0xcc, 255));
				TTASSERT(res.RS->Get(xo::CatBorder_Left).GetSize() == xo::Size::Pixels(1));
				TTASSERT(res.RS->Get(xo::CatBorder_Bottom).GetSize() == xo::Size::Pixels(4));
				TTASSERT(res.RS->Get(xo::CatPadding_Bottom).GetSize() == xo::Size::Pixels(2));
			}

			{
				// thing2
				xo::StyleResolveOnceOff res((const xo::DomNode*) doc.Root.ChildByIndex(1));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Left).GetColor() == xo::Color(0xcc, 0xcc, 0xcc, 255));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Bottom).GetColor() == xo::Color(0xcc, 0xcc, 0xcc, 255));
				TTASSERT(res.RS->Get(xo::CatBorder_Left).GetSize() == xo::Size::Pixels(1));
				TTASSERT(res.RS->Get(xo::CatBorder_Bottom).GetSize() == xo::Size::Pixels(1));
			}

			{
				// thing3
				xo::StyleResolveOnceOff res((const xo::DomNode*) doc.Root.ChildByIndex(2));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Left).GetColor() == xo::Color(0xcc, 0xcc, 0xcc, 255));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Bottom).GetColor() == xo::Color(0xcc, 0xcc, 0xcc, 255));
				TTASSERT(res.RS->Get(xo::CatBorder_Left).GetSize() == xo::Size::Pixels(1));
				TTASSERT(res.RS->Get(xo::CatBorder_Bottom).GetSize() == xo::Size::Pixels(4));
			}

			{
				// thing4 - undefined variable gets substituted with empty string
				xo::StyleResolveOnceOff res((const xo::DomNode*) doc.Root.ChildByIndex(3));
				TTASSERT(res.RS->Get(xo::CatBorderColor_Left).GetColor() == xo::Color(0xff, 0xee, 0xdd, 255));
				TTASSERT(res.RS->Get(xo::CatBorder_Left).GetSize() == xo::Size::Pixels(9));
			}
		}
		{
			// Individual overrides generic
			xo::Doc doc(nullptr);
			doc.ParseStyleSheet(R"(
				$border = 1px 2px 3px 4px #abc;
				thing {
					border: $border;
					border-right: 10px #ddd;
				}
			)");
			doc.Root.AddClass("thing");
			if (expandEarly)
				doc.ClassStyles.ExpandVerbatimVariables(&doc);
			xo::StyleResolveOnceOff res(&doc.Root);
			TTASSERT(res.RS->Get(xo::CatBorder_Left).GetSize() == xo::Size::Pixels(1));
			TTASSERT(res.RS->Get(xo::CatBorder_Right).GetSize() == xo::Size::Pixels(10));
			TTASSERT(res.RS->Get(xo::CatBorderColor_Right).GetColor() == xo::Color(0xdd, 0xdd, 0xdd, 255));
		}
		{
			// All
			xo::Doc doc(nullptr);
			doc.ParseStyleSheet(R"(
				everything {
					background: $color;
					color: $color;
					width: $size;
					height: $size;
					padding: $quadsize;
					margin: $quadsize;
					position: $pos;
					border: $quadsize $color;
					border-radius: $quadsize;
					break: $break;
					canfocus: $true;
					cursor: $cursor;
					flow-context: $flow-context;
					box-sizing: $box-sizing;
					font-size: $size;
					font-family: $font;
					left: $hbind;
					hcenter: $hbind;
					right: $hbind;
					top: $vbind;
					vcenter: $vbind;
					bottom: $vbind;
					baseline: $vbind;
					bump: $bump;
				}
			)");

			doc.SetStyleVar("$color", "#abc");
			doc.SetStyleVar("$size", "5px");
			doc.SetStyleVar("$quadsize", "1px 2px 3px 4px");
			doc.SetStyleVar("$pos", "absolute");
			doc.SetStyleVar("$break", "after");
			doc.SetStyleVar("$true", "true");
			doc.SetStyleVar("$cursor", "wait");
			doc.SetStyleVar("$flow-context", "inject");
			doc.SetStyleVar("$box-sizing", "margin");
			doc.SetStyleVar("$font", "wingdings");
			doc.SetStyleVar("$hbind", "right");
			doc.SetStyleVar("$vbind", "bottom");
			doc.SetStyleVar("$bump", "vertical");

			doc.Root.AddClass("everything");

			if (expandEarly)
				doc.ClassStyles.ExpandVerbatimVariables(&doc);

			xo::StyleResolveOnceOff res(&doc.Root);

			auto col   = xo::Color::RGBA(0xaa, 0xbb, 0xcc, 0xff);
			auto size  = xo::Size::Pixels(5);
			auto quad3 = xo::Size::Pixels(4);

			TTASSERT(res.RS->Get(xo::CatBackground).GetColor() == col);
			TTASSERT(res.RS->Get(xo::CatColor).GetColor() == col);
			TTASSERT(res.RS->Get(xo::CatWidth).GetSize() == size);
			TTASSERT(res.RS->Get(xo::CatHeight).GetSize() == size);
			TTASSERT(res.RS->Get(xo::CatPadding_Bottom).GetSize() == quad3);
			TTASSERT(res.RS->Get(xo::CatMargin_Bottom).GetSize() == quad3);
			TTASSERT(res.RS->Get(xo::CatPosition).GetPositionType() == xo::PositionAbsolute);
			TTASSERT(res.RS->Get(xo::CatBorderColor_Bottom).GetColor() == col);
			TTASSERT(res.RS->Get(xo::CatBorder_Bottom).GetSize() == quad3);
			TTASSERT(res.RS->Get(xo::CatBorderRadius_BL).GetSize() == quad3);
			TTASSERT(res.RS->Get(xo::CatBreak).GetBreakType() == xo::BreakAfter);
			TTASSERT(res.RS->Get(xo::CatCanFocus).GetCanFocus() == true);
			TTASSERT(res.RS->Get(xo::CatCursor).GetCursor() == xo::CursorWait);
			TTASSERT(res.RS->Get(xo::CatFlowContext).GetFlowContext() == xo::FlowContextInject);
		}
	} // for (expandEarly...
}

TESTFUNC(StyleSet) {
	{
		// set 0,1,1
		xo::Pool     pool;
		xo::StyleSet set;
		set.Set(AttribAtPos(0), &pool);
		set.Set(AttribAtPos(1), &pool);
		set.Set(AttribAtPos(1), &pool);
		TTASSERT(EQUALS(set.Get(AttribAtPos(0).GetCategory()), AttribAtPos(0)));
		TTASSERT(EQUALS(set.Get(AttribAtPos(1).GetCategory()), AttribAtPos(1)));
	}
	{
		// set 0,1,0
		xo::Pool     pool;
		xo::StyleSet set;
		set.Set(AttribAtPos(0), &pool);
		set.Set(AttribAtPos(1), &pool);
		set.Set(AttribAtPos(0), &pool);
		TTASSERT(EQUALS(set.Get(AttribAtPos(0).GetCategory()), AttribAtPos(0)));
		TTASSERT(EQUALS(set.Get(AttribAtPos(1).GetCategory()), AttribAtPos(1)));
	}

	for (int nstyle = 1; nstyle < xo::CatEND; nstyle++) {
		xo::Pool     pool;
		xo::StyleSet set;
		for (int i = 0; i < nstyle; i++) {
			set.Set(AttribAtPos(i), &pool);
			for (int j = 0; j <= i; j++) {
				xo::StyleAttrib truth = AttribAtPos(j);
				xo::StyleAttrib check = set.Get(truth.GetCategory());
				TTASSERT(EQUALS(truth, check));
			}
		}
	}
}
