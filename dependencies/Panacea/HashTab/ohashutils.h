#ifndef OHASHUTILS_H
#define OHASHUTILS_H

#include "../Other/lmTypes.h"

// Generic hash map
class PAPI ohashgenmap
{
protected:
	typedef HashSigIntMap TMap;
	TMap Map;
	char* Pool;
	size_t PoolCap, PoolUsed;
	size_t SigSize;
public:

	typedef int TValue;

	ohashgenmap()
	{
		SigSize = 0;
		Pool = NULL;
		PoolCap = PoolUsed = 0;
	}
	~ohashgenmap()
	{
		Clear();
	}

	void StaticSigSize( size_t bytes )
	{
		SigSize = bytes;
	}

	void Clear()
	{
		Map.clear();
		free(Pool);
		Pool = NULL;
		PoolCap = PoolUsed = 0;
	}

	void Insert( const void* data, TValue value ) { ASSERT(SigSize); return Insert( data, SigSize, value ); }

	void Insert( const void* data, size_t bytes, TValue value )
	{
		void* p = Alloc(bytes);
		memcpy( p, data, bytes );
		HashSig sig(p, bytes);
		ASSERT( !Map.contains(sig) );
		Map.insert( sig, value, true );
	}

	TValue Get( const void* data ) const { return Get( data, SigSize ); }

	TValue Get( const void* data, size_t bytes ) const
	{
		return Map.get( HashSig(data, bytes) );
	}

	bool GetV( const void* data, TValue& val ) const { return GetV( data, SigSize, val ); }

	bool GetV( const void* data, size_t bytes, TValue& val ) const
	{
		TValue* p = Map.getp( HashSig(data, bytes) );
		if ( p ) val = *p;
		return p != NULL;
	}

	dvect<HashSig> Keys() const
	{
		return Map.keys();
	}

protected:
	void* Alloc( size_t bytes )
	{
		if ( PoolUsed + bytes > PoolCap )
		{
			dvect<HashSig> sigs;
			dvect<TValue> values;
			for ( TMap::iterator it = Map.begin(); it != Map.end(); it++ )
			{
				sigs += it->first;
				values += it->second;
			}
			char* base = Pool;
			size_t ncap = PoolCap * 2 + 16;
			ncap = max(ncap, PoolUsed + bytes);
			Pool = (char*) realloc( Pool, ncap );
			Map.clear();
			size_t delta = Pool - base;
			for ( int i = 0; i < sigs.size(); i++ )
			{
				(char*&) sigs[i].Data += delta;
				Map.insert( sigs[i], values[i] );
			}
			PoolCap = ncap;
		}
		void* pos = Pool + PoolUsed;
		PoolUsed += bytes;
		return pos;
	}
};


#endif
