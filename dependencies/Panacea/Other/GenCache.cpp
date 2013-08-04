#include "pch.h"
#include "../Containers/cont_utils.h"
#include "GenCache.h"
#include "../Other/profile.h"
#include "../strings/fmt.h"
#include "../Platform/debuglib.h"

#ifdef _DEBUG
#define CRASH_ASSERT ASSERT
#else
#define CRASH_ASSERT( condition ) if(!(condition)) RaiseError();
#endif

GenCache::GenCache()
{
	DateWrapAround = AbCore::Traits<TDate>::Max() / 4;
	Head = NULL;
	SigSize = 0;
	ItemSize = 0;
	MaxItems = 0;
	MaxStorage = 0;
	SlotsUsed = 0;
	ErrorOnOverwriteLocked = true;
	DebugVerifySanity_On = false;
	DebugTraceOn = false;
	DebugCrashHard = false;
	AbcAssert( InitializeCriticalSectionAndSpinCount( &Lock, 1000 ) );
	Reset();
}

GenCache::~GenCache()
{
	Reset();
	DeleteCriticalSection( &Lock );
}

void GenCache::Reset()
{
	free(Head); Head = NULL;
	Slots.Clear();
	SlotsUsed = 0;
	CurDate = 0;
	TotalGenerationTime = 0;
	TotalStorage = 0;
	QTotalEvictions = 0;
	QTotalReads = 0;
	QTotalInserts = 0;
	QTotalDateWraps = 0;
	QTotalGoodReads = 0;
	Map.clear();
}

void GenCache::Init( FGenCacheRelease releaseFunc, int sigSize, int maxItems, TAccum maxStorage )
{
	ASSERT( Head == NULL && AllocatedItems() == 0 );
	SigSize = sigSize;
	FRelease = releaseFunc;
	ItemSize = sizeof(ItemHead) + sigSize;
	MaxItems = maxItems;
	MaxStorage = maxStorage;
}

void GenCache::SetMaxStorage( TAccum maxStorage )
{
	MaxStorage = maxStorage;
}

void GenCache::SetMaxItems( int maxItems )
{
	ASSERT( maxItems >= Slots.Size() ); // TODO: Shrink max items
	MaxItems = maxItems;
}

void GenCache::RaiseError()
{
	ASSERT(false);
	if ( DebugCrashHard )
	{
		AbcPanic("GenCache");
	}
}

bool GenCache::Insert( const void* sig, bool overwrite, bool initial_lock, void* releaseContext, void* data, TGenTime genTime, TStorage storageCost )
{
	TakeCriticalSection cs( Lock );
	QTotalInserts++;

	// Check for existence
	int existingSlot = FindSlot( sig );
	if ( existingSlot >= 0 )
	{
		if ( overwrite )
		{
			if ( HeadAt(existingSlot)->Lock != 0 )
			{
				if ( ErrorOnOverwriteLocked )
					RaiseError();
				return false;
			}

			// we could perhaps short-circuit stuff here, but it's much simpler just to do an Erase + Insert.
			EvictSingle( existingSlot );
		}
		else
		{
			return false;
		}
	}

	bool storageBlown = TotalStorage + storageCost > MaxStorage;
	bool slotsBlown = SlotsUsed == MaxItems;

	if ( storageBlown || slotsBlown )
		EvictForSpace( slotsBlown, storageCost );

	int slot = Slots.FirstFalseBit();
	if ( slot == -1 )
	{
		// need more slots
		GrowSlots();
		slot = Slots.FirstFalseBit();
		ASSERT( slot != -1 );
	}

	ItemHead* head = HeadAt(slot);
	head->ReleaseContext = releaseContext;
	head->Data = data;
	head->GenerationTime = genTime;
	head->LastUsed = NextDate();
	head->SlotPos = slot;
	head->StorageCost = storageCost;
	head->Lock = initial_lock ? 1 : 0;
	
	memcpy( SigAt(slot), sig, SigSize );
	Map.insert( HSigAt(slot), slot + SlotMapOffset );
	SlotsUsed++;
	Slots.Set( slot, true );
	TotalStorage += storageCost;
	TotalGenerationTime += genTime;
	
	return true;
}

