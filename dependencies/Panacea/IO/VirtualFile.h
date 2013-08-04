#ifndef ABCORE_INCLUDED_VIRTUAL_FILE_H
#define ABCORE_INCLUDED_VIRTUAL_FILE_H

#include <stdio.h>
#include <fcntl.h>
#include "../Strings/XString.h"
#include "../Containers/pvect.h"

#include "IFile.h"

namespace AbCore
{

#ifdef _WIN32

/** Implementation of IFile utilizing Windows API functions.

This object is quite a raw interface over the Windows API. If you are going to
be reading in chunks less than a couple of kilobytes, then I suggest you use
Panacea.IO.BufferedFile, which wraps any IFile with memory buffers.

It turns out to be rather important to cache the file length, whenever we can, since
BufferedFile reads our length very frequently. For files on the network, this can be
a killer. For that reason, if we are opened without SHARE_WRITE, then we can safely
cache our length, since we are the only writer.


**/
class PAPI DiskFileWin32 : public IFile
{
public:
	HANDLE		FH;
	bool		OwnHandle;
	int			WinErr;

					DiskFileWin32();
					DiskFileWin32( HANDLE fh, bool ownHandle, const wchar_t* filename = NULL );
	virtual			~DiskFileWin32();
	virtual size_t	Write(const void* data, size_t bytes);
	virtual size_t	Read(void* data, size_t bytes);
	virtual bool	Error();
	virtual XString	ErrorMessage();
	virtual void	ClearError();
	virtual bool	Flush( int flags );
	virtual UINT64	Position(); 
	virtual UINT64	Length(); 
	virtual bool	SetLength( UINT64 len );
	virtual bool	Seek( UINT64 pos ); 
	virtual bool	SeekRel( INT64 pos );

	UINT64	DebugGetCachedLength() { return CachedLength; }	// Used for testing
	UINT64	DebugGetCachedPos() { return CachedPos; }		// Used for testing

	void	Init( HANDLE fh, bool ownHandle, const wchar_t* filename = NULL );
	bool	Open( const char* filename, const char* flags, int shareMode = ShareRead );
	bool	Open( const wchar_t* filename, const wchar_t* flags, int shareMode = ShareRead );
	bool	IsOpen() const;
	void	Close();
	bool	FlushOSBuffers();								// Issue an OS flush instruction on the file handle.
	void	SyncCache();									// Use this only if you use the file handle from multiple locations. It syncs up the CachedLength and CachedPos fields.
	size_t	Append( const void* data, size_t bytes );		// Built for simultaneous appending to a log file. It simply uses LockFile( very_high_bytes ) as a synchronization mechanism.
	size_t	AppendStrict( const void* data, size_t bytes );	// This locks the file strictly, but it causes "tail -f logfile" to abort

protected:
	XString		LastError;
	bool		IsShareWrite;
	UINT64		CachedLength;
	UINT64		CachedPos;

	bool	UseCache() { return !IsShareWrite; }
	void	Construct();
	size_t	AppendInternal( bool strict, const void* data, size_t bytes );
};
#endif


/** Implementation of IFile utilizing fopen/fread etc functions.
**/
class PAPI DiskFilePosix : public IFile
{
public:
	FILE*	FH;
	bool	OwnHandle;

					DiskFilePosix();
					DiskFilePosix( FILE* fh, bool ownHandle );
	virtual			~DiskFilePosix();
	virtual size_t	Write( const void* data, size_t bytes );
	virtual size_t	Read( void* data, size_t bytes );
	virtual bool	Error();
	virtual XString ErrorMessage();
	virtual void	ClearError();
	virtual bool	Flush( int flags );
	virtual UINT64	Position(); 
	virtual UINT64	Length(); 
	virtual bool	SetLength(UINT64 len);
	virtual bool	Seek(UINT64 pos); 

	bool	Open( const char* filename, const char* flags, int shareMode = ShareRead );
	bool	Open( const wchar_t* filename, const wchar_t* flags, int shareMode = ShareRead );
	void	Close();
	void	FlushOSBuffers();	///< Issue an OS flush instruction on the file handle.

protected:
	enum OpType
	{
		OpNone,
		OpRead,
		OpWrite,
		OpSeek
	};

	XString		LastError;
	int			SysErr;
	OpType		LastOp;

	void	FlushFor( OpType nextOp );
	void	Construct();
};

#ifdef _WIN32
typedef DiskFileWin32 DiskFile;
#else
typedef DiskFilePosix DiskFile;
#endif

/// Really simple memory file that can take an initial static buffer for small writes.
class PAPI TinyMemFile : public IFile
{
public:
	BYTE*		Data;
	size_t		Count;
	size_t		Allocated;
	size_t		Pos;
	bool		OwnData;
	bool		AllowGrow;
	bool		ZeroFill;
	bool		BufferOutOfSpace;	///< This is flagged if AllowGrow is false, and we run out of space, or if a malloc() fails

				TinyMemFile();
				TinyMemFile( void* buffer, size_t size, bool allowGrow = true );
				~TinyMemFile();

	void		InitStatic( void* buffer, size_t size, bool allowGrow = true );

	static TinyMemFile ReadOnlyBuffer( const void* buffer, size_t size );

	virtual bool		Error();
	virtual XString		ErrorMessage();
	virtual void		ClearError();
	virtual bool		Flush( int flags );

	virtual size_t		Write( const void* data, size_t bytes );
	virtual size_t		Read( void* data, size_t bytes );
	virtual bool		SetLength( UINT64 len );
	virtual UINT64		Position();
	virtual UINT64		Length();
	virtual bool		Seek( UINT64 pos );

