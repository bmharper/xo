#pragma once

#include "../Strings/XString.h"
#include "CompoundFile.h"

/* We use zlib and our compound file class to do a packed file system. write-once-able.
See CompoundFile for more details. This is actually a thin (gasp!) wrapper around
CompoundFile. If I had been wiser in my youth I would simply have used the .zip format.
*/
class PAPI PackFile
{
public:
	PackFile();
	~PackFile();

	// Reading functions
	bool		Open( LPCWSTR pakfile );
	bool		Open( void* mem, size_t bytes, bool copy_memory );
	size_t		GetFileSize( LPCWSTR fname );
	int			GetFileCount();
	XStringW	GetFileName( int index );
	bool		ReadFile( LPCWSTR fname, void* buff );
	XStringA	ReadTextFileA( LPCWSTR fname );
	XStringW	ReadTextFileW( LPCWSTR fname );

	// Writing functions
	bool	Create( LPCWSTR pakfile );

	/** Recursively add a path.
	@param ignore Semicolon-separated list of wildcards to ignore.
	**/
	int		AddPath( LPCWSTR root, LPCWSTR wildcard = NULL, LPCWSTR ignore = NULL );
	bool	AddFile( LPCWSTR nameInPakFile, LPCWSTR filepath );
	bool	Save();

	// Common functions
	void Close();
	void SetCaseSensitivity( bool on );

	/// Returns the amount of memory used by the packfile
	size_t MemoryUsed();

protected:
	wchar_t			DirChar;
	bool			CaseSense;
	CompoundFile*	CF;
	void*			OwnMem;
	size_t			OwnMemSize;
	
	int AddPath( const XStringW& root, const XStringW& relpath, const XStringW& wildcard, const vector< XStringW >& ignore, vector< XStringW >& files, vector< int >& filesizes );
};
