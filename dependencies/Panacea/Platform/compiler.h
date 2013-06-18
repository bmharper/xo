#pragma once

// This file contains macros for compiler-specific things such as specifying struct alignment

#ifdef _WIN32
#define ABC_ALIGN(alignment) __declspec(align(alignment))
#else
#define ABC_ALIGN(alignment) __attribute__ ((aligned(alignment)))
#endif

#define ABC_ALIGNED_TYPE(_type, alignment) typedef _type ABC_ALIGN(alignment)
