#ifndef OHASHMAP_H
#define OHASHMAP_H

#include "ohashtable.h"

namespace ohash
{

template <	typename TKey,
						typename TVal,
						typename THashFunc = ohashfunc_cast< TKey >,
						typename TGetKeyFunc = ohashgetkey_pair< TKey, TVal > ,
						typename TGetValFunc = ohashgetval_pair< TKey, TVal >
					>
class ohashmap : public ohashtable< TKey, std::pair< TKey, TVal >, THashFunc, TGetKeyFunc, TGetValFunc >
{
public:
	typedef std::pair< TKey, TVal > pair_type;
	typedef ohashtable< TKey, pair_type, THashFunc, TGetKeyFunc, TGetValFunc > base;
	// GCC 4 requires this (note the 'typename'). it's something about 'two-phase name lookup'. I don't want to know!
	typedef typename base::iterator iterator;

	ohashmap()
	{
	}

	bool insert( const pair_type& obj )
	{
		return insert_check_exist( obj ) != npos;
	}

	bool insert( const TKey& key, const TVal& val, bool overwrite = false )
	{
		return insert_check_exist( pair_type( key, val ), overwrite ) != npos;
	}

	/** Insert, always overwriting any existing value. 
	**/
	void set( const TKey& key, const TVal& val )
	{
		insert( key, val, true );
	}

#ifdef DVECT_DEFINED
	/// Returns a list of all our keys
	dvect<TKey> keys() const
	{
		dvect<TKey> k;
		for ( iterator it = base::begin(); it != base::end(); it++ )
			k += it->first;
		return k;
	}
#endif


	/// Get an item in the set
	/**
	\return The object if found, or TVal() if not found.
	**/
	TVal get( const TKey& Key ) const
	{
		hashsize_t pos = _find(Key);
		if ( pos != npos ) return base::mData[pos].second;
		else return TVal();
	}

	/// Get a pointer to an item in the map
	/**
	\return A pointer to an item in the set, NULL if object is not in set.
	**/
	TVal* getp( const TKey& Key ) const
	{
		hashsize_t pos = _find(Key);
		if (pos != npos) return &(base::mData[pos].second);
		else return NULL;
	}

	// const version- does not create object if it does not exist
	const TVal& operator[]( const TKey& index ) const
	{
		// This was my original implementation, but the fact that npos = -1 means that base::mdata[pos] is usually not
		// an invalid memory location, so we don't trap the error at site.
		
		// hashsize_t pos = _find( index );
		// return base::mData[pos].second;

		// This is better, since it has a much higher likelihood of tripping an access violation
		TVal* p = getp(index);
		return *p;
	}

	TVal& operator[]( const TKey& index )
	{
		hashsize_t pos = _find( index );
		if ( pos == npos ) 
		{
			// create it.
			pos = insert_no_check( pair_type(index, TVal()) );
		}
		return base::mData[pos].second;
	}

};

}

#endif
