#include "pch.h"
#include "FileLock.h"
#include "../Xml/Xml.h"
#include "../Windows/Registry.h"
#include "../Platform/debuglib.h"
#include <Aclapi.h>
#include <Sddl.h>

DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_MIN_MAX_28285

using namespace AbCore;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define DMSG printf
#define DMSG(...) ((void)0)

// FileLock object must never grow larger than this
static const int LockRegionSize = 1024 * 1024;

namespace Panacea
{
namespace IO
{

FileLock::FileLock()
{
	CurSlot = -1;
	MutexAcquired = 0;
	UseProcessId = true;
	DeleteFileAtOpenClose = true;
	
	// this must be explicitly enabled, because the clients must be aware that they need to poll with
	// TouchLock(), which adb does not do right now.
	ExpireTimeSeconds = 1e9 * 120 * 60; 

	// 64 * 128 = 8192, which is OK in my book.
	ASSERT( sizeof(LockData) == 128 );
	EntropyN = 64;
}

FileLock::~FileLock()
{
	ASSERT( MutexAcquired == 0 );
	Close();
}

void FileLock::SetError( XString msg )
{
	LastError = msg;
}

void FileLock::SetIdentity( Guid id, XStringA user )
{
	ASSERT( !IsOpen() );
	IdentId = id;
	IdentUser = user.Left( LockData::UserBytes - 1 );
	IdentProcessId = GetCurrentProcessId();
	
	// compute an ID unique to this machine. We should really read the logon token too.. since multiple users may be
	// logged onto the same machine.. windows server scenario, and we probably won't have the necessary rights to tell
	// whether a process ID is valid for other users.
	dvect<BYTE> prodId, licInfo;
	Panacea::Win::Registry::ReadBinStr( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "DigitalProductId", prodId );
	Panacea::Win::Registry::ReadBinStr( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "LicenseInfo", licInfo );
	DWORD installDate = Panacea::Win::Registry::ReadDword( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "InstallDate" );
	AbCore::MD5 c;
	uint64 chash[2];
	c.Append( prodId.data, prodId.size() );
	c.Append( licInfo.data, licInfo.size() );
	c.Append( &installDate, sizeof(installDate) );
	c.Finish( chash );
	IdentMachineId = chash[0];
}

bool FileLock::IsOpen()
{
	return File.FH != INVALID_HANDLE_VALUE;
}

bool FileLock::Open( XString filename )
{
	if ( IdentId.IsNull() || IdentUser == "" )
	{
		LastError = "No Identity, or User is blank";
		return false;
	}

	//DocVer = 0;
	//Rnd.Initialise( clock() );

	bool del = false;
	DWORD delErr = 0;

	if ( DeleteFileAtOpenClose )
	{
		// This behaviour is described in the class' main docs.
		del = DeleteFile( filename ) != 0;
		delErr = del ? 0 : GetLastError();
		// if ( del ) wprintf( L"Open: deleted %s\n", (LPCWSTR) filename );
	}

	// if ( !File.Open( filename, L"arD", IFile::ShareDelete | IFile::ShareRead | IFile::ShareWrite ) )

	// Only attempt to alter the ACL if the file does not already
	bool creator = del || delErr == ERROR_FILE_NOT_FOUND;
	bool writeDACL = creator;
	while ( true )
	{
		DWORD access = GENERIC_WRITE | GENERIC_READ;
		if ( writeDACL ) access |= WRITE_DAC;
		HANDLE fh = CreateFile( filename, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		//if ( !File.Open( filename, L"ar", IFile::ShareRead | IFile::ShareWrite ) )
		if ( fh == INVALID_HANDLE_VALUE )
		{
			DWORD err = GetLastError();
			if ( err == ERROR_LOCK_VIOLATION )
			{
				// Try again for our chance. Somebody is holding the mutex, but shouldn't be for long.
				continue;
			}
			else if ( err == ERROR_ACCESS_DENIED && writeDACL )
			{
				// Try without WRITE_DAC
				writeDACL = false;
				continue;
			}
			LastError = AbcSysErrorMsg( err );
			return false;
		}
		else
		{
			// Give all users permissions on the lock file
			if ( writeDACL ) VERIFY( GrantAllAccess( fh ) );
			File.Init( fh, true, filename );
			break;
		}
	}

	if ( creator )
	{
		// write the header
		File.Seek( 0 );
		LockData ld0;
		ld0.SetMagicHeader();
		size_t bb = File.Write( &ld0, sizeof(ld0) );
		if ( bb != sizeof(ld0) )
		{
			ASSERT(false);
			File.Close();
			return false;
		}
	}

	// Do an acquire/release test.
	if ( !Acquire() )
	{
		File.Close();
		return false;
	}
	Release();

	Filename = filename;
	return true;
}

void FileLock::Close()
{
	ASSERT( CurLock == "" );
	//ASSERT( MutexFile.FH == INVALID_HANDLE_VALUE );
	File.Close();

	BOOL del = false;
	if ( DeleteFileAtOpenClose )
	{
		del = DeleteFile( Filename );
		//if ( del ) wprintf( L"Close: deleted %s\n", (LPCWSTR) Filename );
	}

	delete_all( Locks );

	Filename = "";
	LastError = "";
	CurSlot = -1;
}

bool FileLock::GetAllLocks( pvect<LockData*>& locks, bool useMutex, dvect<int>* slots )
{
	if ( useMutex )
	{
		if ( !Acquire() )
		{
			ASSERT(false);
			return false;
		}
	}
	else
	{
		while ( true )
		{
			ReadDocStatus st = ReadDoc();
			if ( st == ReadDocOk )
				break;
			else if ( st == ReadDocAbort )
				return false;
			else
				Sleep(1);
		}
	}

	for ( int i = 0; i < Locks.size(); i++ )
	{
		if ( !Locks[i]->IsIdNull() && !Locks[i]->IsLockEmpty() )
		{
			locks += new LockData( *Locks[i] );
			if ( slots ) *slots += i;
		}
	}

	if ( useMutex )
	{
		Release();
	}

	return true;
}

int FileLock::Find( Guid id, pvect<LockData*>& locks, bool forWrite )
{
	// Open slots is any slot who's ID is null, as well as X slots above the last slot,
	// where X is the entropy number that we choose minus the number of free existing slots.
	// The higher the entropy number, the lower the chance of collisions.
	// I most certainly don't like this situation, but short of introducing heinous pauses into
	// mutex acquisition, it is the best that I can do, since I cannot get LockFile to create a
	// 100% dependable semaphore.
	dvect<int> openSlots;

	for ( int i = 0; i < locks.size(); i++ )
	{
		if ( memcmp(locks[i]->Id, &id, 16) == 0 )
		{
			if ( CurSlot != -1 ) ASSERT( CurSlot == i );
			return i;
		}
		else if ( locks[i]->IsIdNull() )
		{
			openSlots += i;
		}
	}

	if ( forWrite )
	{
		while ( openSlots.size() < EntropyN )
		{
			openSlots += locks.size();
			locks += LockData::Create();
		}
		int rand = Panacea::Sys::Date::Now().MicroSecondsUTC_1601() % EntropyN;
		return openSlots[rand];
	}
	else
	{
		return -1;
	}
}

bool FileLock::Begin()
{
	if ( !Acquire() ) return false;
	return true;
}

bool FileLock::Commit()
{
	bool error = !WriteDoc();
	Release();
	return !error;
}

XStringA FileLock::GetLockInternal()
{
	return CurLock;
}

bool FileLock::RemoveLock()
{
	if ( !Begin() ) return false;

	int ix = Find( IdentId, Locks, false );
	if ( ix >= 0 )
	{
		Locks[ix]->Reset();
		CurLock = "";
		CurSlot = ix;
		bool ok = Commit();
		CurSlot = -1;
		return ok;
	}
	else
	{
		Release();
		return true;
	}
}

bool FileLock::HijackSetLock( int slot, XStringA lockType, bool clear )
{
	if ( !Begin() ) return false;

	if ( slot < 0 || slot >= Locks.size() ) { ASSERT(false); return false; }

	LockData* ld = Locks[slot];

	if ( clear )
	{
		ld->Reset();
	}
	else
	{
		ld->SetLock( lockType );
		ld->TerminateStringsAndComputeAdler32();
	}

	int prevSlot = CurSlot;
	CurSlot = slot;
	bool ok = Commit();
	CurSlot = prevSlot;

	return ok;
}

bool FileLock::SetLock( XStringA lockType )
{
	if ( lockType.Length() > LockData::LockTypeBytes - 1 ) { ASSERT(false); return false; }

	if ( !Begin() ) return false;

	int ix = Find( IdentId, Locks, true );
	LockData* ld = Locks[ix];
	ld->Reset();
	ld->SetId( IdentId, IdentProcessId, IdentMachineId );
	ld->SetTouchTime( CurrentTimeMax() );
	memcpy( ld->User, (LPCSTR) IdentUser, IdentUser.Length() );
	ld->SetLock( lockType );
	ld->TerminateStringsAndComputeAdler32();

	CurSlot = ix;
	if ( Commit() )
	{
		CurLock = lockType;
		return true;
	}
	CurSlot = -1;
	return false;
}

bool FileLock::TouchLock()
{
	if ( !Begin() ) return false;
	bool ok = false;
	if ( CurSlot >= 0 && CurSlot < Locks.size() && Locks[CurSlot]->GetId() == IdentId && Locks[CurSlot]->VerifyAdler32() )
	{
		Locks[CurSlot]->SetTouchTime( CurrentTimeMax() );
		Locks[CurSlot]->Adler32 = Locks[CurSlot]->ComputeAdler32();
		ok = Commit();
	}
	else Release();
	return ok;
}

bool FileLock::WriteDoc()
{
	return WriteSlot( CurSlot );
}

bool FileLock::WriteSlot( int slot )
{
	ASSERT( slot >= 0 && slot < Locks.size() );

	// 1 for the header slot
	int slotPos = slot + 1;

	if ( !File.Seek( slotPos * sizeof(LockData) ) ) { ASSERT(false); return false; }
	if ( File.Write( Locks[slot], sizeof(LockData) ) != sizeof(LockData) ) { ASSERT(false); return false; }

	return true;
}

FileLock::ReadDocStatus FileLock::ReadDoc()
{
	UINT32 size = File.Length();
	if ( size % sizeof(LockData) != 0 ) { ASSERT(false); return ReadDocAbort; }

	File.Seek(0);

	// read header
	LockData ld0;
	size_t br = File.Read( &ld0, sizeof(LockData) );
	// This is very common on a single machine with two lockers thrashing. br will be zero. It's the LockFile API coming in.
	if ( br != sizeof(LockData) ) { /*DMSG("MagicHeader read failed (%d read)\n", (int) br);*/ return ReadDocTryAgain; }
	if ( !ld0.IsMagicHeader() ) { DMSG("IsMagicHeader failed\n"); return ReadDocAbort; }

	// -1 for the header
	int slots = (size / sizeof(LockData)) - 1;
	delete_all(Locks);

	for ( int i = 0; i < slots; i++ )
	{
		LockData* ld = LockData::Create();
		Locks += ld;
		size_t brl = File.Read( ld, sizeof(LockData) );
		if ( brl != sizeof(LockData) )
			return ReadDocTryAgain;
		if ( !ld->IsIdNull() && !ld->VerifyAdler32() )
		{
			// This is potentially dangerous, as it could cause an infinite loop. However, I have yet to come across
			// this happening in practice. I did come across it once when I was trying to hijack a busy running process,
			// but that was due to me not computing the checksum before writing the lock record.
			DMSG( "Adler32 failed in ReadDoc\n" );
			return ReadDocTryAgain;
		}
		ld->TerminateStrings();
	}

	return ReadDocOk;
}

bool FileLock::Acquire()
{
	if ( MutexAcquired ) { MutexAcquired++; return true; }
	
	while ( true )
	{
		BOOL ok = LockFile( File.FH, 0, 0, LockRegionSize, 0 );
		if ( ok )
		{
			ReadDocStatus rd = ReadDoc();
			if ( rd != ReadDocOk )
			{
				// try again
				if ( rd == ReadDocTryAgain )
					DMSG( "LockFile passed, but ReadDoc failed- Unlocking and trying again\n" );
				else
					DMSG( "ReadDoc bad karma. Aborting\n" );
				UnlockFile( File.FH, 0, 0, LockRegionSize, 0 );
				if ( rd != ReadDocTryAgain ) return false;
			}
			else
			{
				MutexAcquired++;
				UpdateCurrentTime();
				CleanupDeadProcessesOnThisMachine();
				CleanupDeadProcessesByTime();
				return true;
			}
		}
		else
		{
			int er = GetLastError();
			if ( er != ERROR_LOCK_VIOLATION )
			{
				LastError = AbcSysErrorMsg( er );
				DMSG( "LockFile failed: %d, %s\n", er, (LPCSTR) LastError.ToAscii() );
				return false;
			}
		}
		Sleep(1);
	}
}

void FileLock::Release()
{
	ASSERT( MutexAcquired > 0 );
	MutexAcquired--;
	if ( MutexAcquired > 0 ) return;

	BOOL ok = UnlockFile( File.FH, 0, 0, LockRegionSize, 0 );
	ASSERT( ok );

	delete_all(Locks);
}

Sys::Date FileLock::CurrentTimeByEpoch()
{
	double delta = Sys::Date::Now().SecondsSince( EpochFromClock );
	Sys::Date d = EpochFromFile;
	d.AddSeconds( delta );
	return d;
}

Sys::Date FileLock::CurrentTimeMin()
{
	return min( CurrentTimeByEpoch(), Sys::Date::Now() );
}

Sys::Date FileLock::CurrentTimeMax()
{
	return max( CurrentTimeByEpoch(), Sys::Date::Now() );
}

void FileLock::UpdateCurrentTime()
{
	FILETIME create, access, modify;
	GetFileTime( File.FH, &create, &access, &modify );
	Sys::Date dcreate(create), daccess(access), dmodify(modify);
	Sys::Date dmax = max( dcreate, daccess );
	dmax = max( dmax, dmodify );
	if ( dmax != EpochFromFile )
	{
		EpochFromFile = dmax;
		EpochFromClock = Sys::Date::Now();
	}
	//printf( "FileTime: %s\n", (LPCSTR) CurFileTime.FormatSimple().ToAscii() );
}

void FileLock::CleanupDeadProcessesOnThisMachine()
{
	// Detect processes on this machine that have died, and clear their locks.
	for ( int i = 0; i < Locks.size(); i++ )
	{
		if (	Locks[i]->VerifyAdler32() &&
					Locks[i]->GetId() != IdentId &&
					Locks[i]->ProcessId != IdentProcessId &&
					Locks[i]->MachineId == IdentMachineId &&
					XStringA(Locks[i]->User) == IdentUser )
		{
			HANDLE hproc = OpenProcess( SYNCHRONIZE, false, Locks[i]->ProcessId );
			if ( hproc == NULL )
			{
				// Process is dead
				DMSG( "Found a dead process (%X) on this machine\n", (UINT) Locks[i]->ProcessId );
				Locks[i]->Reset();
				WriteSlot( i );
			}
			else
			{
				// Process is still alive
				CloseHandle( hproc );
			}
		}
	}
}

void FileLock::CleanupDeadProcessesByTime()
{
	// Detect locks that are stale
	for ( int i = 0; i < Locks.size(); i++ )
	{
		if (	Locks[i]->VerifyAdler32() &&
					Locks[i]->GetId() != IdentId )
		{
			double age = CurrentTimeMin().SecondsSince( Sys::Date::NEWTOR_1601(Locks[i]->Touch) );
			if ( age > ExpireTimeSeconds )
			{
				// Slot has expired
				DMSG( "Found an expired slot (belonging to %s)\n", Locks[i]->User );
				Locks[i]->Reset();
				WriteSlot( i );
			}
		}
	}
}

bool FileLock::GrantAllAccess( HANDLE file )
{
	//DMSG( "Writing All Access DACL\n" );
	// This was copied from lic3, which is where I first wrote this.

	// This little here routine is helpful in learning how to set DACLs... so keep it around.
	//HANDLE hf = CreateFile( LicFilePath( true ), GENERIC_WRITE | WRITE_DAC, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	// Everyone has full access:				D:PAI(A;;FA;;;WD)
	// Everyone and SYSTEM has full access:		D:PAI(A;;FA;;;WD)(A;;FA;;;SY) 

	// This is Everyone and SYSTEM:
	LPWSTR sddl = _T("D:PAI(A;;FA;;;WD)(A;;FA;;;SY)");

	PSECURITY_DESCRIPTOR desc;
	ULONG descSize;

	BOOL okc = ConvertStringSecurityDescriptorToSecurityDescriptor( sddl, SDDL_REVISION_1, &desc, &descSize );

	BOOL present, defaulted;
	PACL dacl;
	BOOL ok = GetSecurityDescriptorDacl( desc, &present, &dacl, &defaulted );

	DWORD res = SetSecurityInfo( file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, dacl, NULL );
	if ( res != ERROR_SUCCESS )
	{
		DWORD err = GetLastError();
	}

	LocalFree( desc );

	//CloseHandle( hf );
	return res == ERROR_SUCCESS;
}

}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plain C interface for FileLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace Panacea::IO;

extern "C"
{

	PAPI FileLock* FileLock_Create()
	{
		return new FileLock();
	}

	PAPI void FileLock_Destroy( FileLock* fl )
	{
		delete fl;
	}

	PAPI void FileLock_SetIdentity( FileLock* fl, const GUID* id, LPCSTR userName )
	{
		fl->SetIdentity( Guid(*id), userName );
	}

	PAPI LPCWSTR FileLock_LastError( FileLock* fl )
	{
		return fl->LastError;
	}

	PAPI INT32 FileLock_Open( FileLock* fl, LPCWSTR filename )
	{
		return fl->Open( filename );
	}

	PAPI void FileLock_Close( FileLock* fl )
	{
		return fl->Close();
	}

	PAPI INT32 FileLock_SetLock( FileLock* fl, LPCSTR lockType )
	{
		return fl->SetLock( lockType );
	}

	enum GetLockFlags
	{
		GetLockFlag_UseMutex = 1,
		GetLockFlag_OnlyForeign = 2
	};

	PAPI INT32 FileLock_GetLocks( FileLock* fl, FileLock::LockData* data, INT32* record_count, INT32 flags )
	{
		pvect<FileLock::LockData*> all;
		if ( fl->GetAllLocks( all, flags & GetLockFlag_UseMutex ) )
		{
			bool need_more = false;
			int slots_left = *record_count;
			*record_count = 0;
			for ( int i = 0; i < all.size(); i++ )
			{
				if ( all[i]->VerifyAdler32() )
				{
					if ( all[i]->GetId() == fl->GetIdentity() && 0 != (flags & GetLockFlag_OnlyForeign) )
						continue;
					if ( slots_left > 0 )
					{
						memcpy( &data[*record_count], all[i], sizeof(FileLock::LockData) );
						slots_left++;
						(*record_count)++;
					}
					else need_more = true;
				}
			}
			delete_all( all );
			return need_more ? -1 : 1;
		}
		return 0;
	}

	PAPI INT32 FileLock_TouchLock( FileLock* fl )
	{
		return fl->TouchLock();
	}

	PAPI INT32 FileLock_Acquire( FileLock* fl )
	{
		return fl->Acquire();
	}

	PAPI void FileLock_Release( FileLock* fl )
	{
		fl->Release();
	}

	PAPI INT32 FileLock_RemoveLock( FileLock* fl )
	{
		return fl->RemoveLock();
	}

}
