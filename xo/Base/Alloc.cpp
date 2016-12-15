#include "pch.h"
#include "Alloc.h"

namespace xo {

void* MallocOrDie(size_t bytes) {
	void* b = malloc(bytes);
	XO_ASSERT(b);
	return b;
}

void* ReallocOrDie(void* buf, size_t bytes) {
	void* b = realloc(buf, bytes);
	XO_ASSERT(b);
	return b;
}

}