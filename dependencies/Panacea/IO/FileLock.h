#pragma once

namespace Panacea
{
	namespace IO
	{

		/** Create and manage a single file that is used to control multi-user locks on a resource.

		Overview
		--------

		What this thing does is create and use a file that holds X number of lock records. Every lock
		record is a fixed binary layout and is 128 bytes. There is no header in the file. We use
		the FileLock API to acquire a mutex on the lock file. This is not entirely dependable. My original
		scheme here was to use XML to describe the lock records, but while testing that, I came to the 
		conclusion that the LockFile API, when used to lock a file on a network share, is not dependable
		as a semaphore across multiple client machines. This is unfortunate. My solution is to lower
		the probability of a collision happening.

		At startup, a process generates a GUID, and uses that GUID to identify itself within the lockfile,
		for the duration of that process. During the first call to SetLock(), we find a slot for ourselves.
		This is hopefully the only step where things can go wrong, although the odds are hopefully small.
		If the client OS always writes chunked data, and those chunks are larger than 128 bytes, then we
		are truly screwed, but this does not seem to be the case. We find a slot for ourselves among
		64 random open slots. If 64 open slots do not exist within the file, then we add those to the end
		(conceptually) and pick one out of the total 64. The potential pitfall here is when 2 clients
		initialize their lock slots, and they both pick the same one. Ideally, the LockFile API should
		protect us against such an issue, and it does appear to help, but it does also appear to be
		broken in some sense, although it is beyond me to determine exactly under what conditions it
		fails. In any case, once we have a slot, we keep that slot for the duration of our dealings
		with this locked resource. When we leave, we clear our lock record, and close the lock file.
		Note that setting our lock to 'none' is not equivalent to leaving completely.

		-- This is now implemented and usable with GetAllLocks( useMutex = false ) --
		Another potential improvement is to allow reading the status of the file without acquiring the mutex.
		This could help a lot in improving the speed of the pollers. We could simply use the validity of
		the record's Adler32 checksum to verify integrity of records (although my guess is that the server
		OS probably gives us atomicity at a level of 128 bytes, but we can't know that for sure).

		-- This is now implemented and usable with ExpireTimeSeconds --
		One thing which would be good to implement is use of the Touched field, which marks, in client
		machine time, the last time that the lock record was touched. This is not really used yet, but it
		is set by SetLock(). I envision using this to detect when a machine has 'gone away', and left
		some kind of lock. I initially thought that I would have to use the clocks on the client machines,
		but then I realized that one can simply use the file modification times to provide a common time
		mechanism.
		My very brief experiments with windows file times on SAMBA and Win2k3 servers shows that the last 
		access time is updated, but not reliably more than once a minute. I think a timeout of less than 5
		minutes is dangerous. A timeout of 15 minutes is probably quite safe. NEGATIVE. Some more tests
		prove that when a file handle is kept open, we can have gigantic delays in updates of all 3 of the
		file times. This is really not reliable.

		Future improvements
		-------------------

		There is this problem with this thing, and that is how do we manage clients that 'go away'. That is,
		clients which take out a lock, and then crash or whatnot, and leave their lock lingering.

		Originally, I wanted to use FILE_FLAG_DELETE_ON_CLOSE. However, that has this problem:
		After the first CloseHandle(), all subsequent Open operations on that machine, to that file, will fail.

		Our solution?
		Our solution is to always try and delete the lock file before starting. That will cause the OS to
		do its homework, and check whether there are any handles open on the file. If the delete succeeds, then
		we assume that any supposed readers have crashed and did not properly release their locks.

		This means that should users encounter locked files, which they believe are not locked, their instructions are:
		Visit all the machines that claim to have locks on the file. Make sure that none of them do indeed
		have locks. Try and open the file. If the lock file really is not in use, then the OS will allow it to be
		deleted, and we will start with a clean slate.

		All functions wrap themselves in Acquire/Release calls. If you would like to make a bunch of operations atomic,
		then you can do Acquire/Release manually. A typical case might be:
		
		- Acquire
		- GetAllLocks
		- If (highest lock is Read lock) then SetLock(Modify)
		- Release


		General Usage:
		- SetIdentity
		- Open
		- Use Lock functions (GetAllLocks, RemoveLock, SetLock), optionally wrapped inside Acquire/Release calls.
		- RemoveLock, if you currently hold one.
		- Close

		**/
		class PAPI FileLock
		{
		public:
			FileLock();
			~FileLock();

			// 128 byte lock record
			struct LockData
			{
				static const int UserBytes = 32;
				static const int LockTypeBytes = 32;

				UINT32 Adler32;
				UINT32 ProcessId;	///< ProcessID. This is the Windows GetCurrentProcessID().
				UINT64 Touch;
				BYTE Id[16];	// A regular GUID
				char User[UserBytes];
				char LockType[LockTypeBytes];
				UINT64 MachineId;	///< Some id that, together with the User field, uniquely identifies a machine. Zero means unknown.
													// 'Machine' in this definition means a domain where ProcessId is meaningful.
				BYTE ReservedB[24];
				
				static LockData* Create()
				{
					LockData* d = new LockData;
					d->Reset();
					return d;
				}

				void Reset()
				{
					memset( this, 0, sizeof(*this) );
				}

				/// True if this is the magic header, which we always leave at lock 0 (the first lock record).
				bool IsMagicHeader()
				{
					return Adler32 == 0xB00BD00D && ProcessId == 0xBAADD0D0;
				}
				void SetMagicHeader()
				{
					Reset();
					Adler32 = 0xB00BD00D;
					ProcessId = 0xBAADD0D0;
				}

