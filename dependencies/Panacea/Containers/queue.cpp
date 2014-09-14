#include "pch.h"
#include "queue.h"
//#include <mtlog/mtlog.h>

AbcQueue::AbcQueue()
{
	//mtlog_open();
	Tail = 0;
	Head = 0;
	RingSize = 0;
	ItemSize = 0;
	Buffer = NULL;
	HaveSemaphore = false;
	AbcCriticalSectionInitialize( Lock, 4000 );
}

AbcQueue::~AbcQueue()
{
	free(Buffer);
	if ( HaveSemaphore ) AbcSemaphoreDestroy( Semaphore );
	AbcCriticalSectionDestroy( Lock );
}

void AbcQueue::Initialize( bool useSemaphore, size_t itemSize )
{
	AbcAssert( SizeInternal() == 0 );
	if ( itemSize != this->ItemSize )
	{
		AbcAssert( this->ItemSize == 0 );
		AbcAssert( itemSize > 0 );
		AbcAssert( itemSize < 0xffff );		// sanity
		ItemSize = (u32) itemSize;
	}
	AbcAssert( !HaveSemaphore );
	if ( useSemaphore )
	{
		HaveSemaphore = true;
		AbcSemaphoreInitialize( Semaphore );
	}
}

void AbcQueue::Add( const void* item )
{
	TakeCriticalSection lock(Lock);
	
	if ( SizeInternal() >= (int32) RingSize - 1 )
		Grow();

	memcpy( Slot(Head), item, ItemSize );
	Increment( Head );

	if ( HaveSemaphore )
		AbcSemaphoreRelease( Semaphore, 1 );
}

bool AbcQueue::PopTail( void* item )
{
	TakeCriticalSection lock(Lock);
	if ( SizeInternal() == 0 ) return false;
	memcpy( item, Slot(Tail), ItemSize );
	//mtlog( "pop %d:%d", (int) Tail, (int) *((u64*) item) );
	Increment( Tail );
	return true;
}

bool AbcQueue::PeekTail( void* item )
{
	TakeCriticalSection lock(Lock);
	if ( SizeInternal() == 0 ) return false;
	memcpy( item, Slot(Tail), ItemSize );
	return true;
}

void AbcQueue::Grow()
{
	u32 newsize = std::max(RingSize * 2, (u32) 2);
	void* nb = realloc( Buffer, (size_t) ItemSize * newsize );
	AbcAssert(nb != NULL);
	Buffer = nb;
	// If head is behind tail, then we need to copy the later items in front of the earlier ones.
	if ( Head < Tail )
	{
		//mtlog( "Grow + swap (%d - %d)\n", (int) RingSize, (int) newsize );
		memcpy( Slot(RingSize), Slot(0), (size_t) ItemSize * Head );
		Head = RingSize + Head;
	}
	else
	{
		//mtlog( "Grow straight (%d - %d)\n", (int) RingSize, (int) newsize );
	}
	RingSize = newsize;
}

int32 AbcQueue::Size()
{
	TakeCriticalSection lock(Lock);
	return SizeInternal();
}
