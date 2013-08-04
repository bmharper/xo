#include "pch.h"

#ifdef _M_X64
PAPI bool ismemzero( const void* b, size_t bytes )		{ return ismemzeroT<u64>(b, bytes); }
#else
PAPI bool ismemzero( const void* b, size_t bytes )		{ return ismemzeroT<u32>(b, bytes); }
#endif
