#include "pch.h"
#include "../nudom/nuDom.h"

static nuStyleAttrib AttribAtPos( int pos )
{
	nuStyleAttrib a;
	a.SetSize( (nuStyleCategories) (pos + 1), nuSize::Pixels((float) pos) );
	return a;
}

static bool EQUALS( const nuStyleAttrib& a, const nuStyleAttrib& b )
{
	if ( a.Category != b.Category ) return false;
	if ( a.SubType != b.SubType ) return false;
	if ( a.Flags != b.Flags ) return false;
	if ( a.ValU32 != b.ValU32 ) return false;
	return true;
}

TESTFUNC(StyleSet)
{
	{
		// set 0,1,1
		nuPool pool;
		nuStyleSet set;
		set.Set( AttribAtPos(0), &pool );
		set.Set( AttribAtPos(1), &pool );
		set.Set( AttribAtPos(1), &pool );
		TTASSERT( EQUALS(set.Get(AttribAtPos(0).GetCategory()), AttribAtPos(0)) );
		TTASSERT( EQUALS(set.Get(AttribAtPos(1).GetCategory()), AttribAtPos(1)) );
	}
	{
		// set 0,1,0
		nuPool pool;
		nuStyleSet set;
		set.Set( AttribAtPos(0), &pool );
		set.Set( AttribAtPos(1), &pool );
		set.Set( AttribAtPos(0), &pool );
		TTASSERT( EQUALS(set.Get(AttribAtPos(0).GetCategory()), AttribAtPos(0)) );
		TTASSERT( EQUALS(set.Get(AttribAtPos(1).GetCategory()), AttribAtPos(1)) );
	}

	for ( int nstyle = 1; nstyle < nuCatEND; nstyle++ )
	{
		nuPool pool;
		nuStyleSet set;
		for ( int i = 0; i < nstyle; i++ )
		{
			set.Set( AttribAtPos(i), &pool );
			for ( int j = 0; j <= i; j++ )
			{
				nuStyleAttrib truth = AttribAtPos(j);
				nuStyleAttrib check = set.Get( truth.GetCategory() );
				TTASSERT( EQUALS(truth, check) );
			}
		}
	}
}
