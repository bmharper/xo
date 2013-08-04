#ifndef NICEMAPS_H
#define NICEMAPS_H

// This code was written around 2001. I leave it around as a reminder
// of how awful the std::hash_map API is, and what motivated me to write my own.
// BMH 2013-03-13

/*
#include "../Strings/supp_std.h"

// A class derived from hash_map that provides some sensible overloads.


#ifdef _WIN32
#if _MSC_VER > 1300
#include <hash_map>
#include <hash_set>
using namespace stdext;
#else
#include <hash_map>
#include <hash_set>
using namespace std;
#endif
#else
// gcc
#include <ext/hash_map>
#include <ext/hash_set>
using namespace __gnu_cxx;
#endif

namespace std
{
	
#ifdef _WIN32
////////////////
//  VC

	template
	<
		class _Key,
		class _Tp,
		class _Tr = hash_compare<_Key, less<_Key> >,
		class _Alloc = allocator< pair<const _Key, _Tp> > 
	>

	#if _MSC_VER > 1300
	class hash_map_nice : public stdext::hash_map< _Key, _Tp, _Tr, _Alloc >
	#else
	class hash_map_nice : public std::hash_map< _Key, _Tp, _Tr, _Alloc >
	#endif


#else
////////////////
//  gcc

	template 
	<
		class _Key, 
		class _Tp,
		class _HashFcn  = hash<_Key>,
		class _EqualKey = equal_to<_Key>,
		class _Alloc =  allocator<_Tp> 
	>

	class hash_map_nice : public hash_map< _Key, _Tp, _HashFcn, _EqualKey, _Alloc >

#endif

	{
	public:

#ifdef _WIN32
		typedef pair< _Key, _Tp > pair_type;
		#if _MSC_VER > 1300
		typedef stdext::hash_map< _Key, _Tp, _Tr, _Alloc > base;
		#else
		typedef std::hash_map< _Key, _Tp, _Tr, _Alloc > base;
		#endif
		typedef typename base::iterator base_iterator;
		typedef typename base::const_iterator base_const_iterator;
#else
		typedef pair< _Key, _Tp > pair_type;
		typedef hash_map< _Key, _Tp, _HashFcn, _EqualKey, _Alloc > base;
		typedef typename base::iterator base_iterator;
		typedef typename base::const_iterator base_const_iterator;
#endif

		/// Returns true if the map contains the given key
		bool contains( const _Key& key ) const
		{
			return find(key) != end();
		}

		/// safe get. returns _Tp() if not found.
		_Tp get( const _Key& key ) const
		{
			base_const_iterator it = base::find( key );
			if ( it == end() ) return _Tp();
			return it->second;
		}

		/// Neat insert.
		pair< base_iterator, bool > insert( const _Key& key, const _Tp& val )
		{
			return base::insert( pair_type(key, val) );
		}

		/// Recreate destroyed overriden version.
		pair< base_iterator, bool > insert( const pair_type& val )
		{
			return base::insert( val );
		} 

		/// Realistic erase
		void erase( const _Key& key )
		{
			base_iterator it = base::find(key);
			if ( it != end() )
				base::erase( it );
		}

	}; 



}


*/
#endif
