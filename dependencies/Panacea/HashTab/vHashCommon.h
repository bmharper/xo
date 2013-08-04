#ifndef VHASHCOMMON_H
#define VHASHCOMMON_H

#include "../Other/lmDefs.h"

#include <sys/types.h>
#include <wchar.h>
#include <string>

#include "../Strings/XString.h"
#include "../Containers/dvec.h"

/// vHashTables- Simpler, faster hash tables.
namespace vHashTables 
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

	/// Base class for all vHashTables exceptions
	class vHashException 
	{
	};

	/// Exception thrown when the hash table can't grow big enough or a similar issue. Analogous to Out Of Memory.
	class vHashSizeException : public vHashException
	{
	};

	/// Exception thrown when an item is not found, and it is likely going to cause unsuitable behaviour (such as nullptr dereference)
	class vHashNotFoundException : public vHashException
	{
	};
	///@}

	/// Returns us a prime number that is a bit higher or equal to the requested value.
	/**
	Throws vHashSizeException is the requested size is over 1GB.
	**/
	inline hashkey_t NextPrime( hashkey_t v )
	{
		if ( v > 969769039 ) throw vHashSizeException();

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
		throw vHashSizeException();
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

	/**
	\defgroup Providers Hash Function Providers
	**/
	//@{

	/// Provides a default function that casts the object to type hashkey_t
	template< class key_t >
	class vHashFunction_Cast
	{
	public:
		static hashkey_t GetHashCode( const key_t& elem )
		{
			//return elem.GetHashCode();
			return (hashkey_t) elem;
		}
	};

	/// Provides a default function that calls GetHashCode on the object
	template< class key_t >
	class vHashFunction_GetHashCode
	{
	public:
		static hashkey_t GetHashCode( const key_t& elem )
		{
			return elem.GetHashCode();
		}
	};

	/// Provides a hash function for 64-bit integers that XOR's the upper and lower halves
	class vHashFunction_INT64
	{
	public:
		static hashkey_t GetHashCode( const INT64& elem )
		{
			hashkey_t high1 = (hashkey_t) (elem >> 16);
			hashkey_t high2 = (hashkey_t) (elem >> 48);
			return (hashkey_t) ( (elem ^ high1) ^ high2 );
		}
	};

	/// Provides a hash function for pointers
	class vHashFunction_VoidPtr
	{
	public:
		static hashkey_t GetHashCode( const void* elem )
		{
			size_t v = (size_t) elem;
#ifdef _WIN64
			return (hashkey_t) ( v ^ (v >> 32) );
#else
			return (hashkey_t) ( v );
#endif

		}
	};

	/// Provides a hash function for std::string.
	template < class t_string >
	class vHashFunction_stringT
	{
	public:
		static hashkey_t GetHashCode( const t_string& str )
		{
			hashkey_t key = 0;
			for (size_t i = 0; i < str.length(); i++)
			{
				key = 5 * key + str[i];
			}
			return key;
		}
	};

	typedef vHashFunction_stringT< std::string >  vHashFunction_string;
	typedef vHashFunction_stringT< std::wstring >  vHashFunction_wstring;

	/// Provides a hash function for XString
	template < class t_string >
	class vHashFunction_XString
	{
	public:
		static hashkey_t GetHashCode( const t_string& str )
		{
			return str.GetHashCode();
		}
	};

	typedef vHashFunction_XString< XStringA >  vHashFunction_XStringA;
	typedef vHashFunction_XString< XStringW >  vHashFunction_XStringW;

	/// Provides a hash function for CString.
	template < class t_string >
	class vHashFunction_CStringT
	{
	public:
		static hashkey_t GetHashCode( const t_string& str )
		{
			hashkey_t key = 0;
			int len = str.GetLength();
			for (int i = 0; i < len; i++)
			{
				key = 5 * key + str.GetAt(i);
			}
			return key;
		}
	};

#ifdef _AFX
	typedef vHashFunction_CStringT< CStringA >  vHashFunction_CStringA;
	typedef vHashFunction_CStringT< CStringW >  vHashFunction_CStringW;
#endif


	//@}
	// end group hash function providers


};

#endif

