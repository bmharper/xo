#pragma once

#include "../Bits/BitTree.h"
#include "../Containers/podvec.h"

/** A bitmap tree that maintains strict ordering.

Exclusive:
	If Exclusive = true, then we are strict about not inserting the same
	object twice. Turning this off is a performance optimization, but obviously can't
	be used unless you are sure that you won't be inserting the same
	object twice. Turning Exclusive off turns the object into a list-biased object,
	in that by default it only stores a list, and only when Erase or Contains is called,
	is the set generated, and thereafter kept in sync.

ListMode:
	When in ListMode, Bits is empty. 
	ListMode is never set unless Exclusive = false.
	ListMode is set to true upon construction, or when the list is cleared.
	Calls to Erase( const T& val), Contains( const T& val ), Synchronize(), and GetBitTree() bring us out of ListMode.
	
**/
template< typename T >
class LinearBitSet
{
public:
	typedef podvec<T> TList;

	static const intp DefaultLinCutoff = 20;

	/* For small POD objects (ie INT32, Ptr, etc), searching through a linear set
	is faster than doing the bit tree lookup. It is also faster
	at a certain threshold (a lower one) to do memory bumps than to remove an item from the bit tree.
	It is for this reason that we provide a linear cutoff threshold. So long as the number of objects 
	inside the list is less than this, and we are operating in Exclusive = false mode, we avoid using 
	the tree. To avoid complexity, this is only implemented for the following common case:
		- Create the LinearBitSet in Exclusive = false mode,
		- Use only the following functions:
			- Add()
			- Contains()
			- Erase( const T& val )

	*/
	intp LinCutoff;

	LinearBitSet()
	{
		Bits.NewSpaceStatus = false;
		LinCutoff = DefaultLinCutoff;
		Exclusive = true;
		Clear();
	}

	~LinearBitSet()
	{
	}

	intp Size() const
	{ 
		return ListMode ? List.size() : (intp) Bits.GetSlotOnCount(); 
	}

	void SetExclusiveMode( bool exclusive )
	{
		Exclusive = exclusive;
	}

	void Clear()
	{
		List.clear();
		Bits.Reset();
		ListDirty = false;
		ListMode = !Exclusive;
	}

	// Adds an item if it does not already exist in the set.
	// returns True if the item does not exist.
	bool Add( const T& val )
	{
		if ( !ListMode )
		{
			if ( Bits.GetCapped( val ) ) return false;
			Bits.Set( val, true );
		}
		List.push_back( val );
		if ( !ListMode && !ListDirty ) ASSERT( List.size() == Bits.GetSlotOnCount() );
		return true;
	}

	// Inserts a non-existent item at a specified location.
	// For performance, do not mix this with erasures
	// returns True if the item does not exist.
	bool Insert( intp beforeItem, const T& val )
	{
		if ( Bits.GetCapped( val ) ) return false;
		CleanListAuto();
		if ( !ListMode )
			Bits.Set( val, true );
		List.insert( beforeItem, val );
		return true;
	}

	// Erases an item by value.
	// For performance, do not mix this with calls to Insert( int, val )
	// returns True if the item was in the set.
	bool Erase( const T& val )
	{
		if ( ListMode && List.size() < LinCutoff )
		{
			intp pos = List.find( val );
			if ( pos < 0 ) return false;
			List.erase( pos );
			return true;
		}
		else
		{
			BuildSetAuto();
			if ( !Bits.GetCapped( val ) ) return false;
			Bits.Set( val, false );
			ListDirty = true;
			return true;
		}
	}

	// Erases a range by index.
	void EraseByIndex( intp index, intp index_last_inclusive = 0 )
	{
		CleanListAuto();
		if ( index_last_inclusive < index ) index_last_inclusive = index;
		if ( !ListMode )
		{
			for ( intp ix = index; ix <= index_last_inclusive; ix++ )
			{
				Bits.Set( List[ix], false );
			}
		}
		List.erase( index, index_last_inclusive + 1 );
	}

