#pragma once

#include "../Containers/podvec.h"
#include "../Containers/pvect.h"
#include "../System/Date.h"
#include "../IO/IO.h"

#ifdef _WIN32
	typedef HANDLE AbcFileSysTxHandle;
#else
	// We simply don't support the concept of file-system transactions on other platforms
	typedef void* AbcFileSysTxHandle;
#endif

/*
PanPath____    My new namespace-less path util functions.

Note also -- the usage of the word FOLDER in Panacea::IO::Path wrong. It should be directory. Folder refers to the managed things such as "My Documents".. they are a shell level concept,
but we're dealing here mostly with file-system level concepts.

Why const XString& instead of LPCWSTR? Because it allows us to use ascii strings instead of L"" strings.

*/
enum PanPath_ResolveFlags
{
	PanPath_AllowCurrentDir = 1,
	PanPath_OnlyFile = 2,
	PanPath_OnlyDir = 4,
};

PAPI	bool	PanPath_IsRelative( const XString& path );
PAPI	bool	PanPath_ResolveInplace( XString& path, u32 flags = 0 );		// This leaves 'path' untouched if the resolve fails. Use the bool return value to detect success of failure
inline	bool	PanPath_ResolveFileInplace( XString& path, u32 flags = 0 )									{ return PanPath_ResolveInplace(path, (flags | PanPath_OnlyFile) & ~PanPath_OnlyDir); }
inline	bool	PanPath_ResolveDirInplace( XString& path, u32 flags = 0 )									{ return PanPath_ResolveInplace(path, (flags | PanPath_OnlyDir) & ~PanPath_OnlyDir); }	
PAPI	XString	PanPath_Resolve( const XString& path, u32 flags = 0 );
inline	XString	PanPath_ResolveFile( const XString& path, u32 flags = 0 )									{ return PanPath_Resolve(path, (flags | PanPath_OnlyFile) & ~PanPath_OnlyDir); }
inline	XString	PanPath_ResolveDir( const XString& path, u32 flags = 0 )									{ return PanPath_Resolve(path, (flags | PanPath_OnlyDir) & ~PanPath_OnlyFile); }
PAPI	XString PanPath_Resolve( intp n, const XString* roots, const XString& path, u32 flags = 0 );
inline	XString PanPath_Resolve( const podvec<XString>& roots, const XString& path, u32 flags = 0 )			{ return PanPath_Resolve(roots.size(), &roots[0], path, flags); }
inline	XString PanPath_ResolveFile( const podvec<XString>& roots, const XString& path, u32 flags = 0 )		{ return PanPath_Resolve(roots.size(), &roots[0], path, (flags | PanPath_OnlyFile) & ~PanPath_OnlyDir); }
inline	bool	PanPath_FileExists( const XString& path, u32 flags = 0 )									{ return !PanPath_ResolveFile(path, flags).IsEmpty(); }
inline	bool	PanPath_DirExists( const XString& path, u32 flags = 0 )										{ return !PanPath_ResolveDir(path, flags).IsEmpty(); }

// Any pointer parameter may be NULL
PAPI	bool	PanPath_GetFileTimes( const XString& name, AbcDate* timeCreate, AbcDate* timeAccess, AbcDate* timeWrite, UINT64* fileSize = NULL, AbcFileSysTxHandle transaction = NULL );

PAPI	XString	PanPath_RollingPair( const XString& filename, u64 maxFileSize );
PAPI	void	PanPath_RollingPair_Paths( const XString& filename, XString& xpath, XString& ypath );
PAPI	bool	PanPath_SetFileLength( const XString& filename, u64 size );

PAPI	bool	PanPath_DeleteFile( const XString& filename );

