#pragma once

#include "vHashMap.h"
#include "dvec.h"

/** A hybrid map designed for integer-integer relationships that have a near 1:1 binding.
This map was created for keeping database records in sync with shapefile features, and for
keeping database joins in sync. The reason is that storing a 1:1 vector map is much faster
and mem-cheaper if the record relationship is close to 1:1. However, one does not want to
have to update the entire linear map every time a modification is made. Thus, what we want
is a map that behaves like a hash map, but internally uses a linear map first, then a hash 
map second, in order to record recent changes.

Key must be 32-bit signed integer.
Value can be either 32-bit signed integer or 64-bit signed integer.
-1 Is a missing/invalid key/value.

Note that the hash map is not yet used. It's sort of ready.... my usual style.
**/
template< typename TValue >
class One2OneMap
{
public:
	One2OneMap(void)
	{
		Clear();
	}
	~One2OneMap(void)
	{
	}

	typedef int TKey;

	static const int NullKey = -1;
	static const TValue NullValue = -1;

	void Clear()
	{
		OneToOne = true;
		OneToOneSize = 0;
		Linear.clear();
		Map.clear();
	}

	void Insert( TKey key, TValue value )
	{
		if ( OneToOne )
		{
			if ( key != value || key > OneToOneSize ) 
			{
				// lost our one-to-one status
				OneToOne = false;
				Linear.resize( OneToOneSize );
				for ( int i = 0; i < OneToOneSize; i++ )
					Linear[i] = i;
			}
			else
			{
				OneToOneSize++;
				return;
			}
		}
		if ( key == Linear.size() )
		{
			// most common growing method
			Linear.push_back( value );
			Map.erase( key );
		}
		else if ( key < Linear.size() )
		{
			// key is inside- store in linear
			Linear[ key ] = value;
			Map.erase( key );
		}
		else if ( key > Linear.size() )
		{
			// key is outside linear so we: grow linear and commit map in growth region
			TKey oldSize = Linear.size();
			Linear.resize( key + 1 );

			// commit map
			for ( TKey i = oldSize; i < Linear.size(); i++ )
			{
				if ( Map.contains( i ) )	Linear[i] = Map[i];
				else						Linear[i] = NullValue;
				Map.erase( i );
			}
		}
	}

	TValue Get( TKey key )
	{
		ASSERT( key >= 0 );
		if ( OneToOne )
		{
			if ( key >= OneToOneSize )	return NullValue;
			else						return key;
		}
		if ( Map.contains( key ) ) return Map[key];
		else if ( key < Linear.size() ) return Linear[key];
		else return NullValue;
	}

	void Erase( TKey key )
	{
		OneToOne = false;
		if ( Map.contains( key ) ) Map.erase( key );
		if ( key < Linear.size() ) Linear[key] = NullValue;
	}

protected:

	// in order to templatize key, we'd have to provide a 64-bit dvect
	dvect<TValue> Linear;
	vHashMap<TKey, TValue> Map;

	bool OneToOne;
	int OneToOneSize;

};
