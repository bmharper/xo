#ifndef OHASHSET_H
#define OHASHSET_H

#include "ohashtable.h"

namespace ohash
{

template <	typename TKey,
						typename THashFunc = ohashfunc_cast< TKey >,
						typename TGetKeyFunc = ohashgetkey_self< TKey, TKey >,
						typename TGetValFunc = ohashgetval_self< TKey, TKey > 
					>
class ohashset : public ohashtable< TKey, TKey, THashFunc, TGetKeyFunc, TGetValFunc >
{
public:
	typedef ohashtable< TKey, TKey, THashFunc, TGetKeyFunc, TGetValFunc > base;
	// GCC 4 requires this (note the 'typename'). it's something about 'two-phase name lookup'. I don't want to know!
	typedef typename base::iterator iterator;

	ohashset()
	{
	}

	bool insert( const TKey& obj )
	{
		return insert_check_exist( obj ) != npos;
	}

	/// Wrapper for contains()
	bool operator[] ( const TKey& obj ) const { return contains(obj); }

	/// Calls insert()
	ohashset& operator+=( const TKey& obj )
	{
		insert( obj );
		return *this;
	}

	/// Calls erase()
	ohashset& operator-=( const TKey& obj )
	{
		erase( obj );
		return *this;
	}

	// Stupid C++ overload breaks inheritance rule
	ohashset& operator+=( const ohashset& obj )
	{
		base::operator +=( obj );
		return *this;
	}

	// Stupid C++ overload breaks inheritance rule
	ohashset& operator-=( const ohashset& obj )
	{
		base::operator -=( obj );
		return *this;
	}

#ifdef DVECT_DEFINED
	/// Returns a list of all our keys
	dvect<TKey> keys() const
	{
		dvect<TKey> k;
		for ( iterator it = base::begin(); it != base::end(); it++ ) 
			k += *it;
		return k;
	}
#endif

	/** Determine if the set is not equal to set @a b.
	**/
	bool operator!=( const ohashset& b ) const
	{
		return !( *this == b );
	}

	/** Determine if the set is equal to set @a b.
	**/
	bool operator==( const ohashset& b ) const
	{
		if ( base::size() != b.size() ) return false;

		for ( iterator it = base::begin(); it != base::end(); it++ )
			if ( !b.contains( *it ) ) return false;

		return true;
	}

};

}
#endif
