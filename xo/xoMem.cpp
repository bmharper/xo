#include "pch.h"
#include "xoDefs.h"
#include "xoMem.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6001)		// using uninitialized memory. False static analysis warning
#endif

xoPool::xoPool()
{
	ChunkSize = 64 * 1024;
	TopRemain = 0;
}

xoPool::~xoPool()
{
	FreeAll();
}

void xoPool::SetChunkSize(size_t size)
{
	// If this were not true, then FreeAllExceptOne would be wrong.
	// Also, this just seems like sane behaviour (i.e. initializing chunk size a priori).
	XOASSERT(Chunks.size() == 0);
	ChunkSize = size;
}

void xoPool::FreeAll()
{
	for (intp i = 0; i < Chunks.size(); i++)
		free(Chunks[i]);
	for (intp i = 0; i < BigBlocks.size(); i++)
		free(BigBlocks[i]);
	Chunks.clear();
	BigBlocks.clear();
	TopRemain = 0;
}

/* This is an optimization for a pool that is frequently re-used.
The pool must have quite a predictable size for this to be effective.
*/
void xoPool::FreeAllExceptOne()
{
	if (Chunks.size() == 1 && BigBlocks.size() == 0)
		TopRemain = ChunkSize;
	else
		FreeAll();
}

void* xoPool::Alloc(size_t bytes, bool zeroInit)
{
	XOASSERT(bytes != 0);
	if (bytes > ChunkSize)
	{
		BigBlocks += xoMallocOrDie(bytes);
		if (zeroInit) memset(BigBlocks.back(), 0, bytes);
		return BigBlocks.back();
	}
	else
	{
		if ((intp)(TopRemain - bytes) < 0)
		{
			Chunks += xoMallocOrDie(ChunkSize);
			TopRemain = ChunkSize;
		}
		byte* p = ((byte*) Chunks.back()) + ChunkSize - TopRemain;
		if (zeroInit) memset(p, 0, bytes);
		TopRemain -= bytes;
		return p;
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoLifoBuf::xoLifoBuf()
{
	Buffer = nullptr;
	Size = 0;
	Pos = 0;
}

xoLifoBuf::xoLifoBuf(size_t size) : xoLifoBuf()
{
	Init(size);
}

xoLifoBuf::~xoLifoBuf()
{
	XOASSERT(Pos == 0);   // This assertion might be a nuisance. If so, just remove it.
	free(Buffer);
}

void xoLifoBuf::Init(size_t size)
{
	XOASSERT(Pos == 0);
	Size = size;
	free(Buffer);
	Buffer = xoMallocOrDie(size);
}

void* xoLifoBuf::Alloc(size_t _bytes)
{
	XOASSERT((size_t) Pos + _bytes <= (size_t) Size);
	XOASSERT((intp) _bytes >= 0);
	intp bytes = _bytes;

	void* pos = (byte*) Buffer + Pos;
	ItemSizes += bytes;
	Pos += bytes;

	return pos;
}

void xoLifoBuf::Realloc(void* buf, size_t bytes)
{
	XOASSERT(ItemSizes.size() > 0 && buf == (byte*) Buffer + (Pos - ItemSizes.back()));
	XOASSERT((size_t) Pos - (size_t) ItemSizes.back() + bytes <= (size_t) Size);
	XOASSERT((intp) bytes >= 0);
	intp delta = (intp) bytes - ItemSizes.back();
	ItemSizes.back() += delta;
	Pos += delta;
}

void xoLifoBuf::GrowLast(size_t moreBytes)
{
	XOASSERT((size_t) Pos + moreBytes <= (size_t) Size);
	ItemSizes.back() += (intp) moreBytes;
	Pos += (intp) moreBytes;
}

void xoLifoBuf::Free(void* buf)
{
	if (buf == nullptr)
		return;
	XOASSERT(ItemSizes.size() > 0 && buf == (byte*) Buffer + (Pos - ItemSizes.back()));
	Pos -= ItemSizes.back();
	ItemSizes.pop();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoFixedSizeHeap::xoFixedSizeHeap()
{
}

xoFixedSizeHeap::~xoFixedSizeHeap()
{
	free(Heap);
}

void xoFixedSizeHeap::Initialize(uint32 maxAllocations, uint32 allocationSize)
{
	XOASSERT(maxAllocations * allocationSize < 2147483648u);
	XOASSERT(((allocationSize - 1) & allocationSize) == 0);		// allocationSize must be a power of 2

	Heap = xoMallocOrDie(maxAllocations * allocationSize);
	FreeList.resize(maxAllocations);
	for (uint32 i = 0; i < maxAllocations; i++)
		FreeList[i] = i;
	MaxAllocations = maxAllocations;
	AllocationShift = 1;
	while ((1u << AllocationShift) < allocationSize)
		AllocationShift++;
}

void* xoFixedSizeHeap::Alloc(size_t bytes)
{
	if (FreeList.size() == 0 || bytes > (size_t) AllocationSize())
		return xoMallocOrDie(bytes);

	uint32 slot = FreeList.rpop();
	return ((byte*) Heap) + (slot << AllocationShift);
}

void* xoFixedSizeHeap::Realloc(void* buf, size_t bytes)
{
	if (buf == nullptr)
		return Alloc(bytes);

	uint32 slot = SlotFromPtr(buf);
	if (slot != -1)
	{
		if (bytes <= AllocationSize())
			return buf;

		void* newBuf = xoMallocOrDie(bytes);
		memcpy(newBuf, buf, AllocationSize());
		Free(buf);
		return newBuf;
	}
	else
	{
		// never attempt to shrink into an internal buffer
		return xoReallocOrDie(buf, bytes);
	}
}

void xoFixedSizeHeap::Free(void* buf)
{
	if (buf == nullptr)
		return;

	uint32 slot = SlotFromPtr(buf);
	if (slot != -1)
		FreeList += slot;
	else
		free(buf);
}

uint32 xoFixedSizeHeap::SlotFromPtr(void* p) const
{
	uint32 slot = (uint32) (((byte*) p - (byte*) Heap) >> AllocationShift);
	if (slot < MaxAllocations)
		return slot;
	else
		return -1;
}
