#include "pch.h"
#ifdef _WIN32
#include <io.h>
#endif
#include "../Other/lmTypes.h"
#include "VirtualFile.h"
#include "../Strings/ConvertUTF.h"

#ifdef _WIN32
#define ftello		_ftelli64
#define fseeko		_fseeki64
#define fileno		_fileno
#define ftruncate	_chsize_s
#define fsync		_commit
#endif

#ifndef _WIN32
	#define INVALID_HANDLE_VALUE 0 // actual value is 0xFFFF...FFF on windows.
#endif

namespace AbCore
{

const int LargeAllocThreshold = 512 * 1024;

/// Test whether we can allocate a chunk of N bytes. 
/// This is a relatively slow operation, and is not intended for sizes below 512k.
#ifdef _WIN32
bool TestAlloc( size_t bytes )
{
	void* block = VirtualAlloc( NULL, bytes, MEM_RESERVE, PAGE_READWRITE );
	if ( block == NULL ) return false;
	VERIFY( VirtualFree( block, 0, MEM_RELEASE ) );
	return true;
}
#else
bool TestAlloc( size_t bytes )
{
	// I am assuming that on libc malloc does not throw an exception. The C docs say it just returns NULL.
	void* block = malloc( bytes );
	if ( block == NULL ) return false;
	free( block );
	return true;
}
#endif


#ifdef _WIN32

DiskFileWin32::DiskFileWin32()
{
	Construct();
}

DiskFileWin32::DiskFileWin32( HANDLE fh, bool ownHandle, const wchar_t* filename )
{
	Construct();
	Init( fh, ownHandle, filename );
}

void DiskFileWin32::Construct()
{
	FH = INVALID_HANDLE_VALUE;
	WinErr = 0;
	OwnHandle = true;
	CachedLength = -1;
	CachedPos = -1;
	IsShareWrite = false;
}

DiskFileWin32::~DiskFileWin32()
{
	if ( OwnHandle )
		Close();
}

bool DiskFileWin32::Flush( int flags )
{ 
	if ( !!(flags & FlushFlagPermanentStorage) )
		return FlushOSBuffers();
	else
		return true;
}

bool DiskFileWin32::FlushOSBuffers()
{
	if ( FH != INVALID_HANDLE_VALUE )
		return FlushFileBuffers( FH ) != 0;
	else
		return true;
}

size_t DiskFileWin32::Append( const void* data, size_t bytes )
{
	return AppendInternal( true, data, bytes );
}

size_t DiskFileWin32::AppendStrict( const void* data, size_t bytes )
{
	return AppendInternal( true, data, bytes );
}

// [2013-06-24 BMH]
// Why is there a non "strict" version of this function?
// I believe my original reason for this was so that you could use Append() to
// write to a log file, and you could use "tail -f" to monitor that log file.
// I thought that by locking a very high byte of the file, I could get tail to
// not die in this mode. Anyway... I never analyzed it carefully enough to actually
// determine whether the non-strict technique worked for that.
// Upon later observations, it appears the "strict" mode works just fine,
// so that is why we use it as the default now.
//
// Another observation: I don't understand why, but if you have two processes
// simultaneously appending to the same file, LockFile seems to never
// fail with ERROR_LOCK_VIOLATION, as I would expect. The only error that I ever see
// is ERROR_INVALID_HANDLE. After trying numerous things, I can only assume that
// the OS (Win7-64) has special logic to detect when you're using LockFile() in order
// to synchronize multiple agents appending to a file.
//
// My original thoughts were that I could use ERROR_INVALID_HANDLE to detect
// improper usage of this function, but that turns out to be a broken solution,
// since that is the only error that I ever manage to coax out of Windows.
size_t DiskFileWin32::AppendInternal( bool strict, const void* data, size_t bytes )
{
	if ( bytes == 0 )
		return 0;

	// [2013-06-22 BMH]
	// If AlbServer's log rotation is no longer hanging the server, then make this function return
	// zero after 100 failed attempts.
	for ( int pass = 0; true; pass++ )
	{
		SyncCache();
		u64 length = Length();
		Seek( length );

		uint64 reserve_length = bytes;
		uint64 reserve_offset = length;
		if ( !strict )
		{
			// Lock only the last byte of the file. This is internally consistent, but not consistent
			// with any other agent that is also writing to this file.
			reserve_offset = ~uint64(0) - 1;
			reserve_length = 1;
		}

		//AbcTrace( "[%8d] Trying to lock %d - %d\n", AbcProcessGetPID(), (int32) reserve_offset, (int32) reserve_length );
		if ( LockFile( FH, reserve_offset & 0xffffffff, reserve_offset >> 32, reserve_length & 0xffffffff, reserve_length >> 32 ) )
		{
			//Sleep(3000);
			SyncCache();
			DWORD written = 0;
			if ( Length() == length )
				WriteFile( FH, data, (DWORD) bytes, &written, NULL );
			BOOL ok = UnlockFile( FH, reserve_offset & 0xffffffff, reserve_offset >> 32, reserve_length & 0xffffffff, reserve_length >> 32 );
			ASSERT(ok);
			if ( written )
				return written;
		}
		else
		{
			//DWORD err = GetLastError();
			//if ( err = ERROR_INVALID_HANDLE )
			//	AbcTrace( "Bingo\n" );
			//else
			//	AbcTrace( "Error: %d\n", err );
		}
		Sleep(0);
	}
	return 0;

	/*
	
	This alternate version has its appeal:
		1. No looping
		2. Kernel can wake us when the lock becomes available
	However, it cannot be made to work in strict=false mode, so I gave up on it.

	if ( bytes == 0 )
		return 0;
	OVERLAPPED olap;
	memset( &olap, 0, sizeof(olap) );
	if ( LockFileEx( FH, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xffffffff, 0xffffffff, &olap ) )
	{
		SyncCache();
		DWORD written = 0;
		if ( Seek( Length() ) )
			WriteFile( FH, data, (DWORD) bytes, &written, NULL );
		BOOL ok = UnlockFile( FH, 0, 0, 0xffffffff, 0xffffffff );
		AbcAssert(ok);
		return written;
	}
	return 0;
	*/
}

size_t DiskFileWin32::Write( const void* data, size_t bytes )
{
	DWORD written = 0;
	if ( !WriteFile( FH, data, (DWORD) bytes, &written, NULL ) )
	{
		WinErr = GetLastError();
	}
	else
	{
		ASSERT( CachedPos != -1 && CachedLength != -1 );
		CachedPos += written;
		CachedLength = max(CachedLength, CachedPos);
	}
	return written;
}

size_t DiskFileWin32::Read( void* data, size_t bytes )
{
	DWORD read = 0;
	if ( !ReadFile( FH, data, (DWORD) bytes, &read, NULL ) )
	{
		WinErr = GetLastError();
	}
	else
	{
		ASSERT( CachedPos != -1 );
		CachedPos += read;
	}
	return read;
}


bool DiskFileWin32::Error()
{
	if ( WinErr == 0 ) return false;
	if ( LastError.IsEmpty() )
	{
		TCHAR* buff = LastError.GetBuffer( 8192 );
		if ( buff != NULL )
		{
			FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, WinErr, 0, buff, 8191, NULL );
			buff[8191] = 0;
			LastError.ReleaseBuffer();
		}
	}
	else
	{
		ASSERT(false);
	}
	return true;
}

