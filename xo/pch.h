#pragma once

#define PROJECT_XO 1

#include "Base/PlatformDefine.h"

#if XO_PLATFORM_WIN_DESKTOP && !defined(XO_EXCLUDE_DIRECTX)
#define XO_BUILD_DIRECTX 1
#else
#define XO_BUILD_DIRECTX 0
#endif

#if !defined(XO_EXCLUDE_OPENGL)
#define XO_BUILD_OPENGL 1
#endif

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

// We keep these includes outside of Base_SystemIncludes, because we
// do not need to pollute the xo client with the OpenGL symbols.
#if defined(XO_BUILD_OPENGL)
#if XO_PLATFORM_WIN_DESKTOP
#include "../dependencies/GL/gl_xo.h"
#include "../dependencies/GL/wgl_xo.h"
#include <GL/gl.h>
#include <GL/glu.h>
#elif XO_PLATFORM_LINUX_DESKTOP
#include "../dependencies/GL/gl_xo.h"
#include "../dependencies/GL/glx_xo.h"
#include <GL/gl.h>
#include <GL/glu.h>
#elif XO_PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
XO_TODO_STATIC
#endif
#endif

#include "Base/warnings.h"
#include "Base/Base.h"

// We do not leak the Freetype definitions to our consumer
#include "../dependencies/freetype/include/ft2build.h"
#include FT_FREETYPE_H

#define STBI_HEADER_FILE_ONLY
#include "../dependencies/stb_image.c"
#undef STBI_HEADER_FILE_ONLY

#ifdef _WIN32
#pragma warning(disable : 4345) // POD initialized with ()
#endif

using namespace std;

#include "Base/MemPoolsAndContainers.h"
#include "Base/OS_IO.h"
