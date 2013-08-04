#pragma once

#include "../Containers/dvec.h"
#include "../HashTab/ohashmultimap.h"
#include "../HashTab/ohashmap.h"

class TinyGC;

class TinyGC_User
{
public:
	/** Called at the beginning of a collection.
	It is time for you to inform the TinyGC all the pointers that you use (pointers to objects allocated on its heap).
	**/
	virtual void GC_Mark( TinyGC* tinyGC ) = 0;
};

#pragma warning( push )
#pragma warning( disable: 4267 ) // size_t to UINT32. Erroneous

//#define GCTRACE TRACE
#define GCTRACE(...) ((void)0)

/** A tiny garbage collected heap.

Rules:
Objects allocated on the heap may not reference each other. This is due to the way we perform
pointer fixups during a sweep phase. The client passes us a list of pointers and we fix them up
on the client's behalf. We COULD allow inter-referentials, but for the purposes for which this
thing was created, I have not yet found it necessary.


**/
class PAPI TinyGC
{
public:

	typedef UINT32 TData;

	enum Flags
	{
		FlagIgnoreWasteFactor = 1,
		FlagMaximumShrink = 2
	};

	/** Toggle if you always explicitly free your objects.
	Turning this on will cause collections to occur when they are more relevant.
	Default = false.
	**/
	bool			UseExplicitFree;
	TinyGC_User*	User;
	double			WasteFactor;		// Threshold of wasted space for automatic collections. If [unused space] / Capacity > WasteFactor, then we do a collection.
	UINT32			WasteAbsolute;		// Threshold of wasted space for automatic collections. If [unused space] > WasteAbsolute, then we do a collection.
	double			GrowFactor;			// Growth Factor.
	UINT			DisableGC;			// Can be used to turn off collections. Any non-zero value causes GC not to occur.
	bool			CollectionPending;	// Collection did not happen because DisableGC was not zero.
	UINT32			Capacity;
	UINT32			Size;
	UINT32			Junk;				// Amount of data that we know is junk (ie because user explicitly freed it).
	TData*			Data;
	UINT32			Collections;
	ohash::ohash_vecmap<UINT32, UINT32*> MarkMap;

	TinyGC();
	~TinyGC();

	void Reset();
	bool IsWasteful( UINT32 unused );
	bool Collect( UINT flags = 0 );

	UINT32 Alloc( UINT32 n, bool zero = true );
	UINT32 Realloc( UINT32 id, UINT32 n, bool zero = true );

	UINT32 BlockSize( UINT32 id ) { return id == 0 ? 0 : Data[id - 1]; }
	TData* BlockPtr( UINT32 id )	{ return id == 0 ? 0 : Data + id; }

	UINT32 CapacityBytes() { return Capacity * sizeof(TData); }

	/** Indicate that you use an object.
	The pointer x may not point to memory that has been allocated by this heap.
	**/
	void GC_Mark( UINT32* x )
	{
		if ( *x == 0 ) return;
		MarkMap.insert( *x, x );
	}

	void Free( UINT32 id )
	{
		// +1 because of the block size slot
		if ( id == 0 ) return;
		Junk += 1 + BlockSize( id );
	}

protected:

	UINT* ReallocBusy;

	void Sweep( UINT32 newRequiredCapacity, dvect<UINT32>& keys, UINT flags );

};

#pragma warning( pop )
