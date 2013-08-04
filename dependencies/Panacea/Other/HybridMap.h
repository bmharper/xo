#pragma once


#include "../HashTab/ohashmap.h"

/** A hybrid map that requires it's key to be an integer.
This map is designed for systems that might have only a very small number
of relevant objects, or a very large number. The map is best explained
by it's application:
A database needs to increment a modification stamp counter on each of it's rows.
When the database is loaded, these stamps are all zero indicating the default value.
This is in effect a large array of zeros. We don't need to store this. 
As the user gradually makes changes to records, we increment the stamp counters 
on the modified rows. There comes a point where it is more effective to simply 
store a linear array of the stamp counters of every record than to store them 
in a hashed map. We automatically alternate between hash map and linear array
depending on the number of zeros in the set.

Usage:
- Use operator[] to set values to non-zero. Setting them to zero is legal, but it is a waste of
	space since the default value is zero (unless of course you're changing a non-zero value to zero, 
	in which case it's necessary and normal).
- Use Get() to retrieve values. Using operator[] to read values will often result in the compiler using
	the non-const version, which messes with our heuristics.
- Use Reset() to clear the whole thing.

**/
template< typename TValue >
class HybridMap
{
public:
	
	// TKey will always be an integer type.
	typedef int TKey;

	typedef ohashmap< TKey, TValue > TMap;
	typedef dvect< TValue > TVect;

	/// So long as the set is smaller than this size, it remains linear (in auto mode). Default = 200.
	int LinearSmallThreshold;

	HybridMap()
	{
		LinearSmallThreshold = 200;
		AutoMode = true;
		Reset();
	}

	~HybridMap()
	{
	}

	void Reset()
	{
		Begin = INTMAX;
		End = -INTMAX;
		ModeMap = false;
		LastFullAudit = 0;
		LastAutoChangeTick = 0;
		Tick = 0;
		SizeEstimate = 0;
		Map.clear();
		Vect.clear();
	}

	/** Same as the const operator[], except that using this method ensures better
	statistics because it prevents the compiler from erroneously calling the modifying operator[].
	**/
	TValue Get( TKey k ) const
	{
		if ( ModeMap )	return Map.get( k );
		else
		{
			if ( k < Begin || k >= End ) { return TValue(); }
			return Vect[k - Begin];
		}
	}

	TValue operator[]( TKey k ) const
	{
		return Get( k );
	}

	TValue& operator[]( TKey k )
	{
		Tick++;
		AutoChange();
		if ( ModeMap )
		{
			Begin = min( Begin, k );
			End = max( End, k + 1 );
			if ( !Map.contains( k ) ) 
			{
				// herein the 'estimate'. The user may simply be setting this value to zero,
				// or may do so later. See below on the same topic.
				SizeEstimate = min( SizeEstimate + 1, (int) Map.size() );
				Map.insert( k, TValue() );
			}
			return Map[ k ];
		}
		else
		{
			if ( k < Begin )
			{
				TVect newVect;
				// grow by at least a factor of 2
				int newBegin = Begin - 2 * Range();
				if ( Range() == 0 ) End = k + 1; // initial conditions
				newBegin = min( newBegin, k );
				newVect.resize( End - newBegin );
				newVect.fill( TValue() );
				memcpy( &newVect[Begin - newBegin], &Vect[0], Vect.size() * sizeof(TValue) );
				Vect = newVect;
				Begin = newBegin;
			}
			else if ( k >= End )
			{
				// grow by at least a factor of 2
				int newEnd = End + 2 * Range();
				newEnd = max( newEnd, k + 1 );
				if ( Range() == 0 ) Begin = k; // initial conditions
				Vect.resize( newEnd - Begin );
				for ( TKey i = End; i < newEnd; i++ )
					Vect[i - Begin] = TValue();
				End = newEnd;
			}
			Begin = min( Begin, k );
			End = max( End, k + 1 );
			
			// herein the 'estimate'. The user may simply be setting this value to zero. We do however assume 
			// here that the user is modifying the value, as per the recommendations 
			// (which are only to use operator[] when modifying, and Get() when reading).
			if ( Vect[k - Begin] == 0 ) 
				SizeEstimate = min( SizeEstimate + 1, Vect.size() );

			return Vect[k - Begin];
		}
	}

	void SetModeAuto()
	{
		AutoMode = true;
	}

	void SetModeHash()
	{
		GoHash(); 
		AutoMode = false;
	}

	void SetModeLinear()
	{
		GoLinear(); 
		AutoMode = false;
	}

	/// Set the map to linear (but don't alter the status of the Auto Mode Adjustment flag)
	void SetLinear() { GoLinear(); }
	
