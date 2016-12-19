#pragma once

#include <malloc.h>
#include <stdlib.h>

namespace xo {

XO_API void* MallocOrDie(size_t bytes);
XO_API void* ReallocOrDie(void* buf, size_t bytes);
XO_API void* AlignedAlloc(size_t bytes, size_t alignment);
XO_API void* AlignedRealloc(size_t original_block_bytes, void* block, size_t bytes, size_t alignment);
XO_API void  AlignedFree(void* block);

template <typename Vec>
void DeleteAll(Vec& v) {
	for (auto p : v)
		delete p;
	v.clear();
}
}
