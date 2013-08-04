#include "pch.h"
#include "TinyGC.h"

TinyGC::TinyGC()
{
	Data = NULL;
	User = NULL;
	ReallocBusy = NULL;

	// user configurable
	WasteFactor = 0.333;
	GrowFactor = 1.5;
	WasteAbsolute = 0; // disable by default, because it can potentially be a very bad heuristic
	UseExplicitFree = false;
	DisableGC = 0;

	Reset();
}

TinyGC::~TinyGC()
{
	Reset();
}

void TinyGC::Reset()
{
	CollectionPending = false;
	Collections = 0;
	Capacity = 0;
	Size = 0;
	Junk = 0;
	free(Data); 
	Data = NULL;
}

bool TinyGC::IsWasteful( UINT32 unused )
{
	if ( WasteFactor != 0 && unused / (double) Capacity > WasteFactor ) return true;
	if ( WasteAbsolute != 0 && unused > WasteAbsolute ) return true;
	return false;
}

bool TinyGC::Collect( UINT flags /*= 0 */ )
{
	if ( Capacity == 0 ) return false;
	if ( DisableGC )
	{
		CollectionPending = true;
		return false;
	}
	if ( Collections == 1 )
		int aaaaaaaaaaa = 1;
	UINT32 orgSize = Size;
	UINT32 orgCap = Capacity;
	Collections++;
	if ( User )
		User->GC_Mark( this );
	if ( ReallocBusy )
		MarkMap.insert( *ReallocBusy, ReallocBusy );
	UINT32 used = 0;
	dvect<UINT32> keys = MarkMap.keys();
	// It is my out-of-the-hat guess that this will improve cache behaviour
	sort( keys );
	for ( int i = 0; i < keys.size(); i++ )
		used += 1 + BlockSize( keys[i]);
	UINT32 unused = Capacity - used;
	bool ignoreWaste = (flags & FlagIgnoreWasteFactor) != 0 || (flags & FlagMaximumShrink) != 0;
	bool sweep = IsWasteful(unused) || (ignoreWaste && unused > 0);
	if ( sweep )
	{
		Sweep( used, keys, flags );
		GCTRACE( "  Collected. Size: %6d - %6d. Capacity: %6d - %6d\n", orgSize, Size, orgCap, Capacity );
	}
	else
	{
		GCTRACE( "  Marked, but did not sweep\n" );
	}
	MarkMap.clear();
	CollectionPending = false;
	return sweep;
}

UINT32 TinyGC::Alloc( UINT32 n, bool zero /*= true */ )
{
	UINT32 require = n + 1 + Size;
	if ( require > Capacity )
	{
		GCTRACE( "Alloc needs more space...\n" );
		if ( UseExplicitFree )
		{
			if ( IsWasteful(Junk) )
				Collect();
		}
		else
		{
			Collect();
		}
		require = n + 1 + Size;
	}
	if ( require > Capacity )
	{
		Capacity = (UINT32) (Capacity * GrowFactor);
		if ( Capacity < require ) Capacity = require;
		TData* ndata = (TData*) realloc( Data, sizeof(TData) * Capacity );
		AbcCheckAlloc(ndata);
		Data = ndata;
		GCTRACE( "Growing to %6d during Alloc\n", Capacity );
	}
	UINT32 p = Size;
	Data[p] = n;
	if ( zero ) memset( Data + p + 1, 0, sizeof(TData) * n );
	Size += 1 + n;
	return p + 1;
}

UINT32 TinyGC::Realloc( UINT32 id, UINT32 n, bool zero /*= true */ )
{
	if ( id == 0 ) return Alloc(n, zero);
	UINT32 osize = BlockSize( id );
	ReallocBusy = &id;
	UINT32 b = Alloc( n, zero );
	id = *ReallocBusy;
	memcpy( BlockPtr(b), BlockPtr(id), osize * sizeof(TData) );
	Free( id );
	ReallocBusy = 0;
	return b;
}

void TinyGC::Sweep( UINT32 newRequiredCapacity, dvect<UINT32>& keys, UINT flags )
{
	UINT32 npos = 0;
	TData* nd = NULL;
	UINT32 newCapacity = 0;
	if ( newRequiredCapacity > 0 )
	{
		// I think our shrink factor should be larger than our growth factor so that
		// a collection before a capacity grow does not always result in us getting shrunk down
		// to our absolute minimum, and then we end up growing back up again, and into a nasty cycle.
		double shrinkFactor = GrowFactor * 1.5;
		if ( newRequiredCapacity <= Capacity / shrinkFactor || (flags & FlagMaximumShrink) != 0 )
		{
			newCapacity = newRequiredCapacity;
			//GCTRACE( "  Sweep: Shrinking capacity to %6d\n", newCapacity );
		}
		else
		{
			newCapacity = Capacity;
			//GCTRACE( "  Sweep: Keeping   capacity at %6d\n", newCapacity );
		}
		nd = (TData*) malloc( sizeof(TData) * newCapacity );
		AbcCheckAlloc( nd );
		// Reallocate every used block, and update their new positions into their referencing pointers
		dvect<UINT32*> values;
		for ( int i = 0; i < keys.size(); i++ )
		{
			UINT32 bsize = BlockSize( keys[i] );
			nd[npos] = bsize;
			memcpy( nd + npos + 1, BlockPtr( keys[i] ), bsize * sizeof(TData) );
			// Update pointers.
			values.clear_noalloc();
			MarkMap.get( keys[i], values );
			for ( int j = 0; j < values.size(); j++ )
			{
				*(values[j]) = npos + 1;
			}
			npos += bsize + 1;
		}
	}

	free(Data);
	Data = nd;

	Size = npos;
	Capacity = newCapacity;
	Junk = 0;
}
