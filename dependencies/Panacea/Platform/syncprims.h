#pragma once

#ifndef _WIN32
#include <semaphore.h>
#endif

#ifdef _WIN32
typedef HANDLE					AbcMutex;
typedef CRITICAL_SECTION		AbcCriticalSection;
typedef HANDLE					AbcSemaphore;
#define AbcINFINITE				INFINITE
#else
typedef pthread_mutex_t			AbcMutex;
typedef pthread_mutex_t			AbcCriticalSection;
typedef sem_t					AbcSemaphore;
#define AbcINFINITE				-1
#endif

PAPI void				AbcMutexCreate( AbcMutex& mutex, LPCSTR name );
PAPI void				AbcMutexDestroy( AbcMutex& mutex );
PAPI bool				AbcMutexWait( AbcMutex& mutex, DWORD waitMS );
PAPI void				AbcMutexRelease( AbcMutex& mutex );

// On Windows CRITICAL_SECTION is re-enterable, but not so on linux (where we use a pthread mutex).
// So don't write re-entering code.
// This is a good principle to abide by regardless of your platform: http://cbloomrants.blogspot.com/2012/06/06-19-12-two-learnings.html
PAPI void				AbcCriticalSectionInitialize( AbcCriticalSection& cs, unsigned int spinCount = 0 );
PAPI void				AbcCriticalSectionDestroy( AbcCriticalSection& cs );
PAPI bool				AbcCriticalSectionTryEnter( AbcCriticalSection& cs );
PAPI void				AbcCriticalSectionEnter( AbcCriticalSection& cs );
PAPI void				AbcCriticalSectionLeave( AbcCriticalSection& cs );

PAPI void				AbcSemaphoreInitialize( AbcSemaphore& sem );
PAPI void				AbcSemaphoreDestroy( AbcSemaphore& sem );
PAPI bool				AbcSemaphoreWait( AbcSemaphore& sem, DWORD waitMS );
// On linux we can only release one semaphore at a time, so the 'count' > 1 is not atomic.
// Be careful not to architect your applications around that assumption.
// Also, this operation is O(count) on linux.
PAPI void				AbcSemaphoreRelease( AbcSemaphore& sem, DWORD count );

PAPI void				AbcSleep( int milliseconds );

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: Get rid of these functions, and replace them with mintomic
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// AbcInterlockedAdd		returns the PREVIOUS value
// AbcInterlockedOr			returns the PREVIOUS value
// AbcInterlockedAnd		returns the PREVIOUS value
// AbcInterlockedIncrement	returns the NEW value
// AbcInterlockedDecrement	returns the NEW value
// AbcCmpXChg				returns the PREVIOUS value
#ifdef _WIN32
inline void			AbcInterlockedSet( volatile unsigned int* p, int newval )				{ _InterlockedExchange( (volatile long*) p, (long) newval ); }
inline unsigned int	AbcInterlockedAdd( volatile unsigned int* p, int addval )				{ return (unsigned int) _InterlockedExchangeAdd( (volatile long*) p, (long) addval ); }
inline unsigned int	AbcInterlockedOr( volatile unsigned int* p, int orval )					{ return (unsigned int) _InterlockedOr( (volatile long*) p, (long) orval ); }
inline unsigned int	AbcInterlockedAnd( volatile unsigned int* p, int andval )				{ return (unsigned int) _InterlockedAnd( (volatile long*) p, (long) andval ); }
inline unsigned int	AbcInterlockedXor( volatile unsigned int* p, int xorval )				{ return (unsigned int) _InterlockedXor( (volatile long*) p, (long) xorval ); }
inline void			AbcSetWithRelease( volatile unsigned int* p, int newval )				{ *p = newval; _WriteBarrier(); }
inline unsigned int	AbcCmpXChg( volatile unsigned int* p, int newval, int oldval )			{ return _InterlockedCompareExchange( (volatile long*) p, (long) newval, (long) oldval ); }
inline unsigned int	AbcInterlockedIncrement( volatile unsigned int* p )						{ return (unsigned int) _InterlockedIncrement( (volatile long*) p ); }
inline unsigned int	AbcInterlockedDecrement( volatile unsigned int* p )						{ return (unsigned int) _InterlockedDecrement( (volatile long*) p ); }
#else
// Clang has __sync_swap(), which is what you want here when compiling with Clang
// #	ifdef ANDROID
// //inline void			AbcInterlockedSet( volatile unsigned int* p, int newval )				{ __atomic_swap( newval, (volatile int*) p ); }
// //inline void			AbcSetWithRelease( volatile unsigned int* p, int newval )				{ __atomic_store_n( p, newval, __ATOMIC_RELEASE ); }
// inline void			AbcInterlockedSet( volatile unsigned int* p, int newval )				{ *p = newval; }
// inline void			AbcSetWithRelease( volatile unsigned int* p, int newval )				{ *p = newval; __sync_synchronize(); }
// inline unsigned int AbcCmpXChg( volatile unsigned int* p, int newval, int oldval )			{ return __sync_val_compare_and_swap( p, oldval, newval ); }
// #	elif __GNUC__
inline void			AbcInterlockedSet( volatile unsigned int* p, int newval )				{ *p = newval; }
inline unsigned int	AbcInterlockedAdd( volatile unsigned int* p, int addval )				{ return __sync_fetch_and_add( p, addval ); }
inline unsigned int	AbcInterlockedOr( volatile unsigned int* p, int orval )					{ return __sync_fetch_and_or( p, orval ); }
inline unsigned int	AbcInterlockedAnd( volatile unsigned int* p, int andval )				{ return __sync_fetch_and_and( p, andval ); }
inline unsigned int	AbcInterlockedXor( volatile unsigned int* p, int xorval )				{ return __sync_fetch_and_xor( p, xorval ); }
inline void			AbcSetWithRelease( volatile unsigned int* p, int newval )				{ *p = newval; __sync_synchronize(); } // I think there is a better implementation of this, using __sync_lock_test_and_set followed by __sync_lock_release
inline unsigned int AbcCmpXChg( volatile unsigned int* p, int newval, int oldval )			{ return __sync_val_compare_and_swap( p, oldval, newval ); }
inline unsigned int	AbcInterlockedIncrement( volatile unsigned int* p )						{ return __sync_add_and_fetch( p, 1 ); }
inline unsigned int	AbcInterlockedDecrement( volatile unsigned int* p )						{ return __sync_sub_and_fetch( p, 1 ); }
//#	endif
#endif


