#pragma once

#include "../Containers/pvect.h"
#include "lmIVFile.h"
#include "lmMemFile.h"
#include "lmFFile.h"
#include "VirtualFile.h"
#include <zlib/zlib.h>

/*
Compression:
We have the option of using zlib compression on the compound file.
In order to do this, open the file for creation with the 
OpenFlagCompressAll flag. The file is stored in memory.
When the compound file is saved, everything after the main header 
is compressed and then written to disk.
When the compound file is opened, it is automatically uncompressed
and loaded into memory. Thus it can serve as an in-memory archive.
*/

// Header that goes right at the beginning of a compound file
struct CFTopHeader
{
	UINT		id; // always 0xabba0070
	UINT		Version; // 1
	UINT		SubCount; // number of subfiles
	INT64		SubStart; // start of subfile header array
	UINT		Flags; // compressed, sorted, case sensitive ?
	INT64		CompressedSize; // size of compressed section following header
	INT64		UncompressedSize; // natural size of compressed section following header

	BYTE		Reserved[100]; // future
	
	// define this so its easy to see in a hex editor
	static const int HEAD_RESERVED = 100;
};

/** Compound file.

[2013-07-31 BMH] This library is positively ancient, and definitely a re-invented square
wheel. If I was to redo this, I would probably just use a plain old zip file (provided
that the index in a zip file is fast to search through, which I assume it is).

This does not support modification of existing files.

**/
class PAPI CompoundFile
{
public:

	static const int MaxFilenameLength = 127;

	enum OpenFlags
	{
		OpenFlagNone = 0,			///< Zero
		OpenFlagCreate = 1,			///< Create new compound file (or overwrite existing).
		OpenFlagModify = 2,			///< Open an existing compound file for modification.
		OpenFlagCompressAll = 4		///< Open in whole file compression mode.
	};

	enum HeadFlags
	{
		HeadFlagCompressAll = 1,	///< Whole file is compressed.

		/** File is sorted lexicographically.
		This was introduced in November 2005. Files prior to that date were not sorted.
		
		Sorting an old file is trivial:
			-# Open it for modification.
			-# Call Save().
			-# Close it.
		**/
		HeadFlagSorted = 2,
		HeadFlagCaseSensitive = 4	///< File is case sensitive.
	};

	CompoundFile();
	~CompoundFile();

	/** Set case sensitivity.
	This function will fail if any subfiles have already been written. It is read-only for
	a file that is not opened for creation.
	@return True if the file is being created and is empty.
	**/
	bool SetCaseSensitive( bool on );

	bool IsCaseSensitive() { return CaseSense; }

	/** Returns true if an existing file is sorted.
	Meaningless when creating a file, because all files that are saved are sorted.
	**/
	bool IsSorted() { return 0 != (Head.Flags & HeadFlagSorted); }

	/// Release all cached memory.
	void FlushCache();

	bool IsOpen();					///< Returns true if the file is open.
	bool IsOpenForCreate();			///< Returns true if the file is open for creation.
	bool IsOpenForModification();	///< Returns true if the file is open for modification.

	/// Returns the amount of memory used by the compound file (does not attempt to estimate heap overhead)
	size_t MemoryUsed();

	// Operations for dealing with real files
	bool Open( LPCWSTR fname, UINT flags );
	bool Open( AbCore::IFile* file, UINT flags );

	/// Open an existing compound file from memory.
	bool Open( void* mem, size_t bytes, UINT flags = OpenFlagNone );
	bool Save();
	void Close();

	/** @name Operations for dealing with sub files.
	**/
	//@{
	INT64 SubSize( LPCWSTR fname );
	bool SeekSub( LPCWSTR fname, INT64 pos );

	/** Reads a subfile into the specified buffer.
	@param bytes Number of bytes to read. If negative, the remainder of the file is read.
	**/
	bool ReadSub( LPCWSTR fname, void* buffer, INT64 bytes = -1 );

	/** Reads a subfile into the specified buffer.
	@param buffer The buffer to read into. The buffer is resized to the exact bytes if it is not initially large enough.
		The buffer is never truncated.
	@param bytes Number of bytes to read. If negative, the remainder of the file is read.
	**/
	bool ReadSub( LPCWSTR fname, dvect<BYTE>& buffer, INT64 bytes = -1 );

