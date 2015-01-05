   #pragma once

// A memory pool
class XOAPI xoPool
{
public:
	xoPool();
	~xoPool();

	void	SetChunkSize( size_t size );

	void*	Alloc( size_t bytes, bool zeroInit );
	
	template<typename T>
	T*		AllocT( bool zeroInit ) { return (T*) Alloc( sizeof(T), zeroInit ); }

	template<typename T>
	T*		AllocNT( size_t count, bool zeroInit ) { return (T*) Alloc( count * sizeof(T), zeroInit ); }

	void	FreeAll();
	void	FreeAllExceptOne();

protected:
	size_t			ChunkSize;
	size_t			TopRemain;
	podvec<void*>	Chunks;
	podvec<void*>	BigBlocks;
};

// A vector that allocates its storage from a xoPool object
template<typename T>
class xoPoolArray
{
public:
	xoPool*		Pool;
	T*			Data;
	uintp		Count;
	uintp		Capacity;

	xoPoolArray()
	{
		Pool = nullptr;
		Data = nullptr;
		Count = 0;
		Capacity = 0;
	}

	xoPoolArray& operator+=( const T& v )
	{
		add( &v );
		return *this;
	}

	T& operator[]( intp _i )
	{
		return Data[_i];
	}

	const T& operator[]( intp _i ) const
	{
		return Data[_i];
	}

	T& back()
	{
		return Data[Count - 1];
	}

	const T& back() const
	{
		return Data[Count - 1];
	}

	void pop()
	{
		XOASSERTDEBUG(Count > 0);
		Count--;
	}

	T& add( const T* v = nullptr )
	{
		if ( Count == Capacity )
			grow();

		if ( v )
			Data[Count++] = *v;
		else
			Data[Count++] = T();

		return Data[Count - 1];
	}

	intp size() const { return Count; }

	void resize( intp n )
	{
		if ( n != Count )
		{
			clear();
			if ( n != 0 )
			{
				growto( n );
				Count = n;
			}
		}
	}

	void reserve( intp n )
	{
		if ( n > (intp) Capacity )
			growto( n );
	}

	void clear()
	{
		Data = nullptr;
		Count = 0;
		Capacity = 0;
	}

protected:
	void grow()
	{
		uintp ncap = std::max(Capacity * 2, (uintp) 2);
		growto( ncap );
	}

	void growto( uintp ncap )
	{
		T* ndata = (T*) Pool->Alloc( sizeof(T) * ncap, false );
		XOCHECKALLOC(ndata);
		memcpy( ndata, Data, sizeof(T) * Capacity );
		memset( ndata + Capacity, 0, sizeof(T) * (ncap - Capacity) );
		Capacity = ncap;
		Data = ndata;
	}
};

/*
// A vector that allocates its storage from a xoPool object
// This has less storage requirements than an xoPoolArray (16 bytes vs 32 bytes on 64-bit platforms)
template<typename T>
class xoPoolArrayLite
{
public:
	T*			Data;
	uint		Count;
	uint		Capacity;

	xoPoolArrayLite()
	{
		Data = nullptr;
		Count = 0;
		Capacity = 0;
	}

	T& operator[]( intp _i )
	{
		return Data[_i];
	}

	const T& operator[]( intp _i ) const
	{
		return Data[_i];
	}

	void add( const T& item, xoPool* pool )
	{
		if ( Count == Capacity )
			grow( pool );
		Data[Count++] = item;
	}

	intp size() const { return Count; }

protected:
	void grow( xoPool* pool )
	{
		uint ncap = std::max(Capacity * 2, (uint) 2);
		growto( ncap );
	}

	void growto( uint ncap, xoPool* pool )
	{
		T* ndata = (T*) pool->Alloc( sizeof(T) * ncap, false );
		XOCHECKALLOC(ndata);
		memcpy( ndata, Data, sizeof(T) * Capacity );
		memset( ndata + Capacity, 0, sizeof(T) * (ncap - Capacity) );
		Capacity = ncap;
		Data = ndata;
	}
};
*/

// Last-in-first-out buffer. This is used instead of stack storage.
// There is a hard limit here - the buffer size you request is
// allocated up front. The assumed usage is identical to that of a stack.
// This was created to lessen the burden on the actual thread stack during
// layout.
// You must free objects in the reverse order that you allocated them.
// GET RID OF THIS ALONG WITH xoLayout2
class XOAPI xoLifoBuf
{
public:
			xoLifoBuf();
			xoLifoBuf( size_t size );	// This simply calls Init(size)
			~xoLifoBuf();

	// Initialize the buffer.
	// This will panic if the buffer is not empty.
	void	Init( size_t size );

	// Allocate a new item.
	// It is legal to allocate 0 bytes. You can then us Realloc to grow. Regardless
	// of the size of your initial allocation, you must always call Free() on anything
	// that you have Alloc'ed.
	// This will panic if the buffer size is exhausted.
	void*	Alloc( size_t bytes );
	
	// Grow the most recently allocated item to the specified size.
	// This will panic if buf is not the most recently allocated item, or if the buffer size is exhausted.
	void	Realloc( void* buf, size_t bytes );

	// This is a less safe version of Realloc. The only check it performs is whether we will run out of space.
	void	GrowLast( size_t moreBytes );

	// Free the most recently allocated item
	// This will panic if buf is not the most recently allocated item, and buf is not null.
	void	Free( void* buf );

private:
	podvec<intp>	ItemSizes;
	void*			Buffer;
	intp			Size;
	intp			Pos;
};

