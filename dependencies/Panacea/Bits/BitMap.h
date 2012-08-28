#pragma once

#include "../Other/aligned_malloc.h"

inline int MemCmpBits( const void* a, const void* b, size_t bits )
{
	size_t bytes = bits >> 3;
	size_t remain = bits & 7;
	if ( memcmp( a, b, bytes ) != 0 ) return 1;
	if ( remain != 0 )
	{
		BYTE av = *(((BYTE*) a) + bytes);
		BYTE bv = *(((BYTE*) b) + bytes);
		for ( size_t i = 0; i < remain; i++ )
		{
			if ( (1 & (av >> i)) != (1 & (bv >> i)) )
				return 1;
		}
	}
	return 0;
}

/** A simple Bit Map.
The raw size of the map is always guaranteed to be on 128-bit boundaries,
which allows fast searching for on/off bits, using the raw map.

Usage:
Most functions require a static sized bitmap, ie create a bitmap of a specific size
using Resize(), then use Get() and Set().<br/>
However, the Add() function will intelligently increase the capacity 
of the bitmap if necessary. For this purpose the bitmap holds a CapacityBits field,
which is used to grow the bitmap at a 2x rate, like vectors.
**/
class PAPI BitMap
{
public:

#ifdef _WIN64
	typedef UINT64 TUnit;
#else
	typedef UINT32 TUnit;
#endif

	static const int UnitSize = sizeof(TUnit) * 8;
	static const TUnit UnitFill = -1;

#ifndef MIN
	template<typename TV> static TV MIN(TV a, TV b) { return a < b ? a : b; }
#endif
#ifndef MAX
	template<typename TV> static TV MAX(TV a, TV b) { return a < b ? b : a; }
#endif

	BitMap()
	{
		Construct();
	}
	BitMap( const BitMap& b )
	{
		Construct();
		*this = b;
	}
	~BitMap()
	{
		Clear();
	}

	/** Necessary raw block byte alignment.
	16 might seem wide, but it's in view of SSE optimizations. I think SSE4 has a count bits instruction that operates
	on a 128 bit word.
	WARNING: This is not currently used. If you DO use it, then remove the non-aligned BmpAlloc/BmpFree implementations.
	**/
	static const int Alignment = 16;

	/** Set the buffer to an external source. 
	**/
	void SetExternal( int bits, void* buffer, int allocatedBits )
	{
		ASSERT( allocatedBits >= 8 && allocatedBits % 8 == 0 );
		CanFind = allocatedBits % (Alignment * 8) == 0;
		Clear();
		External = true;
		SizeBytes = bits / 8;
		CapacityBits = bits;
		SizeBits = CapacityBits;
		Map = (BYTE*) buffer;
	}

	BitMap& operator=( const BitMap& b )
	{
		ASSERT( !External && !b.External );
		CapacityBits = b.SizeBits;
		SizeBytes = b.SizeBytes;
		SizeBits = b.SizeBits;
		CanFind = b.CanFind;
		BmpFree( Map );
		Map = (BYTE*) BmpAlloc( SizeBytes );
		memcpy( Map, b.Map, SizeBytes );
		return *this;
	}

	/// Will shrink the bitmap to it's minimum required size (only necessary after using Add()).
	void Shrink()
	{
		if ( External ) { ASSERT(false); return; }
		if ( CapacityBits == SizeBits ) return;
		if ( SizeBits == 0 ) { Clear(); return; }
		Resize( SizeBits );
	}

	/// Size in bits
	int Size() const { return SizeBits; }

	/// Capacity in bits (used for testing)
	int Capacity() const { return CapacityBits; }

	/// Set size to zero and free memory.
	void Clear()
	{
		if ( !External ) BmpFree( Map );
		Map = NULL;
		SizeBits = 0;
		SizeBytes = 0;
		CapacityBits = 0;
		CanFind = true;
		External = false;
	}

