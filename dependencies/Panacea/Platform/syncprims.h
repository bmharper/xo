#pragma once

#ifdef _WIN32
typedef HANDLE					AbcMutex;
typedef CRITICAL_SECTION		AbcCriticalSection;
typedef HANDLE					AbcSemaphore;
typedef HANDLE					AbcThreadHandle;
typedef DWORD					AbcThreadReturnType;
typedef LPTHREAD_START_ROUTINE	AbcThreadFunc;
typedef HANDLE					AbcForkedProcessHandle;
typedef DWORD					AbcProcessID;
#define AbcKernelCallbackDecl	WINAPI
#define AbcINFINITE				INFINITE
#else
typedef pthread_mutex_t			AbcMutex;
typedef pthread_mutex_t			AbcCriticalSection;
typedef sem_t					AbcSemaphore;
typedef pthread_t				AbcThreadHandle;
typedef void*					AbcThreadReturnType;
typedef void* (*AbcThreadFunc)(void*);
typedef FILE*					AbcForkedProcessHandle;
typedef pid_t					AbcProcessID;
#define AbcKernelCallbackDecl
#define AbcINFINITE				-1
#endif

PAPI void		AbcMutexCreate( AbcMutex& mutex, bool initialowner, LPCSTR name );
PAPI void		AbcMutexDestroy( AbcMutex mutex );
PAPI DWORD		AbcMutexWait( AbcMutex mutex, DWORD milliseconds );
PAPI void		AbcMutexRelease( AbcMutex mutex );
PAPI void		AbcCriticalSectionInitialize( AbcCriticalSection& cs, unsigned int spinCount = 0 );
PAPI void		AbcCriticalSectionDestroy( AbcCriticalSection& cs );
PAPI bool		AbcCriticalSectionTryEnter( AbcCriticalSection& cs );
PAPI void		AbcCriticalSectionEnter( AbcCriticalSection& cs );
PAPI void		AbcCriticalSectionLeave( AbcCriticalSection& cs );
PAPI void		AbcSemaphoreInitialize( AbcSemaphore& sem );
PAPI void		AbcSemaphoreDestroy( AbcSemaphore& sem );
PAPI bool		AbcSemaphoreWait( AbcSemaphore& sem, DWORD waitMS );
PAPI void		AbcSemaphoreRelease( AbcSemaphore& sem );

PAPI bool		AbcThreadCreate( AbcThreadFunc threadfunc, void* context, AbcThreadHandle& handle );
PAPI bool		AbcThreadJoin( AbcThreadHandle handle );
PAPI void		AbcThreadCloseHandle( AbcThreadHandle handle );

PAPI bool		AbcProcessCreate( const char* cmd, AbcForkedProcessHandle* handle, AbcProcessID* pid );
PAPI bool		AbcProcessWait( AbcForkedProcessHandle handle, int* exitCode );
PAPI void 		AbcProcessCloseHandle( AbcForkedProcessHandle handle );
PAPI void		AbcProcessGetPath( wchar_t* path, size_t maxPath );		// Return full path of executing process, for example "c:\programs\notepad.exe"
#ifdef XSTRING_DEFINED
PAPI XString	AbcProcessGetPath();									// Return full path of executing process, for example "c:\programs\notepad.exe"
#endif
PAPI void		AbcEnumProcesses( podvec<AbcProcessID>& pids );

/// Scope-based critical section acquisition
class TakeCriticalSection
{
public:
	AbcCriticalSection* CS;
	TakeCriticalSection( AbcCriticalSection& cs )
	{
		CS = &cs;
		AbcCriticalSectionEnter( *CS );
		//OutputDebugStringA( XStringPFA( "%d: Taking critical section %x\n", GetCurrentThreadId(), (int) (size_t) &cs ) );
	}
	~TakeCriticalSection()
	{
		//OutputDebugStringA( XStringPFA( "%d: Leaving critical section %x\n", GetCurrentThreadId(), (int) (size_t) CS ) );
		AbcCriticalSectionLeave( *CS );
	}
};

struct AbcMutexStackEnter
{
	AbcMutex* Mutex;
	AbcMutexStackEnter( AbcMutex& m )
	{
		Mutex = &m;
		AbcMutexWait( *Mutex, AbcINFINITE );
	}
	~AbcMutexStackEnter()
	{
		AbcMutexRelease( *Mutex );
	}
};
