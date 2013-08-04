#pragma once

#ifdef _WIN32
#include <intrin.h>
#endif

#include "../Platform/syncprims.h"

namespace AbCore
{

	class PAPI IReaderWriterLock
	{
	public:
		virtual ~IReaderWriterLock() {}
		virtual void Init( int maxReaders ) = 0;
		virtual void InitPassThrough() { ASSERT(false); } ///< Intended to be implemented by wrapper
		virtual void Reset() = 0;
		virtual void EnterRead() = 0;
		virtual void LeaveRead() = 0;
		virtual void EnterWrite() = 0;
		virtual void LeaveWrite() = 0;
	};

#ifdef _WIN32
	/** Multiple Readers and one Writer.

	This is an absurd implementation. Use the newer _NG implementation. Having a limited number of
	readers is completely unnecessary.

	Important to note that the ReadClear event is not necessary for correct operation (depends of course
	on your definition of correct. I am saying here that correctness tolerates an infinite wait).
	It is merely there to prevent writer starvation due to readers continuously acquiring read locks.

	Acquiring the Write Lock is O(n) on the maximum number of readers, but I think the constant is small.

	Write Sequence
	--------------

	* Acquire Write Lock
	* Clear ReadClear Event
	* Acquire all Reader Semaphores
	Write
	* Release all Reader Semaphores
	* Set ReadClear Event
	* Release the Write Lock

	Read Sequence
	-------------

	* Wait for ReadClear event
	* Acquire Reader Semaphore
	Read
	* Release Reader Semaphore

	**/
	class PAPI ReaderWriterLock_Mutex : public IReaderWriterLock
	{
	public:
		HANDLE WriteLock;
		HANDLE ReadClear;
		HANDLE ReadSemaphore;
		int MaxReaders;

		// Turning this off doesn't have a measurable effect on time to acquire read locks, but it does have a very noticeable effect on the time
		// to acquire write locks (like a 4x speedup.. perhaps because I have 4 cores).
		// However, as you increase the number of threads well beyond your number of cores, then this has a much smaller impact on write lock acquisition.
		bool UseReadClear;

		ReaderWriterLock_Mutex();
		virtual ~ReaderWriterLock_Mutex();
		virtual void Reset();;
		virtual void Init( int maxReaders );
		virtual void EnterRead();
		virtual void LeaveRead();
		virtual void EnterWrite();
		virtual void LeaveWrite();

	};
#endif


	/** A better (and simpler) Reader/Writer Lock for XP.
	
	This RWL has no events - we rely solely on spinlocks and Sleep() to yield control to other threads.
	I don't know what the exact penalty is for that, but at least when there is no contention, we are basically
	as fast as a critical section.

	**/
	class PAPI ReaderWriterLock_NG : public IReaderWriterLock
	{
	public:
		static const uint32 WriteDesireBit = 0x80000000;

		volatile uint32 Main;

						ReaderWriterLock_NG();
		virtual			~ReaderWriterLock_NG();
		virtual void	Reset();
		virtual void	Init( int maxReaders );
		virtual void	EnterRead();
		virtual void	EnterWrite();
		virtual void	LeaveRead();
		virtual void	LeaveWrite();

		void Noop( uint32 tick );
	};

#ifndef _WIN32
	class PAPI ReaderWriterLock_PThreads : public IReaderWriterLock
	{
	public:
		pthread_rwlock_t	Lock;

						ReaderWriterLock_PThreads();
		virtual			~ReaderWriterLock_PThreads();
		virtual void	Reset();
		virtual void	Init( int maxReaders );
		virtual void	EnterRead();
		virtual void	EnterWrite();
		virtual void	LeaveRead();
		virtual void	LeaveWrite();
	};
#endif

#ifdef _WIN32
	class PAPI ReaderWriterLock_Slim : public IReaderWriterLock
	{
	public:
		SRWLOCK Lock;

		/// Returns true if we can find our functionality in the kernel (Vista and Above).
		static bool IsAvailableInKernel();

		ReaderWriterLock_Slim()
		{
		}
		virtual ~ReaderWriterLock_Slim()
		{
		}

		virtual void Init( int maxReaders );
		virtual void Reset() {}

		virtual void EnterRead();
		virtual void LeaveRead();
		virtual void EnterWrite();
		virtual void LeaveWrite();

	};
#endif


	/// Automatically chooses the best available
	class PAPI ReaderWriterLock : public IReaderWriterLock
	{
	public:
		IReaderWriterLock* Actual;
		BYTE PassThrough;

		ReaderWriterLock();
		virtual ~ReaderWriterLock();
		void InitAtCoreMultiple( int maxReaders_per_core );	///< Initialize and set number of readers to a multiple of the number of CPU cores in the system
		virtual void Init( int maxReaders );
		virtual void InitPassThrough();
		virtual void Reset();
		virtual void EnterRead();
		virtual void LeaveRead();
		virtual void EnterWrite();
		virtual void LeaveWrite();

		// NOT ATOMIC!
		void LeaveReadThenEnterWrite()
		{
			LeaveRead();
			EnterWrite();
		}
	};

	class TakeWriterLock
	{
	public:
		IReaderWriterLock* RWL;

		TakeWriterLock( IReaderWriterLock& rwl )
		{
			RWL = &rwl;
			RWL->EnterWrite();
		}
		~TakeWriterLock()
		{
			RWL->LeaveWrite();
		}

	};

	class TakeReaderLock
	{
	public:
		IReaderWriterLock* RWL;

		TakeReaderLock( IReaderWriterLock& rwl )
		{
			RWL = &rwl;
			RWL->EnterRead();
		}
		~TakeReaderLock()
		{
			RWL->LeaveRead();
		}

	};

	// Starts off taking a read lock, which you can upgrade if you need.
	// Note that the 'upgrade' is not atomic. If it was atomic, then you would easily fall into
	// the trap of writing deadlocking code.
	class TakeReaderWriterLock
	{
	public:
		IReaderWriterLock*	RWL;
		bool				HaveWrite;

		TakeReaderWriterLock( IReaderWriterLock& rwl )
		{
			HaveWrite = false;
			RWL = &rwl;
			RWL->EnterRead();
		}
		~TakeReaderWriterLock()
		{
			if ( HaveWrite )	RWL->LeaveWrite();
			else				RWL->LeaveRead();
		}

		// NOT ATOMIC!
		// Indeed, if this was atomic, then you would likely end up writing a lot of deadlocking code.
		// If you already have a write lock, then this is a no-op.
		void LeaveReadThenEnterWrite()
		{
			if ( HaveWrite ) return;
			RWL->LeaveRead();
			RWL->EnterWrite();
			HaveWrite = true;
		}

	};

}
