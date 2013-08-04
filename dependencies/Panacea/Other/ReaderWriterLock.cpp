#include "pch.h"
#include "ReaderWriterLock.h"
#include "../Platform/cpu.h"

//#define FORCE_SINGLE_CORE
//#define FORCE_XP

namespace AbCore
{

void ReaderWriterLock_NG::Noop( uint32 tick )
{
	if ( ((tick + 1) & 0xfff) == 0 )
		AbcSleep(0);
}

ReaderWriterLock_NG::ReaderWriterLock_NG()			{ Main = 0; }
ReaderWriterLock_NG::~ReaderWriterLock_NG()			{ ASSERT( Main == 0 ); }
void ReaderWriterLock_NG::Reset()					{ Main = 0; }
void ReaderWriterLock_NG::Init( int maxReaders )	{ Main = 0; }

void ReaderWriterLock_NG::EnterRead()
{
	for ( uint32 tick = 0 ;; tick++ )
	{
		uint32 oldVal = Main;
		if ( (oldVal & WriteDesireBit) == 0 )
		{
			if ( AbcCmpXChg( &Main, oldVal + 1, oldVal ) == oldVal )
				break;
		}
		Noop(tick);
	}
}

void ReaderWriterLock_NG::EnterWrite()
{
	for ( uint32 tick = 0 ;; tick++ )
	{
		if ( (tick & 0xfff) == 0 )
			AbcInterlockedOr( &Main, WriteDesireBit );

		uint32 oldVal = Main;
		if ( oldVal == WriteDesireBit )
		{
			if ( AbcCmpXChg( &Main, -1, WriteDesireBit ) == WriteDesireBit )
				break;
		}
		Noop(tick);
	}
}

void ReaderWriterLock_NG::LeaveRead()
{
	ASSERT( Main != -1 );
	AbcInterlockedDecrement( &Main );
}

void ReaderWriterLock_NG::LeaveWrite()
{
	ASSERT( Main == -1 );
	AbcInterlockedIncrement( &Main );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
ReaderWriterLock_PThreads::ReaderWriterLock_PThreads()
{
	memset( &Lock, 0, sizeof(Lock) );
}
ReaderWriterLock_PThreads::~ReaderWriterLock_PThreads()
{
}
void ReaderWriterLock_PThreads::Reset()
{
	pthread_rwlock_destroy( &Lock );
	memset( &Lock, 0, sizeof(Lock) );
}
void ReaderWriterLock_PThreads::Init( int maxReaders )
{
	VERIFY( 0 == pthread_rwlock_init( &Lock, NULL ) );
}
void ReaderWriterLock_PThreads::EnterRead()
{
	VERIFY( 0 == pthread_rwlock_rdlock( &Lock ) );
}
void ReaderWriterLock_PThreads::EnterWrite()
{
	VERIFY( 0 == pthread_rwlock_wrlock( &Lock ) );
}
void ReaderWriterLock_PThreads::LeaveRead()
{
	 VERIFY( 0 == pthread_rwlock_unlock( &Lock ) );
}
void ReaderWriterLock_PThreads::LeaveWrite()
{
	 VERIFY( 0 == pthread_rwlock_unlock( &Lock ) );
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

	typedef void (WINAPI *fInitializeSRWLock)(__out PSRWLOCK SRWLock);
	typedef void (WINAPI *fReleaseSRWLockExclusive)(__inout PSRWLOCK SRWLock);
	typedef void (WINAPI *fReleaseSRWLockShared)(__inout PSRWLOCK SRWLock);
	typedef void (WINAPI *fAcquireSRWLockExclusive)(__inout PSRWLOCK SRWLock);
	typedef void (WINAPI *fAcquireSRWLockShared)(__inout PSRWLOCK SRWLock);

	static int SRWInitialized = 0;
	static fInitializeSRWLock mInitializeSRWLock = NULL;
	static fReleaseSRWLockExclusive mReleaseSRWLockExclusive = NULL;
	static fReleaseSRWLockShared mReleaseSRWLockShared = NULL;
	static fAcquireSRWLockExclusive mAcquireSRWLockExclusive = NULL;
	static fAcquireSRWLockShared mAcquireSRWLockShared = NULL;

	bool ReaderWriterLock_Slim::IsAvailableInKernel()
	{
#ifdef FORCE_XP
		return false;
#endif
		if ( SRWInitialized == 0 )
		{
			HMODULE kernel = GetModuleHandle( L"Kernel32.dll" );
			AbcCheckNULL( kernel );
			mInitializeSRWLock =		(fInitializeSRWLock)		GetProcAddress( kernel, "InitializeSRWLock" );
			mReleaseSRWLockExclusive =	(fReleaseSRWLockExclusive)	GetProcAddress( kernel, "ReleaseSRWLockExclusive" );
			mReleaseSRWLockShared =		(fReleaseSRWLockShared)		GetProcAddress( kernel, "ReleaseSRWLockShared" );
			mAcquireSRWLockExclusive =	(fAcquireSRWLockExclusive)	GetProcAddress( kernel, "AcquireSRWLockExclusive" );
			mAcquireSRWLockShared =		(fAcquireSRWLockShared)		GetProcAddress( kernel, "AcquireSRWLockShared" );
			SRWInitialized = mInitializeSRWLock != NULL ? 1 : -1;
		}
		return SRWInitialized == 1;
	}

	void ReaderWriterLock_Slim::Init( int maxReaders )
	{
		IsAvailableInKernel();
		mInitializeSRWLock( &Lock );
	}

	void ReaderWriterLock_Slim::EnterRead()
	{
		mAcquireSRWLockShared( &Lock );
	}

	void ReaderWriterLock_Slim::LeaveRead()
	{
		mReleaseSRWLockShared( &Lock );
	}

	void ReaderWriterLock_Slim::EnterWrite()
	{
		mAcquireSRWLockExclusive( &Lock );
	}

	void ReaderWriterLock_Slim::LeaveWrite()
	{
		mReleaseSRWLockExclusive( &Lock );
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ReaderWriterLock_Mutex::ReaderWriterLock_Mutex()
	{
		UseReadClear = true;
		WriteLock = NULL;
		ReadClear = NULL;
		ReadSemaphore = NULL;
		Reset();
	}

	ReaderWriterLock_Mutex::~ReaderWriterLock_Mutex()
	{
		Reset();
	}

	void ReaderWriterLock_Mutex::Reset()
	{
		if ( WriteLock )
		{
			// ensure that there are no locks held
#ifdef _DEBUG
			ASSERT( WaitForSingleObject( WriteLock, 0 ) == WAIT_OBJECT_0 );
			ReleaseMutex( WriteLock );
#endif
			LONG semc = 0;
			ASSERT( ReleaseSemaphore( ReadSemaphore, 1, &semc ) == FALSE );
			//ASSERT( semc == MaxReaders ); // semc is not populated

			CloseHandle( WriteLock ); WriteLock = NULL;
			CloseHandle( ReadClear ); ReadClear = NULL;
			CloseHandle( ReadSemaphore ); ReadSemaphore = NULL;
			MaxReaders = 0;
		}
	}

	void ReaderWriterLock_Mutex::Init( int maxReaders )
	{
		MaxReaders = maxReaders;
		WriteLock = CreateMutex( NULL, false, NULL );
		ReadClear = CreateEvent( NULL, true, true, NULL );
		ReadSemaphore = CreateSemaphore( NULL, maxReaders, maxReaders, NULL );
	}

	void ReaderWriterLock_Mutex::EnterRead()
	{
		if ( UseReadClear )
			WaitForSingleObject( ReadClear, INFINITE );
		WaitForSingleObject( ReadSemaphore, INFINITE );
	}

	void ReaderWriterLock_Mutex::LeaveRead()
	{
		ReleaseSemaphore( ReadSemaphore, 1, NULL );
	}

	void ReaderWriterLock_Mutex::EnterWrite()
	{
		WaitForSingleObject( WriteLock, INFINITE );
		ResetEvent( ReadClear );
		for ( int n = 0; n < MaxReaders; n++ )
			WaitForSingleObject( ReadSemaphore, INFINITE );
	}

	void ReaderWriterLock_Mutex::LeaveWrite()
	{
		ReleaseSemaphore( ReadSemaphore, MaxReaders, NULL );
		SetEvent( ReadClear );
		ReleaseMutex( WriteLock );
	}

#endif


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ReaderWriterLock::ReaderWriterLock()
	{
		Actual = NULL;
		PassThrough = 0;
	}

	ReaderWriterLock::~ReaderWriterLock()
	{
		Reset();
	}

	void ReaderWriterLock::InitAtCoreMultiple( int maxReaders_per_core )
	{
		AbcMachineInformation info;
		AbcMachineInformationGet( info );
#ifdef FORCE_SINGLE_CORE
		Init( 1 * maxReaders_per_core );
#else
		Init( info.CPUCount * maxReaders_per_core );
#endif
	}

	void ReaderWriterLock::Init( int maxReaders )
	{
		ASSERT( Actual == NULL );
		ASSERT( PassThrough == 0 );
		PassThrough = -1;
#ifdef _WIN32
		if ( ReaderWriterLock_Slim::IsAvailableInKernel() )
			Actual = new ReaderWriterLock_Slim();
		else
			Actual = new ReaderWriterLock_NG();
#else
		Actual = new ReaderWriterLock_NG();
#endif
		Actual->Init( maxReaders );
	}

	void ReaderWriterLock::InitPassThrough()
	{
		ASSERT( PassThrough == 0 );
		PassThrough = 1;
	}

	void ReaderWriterLock::Reset()
	{
		delete Actual; Actual = NULL;
		PassThrough = 0;
	}

	void ReaderWriterLock::EnterRead()
	{
		ASSERT( PassThrough != 0 );
		if ( PassThrough == 1 ) return;
		Actual->EnterRead();
	}

	void ReaderWriterLock::LeaveRead()
	{
		ASSERT( PassThrough != 0 );
		if ( PassThrough == 1 ) return;
		Actual->LeaveRead();
	}

	void ReaderWriterLock::EnterWrite()
	{
		ASSERT( PassThrough != 0 );
		if ( PassThrough == 1 ) return;
		Actual->EnterWrite();
	}

	void ReaderWriterLock::LeaveWrite()
	{
		ASSERT( PassThrough != 0 );
		if ( PassThrough == 1 ) return;
		Actual->LeaveWrite();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

// This code is functionally equivalent to ReaderWriterLock_NG. I keep this copy because it's posted on my site www.baboonz.org.
	
/** A simple Reader/Writer Lock.

This RWL has no events - we rely solely on spinlocks and sleep() to yield control to other threads.
I don't know what the exact penalty is for using sleep vs events, but at least when there is no contention, we are basically
as fast as a critical section. This code is written for Windows, but it should be trivial to find the appropriate
equivalents on another OS.

**/
class TinyReaderWriterLock
{
public:
	volatile uint32 Main;
	static const uint32 WriteDesireBit = 0x80000000;

	void Noop( uint32 tick )
	{
		if ( ((tick + 1) & 0xfff) == 0 )     // Sleep after 4k cycles. Crude, but usually better than spinning indefinitely.
			Sleep(0);
	}

	TinyReaderWriterLock()                 { Main = 0; }
	~TinyReaderWriterLock()                { ASSERT( Main == 0 ); }

	void EnterRead()
	{
		for ( uint32 tick = 0 ;; tick++ )
		{
			uint32 oldVal = Main;
			if ( (oldVal & WriteDesireBit) == 0 )
			{
				if ( InterlockedCompareExchange( (LONG*) &Main, oldVal + 1, oldVal ) == oldVal )
					break;
			}
			Noop(tick);
		}
	}

	void EnterWrite()
	{
		for ( uint32 tick = 0 ;; tick++ )
		{
			if ( (tick & 0xfff) == 0 )                                     // Set the write-desire bit every 4k cycles (including cycle 0).
				_InterlockedOr( (LONG*) &Main, WriteDesireBit );

			uint32 oldVal = Main;
			if ( oldVal == WriteDesireBit )
			{
				if ( InterlockedCompareExchange( (LONG*) &Main, -1, WriteDesireBit ) == WriteDesireBit )
					break;
			}
			Noop(tick);
		}
	}

	void LeaveRead()
	{
		ASSERT( Main != -1 );
		InterlockedDecrement( (LONG*) &Main );
	}
	void LeaveWrite()
	{
		ASSERT( Main == -1 );
		InterlockedIncrement( (LONG*) &Main );
	}
};

#endif

};