bool GenCache::CheckExists( const void* sig, bool touch )
{
	bool exists = NULL != GetInternal( sig, true, false, touch );
	return exists;
}

int GenCache::CheckExists( int n, const void** sigs, bool* found, bool touch )
{
	TakeCriticalSection cs( Lock );
	int r = 0;
	for ( int i = 0; i < n; i++ )
	{
		bool f = GetInternal( sigs[i], false, false, touch ) != NULL;
		if ( found ) found[i] = f;
		if ( f ) r++;
	}
	return r;
}

void* GenCache::Acquire( const void* sig )
{
	return GetInternal( sig, true, true, true );
}

void* GenCache::GetInternal( const void* sig, bool acquireCS, bool acquireObject, bool touch )
{
	if ( acquireCS ) EnterCriticalSection( &Lock );
	if ( touch ) QTotalReads++;
	void* dat = NULL;
	int slot = Map.get( HashSig(sig, SigSize) ) - SlotMapOffset;
	if ( slot >= 0 )
	{
		if ( touch ) QTotalGoodReads++;
		ItemHead* h = HeadAt(slot);
		ASSERT( h->Lock < 1000 ); // sanity
		dat = h->Data;
		if ( acquireObject ) h->Lock++;
		if ( touch ) h->LastUsed = NextDate();
	}
	if ( acquireCS ) LeaveCriticalSection( &Lock );
	return dat;
}

void GenCache::Release( const void* sig )
{
	TakeCriticalSection cs( Lock );
	int slot = Map.get( HashSig(sig, SigSize) ) - SlotMapOffset;
	if ( slot >= 0 )
	{
		int lc = HeadAt(slot)->Lock;
		CRASH_ASSERT( HeadAt(slot)->Lock > 0 );
		HeadAt(slot)->Lock--;
	}
	else
		RaiseError();
}

bool GenCache::Remove( const void* sig )
{
	TakeCriticalSection cs( Lock );

	bool found = false;
	int slot = FindSlot( sig );
	if ( slot >= 0 )
	{
		if ( HeadAt(slot)->Lock )
		{
			RaiseError();
		}
		else
		{
			EvictSingle( slot );
			found = true;
		}
	}

	return found;
}

bool GenCache::RemoveAll()
{
	TakeCriticalSection cs( Lock );
	int locked = 0;
	for ( int i = 0; i < Slots.Size(); i++ )
	{
		if ( Slots.Get(i) )
		{
			if ( HeadAt(i)->Lock == 0 )
				EvictSingle( i );
			else
				locked++;
		}
	}
	if ( locked == 0 )
	{
		ASSERT( TotalGenerationTime == 0 );
		ASSERT( TotalStorage == 0 );
	}
	return locked == 0;
}

void GenCache::EvictForSpace( bool outOfSlots, TStorage ensureSpaceFor )
{
	// This is where we should be smart. Until I have built some kind of testing framework, it is really pointless
	// to try and be clever here. For now, I am just going with LRU.
	QTotalEvictions++;

	dvect<ItemHead> all;
	GetAll( all, false );
	sort( all );

	TStorage discardAtLeast = ensureSpaceFor;
	if ( !outOfSlots )
	{
		// If we've run out of storage space, then evict at least 1/4 of our total storage space
		discardAtLeast = max( (TStorage) ensureSpaceFor, (TStorage)  (MaxStorage / 4) );
	}

	TStorage discarded = 0;
	int lim;
	for ( lim = 0; lim < all.size() && discarded < discardAtLeast; lim++ )
		discarded += all[lim].StorageCost;

	// If we're out of slots, evict at least 1/4 of our objects
	if ( outOfSlots )
	{
		lim = max(lim, all.size() / 4);
		lim = max(lim, 1);
	}

	lim = min(lim, all.size());

	if ( DebugTraceOn )
		printf( "Evicting %d/%d items\n", (int) lim, (int) all.size() );

	for ( int i = 0; i < lim; i++ )
	{
		EvictSingle( all[i].SlotPos );
	}
}

