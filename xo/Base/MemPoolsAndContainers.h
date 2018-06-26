#pragma once
namespace xo {

/* A memory pool
This provides an std::swap implementation, so that you can repack a pool
if using it in a garbage-collected situation.
*/
class XO_API Pool {
public:
	Pool();
	~Pool();

	void   SetChunkSize(size_t minSize, size_t maxSize);
	void*  Alloc(size_t bytes, bool zeroInit);
	size_t TotalAllocatedBytes() const { return TotalAllocated; }

	template <typename T>
	T* AllocT(bool zeroInit) { return (T*) Alloc(sizeof(T), zeroInit); }

	template <typename T>
	T* AllocNT(size_t count, bool zeroInit) { return (T*) Alloc(count * sizeof(T), zeroInit); }

	// Allocate a copy of 'src', of length 'size', and return the copy
	void* Copy(const void* src, size_t size);

	// Same as Copy, but adds a null terminator
	char* CopyStr(const void* src, size_t size);

	void FreeAll();

	/* This is an optimization for a pool that is frequently re-used.
	The pool must have quite a predictable size for this to be effective.
	Do not change chunk size while using this.
	*/
	void FreeAllExceptOne();

protected:
	size_t          MinChunkSize   = 256;
	size_t          MaxChunkSize   = 64 * 1024;
	size_t          TopPos         = 0;
	size_t          TopSize        = 0;
	size_t          TotalAllocated = 0;
	cheapvec<void*> Chunks;
	cheapvec<void*> BigBlocks;
};

// A vector that allocates its storage from a Pool object
template <typename T>
class PoolArray {
public:
	xo::Pool* Pool;
	T*        Data;
	size_t    Count;
	size_t    Capacity;

	PoolArray() {
		Pool     = nullptr;
		Data     = nullptr;
		Count    = 0;
		Capacity = 0;
	}

	PoolArray& operator+=(const T& v) {
		add(&v);
		return *this;
	}

	T& operator[](size_t _i) {
		return Data[_i];
	}

	const T& operator[](size_t _i) const {
		return Data[_i];
	}

	T& back() {
		return Data[Count - 1];
	}

	const T& back() const {
		return Data[Count - 1];
	}

	void pop() {
		XO_DEBUG_ASSERT(Count > 0);
		Count--;
	}

	T& add(const T* v = nullptr) {
		if (Count == Capacity)
			grow();

		if (v)
			Data[Count++] = *v;
		else
			Data[Count++] = T();

		return Data[Count - 1];
	}

	size_t size() const { return Count; }

	void resize(size_t n) {
		if (n != Count) {
			clear();
			if (n != 0) {
				growto(n);
				Count = n;
			}
		}
	}

	void reserve(size_t n) {
		if (n > Capacity)
			growto(n);
	}

	void clear() {
		Data     = nullptr;
		Count    = 0;
		Capacity = 0;
	}

protected:
	void grow() {
		size_t ncap = std::max(Capacity * 2, (size_t) 2);
		growto(ncap);
	}

	void growto(size_t ncap) {
		T* ndata = (T*) Pool->Alloc(sizeof(T) * ncap, false);
		XO_ASSERT(ndata != nullptr);
		memcpy(ndata, Data, sizeof(T) * Capacity);
		memset(ndata + Capacity, 0, sizeof(T) * (ncap - Capacity));
		Capacity = ncap;
		Data     = ndata;
	}
};

/* This is a special kind of stack that only initializes objects
the first time they are used. Thereafter, objects are recycled.
When the stack is deleted, it only calls the destructor on the
objects that were actually initialized and used.
You will probably need to add your own "Reset" function to your
objects, and call it on the object immediately after Add(). This
Reset function should reset the object to a clean slate, but
leave any heap-allocated memory intact. For example, cheapvec's
clear_noalloc() function.
A convenient attribute of forcing you to declare your size up-front
is that pointers are stable. It avoids that class of bugs where
you take a reference to an object inside the container, and then
you grow the container, and then your reference is invalid.
*/
template <typename T>
class Stack {
public:
	T*     Items         = nullptr;
	size_t Count         = 0;
	size_t Capacity      = 0;
	size_t HighwaterMark = 0; // The maximum that Count has ever been

	~Stack() {
		for (size_t i = 0; i < HighwaterMark; i++)
			Items[i].~T();
		free(Items);
	}

	void Init(size_t capacity) {
		XO_ASSERT(Items == nullptr);
		Capacity = capacity;
		Items    = (T*) malloc(sizeof(T) * capacity);
		XO_ASSERT(Items != nullptr);
	}

	T& Add() {
		XO_ASSERT(Count < Capacity);
		if (Count == HighwaterMark) {
			new (Items + Count) T();
			HighwaterMark++;
		}
		Count++;
		return Back();
	}

	void Pop() {
		XO_ASSERT(Count > 0);
		Count--;
	}

	T& Back() {
		return Items[Count - 1];
	}

	const T& Back() const {
		return Items[Count - 1];
	}

