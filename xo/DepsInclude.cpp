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

#ifndef XO_NO_STB_IMAGE
#include "../dependencies/stb_image.c"
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#ifndef XO_NO_STB_IMAGE_WRITE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../dependencies/stb_image_write.h"
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifndef XO_NO_TSF
#include "../dependencies/tsf/tsf.cpp"
#endif