// Vector that uses xoLifoBuf for storage. This is made for PODs - it does not do
// object construction or destruction. It also does not zero, unless you use
// AddZeroed().
// meh.. this thing is all kinds of screwed up.. mix of ideas
// GET RID OF THIS ALONG WITH xoLayout2
template<typename T>
class xoLifoVector
{
public:
	xoLifoVector( xoLifoBuf& lifo )
	{
		Lifo = &lifo;
	}
	~xoLifoVector()
	{
		Lifo->Free( Items );
	}

	intp Size() const { return Count; }

	T& Add()
	{
		AddN( 1 );
		return Back();
	}

	T& AddZeroed()
	{
		T& t = Add();
		memset( &t, 0, sizeof(t) );
		return Back();
	}

	void AddN( intp numElementsToAdd )
	{
		if ( Items == nullptr )
			Items = (T*) Lifo->Alloc( numElementsToAdd * sizeof(T) );
		else
			Lifo->GrowLast( numElementsToAdd * sizeof(T) );
		Count += numElementsToAdd;
	}

	void Push( const T& t )
	{
		intp c = Count;
		AddN( 1 );
		Items[c] = t;
	}

	void Pop()
	{
		XOASSERT(Count > 0);
		Count--;
	}

	T& Back()
	{
		return Items[Count - 1];
	}

	xoLifoVector& operator+=( const T& t )
	{
		Push( t );
		return *this;
	}

	T& operator[]( intp i ) { return Items[i]; }

private:
	xoLifoBuf*	Lifo;
	T*			Items = nullptr;
	intp		Count = 0;
};

/* This is a special kind of stack that only initializes objects
the first time they are used. Thereafter, objects are recycled.
When the stack is deleted, it only calls the destructor on the
objects that were actually initialized and used.
You will probably need to add your own "Reset" function to your
objects, and call it on the object immediately after Add(). This
Reset function should reset the object to a clean slate, but
leave any heap-allocated memory intact. For example, podvec's
clear_noalloc() function.
*/
template<typename T>
class xoStack
{
public:
	T*			Items = nullptr;
	intp		Count = 0;
	intp		Capacity = 0;
	intp		HighwaterMark = 0;		// The maximum that Count has ever been

	~xoStack()
	{
		for ( intp i = 0; i < HighwaterMark; i++ )
			Items[i].~T();
		free( Items );
	}

	void Init( intp capacity )
	{
		XOASSERT(Items == nullptr);
		Capacity = capacity;
		Items = (T*) malloc(sizeof(T) * capacity);
		XOCHECKALLOC(Items);
	}

	T& Add()
	{
		XOASSERT(Count < Capacity);
		if ( Count == HighwaterMark )
		{
			new (Items + Count) T();
			HighwaterMark++;
		}
		Count++;
		return Back();
	}

	void Pop()
	{
		XOASSERT(Count > 0);
		Count--;
	}

	T& Back()
	{
		return Items[Count - 1];
	}

	T& operator[]( intp i ) { return Items[i]; }
};

// Circular buffer
// This is only built for PODs, and it does not zero-initialize.
// Automatically grows size. Size must be a power of 2.
// Maximum number of items in the buffer is size - 1.
// There is a debug-only check for popping an empty queue.
template<typename T>
class xoRingBuf
{
public:
	// Arbitrarily chosen constant. Reasoning is that you wouldn't use a
	// ring buffer for less than 32 items. This was originally built
	// to hold glyphs during layout.
	static const uint32 DefaultInitialSize = 32;

	xoRingBuf()
	{
	}
	
	~xoRingBuf()
	{
		free(Ring);
	}

	T& PushHead()
	{
		if ( ((Head + 1) & Mask) == Tail )
			Grow();
		T& item = Ring[Head];
		Head = (Head + 1) & Mask;
		return item;
	}

	const T& PopTail()
	{
		XOASSERTDEBUG( Head != Tail );
		const T& item = Ring[Tail];
		Tail = (Tail + 1) & Mask;
		return item;
	}

	// Access an element with a position that is relative to the head
	// A value of 0 returns the head element.
	// A value of 1 returns the head element - 1.
	// etc
	T& FromHead( int relative )
	{
		uint32 p = (Head - relative - 1) & Mask;
		return Ring[p];
	}

	intp Size() const { return (intp) ((Head - Tail) & Mask); }

private:
	T*			Ring = nullptr;
	uint32		Mask = 0;
	uint32		Head = 0;
	uint32		Tail = 0;

	uint32 RingSize() const { return Mask + 1; }

	void Grow()
	{
		if ( Ring == nullptr )
		{
			Ring = (T*) xoMallocOrDie( DefaultInitialSize * sizeof(T) );
			Mask = DefaultInitialSize - 1;
		}
		else
		{
			uint32 orgSize = RingSize();
			Ring = (T*) xoReallocOrDie( Ring, orgSize * 2 * sizeof(T) );
			if ( Head < Tail )
			{
				// Handle the scenario where the head is behind the tail (numerically)
				// [  H T  ]   =>  [    T     H    ]
				// [c - a b]   =>  [- - a b c - - -]
				for ( uint32 i = 0; i < Head; i++ )
					Ring[orgSize + i] = Ring[i];
				Head += orgSize;
			}
			Mask = (orgSize * 2) - 1;
		}
	}
};
