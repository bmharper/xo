#include "lmdefs.h"

/*
Linear heap. This was created to be a temporary data structure when you have a lot of small objects that require variable storage.
My specific use case was a Graph data structure in adb, where each node has pointers to all of its edges. This data structure is used
to store the list of node edges during the build. Once we're done we pack it out to a condensed array.

Our buffer can be visualized like this:

0123456789012345
	|      |
-fox---bear----+

-  TERMINATOR     (-1)
+  TERMINATOR_END (-2)

An object on the heap is an integer that points to the last element. This gives constant time inserts, bar the array growth time. During readout
you may the price of walking backwards to discover the object's length, but at least it's linear. If we were to store object start positions instead,
then we would pay an O(n^2) price upon insertion, which could bite for really long sequences, and probably has a worse branch profile.
In the above illustration,
fox would be 3.
bear would be 10.
Head is 11, because we have written 'bear-' so far.
Capacity is 16.
Fox can have 2 more characters added.
Bear could have 3 more characters added.
The end of the list is always terminated with a TERMINATOR,TERMINATOR_END pair
When you add an item to the heap, the system first checks whether there are two terminators beyond the object.
If so, then it adds the item. If not, it allocates more space for the bucket at the end of the array, and adds the new data there.

Usage
-----

LinearHeap<int> heap;
int obj1 = 0;
int obj2 = 0;
heap.Add( obj1, 111 );
heap.Add( obj1, 222 );
heap.Add( obj2, 700 );
heap.Add( obj1, 444 );

int o1_size = heap.Size( obj1 );
int* o1_data = heap.Get( obj1, o1_size );
for ( int i = 0; i < o1_size; i++ )
{
	// do something with o1_data[i] etc
}

Limits
------

The tokens -1 and -2 (in terms of T) are not usable. They are used by the data structure. You could easily make these dynamic if you needed.

*/
template< typename T >
struct LinearHeap
{
	intp	InitialBucketSize;	// Initial bucket size

	LinearHeap()
	{
		Data = NULL;
		InitialBucketSize = 1;
		Reset();
	}
	~LinearHeap()
	{
		Reset();
	}

	void Reset()
	{
		free(Data);
		Data = NULL;
		Capacity = 0;
		Head = 0;
		TotalValues = 0;
		TotalKeys = 0;
	}

	const T* Get( intp idx, intp size ) const { return Data + idx - size + 1; }
				T* Get( intp idx, intp size )		{ return Data + idx - size + 1; }

	intp GetKeyCount() const	{ return TotalKeys; }		// Number of times that you called Add( idx = 0 )
	intp GetValueCount() const	{ return TotalValues; }		// Number of times that you called Add()

	void Add32( u32& idx, T item ) // If you want to use 32-bit indexes for permanent storage
	{
		intp pix = idx;
		Add(pix, item);
		ASSERT(pix < _UI32_MAX);
		idx = pix;
	}

	void Add( intp& idx, T item )
	{
		ASSERT( item != TERMINATOR );
		TotalValues++;
		if ( idx == 0 )
		{
			TotalKeys++;
			idx = Alloc( InitialBucketSize );
			Data[idx] = item;
		}
		else
		{
			ASSERT(Data[idx + 1] == TERMINATOR);
			if ( Data[idx + 2] != TERMINATOR )
			{
				// grow buffer
				intp orgdx = idx;
				intp size = Size(idx);
				idx = Alloc( size * 2 );
				orgdx -= size - 1;
				for ( intp i = 0; i < size; i++ ) Data[idx + i] = Data[orgdx + i];
				idx += size - 1;
				ASSERT(Data[idx + 1] == TERMINATOR);
			}
			// eat into buffer
			idx++;
			Data[idx] = item;
		}
		Head = max(Head,idx + 1);
	}
	intp Size( intp idx ) const
	{
		if ( idx == 0 ) return 0;
		intp i = idx;
		for ( ; Data[i] != TERMINATOR; i-- ) {}
		return idx - i;
	}
protected:
	T*		Data;
	intp	Capacity;			// Number of slots allocated in Data
	intp	Head;				// Top-most written position
	intp	TotalValues;		// Total number of values inserted
	intp	TotalKeys;			// Total number of objects inserted

	static const T TERMINATOR		= -1;
	static const T TERMINATOR_END	= -2;

	intp Alloc( intp n )
	{
		intp lim = Head + n + 3;		// + 3 is necessary to ensure that we always end with a [TERMINATOR, TERMINATOR_END] pair, +2 only gives us a TERMINATOR_END guarantee.
		if ( lim > Capacity ) 
		{
			Capacity = 8 + lim * 2;
			T* newdata = (T*) realloc(Data, Capacity * sizeof(T));
			AbcCheckAlloc(newdata);
			Data = newdata;
			for ( intp i = Head; i < Capacity; i++ ) Data[i] = TERMINATOR;
			Data[Capacity - 1] = TERMINATOR_END;
		}
		return Head + 1;
	}
};
