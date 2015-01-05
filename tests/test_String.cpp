#include "pch.h"
#include "../xo/xo.h"

TESTFUNC(String)
{
	{
		// Strange heap issues here when statically linking to MSVC CRT.
		// Mentioned more inside units.lua
		xoString x = "<>";
		x.Split("<>");
		x.Split("<>");
		x.Split("<>");
		x.Split("<>");
		x.Split("<>");
	}

	{
		fhashmap<xoString, int> tab1;
		tab1.insert("a", 1);
	}
	{
		fhashmap<xoString, int> tab1, tab2;
		tab1.insert("a", 1);
		tab1.insert("b", 2);
		tab2 = tab1;
		tab1 = tab2;
	}
	{
		podvec<xoString> p;

		p = xoString("a<>b").Split("<>");
		TTASSERT(p.size() == 2 && p[0] == "a" && p[1] == "b");
		TTASSERT(xoString::Join(p, "()") == "a()b");
		TTASSERT(xoString::Join(p, "") == "ab");
		TTASSERT(xoString::Join(p, nullptr) == "ab");

		p = xoString("a<>b<>").Split("<>");
		TTASSERT(p.size() == 3 && p[0] == "a" && p[1] == "b" && p[2] == "");
		TTASSERT(xoString::Join(p, "()") == "a()b()");

		p = xoString("<>").Split("<>");
		TTASSERT(p.size() == 2 && p[0] == "" && p[1] == "");
		TTASSERT(xoString::Join(p, "()") == "()");

		p = xoString("x").Split("<>");
		TTASSERT(p.size() == 1 && p[0] == "x");
		TTASSERT(xoString::Join(p, "()") == "x");

		TTASSERT(xoString::Join(podvec<xoString>(), "") == "");
		TTASSERT(xoString::Join(podvec<xoString>(), "x") == "");
		TTASSERT(xoString::Join(podvec<xoString>(), nullptr) == "");

	}
}
