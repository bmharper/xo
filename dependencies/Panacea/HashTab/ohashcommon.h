#ifndef OHASHCOMMON_H
#define OHASHCOMMON_H

#define OHASH_DEFINED (1)

#include "../Other/lmDefs.h"

#include <sys/types.h>
#include <wchar.h>
#include <string>
#include <utility>

#include "../Containers/dvec.h"
#include "../Strings/XString.h"

namespace ohash
{
	/// Key type of hash tables
	typedef int hashkey_t;

	/// Size type of hash tables
	typedef unsigned int hashsize_t;

	/// State array type
	typedef unsigned char hashstate_t;

#ifdef _MSC_VER
#if _MSC_VER > 1200
	static const hashsize_t npos = -1;
#else
	enum 
	{
		npos = -1
	};
#endif
#else
	static const hashsize_t npos = (hashsize_t) -1;
#endif

	/**
	\defgroup Exceptions Exceptions
	**/
	//@{

	/* -- Exceptions are not free --
	/// Base class for all vHashTables exceptions
	class ohash_exception
	{
	};

	/// Exception thrown when the hash table can't grow big enough or a similar issue. Analogous to Out Of Memory.
	class ohash_size_exception : public ohash_exception
	{
	};

	/// Exception thrown when an item is not found, and it is likely going to cause unsuitable behaviour (such as nullptr dereference)
	class ohash_notfound_exception: public ohash_exception
	{
	};
	*/
	///@}

	inline hashsize_t NextPowerOf2( hashsize_t v )
	{
		hashsize_t s = 1;
		while ( s < v ) s <<= 1;
		return s;
	}

	/// Returns us a prime number that is a bit higher or equal to the requested value.
	/**
	Throws vHashSizeException is the requested size is over 1GB.
	**/
	inline hashkey_t NextPrime( hashkey_t v )
	{
		// Assume errors never happen
		//if ( v > 969769039 ) throw ohash_size_exception();

		// larger, psuedo power-2 table. not power-2 because values get denser as they get higher. 
		// the premise is that the difference between 200 mb and 400 mb is much more
		// significant than the difference between 200 and 400 bytes, so we try to be a bit
		// more accurate as we get higher.
		hashkey_t primes[] = 
		{
			17,
			37,
			67,
			131,
			257,
			521,
			1049,
			2099,
			4217,
			8501,
			17047,
			34259,
			68767,
			137567,
			273601,
			537793,
			1034003,
			1910339,
			3308027,
			5239061,
			7525307,
			9942311,
			12374809,
			14812727,
			17294111,
			19872169,
			22603543,
			25545589,
			28755983,
			32293091,
			36216457,
			40587223,
			45469387,
			50930729,
			57044279,
			63890261,
			71557231,
			80144143,
			89761471,
			100532827,
			112596761,
			126108401,
			141241393,
			158190379,
			177173189,
			198433969,
			222246041,
			248915573,
			278785471,
			312239689,
			349708453,
			391673473,
			438674267,
			491315179,
			550273001,
			616305757,
			690262451,
			773093957,
			865865237,
			969769039,
			0,0
		};


		/*
		Smaller, original power-2 table.
		hashkey_t primes[] = 
		{
			17,
			37,
			67,
			131,
			257,
			521,
			1031,
			2053,
			4099,
			8209,
			16411,
			32771,
			65537,
			131101,
			262147,
			524309,
			1048583,
			2097169,
			4194319,
			8388617,
			16777259,
			33554467,
			67108879,
			134217757,
			268435459,
			536870923,
			1073741827,
			0,0
		};*/
		for (int i = 0; primes[i] != 0; i++)
			if ( primes[i] >= v ) return primes[i];
		//throw ohash_size_exception();
		return 1073741827;
	}

	enum KeyState 
	{
		// can't have more than 4... only 2 bits available
		SNull = 0,
		SFull = 1,
		SDeleted = 2,
		SERROR = 3
	};

	inline hashsize_t stateArraySize( hashsize_t asize )
	{
		// every item needs 2 bits, so that's 16 items per int32
		//return (asize+32) / 16;
		//return asize;
		return (asize / 4) + 8;
	}

