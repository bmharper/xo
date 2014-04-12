#include "pch.h"
#include "../nudom/nuDom.h"

TESTFUNC(String)
{
	{
		// Strange heap issues here when statically linking to MSVC CRT.
		// Mentioned more inside units.lua
		nuString x = "<>";
		x.Split( "<>" );
		x.Split( "<>" );
		x.Split( "<>" );
		x.Split( "<>" );
		x.Split( "<>" );
	}

	{
		fhashmap<nuString, int> tab1;
		tab1.insert( "a", 1 );
	}
	{
		fhashmap<nuString, int> tab1, tab2;
		tab1.insert( "a", 1 );
		tab1.insert( "b", 2 );
		tab2 = tab1;
		tab1 = tab2;
	}
	{
		podvec<nuString> p;

		p = nuString("a<>b").Split( "<>" );
		TTASSERT( p.size() == 2 && p[0] == "a" && p[1] == "b" );
		TTASSERT( nuString::Join(p, "()") == "a()b" );
		TTASSERT( nuString::Join(p, "") == "ab" );
		TTASSERT( nuString::Join(p, nullptr) == "ab" );

		p = nuString("a<>b<>").Split( "<>" );
		TTASSERT( p.size() == 3 && p[0] == "a" && p[1] == "b" && p[2] == "" );
		TTASSERT( nuString::Join(p, "()") == "a()b()" );

		p = nuString("<>").Split( "<>" );
		TTASSERT( p.size() == 2 && p[0] == "" && p[1] == "" );
		TTASSERT( nuString::Join(p, "()") == "()" );

		p = nuString("x").Split( "<>" );
		TTASSERT( p.size() == 1 && p[0] == "x" );
		TTASSERT( nuString::Join(p, "()") == "x" );

		TTASSERT( nuString::Join(podvec<nuString>(), "") == "" );
		TTASSERT( nuString::Join(podvec<nuString>(), "x") == "" );
		TTASSERT( nuString::Join(podvec<nuString>(), nullptr) == "" );

	}
}