	/** Fill a block of bits with a value.
	This will not automatically increase the size. If the specified bit range
	is invalid, then the function takes no action and debug asserts.
	**/
	void Fill( int bitStart, int bitEndInclusive, bool value )
	{
		int bitEnd = bitEndInclusive;
		if ( bitEnd < bitStart - 1 )	{ ASSERT(false); return; }
		if ( bitStart < 0 )				{ ASSERT(false); return; }
		if ( (UINT32) bitEnd >= SizeBits )		{ ASSERT(false); return; }

		// this covers the case where we're writing within one byte.
		if ( bitEnd - bitStart < 8 )
		{
			for ( UINT32 i = (UINT32) bitStart; i <= (UINT32) bitEnd; i++ )
				Set( i, value );
			return;
		}

		// set beginning odd bits
		while ( bitStart % 8 != 0 )
		{
			Set( bitStart, value );
			bitStart++;
		}

		UINT32 sbyte = bitStart / 8;
		UINT32 ebyte = bitEnd / 8;
		if ( (bitEnd + 1) % 8 == 0 ) ebyte++;
		BYTE bval = value ? 0xFF : 0x00;
		for ( UINT32 i = sbyte; i < ebyte; i++ )
			Map[i] = bval;

		// set end odd bits
		while ( (bitEnd + 1) % 8 != 0 )
		{
			Set( bitEnd, value );
			bitEnd--;
		}
	}

	/** Resizes preserving existing data.
	@return False only if there is a memory allocation failure.
	**/
	bool Resize( UINT32 bits, bool fillNewSectionWith = false )
	{
		if ( bits == 0 ) { Clear(); return true; }
		if ( External ) { ASSERT(false); return false; }
		if ( bits == SizeBits ) return true;

		BYTE* oldMap = Map;
		UINT32 oldBytes = SizeBytes;
		UINT32 oldBits = SizeBits;
		SizeBits = bits;
		SizeBytes = (bits + 7) / 8;
		CapacityBits = SizeBits;
		CanFind = true;
		
		// align size to next 128-bit boundary
		UINT32 alignedSize = ((SizeBytes + Alignment - 1) / Alignment) * Alignment; 
		Map = (BYTE*) BmpAlloc( alignedSize );

		if ( Map != NULL && oldMap != NULL )
		{
			UINT32 copy = MIN( oldBytes, SizeBytes );
			memcpy( Map, oldMap, copy );
			BmpFree( oldMap );
		}

		if ( Map != NULL && bits > oldBits )
		{
			Fill( oldBits, bits - 1, fillNewSectionWith );
		}

		return Map != NULL;
	}

	/** Add a bit.
	This is the only function that will automatically increase the size of the bitmap.
	**/
	void Add( bool value )
	{
		if ( External ) { ASSERT(false); return; }
		if ( SizeBits >= CapacityBits )
		{
			int oldSize = SizeBits;
			int newSize = MAX( SizeBits * 2, 64u );
			if ( !Resize( newSize ) ) return;
			SizeBits = oldSize;
		}
		Set( SizeBits, value );
		SizeBits++;
	}

	/// Alias for Add()
	BitMap& operator+= ( bool value )
	{
		Add( value );
		return *this;
	}

	/// Automatically grow if necessary
	void SetAutoGrow( UINT32 bit, bool value, bool fillNewSectionWith = false )
	{
		if ( bit == SizeBits )
		{
			Add( value );
			return;
		}
		else if ( bit >= SizeBits )
		{
			if ( bit >= CapacityBits )
			{
				UINT32 nCap = MAX( CapacityBits * 2, bit + 1 );
				Resize( nCap, fillNewSectionWith );
			}
			SizeBits = bit + 1;
		}
		Set( bit, value );
	}

