
// Definitions fit for inclusion into any C++ project, for any architecture

// Found this in the Chrome sources, via a PVS studio blog post
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN __attribute__ ((noreturn))
#endif

#ifdef _MSC_VER
#	if _M_X64
#		define ARCH_64 1
#	endif
#else
#	if __SIZEOF_POINTER__ == 8
#		define ARCH_64 1
#	endif
#endif
