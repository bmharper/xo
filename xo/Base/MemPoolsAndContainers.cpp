#include "pch.h"
#include "Defs.h"
#include "MemPoolsAndContainers.h"

namespace xo {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 6001) // using uninitialized memory. False static analysis warning
#endif

Pool::Pool() {
}

Pool::~Pool() {
	FreeAll();
}

void Pool::SetChunkSize(size_t minSize, size_t maxSize) {
	MinChunkSize = minSize;
	MaxChunkSize = maxSize;
}

void Pool::FreeAll() {
	for (size_t i = 0; i < Chunks.size(); i++)
		free(Chunks[i]);
	for (size_t i = 0; i < BigBlocks.size(); i++)
		free(BigBlocks[i]);
	Chunks.clear();
	BigBlocks.clear();
	TopPos         = 0;
	TopSize        = 0;
	TotalAllocated = 0;
}

void Pool::FreeAllExceptOne() {
	if (Chunks.size() == 1 && BigBlocks.size() == 0) {
		TopPos  = 0;
		TopSize = TotalAllocated;
	} else {
		FreeAll();
	}
}

void* Pool::Alloc(size_t bytes, bool zeroInit) {
	XO_ASSERT(bytes != 0);
	if (bytes > MaxChunkSize) {
		TotalAllocated += bytes;
		BigBlocks += MallocOrDie(bytes);
		if (zeroInit)
			memset(BigBlocks.back(), 0, bytes);
		return BigBlocks.back();
	} else {
		if (TopPos + bytes > TopSize) {
			size_t size = MinChunkSize;
			while (size < TotalAllocated || size < bytes)
				size *= 2;
			size = min(size, MaxChunkSize);
			TotalAllocated += size;
			Chunks += MallocOrDie(size);
			TopPos  = 0;
			TopSize = size;
		}
		uint8_t* p = ((uint8_t*) Chunks.back()) + TopPos;
		if (zeroInit)
			memset(p, 0, bytes);
		TopPos += bytes;
		return p;
	}
}

void* Pool::Copy(const void* src, size_t size) {
	void* c = Alloc(size, false);
	memcpy(c, src, size);
	return c;
}

char* Pool::CopyStr(const void* src, size_t size) {
	char* c = (char*) Alloc(size + 1, false);
	memcpy(c, src, size);
	c[size] = 0;
	return c;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FixedSizeHeap::FixedSizeHeap() {
}

FixedSizeHeap::~FixedSizeHeap() {
	free(Heap);
}

void FixedSizeHeap::Initialize(uint32_t maxAllocations, uint32_t allocationSize) {
	XO_ASSERT(maxAllocations * allocationSize < 2147483648u);
	XO_ASSERT(((allocationSize - 1) & allocationSize) == 0); // allocationSize must be a power of 2

	Heap = MallocOrDie(maxAllocations * allocationSize);
	FreeList.resize(maxAllocations);
	for (uint32_t i = 0; i < maxAllocations; i++)
		FreeList[i] = i;
	MaxAllocations  = maxAllocations;
	AllocationShift = 1;
	while ((1u << AllocationShift) < allocationSize)
		AllocationShift++;
}

void* FixedSizeHeap::Alloc(size_t bytes) {
	if (FreeList.size() == 0 || bytes > (size_t) AllocationSize())
		return MallocOrDie(bytes);

	uint32_t slot = FreeList.rpop();
	return ((uint8_t*) Heap) + (slot << AllocationShift);
}

void* FixedSizeHeap::Realloc(void* buf, size_t bytes) {
	if (buf == nullptr)
		return Alloc(bytes);

	uint32_t slot = SlotFromPtr(buf);
	if (slot != -1) {
		if (bytes <= AllocationSize())
			return buf;

		void* newBuf = MallocOrDie(bytes);
		memcpy(newBuf, buf, AllocationSize());
		Free(buf);
		return newBuf;
	} else {
		// never attempt to shrink into an internal buffer
		return ReallocOrDie(buf, bytes);
	}
}

void FixedSizeHeap::Free(void* buf) {
	if (buf == nullptr)
		return;

	uint32_t slot = SlotFromPtr(buf);
	if (slot != -1)
		FreeList += slot;
	else
		free(buf);
}

uint32_t FixedSizeHeap::SlotFromPtr(void* p) const {
	uint32_t slot = (uint32_t)(((uint8_t*) p - (uint8_t*) Heap) >> AllocationShift);
	if (slot < MaxAllocations)
		return slot;
	else
		return -1;
}
} // namespace xo

namespace std {
XO_API void swap(xo::Pool& a, xo::Pool& b) {
	xo::Pool tmp;
	memcpy(&tmp, &a, sizeof(xo::Pool));
	memcpy(&a, &b, sizeof(xo::Pool));
	memcpy(&b, &tmp, sizeof(xo::Pool));
	memset(&tmp, 0, sizeof(xo::Pool));
}
} // namespace std