#pragma once

namespace xo {
/*

Work/job queue
==============

* Multithreaded
* Simple FIFO
* Ring buffer

We follow ryg's recommendations from http://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/

CAVEAT!

If you choose to use the semaphore, then ALL of your queue consumers MUST obey this pattern:
	1 Wait for the semaphore to be signaled
	2 Fetch one item from the queue
	3 Go back to (1)

*/
class XO_API Queue {
public:
	typedef bool (*ScanCallback)(void* context, void* item);

	std::mutex Lock; // Lock on the queue data structures
	Semaphore  Sem;  // Can be used to wait for detection of a non-empty queue. Only valid if semaphore was enabled during call to Initialize(). Read CAVEAT.

	Queue();
	~Queue();

	void    Initialize(bool useSemaphore, size_t itemSize); // Every item must be the same size
	void    Add(const void* item);                          // Add to head. We copy in itemSize bytes, from base address 'item'
	bool    PopTail(void* item);                            // Pop the tail of the queue. Returns false if the queue is empty.
	bool    PeekTail(void* item);                           // Get the tail of the queue, but do not pop it. Obviously useless for multithreaded scenarios, unless you have acquired the lock.
	int32_t Size();

	// Scan through the queue, allowing you to manipulate items inside the queue.
	// The callback function 'cb' is called once for every item in the queue.
	// If forwards is true, then we iterate from Tail to Head.
	// If forwards is false, then we iterate from Head to Tail.
	// Return false from your iterator function to end the scan prematurely.
	// Note that the mutex lock is held during iteration, so you cannot add
	// or remove items from the queue while scanning.
	void Scan(bool forwards, void* context, ScanCallback cb);

private:
	bool     UseSemaphore = false;
	uint32_t Tail         = 0;
	uint32_t Head         = 0;
	uint32_t RingSize     = 0; // Size of the ring buffer. Always a power of 2.
	uint32_t ItemSize     = 0;
	void*    Buffer       = nullptr;

	uint32_t Mask() const { return RingSize - 1; }
	void*    Slot(uint32_t pos) const { return Slot(pos, Mask()); }
	void*    Slot(uint32_t pos, uint32_t mask) const { return (byte*) Buffer + ((pos & mask) * ItemSize); }
	int32_t  SizeInternal() const { return Head - Tail; }

	void Grow();
};

// Typed wrapper around Queue
template <typename T>
class TQueue {
public:
	TQueue() { Q.Initialize(false, sizeof(T)); }
	void Initialize(bool useSemaphore) { Q.Initialize(useSemaphore, sizeof(T)); }
	void Add(const T& item) { Q.Add(&item); }
	bool PopTail(T& item) { return Q.PopTail(&item); }
	bool PeekTail(T& item) { return Q.PeekTail(&item); }
	T    PopTailR() {
        T t = T();
        PopTail(t);
        return t;
	}
	T PeekTailR() {
		T t = T();
		PeekTail(t);
		return t;
	}
	int32_t     Size() { return Q.Size(); }
	std::mutex& LockObj() { return Q.Lock; }
	Semaphore&  SemObj() { return Q.Sem; }

	void Scan(bool forwards, void* context, bool (*cb)(void* context, T* item)) { Q.Scan(forwards, context, (Queue::ScanCallback) cb); }

private:
	Queue Q;
};
}