inline	XString EnsureTrailingSlash( const XString& path, wchar_t slash = '\\' )							{ return path.EndsWith(slash) ? path : path + slash; }
inline	XString RemoveTrailingSlash( const XString& path )													{ return (path.EndsWith(L'/') || path.EndsWith(L'\\')) ? path.Chopped() : path; }
inline	XString FileExtension( const XString& path )														{ int ldot = path.ReverseFind(L'.'); return ldot != -1 ? path.Mid(ldot + 1) : L""; } // dot not included
PAPI	XString FileNameAndExtension( const XString& path );
PAPI	XString FileNameOnly( const XString& path );									// example: C:\dir\filename.txt		==> filename
PAPI	XString DirOnly( const XString& path );											// example: C:\dir\filename.txt		==> C:\dir\   
PAPI	XString ChangeFileNameOnly( const XString& path, const XString& newName );		// example: C:\dir\oldname.txt		==> C:\dir\newname.txt
PAPI	XString ChangeExtension( const XString& path, const XString& extension );		// example: C:\dir\oldname.exe		==> C:\dir\oldname.bak		If extension is empty, then the resulting file has no dot.
PAPI	XString JoinPath( const XString& a, const XString& b );

namespace Panacea
{
	namespace IO
	{
		/// Flags for MoveFile
		enum MoveFileFlags
		{
			/// Default
			MoveFileFlagNone = 0,
			/// Allow the file to be moved across volumes, which simulates the move with a copy + delete.
			MoveFileFlagAllowCopyDelete = 1
		};

		/// Flags for FindFiles
		enum FindFlags
		{
			FindIgnoreHidden = 1,
			FindIgnoreSystem = 2,
			FindRecursiveOnAllDirectories = 8,	///< c:\mypath\*.cpp   will recurse into all directories, looking for files with a .cpp extension.
			FindFullPath = 16,					///< Return full paths, instead of paths relative to base directory.
		};

		/** Result item of a file find.
		**/
		class PAPI FileFindItem
		{
		public:
			XStringW	Name;
			AbcDate		DateCreate;
			AbcDate		DateModify;
			AbcDate		DateAccess;
			INT64		Size;
			bool		Directory;
			bool		AttribArchive;
			bool		AttribReadOnly;
			bool		AttribHidden;
			bool		AttribSystem;

			bool operator<( const FileFindItem& b ) const
			{
				return Name < b.Name;
			}
		};

		/** Path related functions.
		**/
		class PAPI Path
		{
		public:

#ifdef _WIN32
			static TCHAR DirSepChar()		{ return '\\'; }		// Returns the directory separator character.
			static XString DirSepStr()		{ return "\\"; }		// Returns the directory separator character (as a string).
			static XStringA DirSepStrA()	{ return "\\"; }		// Returns the directory separator character (as a string).
			static XStringW DirSepStrW()	{ return L"\\"; }		// Returns the directory separator character (as a string).
#else
			static TCHAR DirSepChar()		{ return '/'; }			// Returns the directory separator character.
			static XString DirSepStr()		{ return "/"; }			// Returns the directory separator character (as a string).
			static XStringA DirSepStrA()	{ return "/"; }			// Returns the directory separator character (as a string).
			static XStringW DirSepStrW()	{ return L"/"; }		// Returns the directory separator character (as a string).
#endif


			/// Combines the path pieces
			static XStringW Combine( const XStringW& left, const XStringW& right );
			static XStringW Combine( const XStringW& p1, const XStringW& p2, const XStringW& p3 );
			static XStringW Combine( const XStringW& p1, const XStringW& p2, const XStringW& p3, const XStringW& p4 );
			static XStringW Combine( const XStringW& p1, const XStringW& p2, const XStringW& p3, const XStringW& p4, const XStringW& p5 );

			/// Combines the path pieces
			static XStringA Combine( const XStringA& left, const XStringA& right );

			static char DirSep( const XStringA& str );
			static wchar_t DirSep( const XStringW& str );

			/** Finds files that match the wildcard search criteria.
			@param searchWildcard The pattern to match the files to (for example "C:\\Audio\\Sample*.wav").
			@param items The resulting items. These must be deleted by the creator of the query.
			@return The number of files found.
			**/
			static int FindFiles( const XStringW& searchWildcard, pvect< FileFindItem* >& items, uint flags = 0, AbcFileSysTxHandle transaction = NULL );

			/// Slimmed down FindFiles. Only returns files (ie no folders). searchWildcard can have multiple wildcards, separated with semicolons.
			static int FindFilesOnly( const XStringW& searchWildcard, dvect<XStringW>& relPaths, uint flags = 0, AbcFileSysTxHandle transaction = NULL );

