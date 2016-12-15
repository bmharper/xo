#include "pch.h"
#include "queue.h"
#include "Asserts.h"

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

	if (SizeInternal() >= (int32_t) RingSize - 1)
		Grow();

	memcpy(Slot(Head), item, ItemSize);
	Increment(Head);

	if (UseSemaphore)
		Sem.signal();
}

bool Queue::PopTail(void* item) {
	std::lock_guard<std::mutex> lock(Lock);
	if (SizeInternal() == 0)
		return false;
	memcpy(item, Slot(Tail), ItemSize);
	Increment(Tail);
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
	// If head is behind tail, then we need to copy the later items in front of the earlier ones.
	if (Head < Tail) {
		memcpy(Slot(RingSize), Slot(0), (size_t) ItemSize * Head);
		Head = RingSize + Head;
	}
	RingSize = newsize;
}

int32_t Queue::Size() {
	std::lock_guard<std::mutex> lock(Lock);
	return SizeInternal();
}

void Queue::Scan(bool forwards, void* context, ScanCallback cb) {
	std::lock_guard<std::mutex> lock(Lock);
	if (forwards) {
		for (uint32_t i = Head; i != Tail; i = (i - 1) & Mask()) {
			if (!cb(context, Slot(i)))
				return;
		}
	} else {
		for (uint32_t i = Tail; i != Head; i = (i + 1) & Mask()) {
			if (!cb(context, Slot(i)))
				return;
		}
	}
}
}