				// Failure to do this is just a pointless security hole
				void TerminateStrings()
				{
					User[ UserBytes - 1 ] = 0;
					LockType[ LockTypeBytes - 1 ] = 0;
				}

				bool VerifyAdler32()
				{
					return Adler32 == ComputeAdler32();
				}

				UINT32 ComputeAdler32()
				{
					return AbCore::Adler32::Compute( &Adler32 + 1, sizeof(*this) - 4 );
				}

				void TerminateStringsAndComputeAdler32()
				{
					TerminateStrings();
					Adler32 = ComputeAdler32();
					ASSERT( VerifyAdler32() );
				}

				void ClearLock()
				{
					memset( LockType, 0, sizeof(LockType) );
				}

				XStringA GetUser()
				{
					return XStringA( User, UserBytes );
				}

				XStringA GetLock()
				{
					return XStringA( LockType, LockTypeBytes );
				}

				void SetLock( const XStringA& lockType )
				{
					memcpy( LockType, (LPCSTR) lockType, min( lockType.Length(), LockTypeBytes ) );
				}

				void SetTouchTime( Sys::Date d )
				{
					Touch = d.MicroSecondsUTC_1601();
				}

				Sys::Date TouchTime()
				{
					return Sys::Date::NEWTOR_1601(Touch);
				}

				Guid GetId()
				{
					Guid g;
					memcpy( &g, Id, sizeof(Id) );
					return g;
				}

				void SetId( Guid id, UINT32 processId, UINT64 machineId )
				{
					memcpy( Id, &id, sizeof(id) );
					this->ProcessId = processId;
					this->MachineId = machineId;
				}

				bool IsIdNull()
				{
					for ( int i = 0; i < sizeof(Id); i++ )
						if (Id[i] != 0) return false;
					return true;
				}

				bool IsLockEmpty()
				{
					for ( int i = 0; i < sizeof(LockType); i++ )
						if (LockType[i] != 0) return false;
					return true;
				}
			};

			/** Grant EVERYONE and SYSTEM full access to the file. The file must have been opened with WRITE_DAC permissions. 
			**/ 
			static bool GrantAllAccess( HANDLE file );

			/** Setup your identity.
			@id The unique identifier that is used to track clients.
			@user A textual name, typically something like your host name.
			**/
			void SetIdentity( Guid id, XStringA user );

			Guid GetIdentity() { return IdentId; }

			/** Open the locking file at the specified path.
			**/
			bool Open( XString filename );

			/// Returns true if we have a valid handle to the locking file.
			bool IsOpen();

			/** Close the locking file.
			It is your responsibility to free your lock before closing down. The system will debug assert if you still have a lock.
			**/
			void Close();

			/// Acquire the Mutex
			bool Acquire();
			
			/// Release the Mutex
			void Release();

			/** Retrieve all locks.
			It is your responsibility to delete_all(locks) after using the lock data.
			**/
			bool GetAllLocks( pvect<LockData*>& locks, bool useMutex = true, dvect<int>* slots = NULL );

			/** Remove your lock.
			**/
			bool RemoveLock();

			/** Set your lock type.
			@param lockType A textual lock type.
			**/
			bool SetLock( XStringA lockType );

			/// Hijack someone else's lock, setting it to whatever you like
			bool HijackSetLock( int slot, XStringA lockType, bool clear );

			/// Touch your lock (keep alive).
			bool TouchLock();

			/** Return the value that we last set.
			This does not read from the file at all.
			**/
			XStringA GetLockInternal();

			/// Return a current time, calibrated to the file access time, and delta-ed with our machine clock.
			Sys::Date CurrentTimeByEpoch();

			// We always take the more conservative approach, using the min/max of epoch time and machine clock time.
			Sys::Date CurrentTimeMin();
			Sys::Date CurrentTimeMax();

			double ComputeMinAge( Sys::Date d ) { return CurrentTimeMin().SecondsSince( d ); }

			/// Holds the most recent error message
			XString LastError;

			/// Entropy number. Higher values reduce the chance of collisions at lock initialization time.
			int EntropyN;

			/// Use the ProcessId field to ignore processes on this machine that have died. Default = true.
			bool UseProcessId;

			/// Try to delete the lock file at Open() and Close()
			bool DeleteFileAtOpenClose;

			/// Use the lock record touch time to expire old locks. Default = 2 hours.
			double ExpireTimeSeconds;

		protected:
			AbCore::DiskFile File;
			XString Filename;

			Guid IdentId;
			XStringA IdentUser;
			DWORD IdentProcessId;
			UINT64 IdentMachineId;
			XStringA CurLock;
			int CurSlot;
			
			Sys::Date EpochFromFile;
			Sys::Date EpochFromClock;
			
			int MutexAcquired;
			pvect<LockData*> Locks;

			void SetError( XString msg );

			bool Begin();
			bool Commit();
			void UpdateCurrentTime();
			void CleanupDeadProcessesOnThisMachine();
			void CleanupDeadProcessesByTime();

			enum ReadDocStatus
			{
				ReadDocOk = 1,
				ReadDocTryAgain = -1,
				ReadDocAbort = -2
			};

			ReadDocStatus ReadDoc();
			bool WriteDoc();
			bool WriteSlot( int slot );
			int Find( Guid id, pvect<LockData*>& locks, bool forWrite );

		};
	}

}
