#include "pch.h"

// This is a wrapper file that includes some other 3rd party "single file" dependencies

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4312) // conversion from int to *
#pragma warning(disable: 6001) // using uninitialized memory
#pragma warning(disable: 6246) // local declaration hides name in outer scope
#pragma warning(disable: 6262) // stack size
#pragma warning(disable: 6385) // reading invalid data
#endif

#include "../dependencies/stb_image.c"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "../dependencies/tsf/tsf.cpp"
