#pragma once

#include "../HashTab/ohashmap.h"
#include "../HashTab/ohashset.h"

/*

Hybrid integer sets and maps. These are made to look and feel like 64-bit integer
hash tables, must they only kick over to 64 bit if an element inside them requires it.

*/


namespace ohash
{



/** Hybrid map that begins life as an unsigned 32-bit <===> 32-bit map, and
if any insertions of 64-bit items are made, it kicks over into a
64-64 map.
**/
class HybridInt64Map
{
public:
	typedef ohashmap< UINT32, UINT32 > TMap32;
	typedef ohashmap< INT64, INT64, ohashfunc_INT64 > TMap64;

	HybridInt64Map()
	{
		Is32 = true;
	}

	void clear()
	{
		Is32 = true;
		Map32.clear();
		Map64.clear();
	}

	
	hashsize_t size() const
	{
		if ( Is32 ) return Map32.size();
		else		return Map64.size();
	}

	void resize( hashsize_t newsize )
	{
		if ( Is32 ) Map32.resize( newsize );
		else		Map64.resize( newsize );
	}

	bool insert( INT64 key, INT64 val, bool overwrite = false )
	{
		check( key );
		check( val );
		if ( Is32 ) return Map32.insert( key, val, overwrite );
		else		return Map64.insert( key, val, overwrite );
	}

	bool erase( INT64 key )
	{
		if ( Is32 ) return Map32.erase( key );
		else		return Map64.erase( key );
	}

	bool contains( INT64 key ) const
	{
		if ( Is32 ) return Map32.contains( key );
		else		return Map64.contains( key );
	}

	INT64 get( INT64 key ) const
	{
		if ( Is32 ) return Map32.get( key );
		else		return Map64.get( key );
	}

	INT64 operator[]( INT64 key ) const
	{
		if ( Is32 ) return Map32.get( key );
		else		return Map64.get( key );
	}

	class iterator
	{
	public:
		iterator( const TMap32::iterator& it )
		{
			is32 = true;
			i32 = it;
		}

		iterator( const TMap64::iterator& it )
		{
			is32 = false;
			i64 = it;
		}

		iterator& operator++(int)
		{
			if ( is32 ) i32++;
			else		i64++;
			return *this;
		}

		iterator& operator--(int)
		{
			if ( is32 ) i32--;
			else		i64--;
			return *this;
		}

		bool operator==( const iterator& b )
		{
			if ( is32 ) return i32 == b.i32;
			else		return i64 == b.i64;
		}

		bool operator!=( const iterator& b )
		{
			if ( is32 ) return i32 != b.i32;
			else		return i64 != b.i64;
		}

		INT64 first() const 
		{
			if ( is32 ) return i32->first;
			else		return i64->first;
		}

		INT64 second() const 
		{
			if ( is32 ) return i32->second;
			else		return i64->second;
		}

	protected:
		bool is32;
		TMap32::iterator i32;
		TMap64::iterator i64;
	};

	HybridInt64Map::iterator begin() const
	{
		if ( Is32 )	return iterator( Map32.begin() );
		else		return iterator( Map64.begin() );
	}

	HybridInt64Map::iterator end() const
	{
		if ( Is32 )	return iterator( Map32.end() );
		else		return iterator( Map64.end() );
	}

	friend class HybridInt64Map::iterator;

protected:

	bool Is32;
	TMap32 Map32;
	TMap64 Map64;

	void check( INT64 v )
	{
		if ( !Is32 ) return;
		if ( (v & 0xFFFFFFFF00000000) != 0 )
		{
			Map64.resize( Map32.size() );
			for ( TMap32::iterator it = Map32.begin(); it != Map32.end(); it++ )
			{
				Map64.insert( it->first, it->second );
			}
			Map32.clear();
			Is32 = false;
		}
	}
};




/** Hybrid map that begins life as an unsigned 32-bit <===> 32-bit map, and
will automatically switch to 32-64, 64-32, or 64-64, when necessary.

Note that the return values after the switches are only there to prevent compiler warnings.
**/
class HybridIntXXMap
{
public:
	enum Modes
	{
		Mode32_32 = 0,
		Mode32_64 = 1,
		Mode64_32 = 2,
		Mode64_64 = 3
	};

