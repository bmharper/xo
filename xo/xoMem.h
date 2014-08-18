#pragma once

// A memory pool
class NUAPI nuPool
{
public:
	nuPool();
	~nuPool();

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

// A vector that allocates its storage from a nuPool object
template<typename T>
class nuPoolArray
{
public:
	nuPool*		Pool;
	T*			Data;
	uintp		Count;
	uintp		Capacity;

	nuPoolArray()
	{
		Pool = NULL;
		Data = NULL;
		Count = 0;
		Capacity = 0;
	}

	nuPoolArray& operator+=( const T& v )
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
		NUASSERTDEBUG(Count > 0);
		Count--;
	}

	T& add( const T* v = NULL )
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
		Data = NULL;
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
		NUCHECKALLOC(ndata);
		memcpy( ndata, Data, sizeof(T) * Capacity );
		memset( ndata + Capacity, 0, sizeof(T) * (ncap - Capacity) );
		Capacity = ncap;
		Data = ndata;
	}
};

// Last-in-first-out buffer. This is used instead of stack storage.
// There is a hard limit here - the buffer size you request is
// allocated up front. The assumed usage is identical to that of a stack.
// This was created to lessen the burden on the actual thread stack during
// layout.
// You must free objects in the reverse order that you allocated them.
class NUAPI nuLifoBuf
{
public:
			nuLifoBuf();
			nuLifoBuf( size_t size );	// This simply calls Init(size)
			~nuLifoBuf();

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

// Vector that uses nuLifoBuf for storage. This is made for PODs - it does not do
// object construction or destruction.
template<typename T>
class nuLifoVector
{
public:
	nuLifoVector( nuLifoBuf& lifo )
	{
		Lifo = &lifo;
	}
	~nuLifoVector()
	{
		Lifo->Free( Data );
	}

	void AddN( intp numElementsToAdd )
	{
		if ( Data == nullptr )
			Data = (T*) Lifo->Alloc( numElementsToAdd * sizeof(T) );
		else
			Lifo->GrowLast( numElementsToAdd * sizeof(T) );
		Count += numElementsToAdd;
	}

	void Push( const T& t )
	{
		intp c = Count;
		AddN( 1 );
		Data[c] = t;
	}

	T& Back()
	{
		return Data[Count - 1];
	}

	nuLifoVector& operator+=( const T& t )
	{
		Push( t );
		return *this;
	}

	T& operator[]( intp i ) { return Data[i]; }

private:
	nuLifoBuf*	Lifo;
	T*			Data = nullptr;
	intp		Count = 0;
};
