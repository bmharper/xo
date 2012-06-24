#pragma once

#define PROJECT_NUDOM 1

#include "nuBase.h"

#ifdef _WIN32
	#pragma warning( disable: 4345 ) // POD initialized with ()
#endif

using namespace std;

#include "nuApiDecl.h"
#include "nuPlatform.h"
#include "nuMem.h"