	typedef ohashmap< UINT32, UINT32 >					TMap32_32;
	typedef ohashmap< UINT32, INT64 >					TMap32_64;
	typedef ohashmap< INT64, INT32, ohashfunc_INT64 > TMap64_32;
	typedef ohashmap< INT64, INT64, ohashfunc_INT64 > TMap64_64;

	HybridIntXXMap()
	{
		Mode = Mode32_32;
	}

	void clear()
	{
		Mode = Mode32_32;
		Map32_32.clear();
		Map32_64.clear();
		Map64_32.clear();
		Map64_64.clear();
	}

	/// Retrieve current mode. This is more for debugging and testing than anything else.
	Modes mode() const { return Mode; }
	
	hashsize_t size() const
	{
		switch ( Mode )
		{
		case Mode32_32: return Map32_32.size();
		case Mode32_64: return Map32_64.size();
		case Mode64_32: return Map64_32.size();
		case Mode64_64: return Map64_64.size();
		}
		return 0;
	}

	void resize( hashsize_t newsize )
	{
		switch ( Mode )
		{
		case Mode32_32: Map32_32.resize( newsize ); break;
		case Mode32_64: Map32_64.resize( newsize ); break;
		case Mode64_32: Map64_32.resize( newsize ); break;
		case Mode64_64: Map64_64.resize( newsize ); break;
		}
	}

	bool insert( INT64 key, INT64 val, bool overwrite = false )
	{
		check( key, val );
		switch ( Mode )
		{
		case Mode32_32: return Map32_32.insert( key, val, overwrite );
		case Mode32_64: return Map32_64.insert( key, val, overwrite );
		case Mode64_32: return Map64_32.insert( key, val, overwrite );
		case Mode64_64: return Map64_64.insert( key, val, overwrite );
		} 
		return false;
	}

	bool erase( INT64 key )
	{
		switch ( Mode )
		{
		case Mode32_32: return Map32_32.erase( key );
		case Mode32_64: return Map32_64.erase( key );
		case Mode64_32: return Map64_32.erase( key );
		case Mode64_64: return Map64_64.erase( key );
		}
		return false;
	}

	bool contains( INT64 key ) const
	{
		switch ( Mode )
		{
		case Mode32_32: return Map32_32.contains( key );
		case Mode32_64: return Map32_64.contains( key );
		case Mode64_32: return Map64_32.contains( key );
		case Mode64_64: return Map64_64.contains( key );
		}
		return false;
	}

	INT64 get( INT64 key ) const
	{
		switch ( Mode )
		{
		case Mode32_32: return Map32_32.get( key );
		case Mode32_64: return Map32_64.get( key );
		case Mode64_32: return Map64_32.get( key );
		case Mode64_64: return Map64_64.get( key );
		}
		return false;
	}

	INT64 operator[]( INT64 key ) const
	{
		switch ( Mode )
		{
		case Mode32_32: return Map32_32.get( key );
		case Mode32_64: return Map32_64.get( key );
		case Mode64_32: return Map64_32.get( key );
		case Mode64_64: return Map64_64.get( key );
		}
		return 0;
	}

	class iterator
	{
	public:
		iterator( const TMap32_32::iterator& it )
		{
			mode = Mode32_32;
			i32_32 = it;
		}

		iterator( const TMap32_64::iterator& it )
		{
			mode = Mode32_64;
			i32_64 = it;
		}

		iterator( const TMap64_32::iterator& it )
		{
			mode = Mode64_32;
			i64_32 = it;
		}

		iterator( const TMap64_64::iterator& it )
		{
			mode = Mode64_64;
			i64_64 = it;
		}

		iterator& operator++(int)
		{
			switch ( mode )
			{
			case Mode32_32: i32_32++; break;
			case Mode32_64: i32_64++; break;
			case Mode64_32: i64_32++; break;
			case Mode64_64: i64_64++; break;
			}
			return *this;
		}

		iterator& operator--(int)
		{
			switch ( mode )
			{
			case Mode32_32: i32_32--; break;
			case Mode32_64: i32_64--; break;
			case Mode64_32: i64_32--; break;
			case Mode64_64: i64_64--; break;
			}
			return *this;
		}