	T& operator[](size_t i) { return Items[i]; }
};

// Circular buffer
// This is only built for PODs, and it does not zero-initialize.
// Automatically grows size. Size must be a power of 2.
// Maximum number of items in the buffer is size - 1.
// There is a debug-only check for popping an empty queue.
template <typename T>
class RingBuf {
public:
	// Arbitrarily chosen constant. Reasoning is that you wouldn't use a
	// ring buffer for less than 32 items. This was originally built
	// to hold glyphs during layout.
	static const uint32_t DefaultInitialSize = 32;

	RingBuf() {
	}

	~RingBuf() {
		free(Ring);
	}

	void Clear() {
		Head = Tail = 0;
	}

	T& PushHead() {
		if (((Head + 1) & Mask) == Tail)
			Grow();
		T& item = Ring[Head];
		Head    = (Head + 1) & Mask;
		return item;
	}

	const T& PopTail() {
		XO_DEBUG_ASSERT(Head != Tail);
		const T& item = Ring[Tail];
		Tail          = (Tail + 1) & Mask;
		return item;
	}

	// Access an element with a position that is relative to the head
	// A value of 0 returns the head element.
	// A value of 1 returns the head element - 1.
	// etc
	T& FromHead(int relative) {
		uint32_t p = (Head - relative - 1) & Mask;
		return Ring[p];
	}

	size_t Size() const { return (size_t)((Head - Tail) & Mask); }

private:
	T*       Ring = nullptr;
	uint32_t Mask = 0;
	uint32_t Head = 0;
	uint32_t Tail = 0;

	uint32_t RingSize() const { return Mask + 1; }

	void Grow() {
		if (Ring == nullptr) {
			Ring = (T*) MallocOrDie(DefaultInitialSize * sizeof(T));
			Mask = DefaultInitialSize - 1;
		} else {
			uint32_t orgSize = RingSize();
			Ring             = (T*) ReallocOrDie(Ring, orgSize * 2 * sizeof(T));
			if (Head < Tail) {
				// Handle the scenario where the head is behind the tail (numerically)
				// [  H T  ]   =>  [    T     H    ]
				// [c - a b]   =>  [- - a b c - - -]
				for (uint32_t i = 0; i < Head; i++)
					Ring[orgSize + i] = Ring[i];
				Head += orgSize;
			}
			Mask = (orgSize * 2) - 1;
		}
	}
};

/*
	A simple fixed-size heap.

	* The total number of allocations is fixed
	* The size of each allocation is fixed (and must be a power of 2)

	This is intended to be used in scenarios where you need a very fast heap, but for objects
	with a fairly predictable size. One advantage when using this for vector storage is that
	you can be guaranteed that the vector will grow at zero cost up to the point where it
	hits the heap allocation size.

	You use it like so:
	
		FixedSizeHeap fheap;
		fheap.Initialize(10, 64);
		void* x = fheap.Alloc(30);
		void* y = fheap.Alloc(70);
		x = fheap.Realloc(x, 60);
		fheap.Free(x);
		fheap.Free(y);

	Basically, treat it like a normal heap. If an allocation cannot be serviced internally, then
	it will simply default to malloc. Any allocation that is smaller than the fixed allocation size
	will be serviced internally. You are simply wasting bytes in that case.

	Any malloc() failure will panic.

*/
class XO_API FixedSizeHeap {
public:
	FixedSizeHeap();
	~FixedSizeHeap();

	void  Initialize(uint32_t maxAllocations, uint32_t allocationSize);
	void* Alloc(size_t bytes);
	void* Realloc(void* buf, size_t bytes);
	void  Free(void* buf);

protected:
	void*              Heap            = nullptr;
	uint32_t*          Used            = nullptr; // Bitmap indicating whether a slot is used
	uint32_t           MaxAllocations  = 0;
	uint32_t           AllocationShift = 0;
	cheapvec<uint32_t> FreeList;

	uint32_t SlotFromPtr(void* p) const;
	uint32_t TotalHeapSize() const { return MaxAllocations << AllocationShift; }
	uint32_t AllocationSize() const { return 1 << AllocationShift; }
};

// Vector that uses FixedSizeHeap
// This was built for Layout3. It doesn't do proper object initialization, but that would be easy to add.
template <typename T>
class FixedVector {
public:
	FixedVector(FixedSizeHeap& heap) : Heap(&heap) {
	}

	~FixedVector() {
		Heap->Free(Items);
	}

	size_t Size() const { return Count; }

	void Push(const T& t) {
		if (Count == Capacity) {
			// Start buffer size at 1, because we happen to know that small reallocs are free
			// (ie until we hit the allocation unit size of FixedSizeHeap)
			Capacity = Max(1u, Capacity * 2);
			Items    = (T*) Heap->Realloc(Items, Capacity * sizeof(T));
		}
		Items[Count++] = t;
	}

	void Pop() { Count--; }

	const T& operator[](size_t i) const { return Items[i]; }
	T&       operator[](size_t i) { return Items[i]; }

protected:
	FixedSizeHeap* Heap;
	uint32_t       Capacity = 0;
	uint32_t       Count    = 0;
	T*             Items    = nullptr;
};
} // namespace xo

namespace std {
XO_API void swap(xo::Pool& a, xo::Pool& b);
}