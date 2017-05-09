#include "pch.h"
#include "Queue.h"
#include "Asserts.h"
#include "../Defs.h"

namespace xo {

Queue::Queue() {
}

Queue::~Queue() {
	free(Buffer);
}

void Queue::Initialize(bool useSemaphore, size_t itemSize) {
	XO_ASSERT(SizeInternal() == 0);
	if (itemSize != ItemSize) {
		XO_ASSERT(ItemSize == 0);
		XO_ASSERT(itemSize > 0);
		XO_ASSERT(itemSize < 0xffff); // sanity
		ItemSize = (uint32_t) itemSize;
	}
	UseSemaphore = useSemaphore;
}

void Queue::Add(const void* item) {
	std::lock_guard<std::mutex> lock(Lock);

	if (SizeInternal() == (int32_t) RingSize)
		Grow();

	memcpy(Slot(Head), item, ItemSize);
	Head++;

	if (UseSemaphore)
		Sem.signal();
}

bool Queue::PopTail(void* item) {
	std::lock_guard<std::mutex> lock(Lock);
	if (SizeInternal() == 0)
		return false;
	memcpy(item, Slot(Tail), ItemSize);
	Tail++;
	return true;
}

bool Queue::PeekTail(void* item) {
	std::lock_guard<std::mutex> lock(Lock);
	if (SizeInternal() == 0)
		return false;
	memcpy(item, Slot(Tail), ItemSize);
	return true;
}

void Queue::Grow() {
	uint32_t newsize = std::max(RingSize * 2, (uint32_t) 2);
	void*    nb      = realloc(Buffer, (size_t) ItemSize * newsize);
	XO_ASSERT(nb != NULL);
	Buffer = nb;
	// If data is currently wrapped around, then we need to unwrap it for the expanded ring size
	uint32_t count   = (uint32_t) SizeInternal();
	uint32_t oldTail = Tail & (RingSize - 1);
	uint32_t oldHead = Head & (RingSize - 1);
	if (oldTail + count > RingSize) {
		memcpy(Slot(RingSize, newsize - 1), Slot(0), (size_t) ItemSize * oldHead);
	}
	// Since our divisor is now different, we need to reset our head and tail numbers. I haven't
	// taken the time to really understand why this is necessary, but I could see obvious bugs
	// when I didn't do this. It sorta makes sense - your modulus is changing, so your old head
	// and tail numbers no longer have the same meaning under this new modulus.
	// A simple example is the case where the ring expands from 2 to 4, and Tail = 6, Head = 8.
	// In that case, both tail and head are pointing to the same slot, meaning the ring is full.
	// When we expand the ring from 2 to 4, Tail now points to slot 2 (because 6 % 4 = 2),
	// and Head now points to slot 0 (because 8 % 4 = 0). So Head has sort-of stayed in the same
	// place, but Tail moved up by two slots.
	Tail     = oldTail;
	Head     = Tail + count;
	RingSize = newsize;
}

int32_t Queue::Size() {
	std::lock_guard<std::mutex> lock(Lock);
	return SizeInternal();
}

void Queue::Scan(bool forwards, void* context, ScanCallback cb) {
	std::lock_guard<std::mutex> lock(Lock);
	uint32_t                    size = SizeInternal();
	if (forwards) {
		for (uint32_t i = Tail; i - Tail != size; i++) {
			if (!cb(context, Slot(i)))
				return;
		}
	} else {
		for (uint32_t i = Head - 1; Head - 1 - i != size; i--) {
			if (!cb(context, Slot(i)))
				return;
		}
	}
}
}