		bool operator==( const iterator& b )
		{
			switch ( mode )
			{
			case Mode32_32: return i32_32 == b.i32_32;
			case Mode32_64: return i32_64 == b.i32_64;
			case Mode64_32: return i64_32 == b.i64_32;
			case Mode64_64: return i64_64 == b.i64_64;
			}
			return false;
		}

		bool operator!=( const iterator& b )
		{
			switch ( mode )
			{
			case Mode32_32: return i32_32 != b.i32_32;
			case Mode32_64: return i32_64 != b.i32_64;
			case Mode64_32: return i64_32 != b.i64_32;
			case Mode64_64: return i64_64 != b.i64_64;
			}
			return false;
		}

		INT64 first() const 
		{
			switch ( mode )
			{
			case Mode32_32: return i32_32->first;
			case Mode32_64: return i32_64->first;
			case Mode64_32: return i64_32->first;
			case Mode64_64: return i64_64->first;
			}
			return 0;
		}

		INT64 second() const 
		{
			switch ( mode )
			{
			case Mode32_32: return i32_32->second;
			case Mode32_64: return i32_64->second;
			case Mode64_32: return i64_32->second;
			case Mode64_64: return i64_64->second;
			}
			return 0;
		}

	protected:
		Modes mode;
		TMap32_32::iterator i32_32;
		TMap32_64::iterator i32_64;
		TMap64_32::iterator i64_32;
		TMap64_64::iterator i64_64;
	};

	HybridIntXXMap::iterator begin() const
	{
		switch ( Mode )
		{
		case Mode32_32: return iterator( Map32_32.begin() );
		case Mode32_64: return iterator( Map32_64.begin() );
		case Mode64_32: return iterator( Map64_32.begin() );
		case Mode64_64: return iterator( Map64_64.begin() );
		}
		return iterator( Map32_32.begin() );
	}

	HybridIntXXMap::iterator end() const
	{
		switch ( Mode )
		{
		case Mode32_32: return iterator( Map32_32.end() );
		case Mode32_64: return iterator( Map32_64.end() );
		case Mode64_32: return iterator( Map64_32.end() );
		case Mode64_64: return iterator( Map64_64.end() );
		}
		return iterator( Map32_32.begin() );
	}

	friend class HybridIntXXMap::iterator;

protected:

	Modes Mode;
	TMap32_32 Map32_32;
	TMap32_64 Map32_64;
	TMap64_32 Map64_32;
	TMap64_64 Map64_64;

	void check( INT64 key, INT64 val )
	{
		// We have to handle the following conversions:
		// 32.32 -> 64.64
		// 32.32 -> 64.32
		// 32.32 -> 32.64
		// 32.64 -> 64.64
		// 64.32 -> 64.64
		
		if ( Mode == Mode64_64 ) return;

		bool key64 = (key & 0xFFFFFFFF00000000) != 0;
		bool val64 = (val & 0xFFFFFFFF00000000) != 0;

		if ( Mode == Mode32_32 )
		{
			if ( key64 && val64 )	doswitch( Mode64_64, Map32_32, Map64_64 );
			else if ( key64 )		doswitch( Mode64_32, Map32_32, Map64_32 );
			else if ( val64 )		doswitch( Mode32_64, Map32_32, Map32_64 );
		}
		else if ( Mode == Mode32_64 )
		{
			if ( key64 )			doswitch( Mode64_64, Map32_64, Map64_64 );
		}
		else if ( Mode == Mode64_32 )
		{
			if ( val64 )			doswitch( Mode64_64, Map64_32, Map64_64 );
		}
	}

