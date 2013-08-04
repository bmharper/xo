#pragma once

#include "dvec.h"

/// A sorted vector. Sorts items in increasing order of TSort.
template <class TSort, class TReference>
class SortedVector
{
public:
	typedef pair< TSort, TReference > TData;
	dvect< TData > Data;
	int MaxLength;

	SortedVector( int maxlen )
	{
		MaxLength = maxlen;
	}

	const TReference& operator[]( int index ) const
	{
		return Data[index].second;
	}

	const int Size() const { return Data.size(); }

	void Insert( const TSort& sort, const TReference& ref )
	{
		// check first if value is too great to make it in
		if ( Data.size() > 0 && Data.back().first < sort ) return;

		int i = 0;
		for (; i < Data.size(); i++)
		{
			if ( sort < Data[i].first ) break;
		}
		
		if ( i >= Data.size() ) 
		{
			if ( Data.size() >= MaxLength )
				return;
			else
				Data.push_back( TData( sort, ref ) );
		}
		else
		{
			Data.insert( i, TData( sort, ref ) );
			if ( Data.size() > MaxLength )
				Data.pop_back();
		}
	}
};