	bool				DirectWritePrepare( size_t bytes );
	void*				DirectWritePos( size_t bytes );
	const void*			DirectReadPos( size_t bytes );

	void				ClearNoAlloc();
	void				Reset();

protected:
	void				IncPos( size_t bytes );
	void				Construct();
	bool				Ensure( size_t p );
	void				Free( bool zero_sizes = true );
};

/// Memory file implementation for IFile. Used for in-process serialization
class PAPI MemFile : public IFile
{
private:

#ifndef _WIN32
	typedef void *HANDLE;
#endif

	void Construct();

public:
	BYTE*		Data;					// Can be used to directly access data
	size_t		Size;					// Size of file
	size_t		Pos;					// Position in file
	size_t		Allocated;				// Size of allocated buffer
	bool		Wrapper;				// Set if we do not manage the memory given to us, but merely act as an intermediate layer.
	bool		ReadOnly;				// If true, then Write() always fails.
	bool		ZeroFill;				// If true, then we fill new space with zero any time we get sized up. Default = false.
	HANDLE		MappedFileBase;			// The handle of a file that we are mapping.
	HANDLE		MappedFile;				// The handle of a file mapping object.
	// True if we're a wrapper around a mapped file and we have permission to increase the file size.
	// Growing the mapped file requires recreation of the file mapping handle.
	bool		MappedAllowGrow;
	bool		MappedAllowWrite;		// True if we're a wrapper around a mapped file and we have write permission.
	bool		MappedFileHasGrown;		// True if we have grown our mapped file.
	bool		MappedDormant;			// True if we have unmapped ourselves because of a Flush().
	bool		MappedDormantStuck;		// True if we failed to awake out of dormancy.
	// We have mapped our pages as FILE_MAP_READ, either because we are opened in read-only
	// mode, or because we want to protect ourselves from mistakenly writing to the user's file.
	bool		MappedAsReadOnly;
	bool		DebugForceRemapFail;	// For testing. Forces the next mapping operation to fail. This is to test the fallback onto non-mapped IO.
	bool		OutOfMemory;			// Toggled if we run out of memory. Makes Error = true.
	bool		ReadPastEnd;			// Toggled if we attempt to read past end of file. Makes Error = true.
	bool		WriteOverWrapper;		// Toggled if we attempt to overwrite a wrapper (ie write more than its size). Makes Error = true.


	MemFile( size_t initial = 0 );

	/** Create a memory file of the data specified.
	\param wrapper If false, then the data specified is copied into the file's
	own buffer. If wrapper is true, then the file acts only as an intermediate
	between the data and the user of the file. If wrapper is true, then the
	file will never free or reallocate the memory.
	**/
	MemFile( void* data, size_t size, bool wrapper = false );

	virtual ~MemFile();

	static MemFile CreateReadOnly( const void* data, size_t size );

#ifdef _WIN32
	/** Create a MemFile that is in fact a complete memory-mapped view of the specified disk file.
	This creates a memory-mapped view of the specified file.
	The view is destroyed when the object is deleted.
	The view can be used to grow the file, but not to truncate it.

	Note that if even if you create the file with write access, you will
	not have underlying memory protection write permission until the first issue
	of a call to Write(). This is in order to safeguard the file's memory, in the case
	where you open it for read/write access, but only read from it.

	Note that when growing a file, the size of the file will be larger than the exact
	number of bytes that you have written. The file's size is only correctly set when
	you issue a Flush(), or when you delete the MemFile object. A Flush() will cause
	the file to be unmapped, placing it in the dormant state. When a Read() or Write()
	is requested, the file will be remapped. Should this remap fail, then the MemFile
	will resort to a straight patch-through of all commands onto the underlying file object,
	which will typically result in very slow IO, but it is better than outright failure.
	On 64-bit systems remapping should never fail.

	@param diskFile The handle of the already-open file on disk.
	@param readWrite If true, then write access is required on the file handle, 
		and subsequently allowed on the mapped memory region.
	@param initialSize The initial size of a new mapped file.
		Setting this parameter to a non-zero value is only used when creating a mapped file.
	@return The memory-mapped file, or NULL upon failure.
	**/
	static MemFile* CreateMappedFile( HANDLE diskFile, bool readWrite = true, INT64 initialSize = 0 );
#endif

	virtual bool		Error();
	virtual XString		ErrorMessage();
	virtual void		ClearError();

	virtual size_t		Write( const void* data, size_t bytes );
	virtual size_t		Read( void* data, size_t bytes );

	virtual UINT64		Position();
	virtual UINT64		Length();
	virtual bool		SetLength( UINT64 len );
	virtual bool		Seek( UINT64 pos );

	/** Flushes memory mapped files and unmaps them from the process address space.
	If operating on a mapped file, this calls the Windows API function FlushViewOfFile; otherwise it does nothing.
	**/
	virtual bool		Flush( int flags );

	void		Reset( size_t newsize = 0 );
	void		Truncate( UINT64 size );
	void		ReAlloc( size_t size = -1 );

protected:
	void		Unmap();
	bool		Remap( INT64 newSize = 0, bool forWrite = false );
	void		WakeUp( bool forWrite = false );
	void		FlushInternal( int flags, bool remapAfterFlush );
	size_t		WriteThrough( const void* data, size_t bytes );
	size_t		ReadThrough( void* data, size_t bytes );
	static bool SetupMappedFile( HANDLE& mapHandle, void*& base, INT64& mappedSize, HANDLE diskFile, bool readWrite, INT64 initialSize, bool allowUnderMapped = false, bool allowWrite = false );
};

}

#endif
