#ifndef OHASHMULTIMAP_H
#define OHASHMULTIMAP_H

#include <vector>
#include "ohashtable.h"
#include "../Containers/smallvec.h"
#include "../Containers/dvec.h"

namespace ohash
{

/** Vectorized multimap
This is a multimap based on ohashtable. The value for the ohashtable is a vector type.
The key is the same as TKey for this class. The default vector is smallvec, which provides
for some static storage, avoiding the need for a malloc in heuristically suitable cases.

Note that the behaviour of insert() differs slightly from other hash maps. See the function
definition for a description of it's behaviour.

The vector type must support the following operations:
- size function (const)
- push_back function
- erase function
- clear function
- operator[]
- copy constructor
- assignment operator
**/

template <	typename TKey,
						typename TVal,
						size_t   smallVecSize = 2,
						typename THashFunc = ohashfunc_cast< TKey >,
						typename TVect = smallvec< TVal, smallVecSize >,
						typename TGetKeyFunc = ohashgetkey_pair< TKey, TVect >, 
						typename TGetValFunc = ohashgetval_pair< TKey, TVect > 
					>
class ohash_vecmap : public ohashtable< TKey, std::pair< TKey, TVect >, THashFunc, TGetKeyFunc, TGetValFunc >
{
protected:
	/// The empty vector that we can return as const for empty results.
	TVect emptyVector;

public:
	typedef std::pair< TKey, TVect > pair_type;
	typedef ohashtable< TKey, pair_type, THashFunc, TGetKeyFunc, TGetValFunc > base;
	typedef typename base::iterator iterator;
	
	typedef TVect TInternalVector;

	ohash_vecmap()
	{
	}

	/// Retrieves all objects with Key = a\ key into an std::vector \a values.
	/**
	If the object does not exist, then values will be unchanged.
	**/
	void get( const TKey& key, std::vector< TVal >& values )
	{
		hashsize_t pos = _find( key );
		if ( pos != npos )
		{
			for ( size_t i = 0; i < vect_at( pos ).size(); i++ )
				values.push_back( vect_at( pos )[i] );
		}
	}

#ifdef DVECT_DEFINED
	/// Retrieves all objects with Key = a\ key into dvect \a values.
	/**
	If the object does not exist, then values will be unchanged.
	**/
	void get( const TKey& key, dvect< TVal >& values )
	{
		hashsize_t pos = _find( key );
		if ( pos != npos )
		{
			for ( size_t i = 0; i < vect_at( pos ).size(); i++ )
				values.push_back( vect_at( pos )[i] );
		}
	}

	/// Returns a list of all our keys
	dvect<TKey> keys() const
	{
		dvect<TKey> k;
		for ( iterator it = base::begin(); it != base::end(); it++ )
			k += it->first;
		return k;
	}
#endif

	/** Resize the table in order to accomodate a large number of objects.
	This can be used to avoid the incremental automatic resize that will
	happen if objects are inserted incrementally.
	\param totalKeysProjected Specify the total number of keys that will be 
	in the table. Be careful not to confuse this with the total number of objects,
	which are key,value pairs.
	**/
	void resize( size_t totalKeysProjected )
	{
		if ( totalKeysProjected < base::mCount ) return;
		base::resize( (hashsize_t) (totalKeysProjected * (base::mFillRatio + 0.01) + 10) );
	}

	/** Provides direct access to a vector. Object MUST EXIST.
	No guarantee is made on the existence of the object \a key. If it does not 
	exist, then behaviour is undefined.
	**/
	TVect& get( const TKey& key )
	{
		hashsize_t pos = _find( key );

		if ( pos != npos )	return base::mData[pos].second;
		else				return emptyVector;
	}

	/** Erases all elements with Key = \a key. Returns number of elements erased.
	Erase on a non-existant object is safe and will return 0.
	**/
	size_t erase( const TKey& key )
	{
		hashsize_t pos = _find( key );
		if ( pos != npos )
		{
			size_t elements = vect_at( pos ).size();
			vect_at( pos ).clear();
			base::erase( key );
			return elements;
		}
		else
		{
			return 0;
		}
	}