	/// Set a bit
	void Set( UINT32 bit, bool value )
	{
		UINT32 pword = bit >> 5;
		UINT32 pbit = bit & 31;
#ifdef _WIN32
		if ( value )	_bittestandset( ((LONG*) Map) + pword, pbit );
		else			_bittestandreset( ((LONG*) Map) + pword, pbit );
#else
		if ( value )	((UINT32*)Map)[pword] |= (1 << pbit);
		else			((UINT32*)Map)[pword] &= ~(1 << pbit);
#endif
	}

	/// Get a bit, but return the passed-in default value if the bit requested is larger than the table.
	bool GetOrDefault( UINT32 bit, bool default_value ) const
	{
		if ( bit >= SizeBits ) return default_value;
		return Get( bit );
	}

	/// Get a bit
	bool Get( UINT32 bit ) const
	{
		ASSERT( bit < SizeBits );
		UINT32 pword = bit >> 5;
		UINT32 pbit = bit & 31; 
#ifdef _WIN32
		return 0 != _bittest( ((LONG*) Map) + pword, pbit );
#else
		return 0 != (((UINT32*)Map)[pword] & (1 << pbit));
#endif
	}

	bool operator[]( UINT32 bit ) const { return Get(bit); }

	/** Counts the number of true bits in the specified range. 
	This is not optimized, but in future I may indeed do that.
	**/
	int CountTrueBits( UINT32 fromBit = 0, UINT32 toBitInclusive = -1 ) const
	{
		if ( toBitInclusive == -1 ) toBitInclusive = SizeBits - 1;
		UINT32 count = 0;
		for ( UINT32 i = fromBit; i < toBitInclusive; i++ )
		{
			if ( Get(i) )
				count++;
		}
		return count;
	}

	/** Counts the number of false bits in the specified range. 
	**/
	int CountFalseBits( UINT32 fromBit = 0, UINT32 toBitInclusive = -1 ) const
	{
		if ( toBitInclusive == -1 ) toBitInclusive = SizeBits - 1;
		UINT32 size = 1 + toBitInclusive - fromBit;
		return size - CountTrueBits( fromBit, toBitInclusive );
	}

	/// Returns the raw map
	void* GetMap() const { return Map; }

	/// Linearly searches the map for the first true bit.
	int FirstTrueBit( UINT32 firstBit = 0, UINT32 lastBitInclusive = -1 ) const { return FirstBit( true, firstBit, lastBitInclusive ); }
	
	/// Linearly searches the map for the first false bit.
	int FirstFalseBit( UINT32 firstBit = 0, UINT32 lastBitInclusive = -1 ) const { return FirstBit( false, firstBit, lastBitInclusive ); }