	template< typename TSrcMap, typename TDstMap > 
	void doswitch( Modes newMode, TSrcMap& src, TDstMap& dst )
	{
		for ( TSrcMap::iterator it = src.begin(); it != src.end(); it++ )
		{
			dst.insert( it->first, it->second );
		}
		src.clear();
		Mode = newMode;
	}

};

/*
template< class TVal >
class HybridInt64Map
{
public:
	typedef ohashmap< INT32, TVal > TMap32;
	typedef ohashmap< INT64, TVal, ohashfunc_INT64 > TMap64;

	class iterator
	{
	public:
		iterator( const TMap32::iterator& it )
		{
			is64 = false;
			i32 = it;
		}

		iterator( const TMap64::iterator& it )
		{
			is64 = true;
			i64 = it;
		}

		iterator& operator++(int)
		{
			if ( is64 ) i64++;
			else		i32++;
			return *this;
		}

		iterator& operator--(int)
		{
			if ( is64 ) i64--;
			else		i32--;
			return *this;
		}

		bool operator==( const iterator& b )
		{
			if ( is64 ) return i64 == b.i64;
			else		return i32 == b.i32;
		}

		bool operator!=( const iterator& b )
		{
			if ( is64 ) return i64 != b.i64;
			else		return i32 != b.i32;
		}

		TVal* operator->() const
		{
			if ( is64 ) return &(*i64);
			else		return &(*i32);
		}

		TVal& operator*() const
		{
			if ( is64 ) return *i64;
			else		return *i32;
		}

	protected:
		bool is64;
		TMap32::iterator i32;
		TMap64::iterator i64;
	};

	friend class HybridInt64Map::iterator;

	void clear()
	{
		M32.clear();
		M64.clear();
		Use64 = false;
	}

	hashsize_t size() const
	{
		return Use64 ? M64.size() : M32.size();
	}

	bool erase( const INT64& key )
	{
		if ( !Use64 && IsBig( key ) ) return false;
		return Use64 ? M64.erase( key ) : M32.erase( key );
	}

	bool contains( const INT64& key ) const
	{
		if ( !Use64 && IsBig( key ) ) return false;

		return Use64 ? M64.contains( key ) : M32.contains( key );
	}

	bool insert( const INT64& key, const TVal& val )
	{
		Switch( key );
		if ( Use64 )	return M64.insert(			key, val );
		else			return M32.insert( (INT32)	key, val );
	}

	TVal get( const INT64& key ) const
	{
		if ( !Use64 && IsBig( key ) ) return TVal();

		if ( Use64 )	return M64.get( key );
		else			return M32.get( key );
	}

	TVal* getp( const TKey& key ) const
	{
		if ( !Use64 && IsBig( key ) ) return NULL;

		if ( Use64 )	return M64.getp( key );
		else			return M32.getp( key );
	}

	const TVal& operator[]( const TKey& key ) const
	{
		if ( Use64 )	return M64[ key ];
		else			return M32[ key ];
	}

	TVal& operator[]( const TKey& key )
	{
		Switch( key );
		if ( Use64 )	return M64[ key ];
		else			return M32[ key ];
	}

	iterator begin() const
	{
		return iterator( Use64 ? M64.begin() : M32.begin() );
	}

	iterator end() const
	{
		return iterator( Use64 ? M64.end() : M32.end() );
	}

	bool using64() const { return Use64; }

protected:

	TMap32 M32;
	TMap64 M64;

	bool Use64;

	bool IsBig( INT64 v )
	{
		return (v & 0xFFFFFFFF00000000) != 0;
	}

	void Switch( INT64 v )
	{
		if ( Use64 ) return;
		if ( (v & 0xFFFFFFFF00000000) != 0 )
		{
			Use64 = true;
			for ( TMap32::iterator it = M32.begin(); it != M32.end(); it++ )
			{
				M64.insert( it->first, it->second );
			}
		}
	}
};
*/

class HybridInt64Set
{
public:

	typedef ohashset< INT32 > TSet32;
	typedef ohashset< INT64, ohashfunc_INT64 > TSet64;

	class iterator
	{
	public:
		
		static iterator new32( TSet32::iterator it )
		{
			iterator me;
			me.is64 = false;
			me.i32 = it;
			return me;
		}

		static iterator new64( TSet64::iterator it )
		{
			iterator me;
			me.is64 = true;
			me.i64 = it;
			return me;
		}

		iterator& operator++(int)
		{
			if ( is64 ) i64++;
			else		i32++;
			return *this;
		}

		iterator& operator--(int)
		{
			if ( is64 ) i64--;
			else		i32--;
			return *this;
		}