void GenCache::EvictSingle( int slot )
{
	ASSERT( Slots.Get(slot) );
	ASSERT( Map.contains(HSigAt(slot)) );
	
	ItemHead* head = HeadAt(slot);
	ASSERT( head->Lock == 0 );

	TotalStorage -= head->StorageCost;
	TotalGenerationTime -= head->GenerationTime;
	FRelease( head->ReleaseContext, head->Data );
	bool era = Map.erase( HSigAt(slot) );
	ASSERT( era );
	Slots.Set( slot, false );
	SlotsUsed--;
	ASSERT( SlotsUsed >= 0 );
}

void GenCache::GetAll( dvect<ItemHead>& all, bool include_locked )
{
	for ( int i = 0; i < Slots.Size(); i++ )
	{
		if ( Slots.Get(i) )
		{
			if ( include_locked || HeadAt(i)->Lock == 0 )
				all += *HeadAt(i);
		}
	}
}

void GenCache::GrowSlots()
{
	ASSERT( Slots.Size() < MaxItems );
	int oldSize = Slots.Size();
	void* oldHead = Head;
	int nsize = max( 16, Slots.Size() * 2 );
	nsize = min( nsize, MaxItems );
	//Map.clear_noalloc();
	Map.clear();
	byte* nhead = (byte*) AbcMallocOrDie( ItemSize * nsize );
	memcpy( nhead, Head, ItemSize * oldSize );
	// Reinsert items into the hash map. We need to do this because the pointers to the signatures
	// have changed when we re-allocated the item list.
	Slots.Resize( nsize, false );
	Head = nhead;
	for ( int i = 0; i < oldSize; i++ )
	{
		ASSERT( Slots.Get(i) ); // should be full
		ASSERT( !Map.contains(HSigAt(i)) );
		Map.insert( HSigAt(i), i + SlotMapOffset );
	}
	free( oldHead );
	
	if ( DebugVerifySanity_On )
	{
		ASSERT( Map.size() == oldSize );
		for ( int i = 0; i < oldSize; i++ )
		{
			int slot = Map.get( HSigAt(i) ) - SlotMapOffset;
			ASSERT( slot == i );
		}
	}
}

GenCache::TDate GenCache::NextDate()
{
	if ( CurDate >= DateWrapAround )
	{
		// overflow
		CurDate = 0;

		QTotalDateWraps++;
		
		// At overflow time, we lose all relativity. This is a compromise, but I think it's fine.
		for ( int i = 0; i < Slots.Size(); i++ )
		{
			if ( Slots.Get(i) )
				HeadAt(i)->LastUsed = 0;
		}
	}
	return CurDate++;
}

void GenCache::DebugDumpStatsHeader()
{
	printf		( "%10s %10s %6s %10s %10s %10s %10s\n", "GenTime", "Storage", "Items", "Date", "TotRead", "TotInsert", "TotEvict" );
	AbcTrace	( "%10s %10s %6s %10s %10s %10s %10s\n", "GenTime", "Storage", "Items", "Date", "TotRead", "TotInsert", "TotEvict" );
}

void GenCache::DebugDumpStats()
{
	//printf( "<" );
	TakeCriticalSection cs( Lock );
	XStringA msg = fmt( "%10v %10v %6v %10v %10v %10v %10v\n", TotalGenerationTime, TotalStorage, SlotsUsed, CurDate, QTotalReads, QTotalInserts, QTotalEvictions );
	puts( msg );
	AbcOutputDebugString( msg );
	//printf( ">" );
}

void GenCache::DebugVerifySanity()
{
	if ( !DebugVerifySanity_On ) return;
	ASSERT( SlotsUsed <= Slots.Size() );
	ASSERT( Slots.Size() <= MaxItems );

	for ( int i = 0; i < Slots.Size(); i++ )
	{

	}
}
