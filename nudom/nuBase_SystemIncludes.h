#pragma once

#ifdef _WIN32
	#ifdef _DEBUG
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
	#include <windows.h>
	#include <windowsx.h>
	#include <mmsystem.h>
#else
	#define NU_BUILD_OPENGL 1
	#ifdef ANDROID
		// Android
		#include <jni.h>
		#include <android/log.h>
	#endif
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#include <sys/atomics.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <math.h>
#endif

#if NU_BUILD_DIRECTX
	#include <D3D11.h>
	#include <d3dcompiler.h>
	#include <directxmath.h>
	#include <directxcolors.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <string>
#include <algorithm>
#include <limits>
#include <functional>
