#ifndef OHASHCOMMON_H
#define OHASHCOMMON_H

#define OHASH_DEFINED (1)

//#include <sys/types.h>
//#include <wchar.h>
//#include <string>
//#include <utility>
#include <stdint.h>

namespace ohash
{

typedef int32_t		hashkey_t;		// Key type of hash tables. Should probably be uint32_t (we cast to that anyway)
typedef uint32_t	hashsize_t;		// Size type of hash tables
typedef uint8_t		hashstate_t;	// State array type

static const hashsize_t npos = (hashsize_t) -1;

inline hashsize_t next_power_of_2(hashsize_t v)
{
	hashsize_t s = 1;
	while (s < v) s <<= 1;
	return s;
}

enum KeyState
{
	// can't have more than 4... only 2 bits available
	SNull = 0,
	SFull = 1,
	SDeleted = 2,
	SERROR = 3
};

inline hashsize_t state_array_size(hashsize_t asize)
{
	// every item needs 2 bits, so that's 16 items per int32
	return (asize / 4) + 8;
}

// returns the state of a given position in the table
inline KeyState get_state(hashstate_t stateArray[], hashsize_t pos)
{
	size_t        bytepos = pos / 4;
	unsigned char bitpos = pos & 3;
	unsigned char masks[4] = { 3, 12, 48, 192 };
	KeyState ks = (KeyState)((stateArray[ bytepos ] & masks[bitpos]) >> (bitpos * 2));
	return ks;
}

inline void set_state(hashstate_t stateArray[], hashsize_t pos, KeyState newState)
{
	size_t        bytepos = pos / 4;
	unsigned char bitpos = pos & 3;
	unsigned char masks[4] = { (unsigned char) ~3u, (unsigned char) ~12u, (unsigned char) ~48u, (unsigned char) ~192u };
	unsigned char state = stateArray[ bytepos ] & masks[bitpos];
	state |= newState << (bitpos * 2);
	stateArray[ bytepos ] = state;
}

// Very simple mix function that at least gives us better behaviour when the only entropy is higher than our mask.
// This simplistic function probably causes evil behaviour in certain pathological cases, but it's better than not having it at all.
// This mixing solves cases such as values of the form 0x03000000, 0x04000000, 0x05000000. Without this folding
// function, those keys would all end up with the same table position, unless the table was larger than 0x0fffffff.
inline hashkey_t fold(hashkey_t k)
{
	uint32 u = k;
	u = u ^ (u >> 16);
	u = u ^ (u >> 8);
	return u;
}

/// The hash function (optimization of generic table_pos with i = 0)
inline hashsize_t table_pos(hashsize_t mask, hashkey_t key)
{
	return (hashsize_t)(fold(key) & mask);
}

/// probe (when i = 0, this function must be identical to table_pos(key))
inline hashsize_t table_pos(hashsize_t mask, hashsize_t probeOffset, hashkey_t key, uint i)
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

	return (hashsize_t)((key + i * mul) & mask);
}

inline hashsize_t probeoffset(hashsize_t size)
{
	return size >> 1;
}

inline hashkey_t hashpointer(const void* p)
{
	size_t v = (size_t) p;
#ifdef ARCH_64
	return (hashkey_t)(v ^ (v >> 32));
#else
	return (hashkey_t)(v);
#endif
}

// Key == data. ie. hash set.
template< class TKey, class TData >
class getkey_self
{
public:
	static const TKey&	getkey(const TData& data)						{ return data; }
	static bool			equals(const TKey& a, const TKey& b)			{ return a == b; }
};

template<class TKey, class TVal>
class getkey_pair
{
public:
	static const TKey&	getkey(const std::pair<TKey, TVal>& data)		{ return data.first; }
	static bool			equals(const TKey& a, const TKey& b)			{ return a == b; }
};

// Key == value. ie. hash set
template< class TKey, class TData >
class getval_self
{
public:
	static const TKey&	getval(const TData& data)		{ return data; }
	static TKey&		getval_mutable(TData& data)		{ return data; }
};

