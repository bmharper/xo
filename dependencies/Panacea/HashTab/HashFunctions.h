#pragma once

#ifndef PANACEA_HASHFUNCTIONS_DEFINED
#define PANACEA_HASHFUNCTIONS_DEFINED

// This is by no means a good hash function. Especially for power-of-2 tables, it isn't hard to imagine really bad collisions.
// It might be worth it replacing this with some more sensible functions, such as Knuth's magic number multiply, etc.
#ifdef _M_X64
#define POINTERHASH( p ) ((uint32) p ^ (uint32)((size_t)p>>32))
#else
#define POINTERHASH( p ) ((uint32)p)
#endif


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Paul Hsieh's SuperFastHash function
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

//#include "pstdint.h" /* Replace with <stdint.h> if appropriate */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
	|| defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
	+(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

inline uint32_t SuperFastHash (const char * data, int len) {
	uint32_t hash = len, tmp;
	int rem;

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
				case 3: hash += get16bits (data);
					hash ^= hash << 16;
					hash ^= data[sizeof (uint16_t)] << 18;
					hash += hash >> 11;
					break;
				case 2: hash += get16bits (data);
					hash ^= hash << 11;
					hash += hash >> 17;
					break;
				case 1: hash += *data;
					hash ^= hash << 10;
					hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

#undef get16bits



//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

inline unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m; 
		k ^= k >> r; 
		k *= m; 

		h *= m; 
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array

	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 

/// Seed value = 0
inline unsigned int MurmurHash2 ( const void * key, int len )
{
	return MurmurHash2( key, len, 0 );
}




//-----------------------------------------------------------------------------
// MurmurHash2A, by Austin Appleby

// This is a variant of MurmurHash2 modified to use the Merkle-Damgard
// construction. Bulk speed should be identical to Murmur2, small-key speed
// will be 10%-20% slower due to the added overhead at the end of the hash.

// This variant fixes a minor issue where null keys were more likely to
// collide with each other than expected, and also makes the algorithm
// more amenable to incremental implementations. All other caveats from
// MurmurHash2 still apply.

#define mmix(h,k) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

inline unsigned int MurmurHash2A ( const void * key, int len, unsigned int seed )
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int l = len;

	const unsigned char * data = (const unsigned char *)key;

	unsigned int h = seed;

	while(len >= 4)
	{
		unsigned int k = *(unsigned int*)data;

		mmix(h,k);

		data += 4;
		len -= 4;
	}

	unsigned int t = 0;

	switch(len)
	{
	case 3: t ^= data[2] << 16;
	case 2: t ^= data[1] << 8;
	case 1: t ^= data[0];
	};

	mmix(h,t);
	mmix(h,l);

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

//-----------------------------------------------------------------------------
// CMurmurHash2A, by Austin Appleby

// This is a sample implementation of MurmurHash2A designed to work
// incrementally.

// Usage -

// CMurmurHash2A hasher
// hasher.Begin(seed);
// hasher.Add(data1,size1);
// hasher.Add(data2,size2);
// ...
// hasher.Add(dataN,sizeN);
// unsigned int hash = hasher.End()

class PAPI CMurmurHash2A
{
public:

	void Begin ( unsigned int seed = 0 )
	{
		m_hash  = seed;
		m_tail  = 0;
		m_count = 0;
		m_size  = 0;
	}

	void Add ( const unsigned char * data, int len )
	{
		m_size += len;

		MixTail(data,len);

		while(len >= 4)
		{
			unsigned int k = *(unsigned int*)data;

			mmix(m_hash,k);

			data += 4;
			len -= 4;
		}

		MixTail(data,len);
	}

	unsigned int End ( void )
	{
		mmix(m_hash,m_tail);
		mmix(m_hash,m_size);

		m_hash ^= m_hash >> 13;
		m_hash *= m;
		m_hash ^= m_hash >> 15;

		return m_hash;
	}

private:

	static const unsigned int m = 0x5bd1e995;
	static const int r = 24;

	void MixTail ( const unsigned char * & data, int & len )
	{
		while( len && ((len<4) || m_count) )
		{
			m_tail |= (*data++) << (m_count * 8);

			m_count++;
			len--;

			if(m_count == 4)
			{
				mmix(m_hash,m_tail);
				m_tail = 0;
				m_count = 0;
			}
		}
	}

	unsigned int m_hash;
	unsigned int m_tail;
	unsigned int m_count;
	unsigned int m_size;
};




inline int GetHashCode_SuperFashHash( const void* data, size_t bytes )	{ return SuperFastHash((const char*) data, (int) bytes); }
inline int GetHashCode_Murmur2( const void* data, size_t bytes )		{ return MurmurHash2(data, (int) bytes); }

inline int GetHashCode_djb2( const void* data, size_t bytes )
{
	// SYNC-DJB2
	// This was copied from http://www.cs.yorku.ca/~oz/hash.html
	// Two names mentioned on that page are djb2 and sdbm
	// This is djb2.
	unsigned int hash = 5381;
	const BYTE* p = (const BYTE*) data;
	for ( size_t i = 0; i < bytes; i++ )
	{
		hash = ((hash << 5) + hash) + (unsigned int) p[i]; // hash * 33 + c
		i++;
	}
	return (int) hash;
}

inline int GetHashCode_sdbm( const void* data, size_t bytes )
{
	// SYNC-SDBM
	// This was copied from http://www.cs.yorku.ca/~oz/hash.html
	// Two names mentioned on that page are djb2 and sdbm
	// This is sdbm.
	unsigned int hash = 0;
	const BYTE* p = (const BYTE*) data;
	for ( size_t i = 0; i < bytes; i++ )
	{
		// hash(i) = hash(i - 1) * 65539 + str[i]
		hash = (unsigned int) p[i] + (hash << 6) + (hash << 16) - hash;
		i++;
	}
	return (int) hash;
}



class IAppendableHash
{
public:
	virtual void	Reset( u32 seed ) = 0;
	virtual void	Append( const void* input, size_t bytes ) = 0;
	virtual void	Finish( void* output ) = 0;
	virtual u32		DigestBytes() = 0;
};

class INonAppendableHash
{
public:
	virtual void	Compute( const void* input, size_t bytes, u32 seed, void* output ) = 0;
	virtual u32		DigestBytes() = 0;
};

// Wrapper that buffers data and feeds it to the hash function all at once, at the end
template<typename TNonAppend>
class NonAppendableHashWrapper : public IAppendableHash
{
public:
	u32					Seed;
	TNonAppend			Hash;
	size_t				Cap;
	size_t				Len;
	unsigned char*		Buf;

	NonAppendableHashWrapper()
	{
		Buf = NULL;
		Len = Cap = 0;
		Seed = 0;
	}
	~NonAppendableHashWrapper() { Reset(0); }

	virtual void	Reset( u32 seed )								{ free(Buf); Buf = NULL; Len = Cap = 0; Seed = seed; }
	virtual void	Append( const void* input, size_t bytes )
	{
		while ( Len + bytes > Cap )
		{
			Cap = Cap << 1;
			Cap = ABC_MAX(Cap,1);
		}
		Buf = (unsigned char*) AbcReallocOrDie(Buf, Cap);
		memcpy(Buf + Len, input, bytes);
		Len += bytes;
	}
	virtual void	Finish( void* output )
	{
		Hash.Compute( Buf, Len, Seed, output );
	}
	virtual u32		DigestBytes() { return Hash.DigestBytes(); }
};

/** A fixed-size signature for use in a hash table.
**/
struct HashSig
{
	size_t Bytes;
	const void* Data;

	HashSig( const void* data = 0, size_t bytes = 0 )
	{
		Data = data;
		Bytes = bytes;
	}

	bool operator==( const HashSig& b ) const
	{
		return memcmp( Data, b.Data, Bytes ) == 0;
	}

	bool operator!=( const HashSig& b ) const { return !(*this == b); }

	int GetHashCode() const
	{
		return GetHashCode_Murmur2( Data, Bytes );
	}
};

#endif
