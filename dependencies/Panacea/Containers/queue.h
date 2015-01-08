#pragma once
#ifndef ABC_QUEUE_H_INCLUDED
#define ABC_QUEUE_H_INCLUDED

#include "../Platform/syncprims.h"

/*

Work/job queue
==============

* Multithreaded
* Simple FIFO
* Ring buffer

We follow ryg's recommendations here from http://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/
Particularly, ring size is always a power of 2, and we use at most N-1 slots. This removes the ambiguity caused
by a full buffer, wherein Head = Tail, which is the same as an empty buffer.

CAVEAT!

If you choose to use the semaphore, then ALL of your queue consumers MUST obey this pattern:
	1 Wait for the semaphore to be signaled
	2 Fetch one item from the queue
	3 Go back to (1)

*/
class PAPI AbcQueue
{
public:
	AbcCriticalSection	Lock;			// Lock on the queue data structures
	AbcSemaphore		Semaphore;		// Can be used to wait for detection of a non-empty queue. Only valid if semaphore was enabled during call to Initialize(). Read CAVEAT.

	AbcQueue();
	~AbcQueue();

	void	Initialize(bool useSemaphore, size_t itemSize);		// Every item must be the same size
	void	Add(const void* item);								// Add to head. We copy in itemSize bytes, from base address 'item'
	bool	PopTail(void* item);								// Pop the tail of the queue. Returns false if the queue is empty.
	bool	PeekTail(void* item);								// Get the tail of the queue, but do not pop it. Obviously useless for multithreaded scenarios, unless you have acquired the lock.
	int32	Size();

private:
	bool				HaveSemaphore;
	u32					Tail;
	u32					Head;
	u32					RingSize;					// Size of the ring buffer. Always a power of 2.
	u32					ItemSize;
	void*				Buffer;

	u32					Mask() const				{ return RingSize - 1; }
	void*				Slot(u32 pos) const			{ return (byte*) Buffer + (pos * ItemSize); }
	void				Increment(u32& i) const		{ i = (i + 1) & Mask(); }
	int32				SizeInternal() const		{ return (Head - Tail) & Mask(); }

	void				Grow();
};

// Typed wrapper around AbcQueue
template<typename T>
class TAbcQueue
{
public:
	TAbcQueue()													{ Q.Initialize(false, sizeof(T)); }
	void					Initialize(bool useSemaphore)		{ Q.Initialize(useSemaphore, sizeof(T)); }
	void					Add(const T& item)					{ Q.Add(&item); }
	bool					PopTail(T& item)					{ return Q.PopTail(&item); }
	bool					PeekTail(T& item)					{ return Q.PeekTail(&item); }
	T						PopTailR()							{ T t = T(); PopTail(t); return t; }
	T						PeekTailR()							{ T t = T(); PeekTail(t); return t; }
	int32					Size()								{ return Q.Size(); }
	AbcCriticalSection&		LockObj()							{ return Q.Lock; }
	AbcSemaphore&			SemaphoreObj()						{ return Q.Semaphore; }
private:
	AbcQueue Q;
};

#endif // ABC_QUEUE_H_INCLUDED