	// returns the state of a given position in the table
	inline KeyState getState( hashstate_t stateArray[], hashsize_t pos ) 
	{
		/*UINT indA = pos / 16;
		UINT indB = (pos % 16) << 1;
		UINT NindB = 31 - indB; // negate indB
		UINT mask = (0x80000000 >> indB) | (0x80000000 >> (indB+1));
		UINT state = (stateArray[indA] & mask) >> (NindB - 1);
		return (KeyState) state; */
		//return (KeyState) stateArray[ pos ];
		// x % 4 = x & 3;
		size_t        bytepos = pos / 4;
		unsigned char bitpos = pos & 3;
		unsigned char masks[4] = { 3, 12, 48, 192 };
		KeyState ks = (KeyState) ( (stateArray[ bytepos ] & masks[bitpos]) >> (bitpos * 2) );
		return ks;
	}

	inline void setState( hashstate_t stateArray[], hashsize_t pos, KeyState newState ) 
	{
		/*UINT indA = pos / 16;
		UINT indB = (pos % 16) << 1;
		UINT NindB = 31 - indB; // negate indB
		UINT mask = (0x80000000 >> indB) | (0x80000000 >> (indB+1));
		mask = ~mask; // invert mask.. to blank our previous state
		UINT currentstate = stateArray[indA] & mask;
		UINT merge = newState << (NindB - 1);
		stateArray[indA] = merge | currentstate; */
		//stateArray[ pos ] = newState;
		size_t        bytepos = pos / 4;
		unsigned char bitpos = pos & 3;
		unsigned char masks[4] = { (unsigned char) ~3u, (unsigned char) ~12u, (unsigned char) ~48u, (unsigned char) ~192u };
		unsigned char state = stateArray[ bytepos ] & masks[bitpos];
		state |= newState << (bitpos * 2);
		stateArray[ bytepos ] = state;
	}

#ifdef OHASH_PRIME_SIZE
	/// The hash function
	inline hashsize_t table_pos( hashsize_t size, hashkey_t key ) const
	{
		return (hashsize_t) (key % size);
	}

	/// probe
	inline hashsize_t table_pos( hashsize_t size, hashkey_t key, uint i ) const
	{
		return (hashsize_t) ( ( (key % size) + i * (1 + (key % (size - 1))) ) % size );
	}
#else

	// Very simple mix function that at least gives us better behaviour when the only entropy is higher than our mask.
	// This simplistic function probably causes evil behaviour in certain pathological cases, but it's better than not having it at all.
	// This mixing solves cases such as values of the form 0x03000000, 0x04000000, 0x05000000. Without this folding
	// function, those keys would all end up with the same table position, unless the table was larger than 0x0fffffff.
	inline hashkey_t fold( hashkey_t k )
	{
		uint32 u = k;
		u = u ^ (u >> 16);
		u = u ^ (u >> 8);
		return u;
	}

	/// The hash function (optimization of generic table_pos with i = 0)
	inline hashsize_t table_pos( hashsize_t mask, hashkey_t key )
	{
		return (hashsize_t) (fold(key) & mask);
	}

	/// probe (when i = 0, this function must be identical to table_pos(key))
	inline hashsize_t table_pos( hashsize_t mask, hashsize_t probeOffset, hashkey_t key, uint i )
	{
		key = fold(key);
		uint mul = key >> 1;
		mul |= 1; // ensure multiplier is odd
		
		/*
		// This offset particularly helps speed up failed lookups when our table is a dense array of integers (consecutive hashes).
		// The layout in that case is that half of the table is completely occupied, and the other half (remember our fill factor -- there is always an 'other half')
		// is completely empty. This probe offset immediately sends us into the territory of the 'other half', thereby reducing the time that we spend walking through
		// the populated half.

		// The difference here seems to be negligible, but the simpler version should be better for future compilers.
		uint offset = i == 0 ? 0 : probeOffset;
		//uint offset = ~(int(i - 1) >> 31) & mProbeOffset; // branch-less version
		return (hashsize_t) ((key + (offset + i) * mul) & mask);
		*/

		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		// That 'offset' idea is broken. It breaks the strict visitation rule (ie that the probes must
		// visit every slot exactly once)
		/////////////////////////////////////////////////////////////////////////////////////////////////////////

		return (hashsize_t) ((key + i * mul) & mask);
	}
#endif

