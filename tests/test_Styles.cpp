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
	{
		xo::Doc doc(nullptr);
		doc.ParseStyleSheet(R"(
			$dark-color2 = #ccc;
			$dark-border = 1px 2px 3px 4px $dark-color2;
			thing {
				border: $dark-border;
				padding: 2px;
			}
		)");
		TTASSERT(EQ(doc.StyleVar("$dark-color2"), "#ccc"));
		TTASSERT(EQ(doc.StyleVar("$dark-border"), "1px 2px 3px 4px $dark-color2"));
		auto thing = doc.ClassStyles.GetOrCreate("thing");
		TTASSERT(thing);
		const auto& attribs = thing->Default.Attribs;
		TTASSERT(attribs.size() == 5);
		TTASSERT(attribs[0].Category == xo::CatGenBorder && attribs[0].Flags == xo::StyleAttrib::FlagVerbatim);
		TTASSERT(attribs[1].Category == xo::CatPadding_Left && attribs[1].GetSize().Val == 2);
	}
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