			/// Only returns folders. searchWildcard can have multiple wildcards, separated with semicolons.
			static int FindFoldersOnly( const XStringW& searchWildcard, dvect<XStringW>& relPaths, uint flags = 0, AbcFileSysTxHandle transaction = NULL );

			/// Wrapper of _splitpath
			static void Split( const XStringA& path, XStringA* drive, XStringA* directory, XStringA* filename = NULL, XStringA* extension = NULL );

			/// Wrapper of _wsplitpath
			static void Split( const XStringW& path, XStringW* drive, XStringW* directory, XStringW* filename = NULL, XStringW* extension = NULL );

			/// Return the name with all illegal characters stripped out (\/*?:<>|).
			static XStringA CleanFilename( const XStringA& path, char sub_char = '-' );

			/// Return the name with all illegal characters stripped out (\/*?:<>|).
			static XStringW CleanFilename( const XStringW& path, wchar_t sub_char = '-' );

			/// Returns fully specified folder of the given filename.
			static XStringA GetFileFolder( const XStringA& filename, bool includeTrailingSlash = true );

			/// Returns fully specified folder of the given filename.
			static XStringW GetFileFolder( const XStringW& filename, bool includeTrailingSlash = true );

			/// Returns the extension of the filename (without the dot).
			static XStringA GetFileExtension( const XStringA& filename );

			/// Returns the extension of the filename (without the dot).
			static XStringW GetFileExtension( const XStringW& filename );

			/// Returns only the filename (without directory or extension).
			static XStringA GetFileNameOnly( const XStringA& filename );

			/// Returns only the filename (without directory or extension).
			static XStringW GetFileNameOnly( const XStringW& filename );

			/// Returns only the filename and extension (without directory).
			static XStringA GetFileNameWithoutFolder( const XStringA& filename );

			/// Returns only the filename and extension (without directory).
			static XStringW GetFileNameWithoutFolder( const XStringW& filename );

			/** Returns the filename with the extension stripped.
			The filename is not stripped of it's drive and directory.
			**/
			static XStringA GetFileNameWithoutExtension( const XStringA& filename );

			/** Returns the filename with the extension stripped.
			The filename is not stripped of it's drive and directory.
			**/
			static XStringW GetFileNameWithoutExtension( const XStringW& filename );


			/** Grows a path from it's full content, to less and less, until it's just the filename.
			For example, if the original path is 'c:/hello/one/two.txt',
			then you'll get:
			0: c:/hello/one/two.txt
			1: hello/one/two.txt
			2: one/two.txt
			3: two.txt
			4: 
			**/
			static XStringW GetPathUpTo( const XStringW& filename, int& portion_start_at_0 );

			static XStringW GetPathReverseToFrom( const XStringW& filename, int token_start, int token_count );

			/// Try to find a file at [base + GetPathUpTo(filename, 0 .. N)]
			static XStringW ResolvePathAnyRelative( const XStringW& base, const XStringW& filename );

			static XStringA RelativePath( bool targetIsDir, const XStringA& target, bool rootIsDir, const XStringA& root );

			/** Returns \a target relative to \a root.
			\param targetIsDir True if \a target is a directory.
			\param target The target filename or directory.
			\param rootIsDir True if \a root is a directory.
			\param root The root directory or file location from which the resulting path should stem.
			\return The relative path, or an empty string if the function failed.
			**/
			static XStringW RelativePath( bool targetIsDir, const XStringW& target, bool rootIsDir, const XStringW& root );

			/** Fixes the extension of the indicated file.
			If extension is blank, then the file is stripped of any extension.
			If force is false and the file already has an extension, then the extension is left as-is.
			If the file already has an extension and force is true, then the required extension will
			be appended to the existing path as-is (so you will get for example drawing.dwg.dxf).
			In order to change the extension, see ChangeExtension.
			\return The fixed filename.
			\sa ChangeExtension.
			**/
			static XString FixExtension( XString path, XString extension, bool force = true );

