#ifndef TINY_STACK_H
#define TINY_STACK_H

// unchecked malloc
DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_PARAMETER_COULD_BE_NULL

/*

A tiny fixed-size stack for allocating temporary memory

It's a stack because you must return memory in the order in which you bought it.

We have a fixed number of slots, which record the size of the allocation.
If we run out of slots, then we just go over to malloc. Also, if we run out
of static space, we go to malloc, as well as if you try to allocate zero bytes.

*/
class TinyStack
{
public:
	TinyStack()
	{
		Pos = 0;
		Size = 0;
		SlotPos = 0;
		SlotCount = 0;
		Slots = NULL;
		Stack = NULL;
		Delayed = false;
	}
	~TinyStack()
	{
		Init(0,0);
	}

	// Designed to be used as a check point, where you expect the stack to be empty
	bool IsEmpty()
	{
		return SlotPos == 0;
	}

	void Init( size_t bytes, size_t slots = 32, bool delay = false )
	{
		free(Stack); Stack = NULL;
		free(Slots); Slots = NULL;
		if ( bytes )
		{
			if ( delay )
			{
				Delayed = true;
			}
			else
			{
				Stack = (BYTE*) malloc( bytes );
				Slots = (size_t*) malloc( sizeof(size_t) * slots );
			}
		}
		Pos = 0;
		Size = bytes;
		SlotPos = 0;
		SlotCount = slots;
	}

	// Just a check
	TinyStack& operator=( const TinyStack& b )
	{
		ASSERT( Stack == NULL );
		ASSERT( b.Stack == NULL );
		return *this;
	}

	template< typename T >
	T* AllocA( size_t n )
	{
		return (T*) Alloc( sizeof(T) * n );
	}

	template< typename T >
	T* ReallocA( void* buf, size_t n )
	{
		return (T*) Realloc( buf, sizeof(T) * n );
	}

	size_t RoundBytes( size_t bytes )
	{
		// round up allocation sizes to 8 bytes (for alignment)
		return ((bytes + 7) / 8) * 8;
	}

	/// Reallocate the top block (this is the only block that may be reallocated)
	void* Realloc( void* buf, size_t newBytes )
	{
		ASSERT( newBytes != 0 );
		newBytes = RoundBytes( newBytes );

		size_t slot = SlotPos - 1;
		if ( slot >= SlotCount || Slots[slot] == 0 )
		{
			return realloc( buf, newBytes );
		}
		else
		{
			size_t oldBytes = Slots[slot];
			Pos -= Slots[slot];
			if ( newBytes + Pos > Size )
			{
				void* n = malloc( newBytes );
				memcpy( n, buf, oldBytes );
				Slots[slot] = 0;
				return n;
			}
			else
			{
				// This is the ideal path
				Slots[slot] = newBytes;
				Pos += newBytes;
				return buf;
			}
		}
	}

	void* Alloc( size_t bytes )
	{
		Wake();
		bytes = RoundBytes( bytes );
		if ( bytes == 0 || bytes + Pos > Size || SlotPos >= SlotCount )
		{
			if ( SlotPos < SlotCount )
				Slots[SlotPos] = 0;
			SlotPos++;
			return malloc(bytes);
		}
		else
		{
			Slots[SlotPos++] = bytes;
			void* p = Stack + Pos;
			Pos += bytes;
			return p;
		}
	}

	void Free( void* p )
	{
		SlotPos--;
		if ( SlotPos < SlotCount )
		{
			if ( Slots[SlotPos] == 0 )
			{
				free( p );
			}
			else
			{
				ASSERT( p == Stack + Pos - Slots[SlotPos] );
				Pos -= Slots[SlotPos];
			}
		}
		else
		{
			free( p );
		}
	}

	void* GetBuffer() { return Stack; }
	size_t StackSpaceRemaining() { return Size - Pos; }
	size_t SlotsRemaining() { return SlotCount - SlotPos; }

protected:
	size_t Pos;
	size_t Size;
	BYTE* Stack;
	
	size_t SlotPos, SlotCount;
	size_t* Slots;

	bool Delayed;

	void Wake()
	{
		if ( !Delayed ) return;
		ASSERT( Stack == NULL );
		ASSERT( Slots == NULL );
		Stack = (BYTE*) malloc( Size );
		Slots = (size_t*) malloc( sizeof(size_t) * SlotCount );
		Delayed = false;
	}
};

DISABLE_CODE_ANALYSIS_WARNINGS_POP

#endif
