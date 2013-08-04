#pragma once

#include "lmTypes.h"

/** Binary Map Table.

WARNING
WARNING
WARNING
WARNING

THIS WAS NEVER USED NOT TESTED

WARNING
WARNING
WARNING
WARNING

This is basically a hash table from some arbitrary fixed size binary key onto an Int32.
I found myself needing this twice. First was for JitPool. Second was for GeoLib's converter cache.
Obviously one could simply instantiate a hash table, but I don't like the bloat.

Notes:
We need to keep a copy of all signatures inserted into the table, because the hash table needs access
to the real values. An alternative strategy is to simply use a 32-bit hash of the signature, but that
opens up a can of worms (because of hash collisions), that probably makes things harder to deal with than simply taking the little
bit of extra effort to do things right, like we do here.

**/
class PAPI BinMap
{
public:
	BinMap()
	{
		Signatures = NULL;
		Reset();
	}

	~BinMap()
	{
		Reset();
	}

	void Reset()
	{
		free(Signatures);
		Signatures = NULL;
		Count = 0;
		MaxElements = 0;
		SigBytes = 0;
		Map.clear();
	}

	void Setup( int sigBytes )
	{
		Reset();
		SigBytes = sigBytes;
	}

	/// Returns the item, or zero if not present
	INT32 Get( const void* sig )
	{
		return Map.get( HashSig(sig, SigBytes) );
	}

	bool Contains( const void* sig )
	{
		return Map.contains( HashSig(sig, SigBytes) );
	}

	void Set( const void* sig, INT32 v )
	{
		if ( Count == MaxElements )
		{
			Grow();
		}
		InsertInternal( sig, v );
	}

protected:
	int Count;
	int MaxElements;
	size_t SigBytes;
	BYTE* Signatures;
	HashSigIntMap Map;

	BYTE* SigAt( BYTE* block, int i ) { return block + i * SigBytes; }
	BYTE* SigAt( int i )				{ return SigAt(Signatures, i); }

	void InsertInternal( const void* sig, INT32 v )
	{
		memcpy( SigAt(Count), sig, SigBytes );
		Map.insert( HashSig(SigAt(Count), SigBytes), v, true );
	}

	void Grow()
	{
		dvect<INT32> oldValues;
		for ( int i = 0; i < Count; i++ )
			oldValues += Map.get( HashSig(SigAt(i), SigBytes) );

		MaxElements = max( MaxElements * 2, 2 );
		BYTE* oldSig = Signatures;
		int oldCount = Count;
		Count = 0;
		Signatures = (BYTE*) malloc( SigBytes * MaxElements );
		for ( int i = 0; i < oldCount; i++ )
		{
			InsertInternal( SigAt(oldSig, i), oldValues[i] );
		}
		free(oldSig);
	}

};