			/** Changes the extension of the given file to the desired extension.
			\param path The filename to be altered.
			\param extension The target extension of the filename. If this is empty, then the 
				file will be ridden of any extension. It does not matter whether the parameter
				has a dot at the beginning.
			\return The altered filename.
			\sa FixExtension
			**/
			static XString ChangeExtension( XString path, XString extension );

			/// Returns true if the two file names are equivalent.
			static bool FileNamesEqual( const XString& filename1, const XString& filename2 );

			/// Returns true if the path is a relative path
			static bool IsRelative( LPCSTR path );

			/// Returns true if the path is a relative path
			static bool IsRelative( LPCWSTR path );

			/// Returns the current directory.
			static XString CurrentDirectory();

			/// Returns the current directory.
			static XStringW CurrentDirectoryW();

			/// Returns true if the folder exists and has at least one file or subdirectory
			static bool FolderHasFiles( const XStringW& folder );

			/** Returns true if the file exists and it is not a directory.
			@param allowRelativeToCurrentDir If false, then we require that the filename path is absolute.
			**/
			static bool FileExists( LPCSTR filename, bool allowRelativeToCurrentDir = true );

			/** Returns true if the file exists and it is not a directory.
			@param allowRelativeToCurrentDir If false, then we require that the filename path is absolute.
			**/
			static bool FileExists( LPCWSTR filename, bool allowRelativeToCurrentDir = true );

			/// Returns true if the folder exists and it is a directory.
			static bool FolderExists( LPCSTR filename, bool allowRelativeToCurrentDir = true );

			/// Returns true if the folder exists and it is a directory.
			static bool FolderExists( LPCWSTR filename, bool allowRelativeToCurrentDir = true );

			static bool FileOrFolderExists( bool file, const char* path, bool allowRelativeToCurrentDir = true )	{ return FileOrFolderExistsT(file, path, allowRelativeToCurrentDir); }
			static bool FileOrFolderExists( bool file, const wchar_t* path, bool allowRelativeToCurrentDir = true ) { return FileOrFolderExistsT(file, path, allowRelativeToCurrentDir); }

			/// Move a file or directory
			static bool MoveFile( const XStringA& src, const XStringA& dst, UINT flags = MoveFileFlagNone );

			/** Move a file or directory.
			If src specifies a directory, then dst must be on the same volume.
			**/
			static bool MoveFile( const XStringW& src, const XStringW& dst, UINT flags = MoveFileFlagNone );

			/// Recursively create a folder. Optional transaction handle.
			static bool CreateFolder( const XStringW& name, AbcFileSysTxHandle transaction = NULL );

			/// Recursively create a folder. Optional transaction handle.
			static bool CreateFolder( const XStringA& name, AbcFileSysTxHandle transaction = NULL );

			/** Recursively deletes a folder and all it's contents.
			@return True only if everything is deleted.
			**/
			static bool DeleteFolder( const XStringA& name, AbcFileSysTxHandle transaction = NULL );

			/** Recursively deletes a folder and all it's contents.
			@return True only if everything is deleted.
			**/
			static bool DeleteFolder( const XStringW& name, AbcFileSysTxHandle transaction = NULL );

			/// Deletes everything inside the directory, but leaves the directory intact
			static bool DeleteFolderContent( const XStringW& name, AbcFileSysTxHandle transaction = NULL );

			/// Sets or clears the readonly flag for a file.
			static bool SetFileReadOnly( const XStringA& name, bool readOnlyOn );

			/// Sets or clears the readonly flag for a file.
			static bool SetFileReadOnly( const XStringW& name, bool readOnlyOn );

			/// Gets a file's attributes
			static bool GetFileAttribs( const XStringA& name, UINT32& attrib );

			/// Gets a file's attributes
			static bool GetFileAttribs( const XStringW& name, UINT32& attrib );

			/// Sets a file's attributes
			static bool SetFileAttribs( const XStringA& name, UINT32 attrib );

			/// Sets a file's attributes
			static bool SetFileAttribs( const XStringW& name, UINT32 attrib );

			/// Returns the size of the file.
			static INT64 GetFileSize( const XStringA& name );

			/// Returns the size of the file.
			static INT64 GetFileSize( const XStringW& name );

