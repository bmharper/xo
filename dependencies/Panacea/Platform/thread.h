#pragma once

/*
Declare a thread function as follows:
	AbcThreadReturnType AbcKernelCallbackDecl MyFunction( void* threadContext )

*/

#ifdef _WIN32
typedef HANDLE					AbcThreadHandle;
typedef DWORD					AbcThreadID;			// Use AbcThreadEqual to compare for equality
typedef DWORD					AbcThreadReturnType;
typedef LPTHREAD_START_ROUTINE	AbcThreadFunc;
#define AbcKernelCallbackDecl	WINAPI
#else
typedef pthread_t				AbcThreadHandle;
typedef pthread_t				AbcThreadID;			// Use AbcThreadEqual to compare for equality
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
PAPI bool				AbcThreadJoinAndCloseHandle( AbcThreadHandle handle );
PAPI void				AbcThreadCloseHandle( AbcThreadHandle handle );
PAPI AbcThreadHandle	AbcThreadCurrent();
PAPI AbcThreadID		AbcThreadCurrentID();
PAPI bool				AbcThreadIDEqual( AbcThreadID a, AbcThreadID b );
// This is currently a no-op on linux
PAPI void				AbcThreadSetPriority( AbcThreadHandle handle, AbcThreadPriority priority );