	inline hashsize_t probeoffset( hashsize_t size )
	{
		return size >> 1;
	}

	// Key == data. ie. hash set.
	template< class TKey, class TData >
	class ohashgetkey_self
	{
	public:
		static const TKey&	getkey( const TData& data )		{ return data; }
		static TKey&		getkey_noconst( TData& data )	{ return data; }
	};

	template< class TKey, class TVal >
	class ohashgetkey_pair
	{
	public:
		static const TKey&	getkey( const std::pair< TKey, TVal >& data )	{ return data.first; }
		static TKey&		getkey_noconst( std::pair< TKey, TVal >& data )	{ return data.first; }
	};

	// Key == value. ie. hash set
	template< class TKey, class TData >
	class ohashgetval_self
	{
	public:
		static const TKey&	getval( const TData& data )		{ return data; }
		static TKey&		getval_noconst( TData& data )	{ return data; }
	};

	template< class TKey, class TVal >
	class ohashgetval_pair
	{
	public:
		static const TVal&	getval( const std::pair< TKey, TVal >& data )	{ return data.second; }
		static TVal&		getval_noconst( std::pair< TKey, TVal >& data ) { return data.second; }
	};

	/**
	\defgroup Providers Hash Function Providers
	**/
	//@{

	/// Provides a default function that casts the object to type hashkey_t
	template< class key_t >
	class ohashfunc_cast
	{
	public:
		static hashkey_t gethashcode( const key_t& elem )
		{
			//return elem.GetHashCode();
			return (hashkey_t) elem;
		}
	};

	/// Provides a default function that casts the object to type hashkey_t
	template< class key_t >
	class ohashfunc_voidptr
	{
	public:
		static hashkey_t gethashcode( const key_t& elem )
		{
			size_t v = (size_t) elem;
#ifdef _M_X64
			return (hashkey_t) ( v ^ (v >> 32) );
#else
			return (hashkey_t) ( v );
#endif

		}
	};

	/// Provides a default function that calls GetHashCode on the object
	template< class key_t >
	class ohashfunc_GetHashCode
	{
	public:
		static hashkey_t gethashcode( const key_t& elem )
		{
			return elem.GetHashCode();
		}
	};

	/// Provides a hash function for 64-bit integers that XOR's the upper and lower halves
	class ohashfunc_INT64
	{
	public:
		static hashkey_t gethashcode( const INT64& elem )
		{
			return (hashkey_t) ( (u32) elem ^ (u32) (elem >> 32) );
		}
	};

	class ohashfunc_UINT64
	{
	public:
		static hashkey_t gethashcode( const UINT64& elem )
		{
			return (hashkey_t) ( (u32) elem ^ (u32) (elem >> 32) );
		}
	};

	/// Provides a hash function for std::string.
	template < class t_string >
	class ohashfunc_stringT
	{
	public:
		static hashkey_t gethashcode( const t_string& str )
		{
			hashkey_t key = 0;
			for (size_t i = 0; i < str.length(); i++)
			{
				key = 5 * key + str[i];
			}
			return key;
		}
	};

	/// Provides a hash function for CStringT
	template < class t_string >
	class ohashfunc_CStringT
	{
	public:
		static hashkey_t gethashcode( const t_string& str )
		{
			hashkey_t key = 0;
			int len = str.GetLength();
			for (size_t i = 0; i < len; i++)
			{
				key = 5 * key + str.GetAt(i);
			}
			return key;
		}
	};

	typedef ohashfunc_stringT< std::string >  ohashfunc_string;
	typedef ohashfunc_stringT< std::wstring >  ohashfunc_wstring;

	/// Provides a hash function for XString
	template < class t_string >
	class ohashfunc_XString
	{
	public:
		static hashkey_t gethashcode( const t_string& str )
		{
			hashkey_t key = 0;
			for (size_t i = 0; i < (size_t) str.Length(); i++)
			{
				key = 5 * key + str.Get( (int) i );
			}
			return key;
		}
	};

	//@}
	// end group hash function providers



};

#endif