			/// Retrieves time information for the file.
			static bool GetFileTimes( const XStringA& name, AbcDate& timeCreate, AbcDate& timeAccess, AbcDate& timeWrite, UINT64* fileSize = NULL, AbcFileSysTxHandle transaction = NULL );

			/// Retrieves time information for the file.
			static bool GetFileTimes( const XStringW& name, AbcDate& timeCreate, AbcDate& timeAccess, AbcDate& timeWrite, UINT64* fileSize = NULL, AbcFileSysTxHandle transaction = NULL );

			/// Retrieves last write time for the file. Returned date is null upon failure.
			static AbcDate GetFileWriteTime( const XStringA& name );

			/// Retrieves last write time for the file. Returned date is null upon failure.
			static AbcDate GetFileWriteTime( const XStringW& name );

			/// Sets the last modified date and last accessed date to NOW
			static bool TouchFile( const XStringW& name );

			/// Sets the last modified date and last accessed date to NOW + nanosecondsOffset
			static bool TouchFileWithEntropy( const XStringW& name, int64 nanosecondsOffset );

			/// Sets time information for a file. Pass a Null date to any field to not set it.
			static bool SetFileTimes( const XStringA& name, AbcDate& timeCreate, AbcDate& timeAccess, AbcDate& timeWrite, AbcFileSysTxHandle transaction = NULL );

			/// Sets time information for a file. Pass a Null date to any field to not set it.
			static bool SetFileTimes( const XStringW& name, AbcDate& timeCreate, AbcDate& timeAccess, AbcDate& timeWrite, AbcFileSysTxHandle transaction = NULL );

			/// Returns true if the file exists and can be read from.
			static bool CheckReadPermission( const XString& filename, OpenFailReason* reason = NULL );

			/** Returns true if the file or directory can be written to.
			If file_or_directory specifies a directory, then the system checks whether a temporary
			file can be created in that folder. This is obviously different from the ability to modify the attributes
			of a directory.
			**/
			static bool CheckWritePermission( const XString& file_or_directory, OpenFailReason* reason = NULL );

			/// Grant world (aka Everyone) and System all rights to the object. The object is created if it does not already exist.
			static bool GrantAllAccess( const XString& file_or_directory );

			/// Returns a temporary file for the given directory.
			static XString GetTempFileAt( XString path, XString prefix, XString suffix = "" );

			/// Returns a temporary file from the temp directory.
			static XString GetTempFile( XString prefix, XString suffix = "" );

			/** Returns a temporary file from the temp directory and cleans up prefixed files older than the indicated age.
			We search for old files with the wildcard "prefix*", meaning suffix is ignored, so you are advised to make your prefix reasonably long and unique.
			**/
			static XString GetTempFileAndCleanOld( XString directory, XString prefix, XString suffix, double expire_age_seconds = 7 * 24 * 3600 );
			static XString GetTempFileAndCleanOld( XString prefix, XString suffix, double expire_age_seconds = 7 * 24 * 3600 );

			/// Returns the temporary folder. Neither write permission nor existence is not guaranteed.
			static XString GetTempFolder();

			/** Returns a special folder.
			addTrailingSlash If true, then we add a trailing backslash.
			**/
			static XString GetSpecialFolder( SpecialFolder folder, bool addTrailingSlash = false );

		private:
			#ifdef _UNICODE
			static XStringW GetWindowsSpecialFolder( int id, bool addTrailingSlash ) { return GetWindowsSpecialFolderW(id, addTrailingSlash); }
			#else
			static XStringA GetWindowsSpecialFolder( int id, bool addTrailingSlash ) { return GetWindowsSpecialFolderA(id, addTrailingSlash); }
			#endif

			static XStringA GetWindowsSpecialFolderA( int id, bool addTrailingSlash );
			static XStringW GetWindowsSpecialFolderW( int id, bool addTrailingSlash );

			template< typename TChar, typename TString >
			static TString TCleanFilename( const TString& path, TChar sub_char );

			template< typename CH >
			static bool FileOrFolderExistsT( bool file, const CH* path, bool allowRelativeToCurrentDir = true );

		};
	}
}
