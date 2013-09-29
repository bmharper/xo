#pragma once

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

/* A very simple heap that is specialized for one type of object only.
When you free an object, it goes into the 'free list'. Allocations are
first made from the free list, before resorting to growing the linear array.
*/
// I started making this for a Doc's list of nuDomEl objects, but I think I'll defer that
// until it's clear that we're really winning over the generic heap.
/*
template<typename T>
class nuSpecialHeap
{
public:
	T* Alloc()
	{
	}


private:
	podvec<T>	List;
};
*/