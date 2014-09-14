#include "pch.h"
#include "syncprims.h"
#include <sys/stat.h>

#ifdef _WIN32
#include <Psapi.h>
#include <io.h>
#include <sys/locking.h>
#include <fcntl.h>
#define open _open
#define close _close
#define lseek _lseek
#else
#include <unistd.h>
#include <fcntl.h>
#endif

// In linux you can use named semaphores for ipc mutex.
// Android doesn't support named semaphores though.
#ifdef _WIN32
PAPI void			AbcMutexCreate( AbcMutex& mutex, const char* name )
{
	mutex = CreateMutexA( NULL, false, name );
}
PAPI void			AbcMutexDestroy( AbcMutex& mutex )
{
	CloseHandle( mutex );
}
PAPI bool			AbcMutexWait( AbcMutex& mutex, DWORD waitMS )
{
	return WaitForSingleObject( mutex, waitMS ) == WAIT_OBJECT_0;
}
PAPI void			AbcMutexRelease( AbcMutex& mutex )
{
	ReleaseMutex( mutex );
}

PAPI void			AbcCriticalSectionInitialize( AbcCriticalSection& cs, unsigned int spinCount )
{
	if ( spinCount != 0 )	AbcVerify( InitializeCriticalSectionAndSpinCount( &cs, spinCount ) );
	else					InitializeCriticalSection( &cs );
}
PAPI void			AbcCriticalSectionDestroy( AbcCriticalSection& cs )
{
	DeleteCriticalSection( &cs );
}
PAPI bool			AbcCriticalSectionTryEnter( AbcCriticalSection& cs )
{
	return !!TryEnterCriticalSection( &cs );
}
PAPI void			AbcCriticalSectionEnter( AbcCriticalSection& cs )
{
	EnterCriticalSection( &cs );
}
PAPI void			AbcCriticalSectionLeave( AbcCriticalSection& cs )
{
	LeaveCriticalSection( &cs );
}

PAPI void			AbcSemaphoreInitialize( AbcSemaphore& sem )
{
	sem = CreateSemaphore( NULL, 0, 0x7fffffff, NULL );
}
PAPI void			AbcSemaphoreDestroy( AbcSemaphore& sem )
{
	CloseHandle( sem );
	sem = NULL;
}
PAPI bool			AbcSemaphoreWait( AbcSemaphore& sem, DWORD waitMS )
{
	return WaitForSingleObject( sem, waitMS ) == WAIT_OBJECT_0;
}
PAPI void			AbcSemaphoreRelease( AbcSemaphore& sem, DWORD count )
{
	ReleaseSemaphore( sem, (LONG) count, NULL );
}

PAPI void			AbcSleep( int milliseconds )
{
	YieldProcessor();
	Sleep( milliseconds );	
}

#else // End Win32

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Linux
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void MillisecondsToAbsTimespec( DWORD ms, timespec& t )
{
	clock_gettime( CLOCK_REALTIME, &t );
	t.tv_sec += ms / 1000;
	t.tv_nsec += (ms % 1000) * 1000;
	if ( t.tv_nsec >= 1000000 )
	{
		t.tv_sec += 1;
		t.tv_nsec -= 1000000;
	}
}

// Android (at ndk-r9) does not have pthread_mutex_timedlock.
// This is an absolutely horrible workaround.
static bool pthread_mutex_timedlock_emulate_ms( AbcMutex& mutex, DWORD waitMS )
{
	AbcAssert( waitMS > 0 );
	for ( DWORD ms = 0; ms < waitMS; ms++ )
	{
		if ( 0 == pthread_mutex_trylock( &mutex ) )
			return true;
		AbcSleep( 1 );
	}
	return false;
}

PAPI void			AbcMutexCreate( AbcMutex& mutex, const char* name )
{
	AbcAssert( name == NULL || name[0] == 0 );
	AbcVerify( 0 == pthread_mutex_init( &mutex, NULL ) );
}
PAPI void			AbcMutexDestroy( AbcMutex& mutex )
{
	AbcVerify( 0 == pthread_mutex_destroy( &mutex ) );
}
PAPI bool			AbcMutexWait( AbcMutex& mutex, DWORD waitMS )
{
	if ( waitMS == 0 )
		return 0 == pthread_mutex_trylock( &mutex );
	else if ( waitMS == AbcINFINITE )
		return 0 == pthread_mutex_lock( &mutex );
	else
	{
#ifdef ANDROID
		return pthread_mutex_timedlock_emulate_ms( mutex, waitMS );
#else
		timespec t;
		MillisecondsToAbsTimespec( waitMS, t );
		return 0 == pthread_mutex_timedlock( &mutex, &t );
#endif
	}
}
PAPI void			AbcMutexRelease( AbcMutex& mutex )
{
	pthread_mutex_unlock( &mutex );
}

PAPI void			AbcCriticalSectionInitialize( AbcCriticalSection& cs, unsigned int spinCount )
{
	AbcVerify( 0 == pthread_mutex_init( &cs, NULL ) );
}
PAPI void			AbcCriticalSectionDestroy( AbcCriticalSection& cs )
{
	AbcVerify( 0 == pthread_mutex_destroy( &cs ) );
}
PAPI bool			AbcCriticalSectionTryEnter( AbcCriticalSection& cs )
{
	return 0 == pthread_mutex_trylock( &cs );
}
PAPI void			AbcCriticalSectionEnter( AbcCriticalSection& cs )
{
	pthread_mutex_lock( &cs );
}
PAPI void			AbcCriticalSectionLeave( AbcCriticalSection& cs )
{
	pthread_mutex_unlock( &cs );
}