	// I suspect this is a bug in the VC 2005 compiler which emits this warning.
	// This doesn't seem to work. I don't know how to shut the compiler up.
//#ifdef _M_IX86
//#pragma warning( push )
//#pragma warning( disable: 4267 )
//#endif

	/** Return an item by index.

	Thread Safety: This function is only const if Synchronize has been called.

	**/
	T Get( intp index )
	{
		CleanListAuto();
		// This stupid cast here is only necessary to shut up the VC 2005 compiler. I suspect what's happening is that
		// it is making dvect<size_t> and dvect<UINT32> aliases, which it should indeed. The problem is that size_t is
		// treated specially by the compiler, in that it emits 64-bit warnings when it is converted to UINT32, and hence
		// the trauma.
		return (T) List[ index ];
	}

//#ifdef _M_IX86
//#pragma warning( pop )
//#endif

	/** Return an item by index.

	Thread Safety: This function is only const if Synchronize has been called.

	**/
	T operator[]( intp index ) { return Get( index ); }

	/** Returns true if we contain the value.

	Thread Safety: This is a const function if the object is made with Exclusive = true.

	**/
	bool Contains( const T& val )
	{ 
		if ( ListMode ) 
		{
			if ( List.size() < LinCutoff )
			{
				return List.find( val ) >= 0;
			}
			BuildSetAuto();
		}
		return Bits.GetCapped( val ); 
	}

	// Ensure that the bitmap tree and linear vector are synchronized.
	// This must be called before the list can be used by multiple threads (and then of course only for read-only usage).
	void Synchronize()
	{
		CleanListAuto();
		BuildSetAuto();
	}

	// Retrieve the underlying set object.
	// This will cause a build of set, if we happen to be running in ListMode.<br>
	// Thread Safety: This is a const function if the object is made with Exclusive = true.
	const AbcBitTree& GetBitTree() { BuildSetAuto(); return Bits; }
	
	// Name must be this for tests that are common across LinearSet and LinearBitSet
	intp DebugGetSetSize() const { return Bits.GetSlotOnCount(); }

	// Retrieve the underlying list object.
	// This will cause a rebuild of the list.\n
	// Thread Safety: This is only thread-safe if Synchronize() has been called.
	const TList& GetList() { CleanListAuto(); return List; }

protected:
	AbcBitTree	Bits;
	TList		List;
	bool		Exclusive;
	bool		ListMode;
	bool		ListDirty;

	void CleanListAuto()
	{
		if ( !ListDirty ) return;
		CleanList();
	}

	void BuildSetAuto()
	{
		if ( !ListMode ) return;
		BuildSet();
	}

	void BuildSet()
	{
		ASSERT( !ListDirty );
		Bits.Reset();

		if ( List.size() == 0 )
		{
			ListMode = !Exclusive;
			return;
		}

		for ( intp i = 0; i < List.size(); i++ )
		{
			// Exclusive has been used inappropriately! An object has been duplicated.
			ASSERT( !Bits.GetCapped( List[i] ) );
			Bits.Set( List[i], true );
		}

		ASSERT( Bits.GetSlotOnCount() == List.size() );

		ListMode = false;
	}

	// Remove objects from the list, if they are not present in the bit tree
	void CleanList()
	{
		ASSERT( !ListMode );
		ListDirty = false;

		if ( Bits.GetSlotOnCount() == 0 )
		{
			List.clear();
			return;
		}

		// rebuild the list in reverse order. This makes the recent changes appear less random in their ordering.
		// I'm not sure whether this is necessary behaviour, but the existing tests for LinearSet verify this condition,
		// so I stick to it.
		TList		newlist;
		AbcBitTree	used;
		used.NewSpaceStatus = false;
		newlist.reserve( Bits.GetSlotOnCount() );
		for ( intp i = List.size() - 1; i >= 0; i-- )
		{
			if ( Bits.GetCapped( List[i] ) && !used.GetCapped( List[i] ) )
			{
				used.Set( List[i], true );
				newlist += List[i];
			}
		}
		reverse( newlist );
		List = newlist;

		ASSERT( Bits.GetSlotOnCount() == List.size() );
	}

};