	/** Linearly searches the map for the first true or false bit, and returns -1 if none is found.
	@firstBit Begin searching at the indicated bit.
	@lastBitInclusive Stop searching on the specified bit.
	@return The first bit found in the requested state, or -1 if there is no such bit.
	**/
	UINT32 FirstBit( bool state, UINT32 firstBit = 0, UINT32 lastBitInclusive = -1 ) const
	{
		if ( !CanFind ) { ASSERT(false); return -1; }
		if ( firstBit < 0 ) firstBit = 0;
		if ( SizeBits == 0 ) return -1;
		if ( lastBitInclusive == -1 ) lastBitInclusive = SizeBits - 1;
		lastBitInclusive = MIN( lastBitInclusive, SizeBits - 1 );
		// the second pass is for the following case:
		// the bitmap looks like this: ([] denotes a 32-bit block).
		// [01000..0][001000..0]
		// and the search is ( true, 3 ).
		// This causes us to begin our search on the first block, and then we
		// false-positively exit on the first byte.. ergo the second pass.
		for ( int pass = 0; pass < 2; pass++ )
		{
			if ( firstBit > lastBitInclusive ) return -1;
			UINT32 searchBytes = 1 + (lastBitInclusive / 8);
			TUnit* base = (TUnit*) Map;
			base += firstBit / UnitSize;
			TUnit* p = base;
			TUnit* porg = base;
			TUnit* pb = (TUnit*) Map;
			TUnit* pt = (TUnit*) (Map + searchBytes);
			if ( state )
			{
				for ( ; p < pt; p++ ) if ( *p != 0 ) break;
			}
			else
			{
				for ( ; p < pt; p++ ) if ( *p != UnitFill ) break;
			}
			UINT32 sbit = UINT32((p - pb) * UnitSize);
			sbit = MAX( sbit, firstBit );
			UINT32 ebit = MIN( sbit + UnitSize, lastBitInclusive + 1 );
			if ( state )
			{
				for ( UINT32 i = sbit; i < ebit; i++ ) if ( Get(i) ) return i;
			}
			else
			{
				for ( UINT32 i = sbit; i < ebit; i++ ) if ( !Get(i) ) return i;
			}
			// this is the only condition necessitating a 2nd pass
			if ( firstBit % UnitSize != 0 && p == porg )
			{
				firstBit = ((firstBit + UnitSize - 1) / UnitSize) * UnitSize;
				continue;
			}
			return -1;
		}
		return -1;
	}

protected:
	bool External;
	bool CanFind;	///< Only true if our data is aligned on 128 bits.
	UINT32 CapacityBits;
	UINT32 SizeBits;
	UINT32 SizeBytes;
	BYTE* Map;

	void Construct()
	{
		External = false;
		CanFind = false;
		CapacityBits = 0;
		SizeBytes = 0;
		SizeBits = 0;
		Map = NULL;
	}

#if defined(ABC_HAVE_ALIGNED_MALLOC)
	void* BmpAlloc( size_t bytes )
	{
		return AbcAlignedMalloc( bytes, Alignment );
	}
	void BmpFree( void* p )
	{
		return AbcAlignedFree( p );
	}
#else
	// This business of being 128-byte aligned is not actually necessary.. it's just a future looking thing
	void* BmpAlloc( size_t bytes )
	{
		return malloc( bytes );
	}
	void BmpFree( void* p )
	{
		return free( p );
	}
#endif

};


/** A free list, built on top of BitMap.

We run a secondary bitmap that records groups of 32 slots, marking a 1
if all 32 slots are occupied, and a zero otherwise. This allows us to
search for 1024 free slots with a single 32-bit integer comparison.
The reason for this is because every bit in the secondary map covers
32 bits in the main map, and 32 * 32 = 1024.

For a 64-bit map, this becomes 4096. I guess optimal performance depends
on the particular use case. Note that it will not be 64 * 64 until we have
made BitMap capable of using 64-bits... let's do it now...

We pad our main map's size up so that it fits the group size,
and we set the extra bits to 1, so that they are marked as 'used'.
**/
template< typename GType >
class TFreeList
{
public:
	TFreeList()
	{
		Guess = 0;
		Used = 0;
		Size = 0;
		External = false;
	}

	/// Group bits
	static const int GBits = sizeof(GType) * 8;
	static const GType GFull = -1;

	void Reset()
	{
		if ( External )
		{
			Secondary.Clear();
			Main.Clear();
			Size = 0;
			External = false;
		}
		else
		{
			Resize(0);
		}
		Used = 0;
		Guess = 0;
	}

	int RequiredMainBits( int bits )
	{
		return ((bits + GBits - 1) / GBits) * GBits;
	}

	void Resize( int bits )
	{
		ResizeInternal( bits, true );
	}

	void SetMap( int bits, int used, const void* raw )
	{
		if ( External ) Reset();
		ResizeInternal( bits, false );
		memcpy( Main.GetMap(), raw, (bits + 7) / 8 );
		SetOverBits( true );
		Used = used;
		RebuildSecondaryInternal();
	}

