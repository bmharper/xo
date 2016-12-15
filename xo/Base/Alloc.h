#pragma once
namespace xo {

void*  MallocOrDie(size_t bytes);
void*  ReallocOrDie(void* buf, size_t bytes);
}