XString DiskFileWin32::ErrorMessage()
{
	if ( WinErr != 0 && LastError.IsEmpty() )
	{
		// refresh error message
		Error();
	}
	return LastError;
}

void DiskFileWin32::ClearError()
{
	WinErr = 0;
	LastError.Clear();
}


void DiskFileWin32::Init( HANDLE fh, bool ownHandle, const wchar_t* filename )
{
	ASSERT( FH == INVALID_HANDLE_VALUE );
	FH = fh;
	OwnHandle = ownHandle;
	SyncCache();
	IsShareWrite = true; // pessimistic
	if ( filename ) FileName = filename;
}

bool DiskFileWin32::Open( const char* filename, const char* flags, int shareMode )
{
	XStringW ws = filename;
	XStringW wf = flags;
	return Open( ws, wf, shareMode );
}

bool DiskFileWin32::IsOpen() const
{
	return FH != INVALID_HANDLE_VALUE;
}

void DiskFileWin32::Close() 
{
	if ( FH != INVALID_HANDLE_VALUE )
	{
		CloseHandle( FH );
		FH = INVALID_HANDLE_VALUE;
	}
	CachedLength = -1;
	CachedPos = -1;
	IsShareWrite = false;
}

UINT64 DiskFileWin32::Position() 
{
	if ( CachedPos != -1 )
	{
		return CachedPos;
	}
	else
	{
		INT64 pos;
		LARGE_INTEGER zero;
		zero.QuadPart = 0;
		SetFilePointerEx( FH, zero, (PLARGE_INTEGER) &pos, FILE_CURRENT );
		CachedPos = pos;
		return pos;
	}
}

UINT64 DiskFileWin32::Length() 
{
	if ( UseCache() && CachedLength != -1 )
	{
		return CachedLength;
	}
	else
	{
		INT64 len = 0;
		BOOL ok = GetFileSizeEx( FH, (PLARGE_INTEGER) &len );
		if ( !ok ) WinErr = GetLastError();
		else CachedLength = len;
		return len;
	}
}

