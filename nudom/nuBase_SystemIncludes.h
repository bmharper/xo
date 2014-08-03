#pragma once

#ifdef _WIN32
	#ifdef _DEBUG
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
	#include <windows.h>
	#include <windowsx.h>
	#include <mmsystem.h>
	#include <Shlobj.h>
#else
	#define NU_BUILD_OPENGL 1
	#ifdef ANDROID
		// Android
		#define NU_BUILD_OPENGL_ES 1
		#include <jni.h>
		#include <android/log.h>
		#include <sys/atomics.h>
	#endif
	#ifdef NU_BUILD_OPENGL_ES
		#include <GLES2/gl2.h>
		#include <GLES2/gl2ext.h>
	#else
		#include <GL/gl.h>
		#include <GL/glx.h>
		#include <GL/glu.h>
	#endif
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
#include <string.h>
#include <float.h>

#include <string>
#include <algorithm>
#include <limits>
#include <functional>
