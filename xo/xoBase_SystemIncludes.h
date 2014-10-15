#pragma once

#if XO_PLATFORM_WIN_DESKTOP
	#ifdef _DEBUG
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
	#include <windows.h>
	#include <windowsx.h>
	#include <mmsystem.h>
	#include <Shlobj.h>
	#include <tchar.h>
#else
	#if XO_PLATFORM_ANDROID
		#include <jni.h>
		#include <android/log.h>
		#include <sys/atomics.h>
	#else
		#include <sys/types.h>
		#include <sys/stat.h>
		#include <unistd.h>
		#include <pwd.h>
	#endif
	#include <pthread.h>
	#include <semaphore.h>
	#include <math.h>
#endif

#if XO_BUILD_DIRECTX
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
