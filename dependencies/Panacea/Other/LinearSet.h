#pragma once

#include "../HashTab/ohashset.h"
#include "../Containers/dvec.h"

/** A hashed set that maintains strict ordering.

@param Exclusive If true, then we are strict about not inserting the same
object twice. Turning this off is a performance optimization, and should not
be used unless you are absolutely sure that you won't be inserting the same
object twice. Turning Exclusive off turns the object into a list-biased object,
in that by default it only stores a list, and only when Erase or Contains is called,
is the set generated, and thereafter kept in sync.

@param LinCutoff For small POD objects (ie INT32, Ptr, etc), searching through a linear set
is faster than doing a hash table lookup, because of the hash table overhead. It is also faster
at a certain threshold (a lower one) to do memory bumps than to remove an item from a hash table.
It is for this reason that we provide a linear cutoff threshold. So long as the number of objects 
inside the list is less than this, and we are operating in Exclusive = false mode, we avoid using 
a hashed set. To avoid complexity, this is only implemented for the following common case:
	- Create the LinearSet in Exclusive = false mode,
	- Use only the following functions:
		- Add()
		- Contains()
		- Erase( const T& val )

The meaning of things:

ListMode:
	When in ListMode, Set is empty. 
	ListMode is never set unless Exclusive = false.
	ListMode is set to true upon construction, or when the list is cleared.
	Calls to Erase( const T& val), Contains( const T& val ), Synchronize(), and GetSet() bring us out of ListMode.
	
**/
template< typename T, typename THashFunc = ohash::ohashfunc_cast<T>, int LinCutoff = 20 >
class LinearSet
{
public:

	typedef ohash::ohashset<T, THashFunc> TSet;
	typedef dvect<T> TList;

	LinearSet()
	{
		Exclusive = true;
		Clear();
	}

	~LinearSet()
	{
	}

	/// Size
	size_t Size() const
	{ 
		return ListMode ? List.size() : Set.size(); 
	}

	void SetExclusiveMode( bool exclusive )
	{
		Exclusive = exclusive;
	}

	/// Clear 
	void Clear()
	{
		List.clear();
		Set.clear();
		ListDirty = false;
		ListMode = !Exclusive;
	}

	/** Adds an item if it does not already exist in the set.

	@return True if the item does not exist.
	**/
	bool Add( const T& val )
	{
		if ( !ListMode )
		{
			if ( Set.contains( val ) ) return false;
			Set.insert( val );
		}
		List.push_back( val );
		if ( !ListMode && !ListDirty ) ASSERT( List.size() == Set.size() );
		return true;
	}

	/** Inserts a non-existent item at a specified location.

	Do not mix this with erasures (for performance).

	@return True if the item does not exist.
	**/
	bool Insert( size_t beforeItem, const T& val )
	{
		if ( Set.contains( val ) ) return false;
		CleanListAuto();
		if ( !ListMode )
			Set.insert( val );
		List.insert( beforeItem, val );
		return true;
	}

	/** Erases an item by value.

	Do not mix this with calls to Insert( int, val ) (for performance).

	@return True if the item was in the set.
	**/
	bool Erase( const T& val )
	{
		if ( ListMode && List.size() < LinCutoff )
		{
			int pos = List.find( val );
			if ( pos < 0 ) return false;
			List.erase( pos );
			return true;
		}
		else
		{
			BuildSetAuto();
			if ( !Set.contains( val ) ) return false;
			Set.erase( val );
			ListDirty = true;
			return true;
		}
	}

	/** Erases a range by index.
	**/
	void EraseByIndex( size_t index, size_t index_last_inclusive = 0 )
	{
		CleanListAuto();
		if ( index_last_inclusive < index ) index_last_inclusive = index;
		if ( !ListMode )
		{
			for ( size_t ix = index; ix <= index_last_inclusive; ix++ )
			{
				Set.erase( List[(TIX) ix] );
			}
		}
		List.erase( (TIX) index, (TIX) (index_last_inclusive + 1) );
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
	T Get( int index )
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
	T operator[]( int index ) { return Get( index ); }

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
		return Set.contains( val ); 
	}

	/** Ensure that the hash set and linear vector are synchronized.

	This must be called before the list can be used by multiple threads 
	(and then of course only for read-only usage).
	**/
	void Synchronize()
	{
		CleanListAuto();
		BuildSetAuto();
	}

	/** Retrieve the underlying set object.

	This will cause a build of set, if we happen to be running in ListMode.<br>
	Thread Safety: This is a @a const function if the object is made with Excusive = true.
	**/
	const TSet& GetSet() { BuildSetAuto(); return Set; }
	
	intp DebugGetSetSize() const { return Set.size(); }

	/** Retrieve the underlying list object.

	This will cause a rebuild of the list.\n
	Thread Safety: This is only thread-safe if Synchronize() has been called.
	**/
	const TList& GetList() { CleanListAuto(); return List; }

protected:
	TSet Set;
	TList List;
	bool Exclusive;
	bool ListMode;
	bool ListDirty;

	// this should be removed if/when dvect is made to use size_t instead of int.
	typedef int TIX;

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
		Set.clear();

		if ( List.size() == 0 )
		{
			ListMode = !Exclusive;
			return;
		}

		for ( size_t i = 0; i < (size_t) List.size(); i++ )
		{
			// Exclusive has been used inappropriately! An object has been duplicated.
			ASSERT( !Set.contains( List[(TIX) i] ) );
			Set.insert( List[(TIX) i] );
		}

		ASSERT( Set.size() == List.size() );

		ListMode = false;
	}

	void CleanList()
	{
		ASSERT( !ListMode );
		ListDirty = false;

		if ( Set.size() == 0 )
		{
			List.clear();
			return;
		}

		// rebuild in reverse, since later objects added with Add() should
		// have more preference than earlier ones. This is only relevant when
		// dealing with duplicates in the list.
		dvect<T> nl;
		TSet used;
		nl.reserve( Set.size() );
		for ( size_t i = List.size() - 1; i != -1; i-- )
		{
			if ( Set.contains( List[(TIX) i] ) && !used.contains( List[(TIX) i] ) )
			{
				used.insert( List[(TIX) i] );
				nl.push_back( List[(TIX) i] );
			}
		}
		reverse( nl );
		List = nl;

		ASSERT( Set.size() == List.size() );
	}

};
