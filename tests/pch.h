#pragma once

#ifdef _WIN32
	#ifndef _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC
	#endif
	#include <stdlib.h>
	#include <crtdbg.h>
	#include <windows.h>
#endif

#include "../nuDom/nuDom.h"

#define TT_MODULE_NAME nudom
#include <TinyTest/TinyTest.h>
#define TESTFUNC(f) TT_TEST_FUNC(NULL, NULL, TTSizeSmall, f, TT_PARALLEL_DONTCARE)
