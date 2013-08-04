#pragma once

/*
Declare a thread function as follows:
	AbcThreadReturnType AbcKernelCallbackDecl MyFunction( void* threadContext )

*/

#ifdef _WIN32
typedef HANDLE					AbcThreadHandle;
typedef DWORD					AbcThreadReturnType;
typedef LPTHREAD_START_ROUTINE	AbcThreadFunc;
#define AbcKernelCallbackDecl	WINAPI
#else
typedef pthread_t				AbcThreadHandle;
typedef void*					AbcThreadReturnType;
typedef void* (*AbcThreadFunc)(void*);
#define AbcKernelCallbackDecl
#endif

// All of these are currently ignored on linux
enum AbcThreadPriority
{
	AbcThreadPriorityIdle,
	AbcThreadPriorityNormal,
	AbcThreadPriorityHigh,
	AbcThreadPriorityBackgroundBegin,
	AbcThreadPriorityBackgroundEnd,
};

PAPI bool				AbcThreadCreate( AbcThreadFunc threadfunc, void* context, AbcThreadHandle& handle );
PAPI bool				AbcThreadJoin( AbcThreadHandle handle );
PAPI void				AbcThreadCloseHandle( AbcThreadHandle handle );
PAPI AbcThreadHandle	AbcThreadCurrent();
// This is currently a no-op on linux
PAPI void				AbcThreadSetPriority( AbcThreadHandle handle, AbcThreadPriority priority );