bool DiskFileWin32::SetLength( UINT64 len )
{
	UINT64 opos = Position();
	Seek( len );
	bool ok = SetEndOfFile( FH ) != 0;
	if ( !ok ) WinErr = GetLastError();
	else CachedLength = len;
	Seek( opos );
	return ok;
}

bool DiskFileWin32::Seek( UINT64 pos )
{
	INT64 newPos;
	LARGE_INTEGER ipos;
	ipos.QuadPart = pos;
	BOOL ok = SetFilePointerEx( FH, ipos, (PLARGE_INTEGER) &newPos, FILE_BEGIN );
	if ( !ok )
		WinErr = GetLastError();
	else
		CachedPos = newPos;
	ASSERT( ok == (newPos == pos) );
	return ok != 0;
}

bool DiskFileWin32::SeekRel( INT64 pos )
{
	INT64 newPos;
	LARGE_INTEGER ipos;
	ipos.QuadPart = pos;
	BOOL ok = SetFilePointerEx( FH, ipos, (PLARGE_INTEGER) &newPos, FILE_CURRENT );
	if ( !ok )
		WinErr = GetLastError();
	else
		CachedPos = newPos;
	return ok != 0;
}

bool DiskFileWin32::Open( const wchar_t* filename, const wchar_t* flags, int shareMode )
{
	ASSERT( FH == INVALID_HANDLE_VALUE );

	// we don't support "a+"
	XStringW ff = flags;
	bool read = ff.Find( 'r' ) >= 0;
	bool append = ff.Find( 'a' ) >= 0;
	bool temp = ff.Find( 'T' ) >= 0;
	bool delete_on_close = ff.Find( 'D' ) >= 0;
	bool plus = ff.Find( '+' ) >= 0;
	bool only_new = ff.Find( 'n' ) >= 0;
	bool write = ff.Find( 'w' ) >= 0 || plus || append;
	ASSERT( ff.Find( 'z' ) == -1 ) ; // This was our ASYNC flag, which was bullshit. I didn't know what the hell I was doing.
	DWORD access = GENERIC_READ;
	DWORD creat = OPEN_ALWAYS;
	//DWORD attrib = FILE_ATTRIBUTE_NORMAL;
	DWORD attrib = 0;
	DWORD share = 0;
	if ( shareMode & ShareDelete ) share |= FILE_SHARE_DELETE;
	if ( shareMode & ShareRead ) share |= FILE_SHARE_READ;
	if ( shareMode & ShareWrite ) share |= FILE_SHARE_WRITE;
	if ( read && !write ) creat = OPEN_EXISTING;
	if ( write && !read && !append ) creat = CREATE_ALWAYS;
	if ( append && (plus || read) ) creat = OPEN_ALWAYS;
	if ( only_new ) { ASSERT(!append && !plus && !write); creat = CREATE_NEW; write = true; }
	if ( !read || write || append ) access |= GENERIC_WRITE;
	if ( temp ) attrib |= FILE_ATTRIBUTE_TEMPORARY;
	//if ( delete_on_close ) attrib = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
	if ( delete_on_close ) attrib |= FILE_FLAG_DELETE_ON_CLOSE;
	FH = CreateFileW( filename, access, share, NULL, creat, attrib, NULL );
	if ( FH == INVALID_HANDLE_VALUE )
	{
		WinErr = GetLastError();
		return false;
	}
	//VERIFY( GetFileSizeEx( FH, (PLARGE_INTEGER) &CachedLength ) );

	FileName = filename;
	IsShareWrite = 0 != (share & FILE_SHARE_WRITE);

	SyncCache();

	if ( append )
	{
		Seek( Length() );
	}

	return true;
}

void DiskFileWin32::SyncCache()
{
	CachedLength = -1;
	CachedPos = -1;
	// We always cache Position because it is unique to our handle
	Position();
	if ( UseCache() )
		Length();
}


#endif

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// DiskFilePosix
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

DiskFilePosix::DiskFilePosix()
{
	Construct();
}

DiskFilePosix::DiskFilePosix( FILE* fh, bool ownHandle )
{
	Construct();
	FH = fh;
	OwnHandle = ownHandle;
}

void DiskFilePosix::Construct()
{
	LastOp = OpNone;
	FH = NULL;
	SysErr = 0;
	OwnHandle = true;
}

DiskFilePosix::~DiskFilePosix()
{
	if ( FH != NULL && OwnHandle )
	{
		Close();
	}
}

bool DiskFilePosix::Flush( int flags )
{ 
	if ( FH != NULL )
		fflush( FH );

	if ( !!(flags & FlushFlagPermanentStorage) )
		FlushOSBuffers(); 

	return true;
}

void DiskFilePosix::FlushOSBuffers()
{
	if ( FH != NULL )
	{
		fflush( FH );
		fsync( fileno(FH) );
	}
}