		bool operator==( const iterator& b )
		{
			if ( is64 ) return i64 == b.i64;
			else		return i32 == b.i32;
		}

		bool operator!=( const iterator& b )
		{
			if ( is64 ) return i64 != b.i64;
			else		return i32 != b.i32;
		}

		INT64 operator*() const
		{
			if ( is64 ) return *i64;
			else		return *i32;
		}

	protected:
		bool is64;
		TSet32::iterator i32;
		TSet64::iterator i64;
	};

	friend class HybridInt64Set::iterator;

	HybridInt64Set()
	{
		Use64 = false;
	}

	void clear()
	{
		M32.clear();
		M64.clear();
		Use64 = false;
	}

	hashsize_t size() const
	{
		return Use64 ? M64.size() : M32.size();
	}

	bool erase( const INT64& key )
	{
		if ( !Use64 && IsBig( key ) ) return false;
		return Use64 ? M64.erase( key ) : M32.erase( key );
	}

	bool contains( const INT64& key ) const
	{
		if ( !Use64 && IsBig( key ) ) return false;

		return Use64 ? M64.contains( key ) : M32.contains( key );
	}

	bool insert( const INT64& key )
	{
		Switch( key );
		if ( Use64 )	return M64.insert( key );
		else			return M32.insert( (INT32) key );
	}

	HybridInt64Set::iterator begin() const
	{
		return Use64 ? iterator::new64( M64.begin() ) : iterator::new32( M32.begin() );
	}

	HybridInt64Set::iterator end() const
	{
		return Use64 ? iterator::new64( M64.end() ) : iterator::new32( M32.end() );
	}

	bool using64() const { return Use64; }

protected:

	TSet32 M32;
	TSet64 M64;
	bool Use64;

	static bool IsBig( INT64 v )
	{
		return (v & 0xFFFFFFFF00000000) != 0;
	}

	void Switch( INT64 v )
	{
		if ( Use64 ) return;
		if ( IsBig( v ) )
		{
			Use64 = true;
			for ( TSet32::iterator it = M32.begin(); it != M32.end(); it++ )
			{
				//TRACE( "copying %I64d\n", (INT64) *it );
				M64.insert( *it );
			}
			M32.clear();
		}
	}
};

/*
// specialization for INT64-INT64 that does optimization on both sides of the glass.
class HybridInt64Map< INT64 >
{
public:

	bool insert( const INT64& key, const INT64& val )
	{
		Switch( key, val );
		if ( Use64 )	return M64.insert(			key, val );
		else			return M32.insert( (INT32)	key, val );
	}

	TVal get( const INT64& key ) const
	{
		if ( !Use64 && IsBig( key ) ) return TVal();

		if ( Use64 )	return M64.get( key );
		else			return M32.get( key );
	}

	TVal* getp( const TKey& key ) const
	{
		if ( !Use64 && IsBig( key ) ) return NULL;

		if ( Use64 )	return M64.getp( key );
		else			return M32.getp( key );
	}

	const TVal& operator[]( const TKey& key ) const
	{
		if ( Use64 )	return M64[ key ];
		else			return M32[ key ];
	}

	TVal& operator[]( const TKey& key )
	{
		Switch( key );
		if ( Use64 )	return M64[ key ];
		else			return M32[ key ];
	}

protected:

	typedef ohashmap< INT32, INT32 > TMap32;
	typedef ohashmap< INT64, INT64, ohashfunc_INT64 > TMap64;

	TMap32 M32;
	TMap64 M64;

	bool Use64;

	bool IsBig( INT64 v )
	{
		return (v & 0xFFFFFFFF00000000) != 0;
	}

	void Switch( INT64 v, INT64 k )
	{
		if ( Use64 ) return;
		if ( IsBig(v) || IsBig(k) )
		{
			Use64 = true;
			for ( TMap32::iterator it = M32.begin(); it != M32.end(); it++ )
			{
				M64.insert( it->first, it->second );
			}
		}
	}
};
typedef HybridInt64Map< INT32 > HybridInt64IntMap;
typedef HybridInt64Map< INT64 > HybridInt64Int64Map;
typedef HybridInt64Map< void* > HybridInt64PtrMap;
*/


}
