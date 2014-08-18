#pragma once

#define PROJECT_XO 1

#ifdef _WIN32
	#define XO_BUILD_DIRECTX 1
#else
	#define XO_BUILD_DIRECTX 0
#endif

#define XO_BUILD_OPENGL 1

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#if defined(XO_BUILD_OPENGL)
	#include "../dependencies/GL/gl_xo.h"
	#ifdef _WIN32
		#include "../dependencies/GL/wgl_xo.h"
	#endif
	#ifdef __linux__
		#include "../dependencies/GL/glx_xo.h"
	#endif
#endif

#include "xoApiDecl.h"

#include "warnings.h"

#include "xoBase_SystemIncludes.h"
#include "xoBase.h"
#include "xoBase_LocalIncludes.h"
#include "xoBase_Vector.h"
#include "xoBase_Fmt.h"
#include "xoString.h"
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

#include "xoPlatform.h"
#include "xoMem.h"