size_t DiskFilePosix::Write( const void* data, size_t bytes )
{
	FlushFor( OpWrite );
	size_t written = fwrite( data, 1, bytes, FH );
	if ( written != bytes )
		SysErr = ferror(FH);
	LastOp = OpWrite;
	return written;
}

size_t DiskFilePosix::Read( void* data, size_t bytes )
{
	FlushFor( OpRead );
	size_t read = fread( data, 1, bytes, FH );
	if ( read != bytes )
		SysErr = ferror(FH);
	LastOp = OpRead;
	return read;
}

bool DiskFilePosix::Error()
{
	if ( FH == NULL ) return false;
	if ( SysErr == 0 ) return false;
	ASSERT(false); // todo
	return true;
}

XString DiskFilePosix::ErrorMessage()
{
	if ( FH != NULL && SysErr != 0 && LastError.IsEmpty() )
	{
		// refresh error message
		Error();
	}
	return LastError;
}

void DiskFilePosix::ClearError()
{
	SysErr = 0;
	LastError.Clear();
}

/** Flush for read/write switching.
When combining reading and writing, one must intercede the different operation types
with a flush. I don't think this is necessary for fread/fwrite, but I leave it in anyway.
It can't hurt too much.
**/
void DiskFilePosix::FlushFor( OpType nextOp )
{
	if (	LastOp == OpRead && nextOp == OpWrite ||
			LastOp == OpWrite && nextOp == OpRead )
	{
		// [2013-02-13 BMH] I don't know where this old mumbo jumbo comes from. I did write it, but I can't find any docs now that
		// advocate this behaviour.
		// Seek( CachedPos );
	}
}

bool DiskFilePosix::Open( const char* filename, const char* flags, int shareMode )
{
	XStringW ws = filename;
	XStringW wf = flags;
	return Open( ws, wf, shareMode );
}

void DiskFilePosix::Close() 
{
	if ( FH != NULL && OwnHandle )
		fclose( FH );
	OwnHandle = false;
	FH = NULL;
	LastOp = OpNone;
}

UINT64 DiskFilePosix::Position() 
{
	return (UINT64) ftello( FH );
}

UINT64 DiskFilePosix::Length() 
{
	uint64 oldPos = Position();
	fseeko( FH, 0, SEEK_END );
	uint64 size = ftello( FH );
	fseeko( FH, oldPos, SEEK_SET );
	return size;
}

bool DiskFilePosix::SetLength( UINT64 len )
{
	if ( ftruncate( fileno(FH), len ) != 0 )
	{
		AbcTrace( "Error setting file length.\n" );
		ASSERT(false);
		return false;
	}
	return true;
}

bool DiskFilePosix::Seek( UINT64 pos )
{
	SysErr = fseeko( FH, pos, SEEK_SET );
	if ( SysErr == 0)
	{
		LastOp = OpSeek;
		return true;
	}
	return false;
}