template< class TKey, class TVal >
class getval_pair
{
public:
	static const TVal&	getval(const std::pair< TKey, TVal >& data)		{ return data.second; }
	static TVal&		getval_mutable(std::pair< TKey, TVal >& data)	{ return data.second; }
};

template<typename TCH>
hashkey_t stringhash_sdbm_nullterm(const TCH* str)
{
	hashkey_t key = 0;
	for (const TCH* s = str; *s; s++)
	{
		// hash(i) = hash(i - 1) * 65539 + str[i] (SDBM)
		key = (unsigned int) *s + (key << 6) + (key << 16) - key;
	}
	return key;
}

template<typename TCH>
hashkey_t stringhash_sdbm(const TCH* str, size_t len)
{
	hashkey_t key = 0;
	for (size_t i = 0; i < len; i++)
	{
		// hash(i) = hash(i - 1) * 65539 + str[i] (SDBM)
		key = (unsigned int) str[i] + (key << 6) + (key << 16) - key;
	}
	return key;
}

// For pointers, we are basically the same as a size_t hash, mixing the upper and lower bits of a 64-bit pointer,
// or using the raw value of a 32-bit pointer on 32-bit architectures.
inline hashkey_t gethashcode(void* const& k)		{ return hashpointer(k); }

inline hashkey_t gethashcode(const bool& k)			{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const char& k)			{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const wchar_t& k)		{ return (hashkey_t) k; }

inline hashkey_t gethashcode(const int8_t& k)		{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const int16_t& k)		{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const int32_t& k)		{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const int64_t& k)		{ return (hashkey_t) ((uint32_t) k ^ (uint32_t) ((uint64_t) k >> 32)); }
inline hashkey_t gethashcode(const uint8_t& k)		{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const uint16_t& k)		{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const uint32_t& k)		{ return (hashkey_t) k; }
inline hashkey_t gethashcode(const uint64_t& k)		{ return (hashkey_t) ((uint32_t) k ^ (uint32_t) (k >> 32)); }

// I make a copy here in an attempt to prevent the compiler from thinking that it actually needs
// to send this parameter by reference. In other words, I avoid using a pointer to k.
inline hashkey_t gethashcode(const float& k)		{ float copy = k; return *((int32*) &copy); }
inline hashkey_t gethashcode(const double& k)		{ double copy = k; int32* p = (int32*) &copy; return p[0] ^ p[1]; }

inline hashkey_t gethashcode(const char* const& k)	{ return stringhash_sdbm_nullterm(k); }
inline hashkey_t gethashcode(const std::string& k)	{ return stringhash_sdbm(k.c_str(), k.length()); }
inline hashkey_t gethashcode(const std::wstring& k)	{ return stringhash_sdbm(k.c_str(), k.length()); }

// Provides a default class that calls ohash::gethashcode(key)
// You must provide a specialization of ohash::gethashcode(key) if the default implementation is not suitable
template<typename key_t>
class func_default
{
public:
	static hashkey_t gethashcode(const key_t& key)
	{
		return (hashkey_t) ohash::gethashcode(key);
	}
};

// Computes a hash on the pointer
template<typename T>
class func_ptr
{
public:
	static hashkey_t gethashcode(const T& key)
	{
		return (hashkey_t) ohash::hashpointer(key);
	}
};

// Calls key.GetHashCode()
template<typename T>
class func_GetHashCode
{
public:
	static hashkey_t gethashcode(const T& key)
	{
		return (hashkey_t) key.GetHashCode();
	}
};

// Set of interfaces for using const char* to store strings
template<class TVal>
class getkey_pair_pchar
{
public:
	typedef const char*				TKey;
	typedef std::pair<TKey, TVal>	TPair;

	static const TKey&	getkey(const TPair& data)					{ return data.first; }
	static bool			equals(const TKey& a, const TKey& b)		{ return strcmp(a, b) == 0; }
};

class getkey_pchar
{
public:
	typedef const char*				TKey;

	static const TKey&	getkey(const TKey& data)					{ return data; }
	static bool			equals(const TKey& a, const TKey& b)		{ return strcmp(a, b) == 0; }
};

}

#endif

