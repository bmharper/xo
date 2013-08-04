#pragma once

#include "lmTypes.h"
#include "../HashTab/HashFunctions.h"
#include "../HashTab/ohashmap.h"
#include "../Bits/BitMap.h"
#include "ReaderWriterLock.h"

typedef void (*FGenCacheRelease) ( void* context, void* data );


/** Yet another cache class.

Design
------

	* Item names are a fixed-size BLOB. You are free to pad names up with zeros.
	* Items have a generation cost (how long did it take to generate this data).
	* Items have a storage cost (how much space is it taking to store this cached data).
	* Items have a 32-bit lock count. An item will only be evicted from the cache if its lock count is zero.
	* Generation cost and Storage cost and unsigned 32-bit integers. You must scale your metrics to fit into that range.
	* The cache has one big lock. The only costly operation that executes inside this lock is a cache audit.

Usage
-----

	* Insert objects using Insert().
	* Use Acquire() to acquire your object.
	* Use Release() to release it, iff your object was found (ie return value of Acquire() was not NULL).


Notes
-----

How does the equation work? Your end goal is to minimize generation time. Sigh.. I should find some papers about this.
Right now it's just going to be dead simple. LRU eviction.

More thoughts...
Hmm.. There are these two competing and different paradigms here. The one is the cost/benefit ratio, and the other
is the likelihood that the object is going to be used again in the near future. As always, the end goal is to
spend as little time generating objects as possible. If there isn't a good writeup about this, then somebody ought
to do it... but there must be.

Considerations for the size of our Time and Storage units:
Table of Header Size, for 16-byte signatures (time and storage unit bits - 32-bit machine):
16 bits: 36 bytes
32 bits: 40 bytes

Therefore, it is not worth it to make these 16 bits. On 64-bit, it makes even less sense.

**/
class PAPI GenCache
{
public:
	GenCache();
	~GenCache();

	// I initially made these 16 bits, but it isn't worth it.
	typedef uint32 TGenTime;
	typedef uint32 TStorage;
	typedef uint32 TDate;
	typedef uint64 TAccum;		// making this 32-bit is not worth the pain. We're not dividing 64-bit integers (which is fairly expensive on a 32-bit machine).

	struct ItemHead
	{
		bool operator< ( const ItemHead& b ) const
		{
			return LastUsed < b.LastUsed;
		}

		TGenTime GenerationTime;
		TStorage StorageCost;
		TDate LastUsed;
		int32 SlotPos;		///< Redundant, but used for convenience when sorting. This is base zero (ie SlotMapOffset is not applied).
		int32 Lock;
		void* ReleaseContext;
		void* Data;
		// signature follows
	};

	// These 6 functions are not synchronized inside the critical section
	void Init( FGenCacheRelease releaseFunc, int sigSize, int maxItems, TAccum maxStorage );
	void Reset();
	void SetMaxItems( int maxItems );
	void SetMaxStorage( TAccum maxStorage );
	TAccum GetMaxStorage()		{ return MaxStorage; }
	int GetMaxItems()			{ return MaxItems; }

	/// If true, then we RaiseError() if you try to insert an item that is resident and locked.
	int ErrorOnOverwriteLocked;


	// Operation

	/** Insert a new element into the cache, or optionally overwrite an existing element.
	@param sig The signature
	@param overwrite If false, and the object exists, then we do not overwrite an existing element, but return false. You cannot overwrite
		an item that is currently locked. An attempt to do so will debug assert and return false.
	@param lock If true, then the object's initial lock count is 1. You must call Release() to release it.
	@param releaseContext The context passed back to the release function.
	@param data The data passed back to the release function.
	@param genTime The time (or effort) it took to generate this item.
	@param storageCost The cost to store this item.
	@return true If any change was made (ie fresh insertion, or an overwrite)
	**/
	bool Insert( const void* sig, bool overwrite, bool initial_lock, void* releaseContext, void* data, TGenTime genTime, TStorage storageCost );

	/** Explicit removal.
	An attempt to remove a locked item will debug assert.
	@return True if the object was resident in the cache and its lock count was zero.
	**/
	bool Remove( const void* sig );

	/** Removes all unlocked items.
	@return True if the cache was emptied.
	**/
	bool RemoveAll();

	/** Retrieve an item from the cache.
	
	When you are finished using the item, you must call Release on it.
	If Get() returns NULL, do not call Release.

	@param sig The signature
	@return The item, or NULL if it is not present in the cache
	**/
	void* Acquire( const void* sig );

	/// Release an item that has been acquired.
	void Release( const void* sig );

	/** Checks if the item exists in the cache.
	This is basically just here to do a 'Touch' on the object.
	BARE IN MIND: By the time this function returns, the result may be invalid. So treat this for what it is: heuristics gathering.
	@param touch Updates the object's last accessed date if true
	@return True if the item was found (of course it may no longer be there by the time the function returns).
	**/
	bool CheckExists( const void* sig, bool touch );
	
	/** Multiple CheckExistsAndTouch. This is more efficient than single CheckExists, because we only enter/exit the critical section once.
	BARE IN MIND: Same comment as above
	@param found May be NULL.
	@return Number of items existing.
	**/
	int CheckExists( int n, const void** sigs, bool* found, bool touch );

	void DebugDumpStatsHeader();
	void DebugDumpStats();
	void DebugVerifySanity();
	int DebugTotalEvictions()	{ return QTotalEvictions; }
	int DebugTotalReads()		{ return QTotalReads; }
	int DebugTotalGoodReads()	{ return QTotalGoodReads; }
	int DebugTotalInserts()		{ return QTotalInserts; }
	int DebugTotalDateWraps()	{ return QTotalDateWraps; }
	
	bool DebugVerifySanity_On;
	bool DebugTraceOn;
	bool DebugCrashHard;

	/// Exposed in order to test date wrap around
	void DebugSetDateWrapAround( TDate wrapAround ) { DateWrapAround = wrapAround; }

protected:

	static const int SlotMapOffset = 1;

	TAccum TotalGenerationTime;
	TAccum TotalStorage;
	byte* Head;
	FGenCacheRelease FRelease;
	bool Threaded;
	int SigSize;
	int ItemSize;		///< sizeof(ItemHead) + SigSize
	HashSigIntMap Map;	///< Maps signature to slot number + 1
	BitMap Slots;		///< Doubles as an indicator of how many slots we have allocated in Head
	int SlotsUsed;
	TDate CurDate;
	TDate DateWrapAround;
	CRITICAL_SECTION Lock;

	volatile LONG QTotalEvictions, QTotalReads, QTotalGoodReads, QTotalInserts, QTotalDateWraps;

	// Limits
	TAccum MaxStorage;
	int MaxItems;
	
	/// Number of slots allocated in Head
	int AllocatedItems() { return Slots.Size(); }

	TDate NextDate();

	int			FindSlot( const void* sig )		{ return Map.get(HashSig(sig, SigSize)) - SlotMapOffset; }
	ItemHead*	HeadAt( int slot )				{ return (ItemHead*) (Head + ItemSize * slot); }
	void*		SigAt( int slot )				{ return ((byte*) HeadAt(slot)) + sizeof(ItemHead); }
	HashSig		HSigAt( int slot )				{ return HashSig(SigAt(slot), SigSize); }

	void RaiseError();

	void EvictSingle( int slot );
	void EvictForSpace( bool outOfSlots, TStorage ensureSpaceFor );
	void GrowSlots();

	void* GetInternal( const void* sig, bool acquireCS, bool acquireObject, bool touch );

	void GetAll( dvect<ItemHead>& all, bool include_locked );
};