PAPI void			AbcSemaphoreInitialize( AbcSemaphore& sem )
{
	AbcVerify( 0 == sem_init( &sem, 0, 0 ) );
}
PAPI void			AbcSemaphoreDestroy( AbcSemaphore& sem )
{
	AbcVerify( 0 == sem_destroy( &sem ) );
}
PAPI bool			AbcSemaphoreWait( AbcSemaphore& sem, DWORD waitMS )
{
	if ( waitMS == 0 )
		return 0 == sem_trywait( &sem );
	else if ( waitMS == AbcINFINITE )
		return 0 == sem_wait( &sem );
	else
	{
		timespec t;
		MillisecondsToAbsTimespec( waitMS, t );
		return 0 == sem_timedwait( &sem, &t );
	}
}
PAPI void			AbcSemaphoreRelease( AbcSemaphore& sem, DWORD count )
{
	for ( DWORD i = 0; i < count; i++ )
		sem_post( &sem );
}


PAPI void			AbcSleep( int milliseconds )
{
	int64 nano = milliseconds * (int64) 1000;
	timespec t;
	t.tv_nsec = nano % 1000000000; 
	t.tv_sec = (nano - t.tv_nsec) / 1000000000;
	nanosleep( &t, NULL );
}

#endif

PAPI int			AbcLockFileLock( const char* path )
{
#ifdef _WIN32
	int mode = _S_IREAD | _S_IWRITE;
#else
	int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif

	int f = open( path, O_CREAT | O_WRONLY, mode );
	if ( f == -1 )
		return -1;

#ifdef _WIN32
	int lock = _locking( f, _LK_NBLCK, 1 );
#else
	int lock = lockf( f, F_TLOCK, 1 );
#endif
	if ( lock == -1 )
	{
		close( f );
		return -1;
	}

	return f;
}

PAPI void			AbcLockFileRelease( int f )
{
	if ( f == -1 )
		return;

	lseek( f, 0, SEEK_SET );
#ifdef _WIN32
	_locking( f, _LK_UNLCK, 1 );
#else
	lockf( f, F_ULOCK, 1 );
#endif
	close( f );
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Cross-platform helpers

PAPI void		AbcSpinLockWait( volatile unsigned int* p )
{
	while ( AbcCmpXChg( p, 1, 0 ) != 0 )
	{
	}
}

PAPI void		AbcSpinLockRelease( volatile unsigned int* p )
{
	AbcAssert( *p == 1 );
	AbcInterlockedSet( p, 0 );
}

PAPI void		AbcCriticalSectionInitialize( AbcGuardedCriticalSection& cs, unsigned int spinCount )
{
	AbcCriticalSectionInitialize( cs.CS, spinCount );
	cs.ThreadID = 0;
}

PAPI void		AbcCriticalSectionDestroy( AbcGuardedCriticalSection& cs )
{
	AbcAssert( cs.ThreadID == 0 );
	AbcCriticalSectionDestroy( cs.CS );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TakeCriticalSection::TakeCriticalSection( AbcCriticalSection& cs )
{
	CS = reinterpret_cast<AbcGuardedCriticalSection*>(&cs);
	UseGuard = false;
	AbcCriticalSectionEnter( CS->CS );
}

TakeCriticalSection::TakeCriticalSection( AbcGuardedCriticalSection& cs )
{
	CS = &cs;
	UseGuard = true;
	AbcCriticalSectionEnter( CS->CS );
	// On ARM one would have to use mintomic here and do a proper acquire on ThreadID
	AbcAssert( CS->ThreadID == 0 );
	CS->ThreadID = AbcThreadCurrentID();
}

TakeCriticalSection::~TakeCriticalSection()
{
	if ( UseGuard )
	{
		AbcAssert( CS->ThreadID == AbcThreadCurrentID() );
		CS->ThreadID = 0;
		// On ARM one would have to use mintomic here and do a proper release on ThreadID
	}
	AbcCriticalSectionLeave( CS->CS );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32
AbcSyncEvent::AbcSyncEvent()
{
	Persistent = false;
	Initialized = false;
}
#endif

AbcSyncEvent::~AbcSyncEvent()
{
	Destroy();
}

void AbcSyncEvent::Initialize( bool persistent )
{
#ifdef _WIN32
	Event = CreateEvent( NULL, persistent, false, NULL );
#else
	Persistent = persistent;
	Initialized = true;
	AbcSemaphoreInitialize( Sem );
#endif
}

void AbcSyncEvent::Destroy()
{
#ifdef _WIN32
	if ( Event != NULL )
		CloseHandle( Event );
	Event = NULL;
#else
	if ( Initialized )
		AbcSemaphoreDestroy( Sem );
	Initialized = false;
#endif
}

void AbcSyncEvent::Signal()
{
#ifdef _WIN32
	AbcAssert( Event != NULL );
	SetEvent( Event );
#else
	AbcAssert( Initialized );
	// When Persistent=true, this is racy (explained in the main comments)
	AbcSemaphoreRelease( Sem, Persistent ? PersistentPostCount : 1 );
#endif
}

bool AbcSyncEvent::Wait( DWORD waitMS )
{
#ifdef _WIN32
	return WaitForSingleObject( Event, waitMS ) == WAIT_OBJECT_0;
#else
	if ( AbcSemaphoreWait( Sem, waitMS ) )
	{
		if ( Persistent )
		{
			// this is racy (explained in the main comments)
			AbcSemaphoreRelease( Sem, 1 );
		}
		return true;
	}
	return false;
#endif
}

#ifdef _WIN32
#undef open
#undef close
#undef lseek
#endif