	/// Set the map to hash (but don't alter the status of the Auto Mode Adjustment flag)
	void SetHash() { GoHash(); }

protected:
	int Tick;
	int LastAutoChangeTick;
	int LastFullAudit;
	TKey SizeEstimate;
	TKey Begin, End;
	TKey Range() const { return End > Begin ? End - Begin : 0; }

	bool AutoMode;
	bool ModeMap;
	TMap Map;
	TVect Vect;

	/// This is only really to prevent complete meltdown.
	static const int FullAuditInterval = 500000;

	/** This is an estimate based on tests performed with ohashmap<int32,int32>.
	The amount of space required by a map varies from 4x to 12x the amount of space 
	required by an equivalently fully populated linear map.
	**/
	static size_t MapMemUsage( size_t n ) { return LinearMemUsage(n) * 6; }
	static size_t LinearMemUsage( size_t n ) { return n * sizeof(TKey); }

	bool IsLinearBetter() const
	{
		return	MapMemUsage( SizeEstimate ) > LinearMemUsage( Range() ) ||
						Range() < LinearSmallThreshold;
	}

	void AutoChange()
	{
		if ( abs( Tick - LastFullAudit ) > FullAuditInterval ) FullAudit();
		if ( AutoMode )
		{
			int elapsed = abs( Tick - LastAutoChangeTick );
			int cutoff = 1000;
			if ( elapsed > cutoff || Tick == 8 || Tick == 64 )
			{
				if ( IsLinearBetter() ) GoLinear();
				else					GoHash();
				LastAutoChangeTick = Tick;
			}
		}
	}

	/// Rebuilds the SizeEstimate and optionally compacts the map
	void FullAudit()
	{
		if ( ModeMap )
		{
			SizeEstimate = 0;
			for ( TMap::iterator it = Map.begin(); it != Map.end(); it++ )
			{
				if ( it->second != TValue() ) SizeEstimate++;
			}
			if ( Map.size() / (float) (1 + SizeEstimate) > 2 )
			{
				// compact the map
				TMap newMap;
				for ( TMap::iterator it = Map.begin(); it != Map.end(); it++ )
				{
					if ( it->second != TValue() )
						newMap.insert( it->first, it->second );
				}
				Map = newMap;
			}
		}
		else
		{
			SizeEstimate = 0;
			int newBegin = INTMAX;
			int newEnd = -INTMAX;
			for ( TKey i = Begin; i < End; i++ )
			{
				if ( Vect[i - Begin] != TValue() ) 
				{
					newBegin = min( newBegin, i );
					newEnd = max( newEnd, i + 1 );
					SizeEstimate++;
				}
			}
			if ( (newEnd - newBegin) / (float) (1 + End - Begin) > 2 )
			{
				// compact list
				TVect newVect;
				newVect.resize( newEnd - newBegin );
				newVect.fill( TValue() );
				for ( TKey i = Begin; i < End; i++ )
				{
					if ( Vect[i - Begin] != TValue() )
						newVect[i - newBegin] = Vect[i - Begin];
				}
				Vect = newVect;
				Begin = newBegin;
				End = newEnd;
			}
		}
		LastFullAudit = Tick;
	}

	void GoLinear()
	{
		if ( !ModeMap ) return;
		ASSERT( Vect.size() == 0 );
		ModeMap = false;
		SizeEstimate = 0;
		Vect.resize( Range() );
		Vect.fill( TValue() );
		for ( TMap::iterator it = Map.begin(); it != Map.end(); it++ )
		{
			ASSERT( it->first - Begin >= 0 && it->first - Begin < Vect.size() );
			Vect[ it->first - Begin ] = it->second;
			if ( it->second != TValue() ) SizeEstimate++;
		}
		
		for ( int i = 0; i < Vect.size(); i++ )
			ASSERT( Vect[i] > -10000 && Vect[i] < 10000 );

		Map.clear();
	}

	void GoHash()
	{
		if ( ModeMap ) return;
		ASSERT( Map.size() == 0 );
		SizeEstimate = 0;
		ModeMap = true;
		int newBegin = INTMAX;
		int newEnd = -INTMAX;
		for ( TKey i = 0; i < Vect.size(); i++ )
		{
			if ( Vect[i] != TValue() )
			{
				newBegin = min( newBegin, i + Begin );
				newEnd = max( newEnd, i + Begin + 1 );
				Map.insert( i + Begin, Vect[i] );
				SizeEstimate++;
			}
		}
		Begin = newBegin;
		End = newEnd;
		//for ( TMap::iterator it = Map.begin(); it != Map.end(); it++ )
		//	ASSERT( it->second > -10000 && it->second < 10000 );
		Vect.clear();
	}

};
