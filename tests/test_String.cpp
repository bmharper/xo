#include "pch.h"

TESTFUNC(String)
{
	{
		// Strange heap issues here when statically linking to MSVC CRT.
		// Mentioned more inside units.lua
		xo::String x = "<>";
		x.Split("<>");
		x.Split("<>");
		x.Split("<>");
		x.Split("<>");
		x.Split("<>");
	}

	{
		ohash::map<xo::String, int> tab1;
		tab1.insert("a", 1);
	}
	{
		ohash::map<xo::String, int> tab1, tab2;
		tab1.insert("a", 1);
		tab1.insert("b", 2);
		tab2 = tab1;
		tab1 = tab2;
	}
	{
		xo::cheapvec<xo::String> p;

		p = xo::String("a<>b").Split("<>");
		TTASSERT(p.size() == 2 && p[0] == "a" && p[1] == "b");
		TTASSERT(xo::String::Join(p, "()") == "a()b");
		TTASSERT(xo::String::Join(p, "") == "ab");
		TTASSERT(xo::String::Join(p, nullptr) == "ab");

		p = xo::String("a<>b<>").Split("<>");
		TTASSERT(p.size() == 3 && p[0] == "a" && p[1] == "b" && p[2] == "");
		TTASSERT(xo::String::Join(p, "()") == "a()b()");

		p = xo::String("<>").Split("<>");
		TTASSERT(p.size() == 2 && p[0] == "" && p[1] == "");
		TTASSERT(xo::String::Join(p, "()") == "()");

		p = xo::String("x").Split("<>");
		TTASSERT(p.size() == 1 && p[0] == "x");
		TTASSERT(xo::String::Join(p, "()") == "x");

		TTASSERT(xo::String::Join(xo::cheapvec<xo::String>(), "") == "");
		TTASSERT(xo::String::Join(xo::cheapvec<xo::String>(), "x") == "");
		TTASSERT(xo::String::Join(xo::cheapvec<xo::String>(), nullptr) == "");

	}
}
