#ifndef ABCORE_INCLUDED_PVECT_SORT_H
#define ABCORE_INCLUDED_PVECT_SORT_H

#include "../Other/lmTypes.h"

#include "pvect.h"


/** Move a selected set of objects within a linear list up or down.
@param TKey The sort key. Float will be fine you have only a handful of objects.
@param itemCount The number of items in @a items.
@param items The list of items.
@param alter The set of items that must move.
@param delta The distance that the chosen objects should move. -1 Will move them 1 position towards the
	beginning of the list.
@return True if any change occured.
**/
template < typename TKey >
bool shift_order( int itemCount, void** items, const PtrSet& alter, int delta )
{
	typedef sort_item< TKey > SortItem;

	TKey vdelta = delta;
	if ( vdelta < 0 )	vdelta -= 0.5;
	else				vdelta += 0.5;

	dvect< SortItem > sorted;
	sorted.reserve( itemCount );

	for ( int i = 0; i < itemCount; i++ )
	{
		SortItem it;
		it.Key = i;
		it.Object = items[i];
		if ( alter.contains( items[i] ) ) it.Key += vdelta;
		sorted.push_back( it );
	}

	sort( sorted );

	int changes = 0;

	for ( int i = 0; i < sorted.size(); i++ )
	{
		if ( items[i] != sorted[i].Object )
			changes++;
		items[i] = sorted[i].Object;
	}

	return changes != 0;
}

template < typename TKey >
bool shift_order( pvect<void*> items, const PtrSet& alter, int delta )
{
	return shift_order<TKey>( items.size(), items.data, alter, delta );
}

#endif
