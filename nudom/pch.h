#pragma once

#define PROJECT_NUDOM 1

#ifdef _WIN32
	#define NU_BUILD_DIRECTX 1
#else
	#define NU_BUILD_DIRECTX 0
#endif

#include "nuApiDecl.h"

#include "nuBase.h"

#include "../dependencies/Panacea/Vec/Mat4.h"

#define NU_MAT4F_DEFINED
typedef Mat4T<float> nuMat4f;

// We do not leak the Freetype definitions to our consumer
#include "../dependencies/freetype/include/ft2build.h"
#include FT_FREETYPE_H

#define STBI_HEADER_FILE_ONLY
#include "../dependencies/stb_image.c"
#undef STBI_HEADER_FILE_ONLY

#ifdef _WIN32
	#pragma warning( disable: 4345 ) // POD initialized with ()
#endif

using namespace std;

#include "nuPlatform.h"
#include "nuMem.h"
