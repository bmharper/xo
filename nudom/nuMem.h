#pragma once

class NUAPI nuPool
{
public:
	nuPool();
	~nuPool();

	void*	Alloc( size_t bytes, bool zeroInit );
	
	template<typename T>
	T*		AllocT( bool zeroInit ) { return (T*) Alloc( sizeof(T), zeroInit ); }
	
	void	FreeAll();

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
		add() = v;
		return *this;
	}

	T& operator[]( intp _i )
	{
		return Data[_i];
	}

	T& add()
	{
		if ( Count == Capacity )
			grow();
		Data[Count++] = T();
		return Data[Count - 1];
	}

	intp size() const { return Count; }

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
		T* ndata = (T*) Pool->Alloc( sizeof(T) * ncap, false );
		NUCHECKALLOC(ndata);
		memcpy( ndata, Data, sizeof(T) * Capacity );
		memset( ndata + Capacity, 0, sizeof(T) * (ncap - Capacity) );
		Capacity = ncap;
		Data = ndata;
	}
};