	/** Writes a subfile.
	The data is written to disk immediately, or to memory
	if OpenFlagCompressAll is true.
	If fname is a new name, then the subfile is created. If it is
	an existing name, then the subfile is appended at its current
	seek pos. If the operation is a modification of an existing compound
	file, then the size of the sub-file may not exceed it's original size.
	Modification is not allowed on files created with the OpenFlagCompressAll flag.
	**/
	bool WriteSub( LPCWSTR fname, const void* buffer, INT64 bytes );

	/** Truncate an existing sub file.
	Truncation is not allowed on a file created with the OpenFlagCompressAll flag.
	**/
	bool TruncateSub( LPCWSTR fname, INT64 size );

	int GetSubCount();				///< Returns number of files in archive
	XStringW GetSubName( int sub ); ///< Returns name of subfile at index \a sub

	//@}

protected:
	bool ReadHeaders();
	bool WriteHeaders();
	void ClearSubStore();
	bool ReadAllSubs();

	bool VerifyStructure();

	void CompressGlobal();
	bool UncompressGlobal();

	bool UsingNameBlocks();

	void ReindexNames();

	bool NamesEqual( LPCWSTR a, LPCWSTR b );
	bool NamesEqual( const UINT16* a, LPCWSTR b );

	enum SubFileFlags
	{
		SubFileFlagDirectory = 1
	};

	struct SubFile
	{
		// null-terminated UTF-16LE name
		UINT16  FileName[CompoundFile::MaxFilenameLength + 1];

		// location in real file
		INT64 location;

		// size of subfile
		INT64 size;
		
		// (runtime only)- the current seek position in the file
		// stored relative to its origin
		INT64 Unused_OldSeekPos;

		// combo of SubFileFlags
		UINT Flags;

		// for future
		BYTE Reserved[12];

		static int Compare( const SubFile& a, const SubFile& b );
		static int CompareUpCase( const SubFile& a, const SubFile& b );
	};

	struct NameBlock
	{
		int First;
		int Last;
		INT64 Age;
		// 20 * 168 = 3k, which is a reasonable block size
		static const int BlockSize = 20;
		SubFile Files[BlockSize];
		bool operator< ( const NameBlock& b ) const { return Age < b.Age; }
	};

	SubFile* GetSub( int index );
	SubFile* GetSub( LPCWSTR fname );
	int GetSubIndex( LPCWSTR fname );

	NameBlock* GetNameBlock( int subIndex );
	bool ReadNameBlock( NameBlock* bl );
	int FindSubIndexFromNameBlock( LPCWSTR fname );
	SubFile* FindSubFromNameBlock( LPCWSTR fname );
	SubFile* GetSubFromNameBlock( int subIndex );

	/// True if case sensitivity is on for filenames [default = off].
	bool CaseSense;

	XStringW DiskFilename;
	bool OwnMemFile;
	bool OwnSourceFile;
	INT64 Age;
	AbCore::IFile	*cf;
	AbCore::MemFile *mMemFile;
	AbCore::IFile	*SourceFile;

	// maps filenames onto indexes in SubStore;
	WStrIntMap SubMap;

	/// Set of sub files that have been created in this session.
	PtrSet		NewSubFiles;
	
	/// Cached blocks of names
	pvect< NameBlock* > NameBlocks;
	int MaxNameBlocks;

	XStringW CachedLowName;		///< Cached upon ReadHeader(), lowest name for sorted file
	XStringW CachedHighName;	///< Cached upon ReadHeader(), highest name for sorted file

	// static header read or written
	CFTopHeader Head;

	/// True if we're busy writing to a new file
	bool bWrite;

	/// True if we're busy modifying an existing file
	bool bModify;

	/// Only used if !UsingNameBlocks()
	pvect< SubFile* > SubStore;

	/// Maps name hashes to subfile indices. Makes no provision for collisions.
	Int32Int32Map LuckyMap;

	/// Always equal to the number of subfiles
	dvect< INT64 > SubSeekPos;
};
