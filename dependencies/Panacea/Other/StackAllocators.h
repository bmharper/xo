#pragma once
#ifndef ABC_STACK_ALLOCATORS_H_INCLUDED
#define ABC_STACK_ALLOCATORS_H_INCLUDED

#include "../Platform/compiler.h"
#include "../Platform/err.h"

namespace AbCore
{
	typedef void* (*CxAllocFunc)	( void* context, size_t bytes );
	typedef void (*CxFreeFunc)		( void* context, void* ptr );
	inline void* CxAllocFunc_Default	( void* context, size_t bytes ) { return malloc(bytes); }
	inline void CxFreeFunc_Default		( void* context, void* ptr )	{ free(ptr); }

	struct ABC_ALIGN(16) Bytes16Aligned
	{
		int64 Space[4];
	};

	struct IAllocator
	{
		virtual void* Alloc( size_t bytes ) = 0;
		virtual void Free( void* ptr ) = 0;
	};

	struct DefaultAllocator : public IAllocator
	{
		virtual void* Alloc( size_t bytes )		{ return malloc(bytes); }
		virtual void Free( void* ptr )			{ return free(ptr); }
	};

	/** Allocator that initially uses a single fixed-size block off the stack, and thereafter goes to the general heap.
	**/
	struct SingleStackAllocator : public IAllocator
	{
		virtual void* Alloc( size_t bytes )
		{
			if ( bytes <= StackBlockSize ) { StackBlockSize = 0; return StackBlock; }
			else return malloc(bytes);
		}

		virtual void Free( void* ptr )
		{
			if ( ptr != StackBlock )
				free(ptr);
		}

		SingleStackAllocator( void* block, size_t bytes )
		{
			StackBlock = block;
			StackBlockSize = bytes;
		}

		void Init( void* block, size_t bytes )
		{
			StackBlock = block;
			StackBlockSize = bytes;
		}
		
		void* StackBlock;
		size_t StackBlockSize;
	};

	template< typename TData >
	class CxStack
	{
	public:
		typedef unsigned int TRef;
		AbCore::IAllocator*		Allocator;
		TData*					Data;
		TRef					Capacity, Count;

		CxStack()
		{
			Allocator = NULL;
			Data = NULL;
			Count = 0;
			Capacity = 0;
		}

		~CxStack()
		{
			Allocator->Free( Data );
		}

		TData& operator[]( int i ) { return Data[i]; }

		TData& Back() { return Data[Count - 1]; }

		void Clear()
		{
			Allocator->Free( Data );
			Data = NULL;
			Count = 0;
			Capacity = 0;
		}

		TRef Size() { return Count; }

		void Reserve( int n )
		{
			ABCASSERT( Capacity == 0 && Data == NULL );
			Data = (TData*) Allocator->Alloc( n * sizeof(TData) );
			Capacity = n;
		}

		void Resize( int n )
		{
			ABCASSERT( Count == 0 );
			if ( (TRef) n > Capacity )
				Grow( n );
			Count = n;
		}

		void Push( const TData& d )
		{
			if ( Count == Capacity ) Grow(1);
			Data[Count++] = d;
		}

		void Pop()
		{
			ABCASSERT( Count > 0 );
			Count--;
		}

	protected:

		void Grow( TRef needed )
		{
			TRef ocap = Capacity;
			if ( Capacity == 0 ) Capacity = 1;
			else Capacity = Capacity * 2;
			if ( needed > Capacity )
				Capacity = needed;
			TData* d = (TData*) Allocator->Alloc( Capacity * sizeof(TData) );
			memcpy( d, Data, ocap * sizeof(TData) );
			Allocator->Free( Data );
			Data = d;
		}

	};

	/// Contiguous block of memory, initially on the stack, then on the heap
	// NOTE! It's probably easier to use smallvec_stack. This was a dumb thing to create.
	// Actually... not so entirely. This thing is in fact useful when you want the size of your stack buffer to be independent of the container.
	// See also StackBufferT, below.
	class StackBuffer
	{
	public:
		typedef unsigned char byte;
		byte*		Buffer;		// The buffer
		size_t		Pos;		// The number of bytes appended
		size_t		Capacity;	// Capacity of 'Buffer'
		bool		OwnBuffer;	// If true, then our destructor does "free(Buffer)"

		template< typename TBuf, size_t elements >
		StackBuffer( TBuf (&stack_buffer)[elements] )
		{
			OwnBuffer = false;
			Pos = 0;
			Buffer = (byte*) stack_buffer;
			Capacity = sizeof(stack_buffer[0]) * elements;
		}

		~StackBuffer()
		{
			if ( OwnBuffer ) free(Buffer);
		}

		void Reserve( size_t bytes )
		{
			if ( Pos + bytes > Capacity )
			{
				size_t ncap = Capacity * 2;
				if ( ncap < Pos + bytes ) ncap = Pos + bytes;
				byte* nbuf = (byte*) AbcMallocOrDie( ncap );
				memcpy( nbuf, Buffer, Pos );
				Capacity = ncap;
				if ( OwnBuffer ) free(Buffer);
				OwnBuffer = true;
				Buffer = nbuf;
			}
		}

		/// Current position
		byte* Current()					{ return Buffer + Pos; }

		template< typename T >
		T* TCurrent()					{ return (T*) (Buffer + Pos); }

		template< typename T >
		T& TItem( int el )				{ return ((T*) Buffer)[el]; }

		template< typename T >
		int TSize() const				{ return int(Pos / sizeof(T)); }

		void MoveCurrentPos( intp bytes )	{ Pos += bytes; ABCASSERT(Pos <= Capacity); }

		byte* Add( size_t bytes )
		{
			Reserve( bytes );
			byte* p = Buffer + Pos;
			Pos += bytes;
			return p;
		}

		void Add( const void* p, size_t bytes )
		{
			memcpy( Add(bytes), p, bytes );
		}

		template< typename T >
		void AddItem( const T& v )
		{
			Add( &v, sizeof(v) );
		}

	};
}

//	Usage pattern:
//	func() {
//		StackBufferT<double,32> zbuf;
//		..
//		zbuf.Init(n);					-- Call Init ONCE
//		zbuf[i..n] = xyz;
// 
//	You can also use the parameterized constructor, which simply calls Init()

template < typename VT, size_t TStaticSize >
class StackBufferT
{
	DISALLOW_COPY_AND_ASSIGN(StackBufferT);
public:
	u32 Count;
	VT* Data;
	VT	Static[TStaticSize];

	StackBufferT()
	{
		// I considered leaving this uninitialized, but I don't think it's worth the robustness tradeoff
		// One might be tempted to make Data = Static initially, but this opens one up to the possibility of forgetting to call Init()
		// when the data size is indeed large enough. Your tests may not stress the oversize case, and you'll blow up when it's inconvenient.
		Data = NULL;
	}

	StackBufferT( int s )
	{
		Data = NULL;
		Init(s);
	}

	~StackBufferT()
	{
		if ( Data != Static ) delete[] Data;
	}

	void Init( int s )
	{
		Count = s;
		if ( Count > TStaticSize )
			Data = new VT[Count];
		else
			Data = Static;
	}
	void Fill( const VT& v )		{ for ( u32 i = 0; i < Count; i++ ) Data[i] = v; }

	operator VT*()					{ return Data; }
};

#endif // ABC_STACK_ALLOCATORS_H_INCLUDED