bool DiskFilePosix::Open( const wchar_t* filename, const wchar_t* flags, int shareMode )
{
	ASSERT( FH == NULL );
	Close();
	SysErr = 0;
	LastError = "";

	XStringW filenameW = filename;
	XStringW flagsW = flags;

	// shareMode is ignored

	FH = fopen( filenameW.ToUtf8(), flagsW.ToUtf8() );
	OwnHandle = true;

	if ( FH == NULL )
		return false;

	FileName = filename;
	LastOp = OpNone;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TinyMemFile::TinyMemFile()
{
	Construct();
}

TinyMemFile::~TinyMemFile()
{
	Free();
}

TinyMemFile::TinyMemFile( void* buffer, size_t size, bool allowGrow )
{
	Construct();
	InitStatic( buffer, size, allowGrow );
}

void TinyMemFile::InitStatic( void* buffer, size_t size, bool allowGrow )
{
	Free(true);
	Data = (BYTE*) buffer;
	OwnData = false;
	AllowGrow = allowGrow;
	Allocated = size;
	BufferOutOfSpace = false;
}

TinyMemFile TinyMemFile::ReadOnlyBuffer( const void* buffer, size_t size )
{
	TinyMemFile mf;
	mf.InitStatic( const_cast<void*>(buffer), size, false );
	mf.Count = size;
	return mf;
}

void TinyMemFile::ClearNoAlloc()
{
	Count = 0;
	Pos = 0;
}

void TinyMemFile::Reset()
{
	Free();
	BufferOutOfSpace = false;
	AllowGrow = true;
}

bool TinyMemFile::Error()
{
	return BufferOutOfSpace;
}

XString TinyMemFile::ErrorMessage()
{
	return "";
}

void TinyMemFile::ClearError()
{
	BufferOutOfSpace = false;
}

bool TinyMemFile::Flush( int flags )
{
	return true;
}

bool TinyMemFile::DirectWritePrepare( size_t bytes )
{
	return Ensure( Pos + bytes );
}

void* TinyMemFile::DirectWritePos( size_t bytes )
{
	ASSERT( Pos + bytes <= Allocated );
	void* p = Data + Pos;
	IncPos( bytes );
	return p;
}

const void* TinyMemFile::DirectReadPos( size_t bytes )
{
	if ( Pos + bytes > Count ) return NULL;
	void* p = Data + Pos;
	Pos += bytes;
	return p;
}

size_t TinyMemFile::Write( const void* data, size_t bytes )
{
	if ( !Ensure( Pos + bytes ) ) return 0;
	memcpy( Data + Pos, data, bytes );
	IncPos( bytes );
	return bytes;
}

size_t TinyMemFile::Read( void* data, size_t bytes )
{
	if ( Pos >= Count ) return 0;
	bytes = min( Count - Pos, bytes );
	memcpy( data, Data + Pos, bytes );
	Pos += bytes;
	return bytes;
}

bool TinyMemFile::SetLength( UINT64 len )
{
	if ( !Ensure( len ) ) return false;
	Count = len;
	return true;
}

UINT64 TinyMemFile::Position()
{
	return Pos;
}

UINT64 TinyMemFile::Length()
{
	return Count;
}

bool TinyMemFile::Seek( UINT64 pos )
{
	Pos = pos;
	return true;
}

void TinyMemFile::IncPos( size_t bytes )
{
	Pos += bytes;
	Count = max(Count, Pos);
}

void TinyMemFile::Construct()
{
	OwnData = true;
	BufferOutOfSpace = false;
	AllowGrow = true;
	ZeroFill = false;
	Data = NULL;
	Pos = 0;
	Count = Allocated = 0;
}

bool TinyMemFile::Ensure( size_t p )
{
	if ( p > Allocated )
	{
		if ( !AllowGrow ) { BufferOutOfSpace = true; return false; }
		size_t ns = max( Allocated * 2, p );
		void* d = AbcMallocOrDie(ns);
		// if ( !d ) { BufferOutOfSpace = true; return false; } -- rather just crash, since very little code handles this path
		memcpy( d, Data, Count );
		if ( ZeroFill ) memset( (char*) d + Count, 0, ns - Count );
		Free(false);
		Data = (BYTE*) d;
		OwnData = true;
		Allocated = ns;
	}
	return true;
}

void TinyMemFile::Free( bool zero_sizes )
{
	if ( OwnData ) free(Data);
	Data = NULL;
	OwnData = false;
	if ( zero_sizes )
	{
		Pos = 0;
		Count = Allocated = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// MemFile
//
// Around every potentially large malloc/realloc, we use VirtualAlloc to detect
// whether the memory allocation will fail.
// The belief is that the user of this class knows about potential memory issues, and if
// necessary, wraps the class in Panacea's AutoMemFile.
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

MemFile::MemFile( size_t initial ) : IFile()
{
	Construct();
	if ( initial != 0 )
		Reset( initial );
}

MemFile::MemFile( void* data, size_t size, bool wrapper ) : IFile()
{
	Construct();
	if ( wrapper )
	{
		Wrapper = true;
		Data = (BYTE*) data;
		Allocated = size;
		Size = size;
		Pos = 0;
	}
	else
	{
		Reset( size );
		memcpy( this->Data, data, size );
	}
}

MemFile MemFile::CreateReadOnly( const void* data, size_t size )
{
	MemFile mf;
	mf.Wrapper = true;
	mf.Data = (BYTE*) const_cast<void*>(data);
	mf.Allocated = size;
	mf.Size = size;
	mf.Pos = 0;
	mf.ReadOnly = true;
	return mf;
}

void MemFile::Construct()
{
	ZeroFill = false;
	ReadPastEnd = false;
	OutOfMemory = false;
	WriteOverWrapper = false;
	ReadOnly = false;
	Wrapper = false;
	MappedAllowWrite = false;
	MappedAllowGrow = false;
#ifdef _WIN32
	MappedFile = INVALID_HANDLE_VALUE;
	MappedFileBase = INVALID_HANDLE_VALUE;
#endif
	MappedFileHasGrown = false;
	MappedDormant = false;
	MappedDormantStuck = false;
	MappedAsReadOnly = false;
	DebugForceRemapFail = false;
	Allocated = 0;
	Data = NULL;
	Pos = 0;
	Size = 0;
}

MemFile::~MemFile() 
{
	if ( Data && !Wrapper ) free( Data );
	if ( MappedFile != INVALID_HANDLE_VALUE )
	{
		// flush will make sure that our length is correct
		FlushInternal( FlushFlagBuffers, false );
	}
}

bool MemFile::Error()
{
	return ReadPastEnd || OutOfMemory || WriteOverWrapper;
}

XString MemFile::ErrorMessage()
{
	if ( ReadPastEnd ) return "Read past end of memory file";
	else if ( OutOfMemory ) return "Out of memory while creating memory file";
	else if ( WriteOverWrapper ) return "Attempted to write beyond or to truncate a wrapped memory file";
	else return "";
}

void MemFile::ClearError()
{
	ReadPastEnd = false;
	OutOfMemory = false;
	WriteOverWrapper = false;
}

UINT64 MemFile::Position() 
{
	return Pos;
}

UINT64 MemFile::Length()
{
	return Size;
}

bool MemFile::Seek( UINT64 pos )
{
	Pos = (size_t) pos;
	return true;
}

#ifdef _WIN32
MemFile* MemFile::CreateMappedFile( HANDLE diskFile, bool readWrite, INT64 initialSize )
{
	ASSERT( diskFile != INVALID_HANDLE_VALUE );

	LARGE_INTEGER properSize;
	GetFileSizeEx( diskFile, &properSize );

	HANDLE hmap;
	void* base;
	INT64 size;
	if ( !SetupMappedFile( hmap, base, size, diskFile, readWrite, initialSize ) )
		return NULL;

	MemFile* mf = new MemFile( base, (size_t) properSize.QuadPart, true );
	mf->Allocated = size;
	mf->MappedFileBase = diskFile;
	mf->MappedFile = hmap;
	mf->MappedAllowGrow = readWrite;
	mf->MappedAllowWrite = readWrite;
	mf->MappedDormant = false;
	mf->MappedDormantStuck = false;
	mf->MappedAsReadOnly = true;

	return mf;
}
#endif

bool MemFile::SetupMappedFile( HANDLE& mapHandle, void*& base, INT64& mappedSize, HANDLE diskFile, bool readWrite, INT64 initialSize, bool allowUnderMapped, bool allowWrite )
{
#ifdef _WIN32
	mapHandle = INVALID_HANDLE_VALUE;
	base = NULL;
	mappedSize = 0;

	LARGE_INTEGER size;
	GetFileSizeEx( diskFile, &size );

#ifndef _M_X64
	// on 32-bit hardware, disallow a mapping larger than 2 gigs
	if ( size.QuadPart >= MAXINT ) { ASSERT(false); return false; }
#endif

	if ( size.QuadPart == 0 && initialSize == 0 )
	{
		// creating a new file.
		initialSize = 65536;
	}

	if ( !allowUnderMapped )
	{
		// force at least the entire file to be mapped
		initialSize = max( initialSize, size.QuadPart );
	}

	HANDLE hmap = CreateFileMapping( diskFile, NULL, readWrite ? PAGE_READWRITE : PAGE_READONLY, initialSize >> 32, (DWORD) initialSize, NULL );
	if ( hmap == NULL ) return false;

	// always map as read-only, until the user issues a Write().
	void* mem = MapViewOfFile( hmap, allowWrite ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, (size_t) initialSize );
	if ( mem == NULL )
	{
		CloseHandle( hmap );
		return false;
	}

	mapHandle = hmap;
	base = mem;
	mappedSize = initialSize;

	return true;
#else
	return false;
#endif
}

void MemFile::Unmap()
{
#ifdef _WIN32
	if ( MappedFile != INVALID_HANDLE_VALUE )
	{
		UnmapViewOfFile( Data );
		CloseHandle( MappedFile );
		MappedFile = INVALID_HANDLE_VALUE;
		Data = NULL;
	}
#endif
}

void MemFile::WakeUp( bool forWrite )
{
#ifdef _WIN32
	if (	(MappedDormant || (forWrite && MappedAsReadOnly)) 
				&& !MappedDormantStuck )
	{
		if ( !Remap( 0, forWrite ) )
		{
			AbcTrace( "MappedDormantStuck !!!!!!\n" );
			MappedDormantStuck = true;
		}
	}
#endif
}

bool MemFile::Remap( INT64 newSize, bool forWrite )
{
#ifdef _WIN32
	Unmap();

	if ( DebugForceRemapFail ) return false;

	if ( newSize == 0 ) newSize = Allocated;

	void* base;
	INT64 hsize;
	if ( SetupMappedFile( MappedFile, base, hsize, MappedFileBase, MappedAllowWrite, newSize, false, forWrite ) )
	{
		/* -- moved to Write().
		if ( newSize > Allocated )
		{
			MappedFileHasGrown = true;
		}
		*/
		MappedAsReadOnly = !forWrite;
		MappedDormant = false;
		Data = (BYTE*) base;
		Allocated = hsize;
		return true;
	}
	else
	{
		// if we were doing a growing remap, then we still have a chance.
		if ( (size_t) newSize > Allocated )
		{
			ASSERT(false);
			// try to remap back into our original size, so that we at least preserve our existing data,
			// since a write failure shouldn't cause all our data to be lost.
			if ( SetupMappedFile( MappedFile, base, hsize, MappedFileBase, MappedAllowWrite, Allocated, true, forWrite ) )
			{
				// phew!
				ASSERT( hsize == Allocated );
				Data = (BYTE*) base;
			}
			OutOfMemory = true;
		}
		return false;
	}
#else
	return false;
#endif
}

size_t MemFile::WriteThrough( const void* data, size_t bytes )
{
#ifdef _WIN32
	// a last resort
	ASSERT( MappedDormantStuck );
	DiskFile df( MappedFileBase, false );
	df.Seek( Pos );
	size_t b = df.Write( data, bytes );
	Pos += b;
	Size = max( Size, Pos );
	return b;
#else
	return 0;
#endif
}

size_t MemFile::ReadThrough( void* data, size_t bytes )
{
#ifdef _WIN32
	// a last resort
	ASSERT( MappedDormantStuck );
	DiskFile df( MappedFileBase, false );
	df.Seek( Pos );
	size_t b = df.Read( data, bytes );
	Pos += b;
	return b;
#else
	return 0;
#endif
}

size_t MemFile::Write( const void* data, size_t bytes )
{
	if ( ReadOnly || bytes == 0 )
		return 0;

	WakeUp( true );
	if ( MappedDormantStuck ) return WriteThrough( data, bytes );

	if ( Pos + bytes > Allocated )
	{
		ReAlloc( max(Pos + bytes + 8, Allocated * 2 + 8) );
	}

	if ( Data != NULL && Pos + bytes <= Allocated )
	{
		memcpy( Data + Pos, data, bytes );
		Pos += bytes;
		if ( Pos > Size && MappedAllowGrow )
		{
			MappedFileHasGrown = true;
		}
		Size = max(Size, Pos);
		return bytes;
	}
	else
	{
		return 0;
	}
}

size_t MemFile::Read( void* data, size_t bytes )
{
	if ( bytes == 0 ) return 0;
	if ( Pos >= Size )
	{
		ReadPastEnd = true;
		return 0;
	}
	WakeUp();
	if ( MappedDormantStuck ) return ReadThrough( data, bytes );
	//ASSERT( Pos + bytes <= Size );
	// clip edge
	size_t read_bytes = bytes;
	if (Pos + bytes > Size)
		read_bytes = Size - Pos;
	if ( read_bytes > 0 )
	{
		memcpy( data, Data + Pos, read_bytes );
		Pos += read_bytes;
	}
	if ( read_bytes < bytes )
	{
		ReadPastEnd = true;
	}
	return read_bytes;
}

bool MemFile::SetLength( UINT64 len )
{
	if ( ReadOnly )
		return false;

	if ( MappedFile != INVALID_HANDLE_VALUE )
	{
		// Windows doesn't support this
		ASSERT(false);
		return false;
	}

	if ( Wrapper )
	{
		ASSERT(false);
		return false;
	}

	if ( len > Allocated )
		ReAlloc( len );

	Size = len;

	// a lie, since we could so easily run out of memory
	return true;
}

bool MemFile::Flush( int flags )
{
	if ( MappedFile != INVALID_HANDLE_VALUE )
	{
		FlushInternal( flags, false );
		MappedDormant = true;
	}
	return true;
}

void MemFile::FlushInternal( int flags, bool remapAfterFlush )
{
#ifdef _WIN32
	if ( MappedFile != INVALID_HANDLE_VALUE )
	{
		FlushViewOfFile( Data, 0 );
		if ( !!(flags & FlushFlagPermanentStorage) )
			FlushFileBuffers( MappedFileBase );

		if ( Allocated != Size )
		{
			// ensure that our length is correct.
			Unmap();
			
			LARGE_INTEGER pold, pos, pdist;

			// query the current file position
			pdist.QuadPart = 0;
			SetFilePointerEx( MappedFileBase, pdist, &pold, FILE_CURRENT );

			// set the file position to our desired size
			pdist.QuadPart = Size;
			VERIFY( SetFilePointerEx( MappedFileBase, pdist, &pos, FILE_BEGIN ) );

			// do the truncate
			VERIFY( SetEndOfFile( MappedFileBase ) );

			// restore the file pointer
			SetFilePointerEx( MappedFileBase, pold, NULL, FILE_BEGIN );

			if ( remapAfterFlush )
				Remap( Allocated, !MappedAsReadOnly );
		}
		else 
		{
			if ( !remapAfterFlush )
				Unmap();
		}
	}
#endif
}

void MemFile::Truncate( UINT64 size )
{
#ifdef _WIN32
	ASSERT( MappedFile == INVALID_HANDLE_VALUE );
#endif
	if ( Wrapper )
	{
		WriteOverWrapper = true;
		return;
	}
	Size = size;
}

void MemFile::Reset( size_t newsize )
{
	if ( Wrapper ) { ASSERT(false); return; }
	if ( Data ) free( Data );
	Data = NULL;
	ReadPastEnd = false;
	OutOfMemory = false;
	Wrapper = false;
	ReadOnly = false;
	Allocated = 0;
	if ( newsize > 0 )
	{
		if ( newsize > LargeAllocThreshold && !TestAlloc( newsize ) )
		{
			// don't even try
		}
		else
		{
			// we live on the assumption that this will never fail
			Data = (BYTE*) malloc( newsize );
		}

		if ( Data != NULL )
			Allocated = newsize;
		else
			OutOfMemory = true;

	}
	Pos = 0;
	Size = 0;
}



void MemFile::ReAlloc( size_t size )
{
	if ( Wrapper )
	{
		if ( MappedAllowGrow )
		{
			Remap( size, true );
		}
		else
		{
			ASSERT(false);
			WriteOverWrapper = true;
		}
		return;
	}
	if ( size == -1 )
		size = _max_( Allocated * 2, 1024 );

	if ( size > 2300 * 1000 )
		int aaaa = 1;

	BYTE* nd = NULL;

	if ( size > LargeAllocThreshold && !TestAlloc( size ) )
	{
		// don't even try.
	}
	else
	{
		nd = (BYTE*) AbcReallocOrDie( Data, size );
		if ( ZeroFill )
		{
			memset( nd + Allocated, 0, size - Allocated );
		}
	}

	if ( nd == NULL )
	{
		// out of memory
		ASSERT( false );
		OutOfMemory = true;
	}
	else
	{
		Data = nd;
		Allocated = size;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t IFile::ReadChunked( void* data, size_t bytes, size_t chunkSize )
{
	if ( chunkSize == 0 ) { ASSERT(false); return 0; }

	size_t total = 0;
	while ( bytes > 0 && !Error() )
	{
		size_t eat = min(bytes, chunkSize);
		size_t read = Read( data, eat );
		total += read;
		bytes -= read;
		(BYTE*&) data += read;
		if ( read != eat ) break;
	}

	return total;
}

UINT64 IFile::Copy( UINT64 bytes, IFile* src, IFile* dst )
{
	return Copy( bytes, src, src->Position(), dst, dst->Position() );
}

UINT64 IFile::Copy( UINT64 bytes, IFile* src, UINT64 srcPos, IFile* dst, UINT64 dstPos )
{
	if ( bytes == 0 ) return 0;

	BYTE statBlock[1024];
	size_t blockSize = AbcMin( (size_t) 65536, (size_t) bytes );
	
	void* tempBuff = blockSize <= sizeof(statBlock) ? statBlock : malloc( blockSize );
	if ( tempBuff == NULL ) return 0;

	size_t avail = src->Length() - srcPos;
	if ( bytes > avail )
	{
		ASSERT( false );
		bytes = avail;
	}

	size_t written = 0;
	size_t read = 0;

	while ( bytes > 0 )
	{
		size_t block = min( (size_t) bytes, blockSize );
		if ( !src->Seek( srcPos ) ) break;
		size_t bytesRead = src->Read( tempBuff, block );
		srcPos += block;
		read += bytesRead;
		bytes -= bytesRead;
		if ( bytesRead < block )
		{
			// read error
			//ASSERT(false);
			break;
		}
		if ( !dst->Seek( dstPos ) ) break;
		size_t writtenNow = dst->Write( tempBuff, block );
		dstPos += block;
		written += writtenNow;
		if ( writtenNow < block )
		{
			// write error
			//ASSERT(false);
			break;
		}
	}

	if ( tempBuff != statBlock )
		free( tempBuff );

	return written;
}

UINT64 IFile::Write( UINT64 bytes, IFile* src )
{
	return Copy( bytes, src, this );
}

UINT64 IFile::Read( UINT64 bytes, IFile* dst )
{
	return Copy( bytes, this, dst );
}

int IFile::Compare( UINT64 bytes, IFile* src1, IFile* src2 )
{
	int csize = 4096;
	
	void* chunk1 = malloc( csize );
	if ( chunk1 == NULL ) { ASSERT(false); return -1; }
	void* chunk2 = malloc( csize );
	if ( chunk2 == NULL ) { ASSERT(false); free(chunk1); return -1; }

	// This ain't correct. 
	ASSERT(false);
	while ( true )
	{
		size_t r1 = src1->Read( chunk1, csize );
		size_t r2 = src2->Read( chunk2, csize );
		if ( r1 != r2 ) return 1;
		if ( r1 == 0 ) break; 
		if ( memcmp( chunk1, chunk2, r1 ) != 0 ) return 1;
	}

	free( chunk2 );
	free( chunk1 );

	return 0;
}

}

#ifndef _WIN32
	#undef INVALID_HANDLE_VALUE
#endif