	/** Erases all elements with Key = \a key and Value = \a val.
	Erase on a non-existant object is safe and will return false.
	@return The number of objects erased.
	**/
	int erase( const TKey& key, const TVal& val )
	{
		hashsize_t pos = _find( key );
		if ( pos != npos )
		{
			int ecount = 0;
			TVect& v = vect_at( pos );
			for ( int i = (int) v.size() - 1; i >= 0; i-- )
			{
				if ( v[i] == val ) v.erase( i );
			}
			if ( v.size() == 0 )
			{
				// erase the empty thing
				base::erase( key );
			}
			return ecount;
		}
		else
		{
			return 0;
		}
	}

	/** Inserts a new value into the map. This does not check for existence of \a val.
	The behaviour of insert in this map differs from certain other hash maps because
	we do not check for the existence of the key,val pair. We allow multiple \a val
	objects for every \a key.
	**/
	void insert( const TKey& key, const TVal& val )
	{
		hashsize_t pos = _find( key );
		if ( pos == npos )
			pos = insert_no_check( pair_type( key, TVect() ) );

		vect_at( pos ).push_back( val );
	}

	/** Inserts a new value into the map. Checks for existence of \a val.
	This function will not allow duplicate key,val pairs.
	@return true if the key,val pair did not exist. false otherwise.
	**/
	bool insert_check( const TKey& key, const TVal& val )
	{
		hashsize_t pos = _find( key );
		if ( pos == npos )
			pos = insert_no_check( pair_type( key, TVect() ) );

		TVect& v = vect_at( pos );
		for ( size_t i = 0; i < v.size(); i++ )
			if ( v[i] == val ) return false;

		v.push_back( val );
		return true;
	}

	/** Const vector accessor. This does not create the vector if it does not exist.
	Accessing this function on a key that does not exist will yield the 
	built-in empty vector.
	**/
	const TVect& operator[]( const TKey& index ) const
	{
		hashsize_t pos = _find( index );

		if ( pos == npos )	return emptyVector;
		else				return vect_at( pos );
	}

	/** Non-const vector accessor. This will create the vector if it does not exist.
	This accessor will create the vector if it does not exist.
	**/
	TVect& operator[]( const TKey& index )
	{
		hashsize_t pos = _find( index );
		if ( pos == npos ) 
		{
			// create it.
			pos = insert_no_check( pair_type(index, TVect()) );
		}
		return vect_at( pos );
	}

protected:
	TVect& vect_at( hashsize_t pos )				{ return base::mData[pos].second; }
	const TVect& vect_at( hashsize_t pos ) const	{ return base::mData[pos].second; }
};


template <	typename TKey,
						typename TVal,
						typename THashFunc = ohashfunc_cast< TKey >,
						typename TGetKeyFunc = ohashgetkey_pair< TKey, TVal >, 
						typename TGetValFunc = ohashgetval_pair< TKey, TVal > 
					>
class ohash_multimap : public ohashtable< TKey, std::pair< TKey, TVal >, THashFunc, TGetKeyFunc, TGetValFunc >
{
public:
	typedef std::pair< TKey, TVal > pair_type;
	typedef ohashtable< TKey, pair_type, THashFunc, TGetKeyFunc, TGetValFunc > base;

	ohash_multimap()
	{
	}

	/// Retrieves all objects with Key = a\ key
	void get( const TKey& key, std::vector< TVal >& values )
	{
		if ( base::mSize == 0 ) return;
		hashkey_t hkey = THashFunc::gethashcode( key );
		hashsize_t pos = base::table_pos( hkey );
		hashsize_t first = pos;
		int i = 0;
		while ( true )
		{
			KeyState state = getState(base::mState, pos);
			if ( state == SFull && TGetKeyFunc::getkey( base::mData[pos] ) == key ) 
			{
				values.push_back( TGetValFunc::getval( base::mData[pos] ) );
			}
			else if ( state == SNull ) break;
			pos = base::table_pos( hkey, ++i );
			if ( pos == first ) break;
		}
	}

	/// Erases all elements with Key = \a key. Returns number of elements erased.
	int erase( const TKey& key )
	{
		return _erase_all( key );
	}

	void insert( const pair_type& obj )
	{
		insert_no_check( obj );
	}

	void insert( const TKey& key, const TVal& val )
	{
		insert_no_check( pair_type( key, val ) );
	}
};

}

#endif