PAPI void		AbcSpinLockWait( volatile unsigned int* p );		// Spins until we can set p from 0 to 1. Assumes your hold your lock for a handful of clock cycles.
PAPI void		AbcSpinLockRelease( volatile unsigned int* p );		// Sets p to 0 with release semantics.

/// Scope-based AbcSpinLock___
class TakeSpinLock
{
public:
	volatile unsigned int* P;
	TakeSpinLock( volatile unsigned int* p )
	{
		P = p;
		AbcSpinLockWait(P);
	}
	~TakeSpinLock()
	{
		AbcSpinLockRelease(P);
	}
};

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

/*

	[2013-10-22 BMH] This thing needs proper testing. I am not sure that the semaphore semantics are as expected.

	Simulates CreateEvent/SetEvent/Wait for linux.

	I don't understand why people say you can use a condition variable to simulate a Windows Event.
	In the condition variable scenario, the waiters need to lock the mutex, which means there can
	only be a single thread waiting on the event at any particular time. This breaks one of the
	primary	uses of an event: multiple workers waiting on a single event.
	
	On Windows, we just use the native Events.

	On linux, we use semaphores.

	Regarding the 'persistent' parameter, we are trying to duplicate the semantics of
	the Windows manual-reset event type.
	The following documentation	is copied from MSDN:
	If this parameter is TRUE, the function creates a manual-reset event object, which requires
	the use of the ResetEvent function to set the event state to nonsignaled. If this parameter
	is FALSE, the function creates an auto-reset event object, and system automatically resets
	the event state to nonsignaled after a single waiting thread has been released.

	Note that internally on linux, since we are using a semaphore, we cannot reproduce the 
	Windows functionality completely.

	What we do on linux is instead a bit of a hack, but I have scanned through all of my present
	usages of events on Windows, and this workaround will suffice for all of them.
	I do fear somebody using this incorrectly in the future though. It would likely
	introduce a subtle and hard to understand bug. I simply cannot find a pthreads
	alternative though.

	The technique we use is this (applies only when persistent = true):
	1. The initial call to SetEvent() calls sem_post 32 times. The number 32 is chosen
		to match the expected number of general purpose threads on a machine. There
		is no way we can avoid the event from flipping in and out of the signaled state,
		so all we're doing here is trying to ensure that performance is decent.
	1. Whenever a thread is awakened, it immediately calls sem_post() again. The semaphore
		number will thus hover around 32.

	Note that this does mean that the event is going to flip in and out of the signaled state.
	Unfortunately I cannot think of a bullet-proof way to implement this.

	The destructor calls Destroy()
*/
struct PAPI AbcSyncEvent
{
#ifdef _WIN32
	HANDLE				Event;
	AbcSyncEvent()		: Event(0) {}
	~AbcSyncEvent();
#else
	static const int	PersistentPostCount = 32;
	AbcSemaphore		Sem;
	bool				Persistent;
	bool				Initialized;
	AbcSyncEvent();
	~AbcSyncEvent();
#endif

	void Initialize( bool persistent );
	void Destroy();
	void Signal();
	bool Wait( DWORD waitMS );
};

