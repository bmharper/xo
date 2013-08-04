#pragma once

// This file contains macros for compiler-specific things such as specifying struct alignment

#ifdef _WIN32
#define ABC_ALIGN(alignment) __declspec(align(alignment))
#else
#define ABC_ALIGN(alignment) __attribute__ ((aligned(alignment)))
#endif

#define ABC_ALIGNED_TYPE(_type, alignment) typedef _type ABC_ALIGN(alignment)

/*
#ifdef _MSC_VER
#define ABC_GCC_WARNING_PUSH_DISABLE( warnings )
#define ABC_GCC_WARNING_POP
#else
#define ABC_GCC_WARNING_PUSH_DISABLE( warnings )	_Pragma(GCC diagnostic push) \
													_Pragma(GCC diagnostic ignored warnings)

#define ABC_GCC_WARNING_POP							_Pragma(GCC diagnostic pop)
#endif
*/

#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
#define GCC_DIAG_STR(s) #s
#define GCC_DIAG_JOINSTR(x,y) GCC_DIAG_STR(x ## y)
# define GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
# define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) \
	GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
#  define GCC_DIAG_ON(x)  GCC_DIAG_PRAGMA(warning GCC_DIAG_JOINSTR(-W,x))
# endif
#else
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
#endif



