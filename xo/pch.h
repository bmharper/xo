#pragma once

#define PROJECT_NUDOM 1

#ifdef _WIN32
	#define NU_BUILD_DIRECTX 1
#else
	#define NU_BUILD_DIRECTX 0
#endif

#define NU_BUILD_OPENGL 1

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#if defined(NU_BUILD_OPENGL)
	#include "../dependencies/GL/gl_nudom.h"
	#ifdef _WIN32
		#include "../dependencies/GL/wgl_nudom.h"
	#endif
	#ifdef __linux__
		#include "../dependencies/GL/glx_nudom.h"
	#endif
#endif

#include "nuApiDecl.h"

#include "warnings.h"

#include "nuBase_SystemIncludes.h"
#include "nuBase.h"
#include "nuBase_LocalIncludes.h"
#include "nuBase_Vector.h"
#include "nuBase_Fmt.h"
#include "nuString.h"
#include "../dependencies/Panacea/Strings/fmt.h"

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