	/** Set the main map from an externally allocated memory block.
	It is illegal to resize the bitmap when it is external. If you want to recycle the object,
	call Clear(), which makes it non-external.
	**/
	bool SetMapExternal( int bits, int used, void* raw, int bitsReserved )
	{
		// bitsReserved is here for no purpose other than to ensure that you understand that you might
		// needs to have extra bits on the end to pad things out to 32/64
		if ( RequiredMainBits(bits) > bitsReserved ) { ASSERT(false); return false; }
		Main.SetExternal( bits, raw, bitsReserved );
		Size = bits;
		External = true;
		SetOverBits( true );
		Used = used;
		RebuildSecondaryInternal();
		return true;
	}

	void* GetMain()
	{
		return Main.GetMap();
	}

	int Capacity() const { return Size; }
	int SlotsUsed() const { return Used; }

	bool Get( int slot ) const { return Main.Get(slot); }

	/** Find a free item.
	
	The item is marked non-free before the function returns.

	@return A free item, or -1 if all items are used.
	**/
	int Acquire()
	{
		if ( Used == Size ) return -1;
		int slot = -1;
		if ( Guess >= 0 && Guess < Size )
		{
			if ( !Main.Get(Guess) )
				slot = Guess;
		}
		if ( slot == -1 )
		{
			slot = Search();
		}
		if ( slot == -1 ) { ASSERT(false); return -1; } // this ought to be impossible.
		Main.Set( slot, true );
		Update( slot );
		Used++;
		Guess = slot + 1;
		return slot;
	}


	void Release( int slot )
	{
		ASSERT( slot >= 0 && slot < Size );
		ASSERT( Main.Get(slot) );
		Main.Set( slot, false );
		Update( slot );
		Used--;
		Guess = slot;
	}

	/** Rebuild the secondary map.
	Use this only if you have updated the underlying bitmap (which you COULD only do if you were operating with external data).
	You must respecify the number of free slots here.
	**/
	void RebuildSecondary( int used )
	{
		Used = used;
		RebuildSecondaryInternal();
	}

	/// If you modify the underlying map, you better call RebuildSecondary afterwards.
	BitMap& GetMainMap()
	{
		return Main;
	}

protected:
	int Guess;
	int Size;
	int GroupSize() { return Size / GBits; }
	int Used;
	bool External;
	BitMap Main, Secondary;

	void Update( int forBit )
	{
		GType* prim = (GType*) Main.GetMap();
		int g = forBit / GBits;
		Secondary.Set( g, prim[g] == GFull );
	}

	int Search()
	{
		int g = Secondary.FirstFalseBit( 0 );
		if ( g < 0 ) { ASSERT(false); return -1; } // by design, should never happen, because we keep track of Used vs Size.
		return Main.FirstFalseBit( g * GBits );
	}

	void SetOverBits( bool used )
	{
		// set the over bits (which pad our main buffer up to a GBit boundary)
		for ( int i = Size; i < Main.Size(); i++ )
			Main.Set( i, used );
	}

	void ResizeInternal( int bits, bool rebuild_secondary = true )
	{
		if ( External ) { ASSERT(false); return; }
		//ASSERT( bits >= Size );
		//Used = 0;
		SetOverBits( false );
		Size = bits;
		int actual = ((bits + GBits - 1) / GBits) * GBits;
		Main.Resize( actual, false );
		SetOverBits( true );
		if ( rebuild_secondary ) 
			RebuildSecondaryInternal();
	}

	void RebuildSecondaryInternal()
	{
		int ns = Main.Size() / GBits;
		Secondary.Resize( ns );
		GType* mb = (GType*) Main.GetMap();
		for ( int i = 0; i < ns; i++ )
		{
			Secondary.Set( i, mb[i] == GFull );
		}
	}

};

typedef TFreeList< UINT32 > FreeList32;
typedef TFreeList< UINT64 > FreeList64;

#ifdef _WIN64
typedef FreeList64 FreeList;
#else
typedef FreeList32 FreeList;
#endif
