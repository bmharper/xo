#include "pch.h"
#include "../Other/lmPlatform.h"
#include "../Windows/Win.h"
#include "../other/profile.h"
#include "../Strings/wildcard.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <io.h>
#include <Aclapi.h>
#include <Sddl.h>

using namespace Panacea::IO;

template<typename TCH>
static bool EQUALS_NOCASE(const TCH* a, const TCH* b)
{
	for ( int i = 0; a[i] && b[i]; i++ )
	{
		if ( toupper(a[i]) != toupper(b[i]) ) return false;
	}
	return true;
}

namespace Panacea
{
	namespace IO
	{

		XStringW Path::Combine( const XStringW& p1, const XStringW& p2, const XStringW& p3 )
		{
			return Combine( Combine(p1, p2), p3 );
		}

		XStringW Path::Combine( const XStringW& p1, const XStringW& p2, const XStringW& p3, const XStringW& p4 )
		{
			return Combine( Combine(p1, p2, p3), p4 );
		}

		XStringW Path::Combine( const XStringW& p1, const XStringW& p2, const XStringW& p3, const XStringW& p4, const XStringW& p5 )
		{
			return Combine( Combine(p1, p2, p3, p4), p5 );
		}

		XStringW Path::Combine( const XStringW& left, const XStringW& right )
		{
			XStringW res;
			bool ok = false;
			XStringW fixedLeft = left;
			XStringW fixedRight = right;
			bool haveBack = fixedLeft.Contains(L'\\') || fixedRight.Contains(L'\\');
			bool haveForward = fixedLeft.Contains(L'/') || fixedRight.Contains(L'/');
			if ( haveBack && haveForward )
			{
				// Unify them, if we have two different slash types
				// Without this, we fail with an input such as "c:\blah\" + "/etc/txt", because the different slashes aren't dissolved into one.
				fixedLeft.Replace(	L'/',  DIR_SEP_WC );
				fixedLeft.Replace(	L'\\', DIR_SEP_WC );
				fixedRight.Replace( L'/',  DIR_SEP_WC );
				fixedRight.Replace( L'\\', DIR_SEP_WC );
			}
			ok = PathCombineW( res.GetBuffer( MAX_PATH * 2 ), fixedLeft, fixedRight ) != NULL;
			ASSERT( ok );
			res.ReleaseBuffer();
			return res;
		}

		XStringA Path::Combine( const XStringA& left,const XStringA& right )
		{
			XStringW wleft = left;
			XStringW wright = right;
			return Combine( wleft, wright ).ToUtf8();
		}

		template< class T >
		char TDirSep( const T& str )
		{
			int f = 0;
			int r = 0;
			for ( int i = 0; i < str.Length(); i++ )
			{
				if ( str[i] == '/' ) f++;
				if ( str[i] == '\\') r++;
			}
			return f > r ? '/' : '\\'; // biased towards '\\'
		}

		char Path::DirSep( const XStringA& str )
		{
			return TDirSep(str);
		}

		wchar_t Path::DirSep( const XStringW& str )
		{
			return TDirSep(str);
		}

		XStringW Path::GetPathUpTo( const XStringW& filename, int& portion_start_at_0 )
		{
			wchar_t sep = DirSep(filename);
			int n = 0;
			for ( int i = 0; i < filename.Length(); i++ )
			{
				if ( n >= portion_start_at_0 ) return filename.Mid( i );
				if ( filename[i] == sep )
					n++;
			}
			return L"";
		}

		XStringW Path::GetPathReverseToFrom( const XStringW& filename, int token_start, int token_count )
		{
			wchar_t sep = DirSep(filename);
			int token_i = 0;
			int char_start = filename.Length() - 1;
			int i = filename.Length() - 1;

			for ( ; i >= 0 ; i-- )
			{
				if ( filename[i] == sep )
				{
					token_i++;
					if ( token_i == token_start )
						char_start = i - 1;
					else if ( token_i - token_start == token_count )
					{
						i++;
						break;
					}
				}
			}
			return filename.Mid( i, char_start - i );;
		}


		XStringW Path::ResolvePathAnyRelative( const XStringW& base, const XStringW& filename )
		{
			for ( int portion = 0; portion < 20; portion++ )
			{
				XStringW piece = GetPathUpTo( filename, portion );
				if ( piece == L"" ) break;
				XStringW full = filename;
				if ( portion > 0 ) full = Combine( base, piece ); // 0 --> absolute path
				if ( FileExists(full, false) )
					return full;
			}
			return L"";
		}

		XStringA Path::RelativePath( bool targetIsDir, const XStringA& target, bool rootIsDir, const XStringA& root )
		{
			XStringW targetw = target;
			XStringW rootw = root;
			return RelativePath( targetIsDir, targetw, rootIsDir, rootw ).ToUtf8();
		}

		XStringW Path::RelativePath( bool targetIsDir, const XStringW& target, bool rootIsDir, const XStringW& root )
		{
			wchar_t dest[ MaxPathLength ];
			if ( PathRelativePathToW( dest, root, rootIsDir ? FILE_ATTRIBUTE_DIRECTORY : 0, target, targetIsDir ? FILE_ATTRIBUTE_DIRECTORY : 0 ) == TRUE )
			{
				XStringW dd = dest;
				// chop ".\"
				if ( dd.Length() > 2 && dd.Left(2) == L".\\" )
					dd = dd.Mid( 2 );
				return dd;
			}
			else
			{
				return L"";
			}
		}

		int FindOnly( bool files, const XStringW& searchWildcard, dvect<XStringW>& relPaths, uint flags, AbcFileSysTxHandle transaction )
		{
			if ( searchWildcard.Contains(';') )
			{
				// Run each search separately
				// searchWildcard typically looks like this: c:\x\y\z\*.txt;*.csv
				// So we need to separate out the directory portion from the filter portion.
				XStringW dir = "";
				XStringW wildcard = searchWildcard;
				int lsep = searchWildcard.ReverseFind( Path::DirSep(searchWildcard) );
				if ( lsep != -1 )
				{
					dir = searchWildcard.Left( lsep + 1 );
					wildcard = searchWildcard.Mid( lsep + 1 );
				}
				dvect<XStringW> filters;
				Split( wildcard, ';', filters );
				int n = 0;
				for ( int i = 0; i < filters.size(); i++ )
					n += FindOnly( files, dir + filters[i], relPaths, flags, transaction );
				return n;
			}
			else
			{
				pvect< FileFindItem* > items;
				Path::FindFiles( searchWildcard, items, flags, transaction );
				int n = 0;
				for ( int i = 0; i < items.size(); i++ )
				{
					if ( files == items[i]->Directory ) continue;
					relPaths += items[i]->Name;
					n++;
				}
				delete_all( items );
				return n;
			}
		}

		int Path::FindFilesOnly( const XStringW& searchWildcard, dvect<XStringW>& relPaths, uint flags, AbcFileSysTxHandle transaction )
		{
			return FindOnly( true, searchWildcard, relPaths, flags, transaction );
		}

		int Path::FindFoldersOnly( const XStringW& searchWildcard, dvect<XStringW>& relPaths, uint flags, AbcFileSysTxHandle transaction )
		{
			return FindOnly( false, searchWildcard, relPaths, flags, transaction );
		}

		int Path::FindFiles( const XStringW& searchWildcard, pvect< FileFindItem* >& items, uint flags, AbcFileSysTxHandle transaction )
		{
			// Separate out into [base directory] + [wildcard]

			// For recurseAll, we do the wildcard matching ourselves
			bool recurseAll = !!(flags & FindRecursiveOnAllDirectories);
			bool fullPath = !!(flags & FindFullPath);
			wchar_t sep = DirSep( searchWildcard );

			XStringW base;
			XStringW wildcard, ffwildcard;
			bool haveWC = false;
			int lastSlash = -1;
			for ( int i = searchWildcard.Length() - 1; i >= 0; i-- )
			{
				if ( searchWildcard[i] == '?' || searchWildcard[i] == '*' )
					haveWC = true;
				if ( searchWildcard[i] == '/' || searchWildcard[i] == '\\' )
				{
					lastSlash = i;
					break;
				}
			}
			if ( lastSlash != -1 && haveWC )
			{
				base = searchWildcard.Left( lastSlash + 1 );
				wildcard = searchWildcard.Mid( lastSlash + 1 );
				if ( recurseAll ) ffwildcard = "*";
				else				ffwildcard = wildcard;
			}
			else
			{
				base = searchWildcard;
				recurseAll = false;
			}

			int found = 0;
			XString rootBase = base;
			dvect<XStringW> dirStack;
			dirStack += base;
			while ( dirStack.size() != 0 )
			{
				XString lbase = dirStack.back();
				dirStack.pop_back();
				XString search = lbase + ffwildcard;
				XString localBase = lbase.Mid( rootBase.Length() );
				
				// there appears to be an error in the function definition of _wfindfirst64..
				// either that or it really does intend to modify the search string.
				const wchar_t* buff = search;

				WIN32_FIND_DATA fd;
				HANDLE ff;
				// FindExInfoBasic doesn't retrieve short file names, which is faster
				if ( transaction )	ff = NTDLL()->FindFirstFileTransacted( search, NTDLL()->AtLeastWin7 ? FindExInfoBasic : FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0, transaction );
				else				ff = FindFirstFile( search, &fd );
				while ( ff != INVALID_HANDLE_VALUE )
				{
					// April 2010 -- I can't see how the '.' or '..' is useful programatically.
					if (	!EQUALS_NOCASE(fd.cFileName, L".") &&
							!EQUALS_NOCASE(fd.cFileName, L"..") )
					{
						bool isDir = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
						bool ignore = false;
						if ( !!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !!(flags & FindIgnoreHidden) ) ignore = true;
						if ( !!(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) && !!(flags & FindIgnoreSystem) ) ignore = true;
						if ( isDir && recurseAll )
						{
							dirStack += lbase + fd.cFileName + XStringW(sep);
						}
						if ( recurseAll && !MatchWildcard( fd.cFileName, wildcard, false ) )
							ignore = true;
						if ( !ignore )
						{
							FileFindItem* it = new FileFindItem();
							it->AttribArchive = (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0;
							it->AttribHidden = (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
							it->AttribReadOnly = (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
							it->AttribSystem = (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
							it->Directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
							it->DateAccess = fd.ftLastAccessTime;
							it->DateCreate = fd.ftCreationTime;
							it->DateModify = fd.ftLastWriteTime;
							if ( fullPath )	it->Name = lbase      + fd.cFileName;
							else			it->Name = localBase  + fd.cFileName;
							it->Size = (((u64) fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
							items.push_back( it );
							found++;
						}
					}
					if ( !FindNextFile( ff, &fd ) ) break;
				} 
				if ( ff != INVALID_HANDLE_VALUE )
					FindClose( ff );
			}
			return found;
		}

		void Path::Split( const XStringA& path, XStringA* drive, XStringA* directory, 
										XStringA* filename, XStringA* extension )
		{
			char _drive[MAX_PATH], _directory[MAX_PATH], _filename[MAX_PATH], _extension[MAX_PATH];
#if _MSC_VER >= 1400
			_splitpath_s( path, _drive, MAX_PATH, _directory, MAX_PATH, _filename, MAX_PATH, _extension, MAX_PATH );
#else
			_splitpath( path, _drive, _directory, _filename, _extension );
#endif
			if (drive)		*drive = _drive;
			if (directory)	*directory = _directory;
			if (filename)	*filename = _filename;
			if (extension)	*extension = _extension;
		}

		void Path::Split( const XStringW& path, XStringW* drive, XStringW* directory, 
										XStringW* filename, XStringW* extension )
		{
			wchar_t _drive[MAX_PATH], _directory[MAX_PATH], _filename[MAX_PATH], _extension[MAX_PATH];
#if _MSC_VER >= 1400
			_wsplitpath_s( path, _drive, MAX_PATH, _directory, MAX_PATH, _filename, MAX_PATH, _extension, MAX_PATH );
#else
			_wsplitpath( path, _drive, _directory, _filename, _extension );
#endif
			if (drive)		*drive = _drive;
			if (directory)	*directory = _directory;
			if (filename)	*filename = _filename;
			if (extension)	*extension = _extension;
		}

		template< typename TChar, typename TString >
		static TString Path::TCleanFilename( const TString& path, TChar sub_char )
		{
			TString str = path;
			int lastSlash = str.ReverseFind('/');
			lastSlash = AbcMax( lastSlash, str.ReverseFind('\\') );
			if ( lastSlash < 0 ) lastSlash = -1;
			for ( int i = lastSlash + 1; i < str.Length(); i++ )
			{
				// \/*?:<>|
				int ch = str[i];
				if ( ch == '/' || ch == '\\' || ch == '*' || ch == '?' || ch == ':' || ch == '<' || ch == '>' || ch == '|' )
					str[i] = sub_char;
			}
			return str;
		}


		XStringA Path::CleanFilename( const XStringA& path, char sub_char )
		{
			return TCleanFilename( path, sub_char );
		}

		XStringW Path::CleanFilename( const XStringW& path, wchar_t sub_char )
		{
			return TCleanFilename( path, sub_char );
		}

		XStringA Path::GetFileFolder( const XStringA& filename, bool includeTrailingSlash )
		{
			XStringA drive, dir;
			Split( filename, &drive, &dir );
			if ( !includeTrailingSlash ) dir.Chop();
			return drive + dir;
		}
		
		XStringW Path::GetFileFolder( const XStringW& filename, bool includeTrailingSlash )
		{
			XStringW drive, dir;
			Split( filename, &drive, &dir );
			if ( !includeTrailingSlash ) dir.Chop();
			return drive + dir;
		}

		XStringA Path::GetFileExtension( const XStringA& filename )
		{
			XStringA ext;
			Split( filename, NULL, NULL, NULL, &ext );
			ext.TrimLeft( '.' );
			return ext;
		}

		XStringW Path::GetFileExtension( const XStringW& filename )
		{
			XStringW ext;
			Split( filename, NULL, NULL, NULL, &ext );
			ext.TrimLeft( '.' );
			return ext;
		}

		XStringA Path::GetFileNameOnly( const XStringA& filename )
		{
			XStringA name;
			Split( filename, NULL, NULL, &name, NULL );
			return name;
		}

		XStringW Path::GetFileNameOnly( const XStringW& filename )
		{
			XStringW name;
			Split( filename, NULL, NULL, &name, NULL );
			return name;
		}

		XStringA Path::GetFileNameWithoutFolder( const XStringA& filename )
		{
			XStringA name, ext;
			Split( filename, NULL, NULL, &name, &ext );
			return name + ext;
		}

		XStringW Path::GetFileNameWithoutFolder( const XStringW& filename )
		{
			XStringW name, ext;
			Split( filename, NULL, NULL, &name, &ext );
			return name + ext;
		}

		XStringA Path::GetFileNameWithoutExtension( const XStringA& filename )
		{
			XStringA drive, dir, name;
			Split( filename, &drive, &dir, &name, NULL );
			return drive + dir + name;
		}

		XStringW Path::GetFileNameWithoutExtension( const XStringW& filename )
		{
			XStringW drive, dir, name;
			Split( filename, &drive, &dir, &name, NULL );
			return drive + dir + name;
		}

		bool Path::FileNamesEqual( const XString& filename1, const XString& filename2 )
		{
			return filename1.CompareNoCase( filename2 ) == 0;
		}

		XString Path::FixExtension( XString path, XString extension, bool force )
		{
			if ( path.IsEmpty() ) return path;
			XString drive, dir, file, ext;
			Split( path, &drive, &dir, &file, &ext );
			if ( extension.IsEmpty() )
			{
				return drive + dir + file;
			}
			if ( extension.Count(';') > 0 )
				extension = extension.Left( extension.Find(';') );			// Turn  *.jpg;*.jpeg  into  *.jpg
			if ( extension.Left() == '*' ) extension = extension.Mid(1);	// Turn  *.tif         into  .tif
			if ( extension.Left() != '.' ) extension = L"." + extension;	// Turn  tif           into  .tif
			bool hasExt = ext.Length() >= 2;
			if ( hasExt && !force ) return path;
			if ( FileNamesEqual( extension, ext ) ) return path;
			path.TrimRight( '.' );
			path = path + extension;
			return path;
		}

		XString Path::ChangeExtension( XString path, XString extension )
		{
			if ( path.IsEmpty() ) return path;
			XString drive, dir, file, ext;
			Split( path, &drive, &dir, &file, &ext );
			if ( extension.IsEmpty() )
				return drive + dir + file;
			if ( extension.Left() != '.' )
				extension = L"." + extension;
			return drive + dir + file + extension;
		}

		bool Path::MoveFile( const XStringA& src, const XStringA& dst, UINT flags )
		{
			XStringW wsrc = src;
			XStringW wdst = dst;
			return MoveFile( wsrc, wdst, flags );
		}

		bool Path::MoveFile( const XStringW& src, const XStringW& dst, UINT flags )
		{
			DWORD wflags = 0;
			if ( flags & MoveFileFlagAllowCopyDelete ) wflags |= MOVEFILE_COPY_ALLOWED;
			return MoveFileExW( src, dst, wflags ) != 0;
		}

		bool Path::CreateFolder( const XStringA& name, AbcFileSysTxHandle transaction )
		{
			return CreateFolder( name.ToWideFromUtf8(), transaction );
		}

		bool Path::CreateFolder( const XStringW& name, AbcFileSysTxHandle transaction )
		{
			if ( FolderExists( name ) ) return true;
			int start = 0;
			int piece = 0;
			bool end = false;
			bool unc = false;
			while ( !end )
			{
				piece++;
				int pos = name.Find( DirSepChar(), start );
				if ( pos < 0 ) { end = true; pos = name.Length(); }
				if ( pos == 0 && name.Left(2) == L"\\\\" )
				{
					// UNC paths
					unc = true;
					start = 2;
					continue;
				}
				else if ( unc && piece == 2 )
				{
					start = pos + 1;
					continue;
				}
				XStringW fpiece = name.Left( pos );
				if ( !FolderExists( fpiece ) )
				{
					// try to create the folder
					BOOL ok;
					if ( transaction )		ok = NTDLL()->CreateDirectoryTransactedW( NULL, fpiece, NULL, transaction );
					else					ok = CreateDirectoryW( fpiece, NULL );
					if ( !ok ) return false;
				}
				start = pos + 1;
			}
			return true;
		}

		bool Path::DeleteFolder( const XStringA& name, AbcFileSysTxHandle transaction )
		{
			return DeleteFolder( name.ToWideFromUtf8(), transaction );
		}

		bool Path::DeleteFolder( const XStringW& name, AbcFileSysTxHandle transaction )
		{
			if ( !FolderExists(name) ) return true;

			if ( DeleteFolderContent( name, transaction ) )
			{
				if ( transaction )	return !!NTDLL()->RemoveDirectoryTransactedW( name, transaction );
				else				return !!RemoveDirectoryW( name );
			}
			
			return false;
		}

		bool Path::DeleteFolderContent( const XStringW& name, AbcFileSysTxHandle transaction )
		{
			pvect< FileFindItem* > items;
			FindFiles( name + DirSepStrW() + L"*.*", items, 0, transaction );
			int fail = 0;
			for ( int i = 0; i < items.size(); i++ )
			{
				if ( items[i]->Name == L"." ) continue;
				if ( items[i]->Name == L".." ) continue;
				XStringW itemName = name + DirSepStrW() + items[i]->Name;
				if ( items[i]->Directory )
				{
					bool ok = DeleteFolder( itemName, transaction );
					if ( !ok ) 
						fail++;
				}
				else
				{
					if ( transaction )
					{
						NTDLL()->SetFileAttributesTransactedW( itemName, FileAttributeNormal, transaction );
						if ( !NTDLL()->DeleteFileTransactedW( itemName, transaction ) )
							fail++;
					}
					else
					{
						SetFileAttribs( itemName, FileAttributeNormal ); // remove read-only. Might be better to do a "&~" flags removal thing.
						if ( !DeleteFileW( itemName ) )
							fail++;
					}
				}
			}
			delete_all( items );
			return fail == 0;
		}

		bool Path::SetFileReadOnly( const XStringA& name, bool readOnlyOn )
		{
			XStringW wname = name;
			return SetFileReadOnly( wname, readOnlyOn );
		}

		bool Path::SetFileReadOnly( const XStringW& name, bool readOnlyOn )
		{
			DWORD attrib = ::GetFileAttributesW( name );
			if ( attrib == INVALID_FILE_ATTRIBUTES ) return false;

			if ( readOnlyOn ) attrib |= FILE_ATTRIBUTE_READONLY;
			else				attrib &= ~FILE_ATTRIBUTE_READONLY;

			return ::SetFileAttributesW( name, attrib ) != 0;
		}

		bool Path::GetFileAttribs( const XStringA& name, UINT32& attrib )
		{
			XStringW wname = name;
			return GetFileAttribs( wname, attrib );
		}

		bool Path::GetFileAttribs( const XStringW& name, UINT32& attrib )
		{
			attrib = FileAttributeNormal;
			DWORD sysAttrib = ::GetFileAttributesW( name );
			if ( sysAttrib == INVALID_FILE_ATTRIBUTES ) return false;
			if ( sysAttrib & FILE_ATTRIBUTE_ARCHIVE ) attrib |= FileAttributeArchive;
			if ( sysAttrib & FILE_ATTRIBUTE_DIRECTORY ) attrib |= FileAttributeDirectory;
			if ( sysAttrib & FILE_ATTRIBUTE_ENCRYPTED ) attrib |= FileAttributeEncrypted;
			if ( sysAttrib & FILE_ATTRIBUTE_HIDDEN ) attrib |= FileAttributeHidden;
			if ( sysAttrib & FILE_ATTRIBUTE_READONLY ) attrib |= FileAttributeReadOnly;
			if ( sysAttrib & FILE_ATTRIBUTE_SYSTEM ) attrib |= FileAttributeSystem;
			if ( sysAttrib & FILE_ATTRIBUTE_TEMPORARY ) attrib |= FileAttributeTemporary;
			if ( sysAttrib == FILE_ATTRIBUTE_NORMAL ) attrib = FileAttributeNormal;
			return true;
		}

		bool Path::SetFileAttribs( const XStringA& name, UINT32 attrib )
		{
			XStringW wname = name;
			return SetFileAttribs( wname, attrib );
		}

		bool Path::SetFileAttribs( const XStringW& name, UINT32 attrib )
		{
			DWORD sysAttrib = 0;
			if ( attrib & FileAttributeArchive  )	sysAttrib |= FILE_ATTRIBUTE_ARCHIVE;
			if ( attrib & FileAttributeHidden )		sysAttrib |= FILE_ATTRIBUTE_HIDDEN;
			if ( attrib & FileAttributeReadOnly )	sysAttrib |= FILE_ATTRIBUTE_READONLY;
			if ( attrib & FileAttributeSystem )		sysAttrib |= FILE_ATTRIBUTE_SYSTEM;
			if ( attrib & FileAttributeTemporary )	sysAttrib |= FILE_ATTRIBUTE_TEMPORARY;
			if ( attrib == FileAttributeNormal )	sysAttrib = FILE_ATTRIBUTE_NORMAL;
			return ::SetFileAttributesW( name, sysAttrib ) != 0;
		}

		template< typename CH >
		bool IsRelativeT( const CH* path )
		{
			if ( path == NULL ) return true;
			if ( path[0] == 0 || path[0] == '/' ) return false;		// empty string or absolute unix path
			if ( path[0] == '\\' && path[1] == '\\' ) return false; // long form windows path
			if ( path[1] == ':' ) return false;						// C: etc
			return true;
		}

		bool Path::IsRelative( LPCSTR path )
		{
			return IsRelativeT(path);
		}

		bool Path::IsRelative( LPCWSTR path )
		{
			return IsRelativeT(path);
		}

		XString Path::CurrentDirectory()
		{
			XString dir;
			GetCurrentDirectory( MAX_PATH * 4, dir.GetBuffer( MAX_PATH * 4 ) );
			dir.ReleaseBuffer();
			return dir;
		}

		XStringW Path::CurrentDirectoryW()
		{
			XStringW dir;
			GetCurrentDirectoryW( MAX_PATH * 4, dir.GetBuffer( MAX_PATH * 4 ) );
			dir.ReleaseBuffer();
			return dir;
		}

		template< typename CH >
		DWORD GetFileAttributesT( const CH* filename )
		{
			if ( sizeof(CH) == sizeof(char) ) return ::GetFileAttributesA( (const char*) filename );
			else								return ::GetFileAttributesW( (const wchar_t*) filename );
		}

		template< typename CH >
		bool Path::FileOrFolderExistsT( bool file, const CH* path, bool allowRelativeToCurrentDir )
		{
			if ( IsRelative( path ) && !allowRelativeToCurrentDir )
				return false;

			DWORD attrib = GetFileAttributesT( path );
			bool isFolder = 0 != (attrib & FILE_ATTRIBUTE_DIRECTORY);
			bool exists = 0 != (attrib != INVALID_FILE_ATTRIBUTES);
			if ( exists && file == !isFolder )
			{
				return true;
			}
			else
				return false;
		}

		bool Path::FolderExists( LPCSTR filename, bool allowRelativeToCurrentDir )	{ return FileOrFolderExists( false, filename, allowRelativeToCurrentDir ); }
		bool Path::FolderExists( LPCWSTR filename, bool allowRelativeToCurrentDir ) { return FileOrFolderExists( false, filename, allowRelativeToCurrentDir ); }
		bool Path::FileExists( LPCSTR filename, bool allowRelativeToCurrentDir )	{ return FileOrFolderExists( true, filename, allowRelativeToCurrentDir ); }
		bool Path::FileExists( LPCWSTR filename, bool allowRelativeToCurrentDir )	{ return FileOrFolderExists( true, filename, allowRelativeToCurrentDir ); }

		INT64 Path::GetFileSize( const XStringA& name )
		{
			XStringW wname = name;
			return GetFileSize( wname );
		}

		INT64 Path::GetFileSize( const XStringW& name )
		{
			Sys::Date d1, d2, d3;
			UINT64 size = 0;
			GetFileTimes( name, d1, d2, d3, &size );
			return size;
		}

		Sys::Date Path::GetFileWriteTime( const XStringA& name )
		{
			XStringW wname = name;
			return GetFileWriteTime( wname );
		}

		Sys::Date Path::GetFileWriteTime( const XStringW& name )
		{
			Sys::Date timeCreate, timeAccess, timeWrite;
			if ( GetFileTimes( name, timeCreate, timeAccess, timeWrite ) )
				return timeWrite;
			else
				return Sys::Date();
		}

		bool Path::GetFileTimes( const XStringA& name, Sys::Date& timeCreate, Sys::Date& timeAccess, Sys::Date& timeWrite, UINT64* fileSize, AbcFileSysTxHandle transaction )
		{
			XStringW wname = name;
			return GetFileTimes( wname, timeCreate, timeAccess, timeWrite, fileSize, transaction );
		}

		bool Path::GetFileTimes( const XStringW& name, Sys::Date& timeCreate, Sys::Date& timeAccess, Sys::Date& timeWrite, UINT64* fileSize, AbcFileSysTxHandle transaction )
		{
			BOOL r;
			WIN32_FILE_ATTRIBUTE_DATA ex;
			if ( transaction )	r = NTDLL()->GetFileAttributesTransactedW( name, GetFileExInfoStandard, &ex, transaction );
			else				r = GetFileAttributesExW( name, GetFileExInfoStandard, &ex );
			if ( r )
			{
				timeCreate = ex.ftCreationTime;
				timeAccess = ex.ftLastAccessTime;
				timeWrite = ex.ftLastWriteTime;
				if ( fileSize ) *fileSize = ((UINT64) ex.nFileSizeHigh) << 32 | (UINT64) ex.nFileSizeLow;
				return true;
			}
			return false;
		}

		bool Path::TouchFile( const XStringW& name )
		{
			Panacea::Sys::Date now = Panacea::Sys::Date::Now();
			return SetFileTimes( name, Panacea::Sys::Date(), now, now );
		}

		bool Path::TouchFileWithEntropy( const XStringW& name, int64 nanosecondsOffset )
		{
			Panacea::Sys::Date now = Panacea::Sys::Date::Now();
			now.AddNanoseconds( nanosecondsOffset );
			return SetFileTimes( name, Panacea::Sys::Date(), now, now );
		}

		bool Path::SetFileTimes( const XStringA& name, Sys::Date& timeCreate, Sys::Date& timeAccess, Sys::Date& timeWrite, AbcFileSysTxHandle transaction )
		{
			return SetFileTimes( name.ToWideFromUtf8(), timeCreate, timeAccess, timeWrite, transaction );
		}

// From MSDN
#define TXFS_MINIVERSION_COMMITTED_VIEW 0x0000

		bool Path::SetFileTimes( const XStringW& name, Sys::Date& timeCreate, Sys::Date& timeAccess, Sys::Date& timeWrite, AbcFileSysTxHandle transaction )
		{
			HANDLE handle;
			if ( transaction )	handle = NTDLL()->CreateFileTransacted(	name, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL, transaction, TXFS_MINIVERSION_COMMITTED_VIEW, NULL );
			else				handle = CreateFileW(					name, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
			if ( handle != INVALID_HANDLE_VALUE )
			{
				FILETIME a = timeCreate.ToFileTime();
				FILETIME b = timeAccess.ToFileTime();
				FILETIME c = timeWrite.ToFileTime();
				SetFileTime( handle,	timeCreate.IsNull() ? NULL : &a,
															timeAccess.IsNull() ? NULL : &b,
															timeWrite.IsNull() ? NULL : &c );
				CloseHandle( handle );
				return true;
			}
			return false;
		}

		OpenFailReason InterpretFileReason( DWORD sysErr )
		{
			OpenFailReason rea = OpenFailPermission;
			int err = GetLastError();
			switch ( err )
			{
			case ERROR_FILE_NOT_FOUND: rea = OpenFailDoesNotExist; break;
			case ERROR_ACCESS_DENIED: rea = OpenFailPermission; break;
			case ERROR_SHARING_VIOLATION: rea = OpenFailShare; break;
			default: ASSERT(false);
			}
			return rea;
		}

		bool Path::CheckReadPermission( const XString& filename, OpenFailReason* reason )
		{
			if ( filename.IsEmpty() )
			{
				if ( reason ) *reason = OpenFailDoesNotExist;
				return false;
			}
			HANDLE h = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( h == INVALID_HANDLE_VALUE )
			{
				if ( reason ) *reason = InterpretFileReason( GetLastError() );
				return false;
			}
			CloseHandle( h );
			return true;
		}

		bool Path::CheckWritePermission( const XString& file_or_directory, OpenFailReason* reason )
		{
			if ( file_or_directory.IsEmpty() )
			{
				if ( reason ) *reason = OpenFailDoesNotExist;
				return false;
			}

			XString testFile = file_or_directory;
			bool exists = true;

			DWORD attrib = ::GetFileAttributes( file_or_directory );
			if ( (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 )
			{
				// 'filename' is a directory, so test whether we can write to a temp file in that directory
				testFile = GetTempFileAt( file_or_directory, "pantest" );
				exists = false;
			}
			else
			{
				// 'filename' is probably a real file, or does not exist.
				exists = attrib != INVALID_FILE_ATTRIBUTES;
			}

			DWORD fatt = FILE_ATTRIBUTE_NORMAL;
			if ( !exists ) fatt |= FILE_ATTRIBUTE_TEMPORARY;
			HANDLE h = CreateFile( testFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, fatt, NULL );
			if ( h == INVALID_HANDLE_VALUE )
			{
				if ( reason ) *reason = InterpretFileReason( GetLastError() );
				return false;
			}
			CloseHandle( h );
			if ( !exists ) 
				DeleteFile( testFile );
			return true;
		}

		bool Path::GrantAllAccess( const XString& file_or_directory )
		{
			DWORD flags = 0;
			HANDLE hf = INVALID_HANDLE_VALUE;
			bool isDir = false;
			for ( int pass = 0; pass < 2; pass++ )
			{
				hf = CreateFile( file_or_directory, GENERIC_WRITE | WRITE_DAC, 0, NULL, OPEN_ALWAYS, flags, NULL );
				// pass 1: open directory
				if ( hf == INVALID_HANDLE_VALUE ) { flags |= FILE_FLAG_BACKUP_SEMANTICS; isDir = true; }
				else break;
			}
			if ( hf == INVALID_HANDLE_VALUE )
				return false;

			// Everyone has full access:				D:PAI(A;;FA;;;WD)
			// Everyone and SYSTEM has full access:		D:PAI(A;;FA;;;WD)(A;;FA;;;SY) 

			// This is Everyone and SYSTEM:
			//LPWSTR sddl = _T("D:PAI(A;;FAGARPWPCCDCLCSWLODTCR;;;WD)(A;;FA;;;SY)");
			//LPWSTR sddl = _T("D:PAI(A;;FA;;;WD)(A;;FA;;;SY)");
			LPWSTR sddl = _T("D:PAI(A;;FA;;;WD)(A;;FA;;;SY)");
			if ( isDir )
				sddl = _T("D:(A;CI;RCSDWDWOGARPWPCCDCLCSWLODTCR;;;WD)(A;CI;RCSDWDWOGARPWPCCDCLCSWLODTCR;;;SY)");

			PSECURITY_DESCRIPTOR desc;
			ULONG descSize;

			BOOL okc = ConvertStringSecurityDescriptorToSecurityDescriptor( sddl, SDDL_REVISION_1, &desc, &descSize );

			BOOL present, defaulted;
			PACL dacl;
			BOOL ok = GetSecurityDescriptorDacl( desc, &present, &dacl, &defaulted );

			DWORD res = SetSecurityInfo( hf, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, dacl, NULL );
			if ( res != ERROR_SUCCESS )
			{
				DWORD err = GetLastError();
			}

			LocalFree( desc );

			CloseHandle( hf );
			return res == ERROR_SUCCESS;
		}

		XString Path::GetTempFolder()
		{
			TCHAR buff[ MaxPathLength ];
			GetTempPath( MaxPathLength - 1, buff );
			XString str = buff;
			str.TrimRight( DirSepChar() );
			return str;
		}

		XString Path::GetTempFile( XString prefix, XString suffix )
		{
			return GetTempFileAt( GetTempFolder(), prefix, suffix );
		}

		XString Path::GetTempFileAt( XString path, XString prefix, XString suffix )
		{
			path.TrimRight( DIR_SEP_WS );
			if ( prefix.IsEmpty() ) prefix = "t1t";
			//if ( prefix.Length() <= 3 )
			//{
			//	TCHAR buff[ MaxPathLength ];
			//	int ok = GetTempFileName( path, prefix, 0, buff ); 
			//	if ( ok ) return buff;
			//}

			// use custom function because system GetTempFileName only allows 3 characters in the prefix
			int v = 0;
			for ( int i = 0; i < 1000; i++ )
			{
				XString name;
				name.Format( _T("%s%05d%s"), (LPCTSTR) prefix, v, (LPCTSTR) suffix );
				XString test = path + DIR_SEP_WS + name;
				if ( !Panacea::IO::Path::FileExists( test ) )
				{
					// erg.. race conditions!
					return test;
				}
				v++;
				if ( i % 50 == 0 ) v *= 2;
				// sanity check
				ASSERT( i < 100 );
			}
			return "";
		}

		XString Path::GetTempFileAndCleanOld( XString directory, XString prefix, XString suffix, double expire_age_seconds )
		{
			using namespace Panacea::Sys;
			dvect<XString> files;
			FindFilesOnly( directory + L"\\" + prefix + L"*", files );
			for ( int i = 0; i < files.size(); i++ )
			{
				XString full = directory + L"\\" + files[i];
				if ( Date::SecondsApart( Date::Now(), Path::GetFileWriteTime(full) ) > expire_age_seconds )
					DeleteFile( full );
			}

			return GetTempFileAt( directory, prefix, suffix );
		}

		XString Path::GetTempFileAndCleanOld( XString prefix, XString suffix, double expire_age_seconds )
		{
			return GetTempFileAndCleanOld( GetTempFolder(), prefix, suffix, expire_age_seconds );
		}

		XStringA Path::GetWindowsSpecialFolderA( int id, bool addTrailingSlash )
		{
			char path[MAX_PATH] = "";
			if ( SUCCEEDED(SHGetFolderPathA( NULL, id, NULL, SHGFP_TYPE_CURRENT, path )) )
				return (XStringA) path + (addTrailingSlash ? "\\" : "");
			else
				return "";
		}

		XStringW Path::GetWindowsSpecialFolderW( int id, bool addTrailingSlash )
		{
			wchar_t path[MAX_PATH] = L"";
			if ( SUCCEEDED(SHGetFolderPath( NULL, id, NULL, SHGFP_TYPE_CURRENT, path )) )
				return (XStringW) path + (addTrailingSlash ? L"\\" : L"");
			else
				return "";
		}

		XString Path::GetSpecialFolder( SpecialFolder folder, bool addTrailingSlash )
		{
			switch ( folder )
			{
			case SpecialFolderSystem: return GetWindowsSpecialFolder( CSIDL_SYSTEM, addTrailingSlash );
			case SpecialFolderCommonAppData: return GetWindowsSpecialFolder( CSIDL_COMMON_APPDATA, addTrailingSlash );
			case SpecialFolderPersonal: return GetWindowsSpecialFolder( CSIDL_PERSONAL, addTrailingSlash );
			case SpecialFolderUser: return GetWindowsSpecialFolder( CSIDL_PROFILE, addTrailingSlash ); // Uncertain whether this works below Win7
			case SpecialFolderUserAppData: return GetWindowsSpecialFolder( CSIDL_APPDATA, addTrailingSlash );
			case SpecialFolderLocalUserAppData: return GetWindowsSpecialFolder( CSIDL_LOCAL_APPDATA, addTrailingSlash );
			case SpecialFolderFonts: return GetWindowsSpecialFolder( CSIDL_FONTS, addTrailingSlash );
			default: ASSERT(false); return "";
			}
		}

		bool Path::FolderHasFiles( const XStringW& folder )
		{
			WIN32_FIND_DATA fd;
			XStringW sp = folder.EndsWith('\\') ? (folder + L"*") : (folder + L"\\*");
			HANDLE hf = FindFirstFile( sp, &fd );
			if ( hf == INVALID_HANDLE_VALUE )
			{
				return false;
			}
			bool any = false;
			for ( int i = 0; i < 5; i++ )
			{
				if ( wcscmp(fd.cFileName, L".") == 0 )  {}
				else if ( wcscmp(fd.cFileName, L"..") == 0 )  {}
				else { any = true; break; }
				BOOL ok = FindNextFile( hf, &fd );
				if ( !ok ) break;
			}
			FindClose( hf );
			return any;
		}

	}
}


PAPI	bool	PanPath_IsRelative( const XString& path )
{
	return IsRelativeT( (LPCWSTR) path );
}

PAPI	bool	PanPath_ResolveInplace( XString& path, u32 flags )
{
	XString r = PanPath_Resolve( path, flags );
	if ( r.IsEmpty() )
		return false;
	path = r;
	return true;
}

PAPI	XString	PanPath_Resolve( const XString& path, u32 flags )
{
	DWORD attrib = GetFileAttributes( path );
	bool exists = attrib != INVALID_FILE_ATTRIBUTES;
	bool isDir = !!(attrib & FILE_ATTRIBUTE_DIRECTORY);
	if ( exists )
	{
		if ( isDir && !!(flags & PanPath_OnlyFile) ) return "";
		if ( !isDir && !!(flags & PanPath_OnlyDir) ) return "";
		if ( PanPath_IsRelative(path) )
		{
			if ( !(flags & PanPath_AllowCurrentDir) ) return "";
			wchar_t cd[MAX_PATH];
			GetCurrentDirectory( MAX_PATH, cd );
			return Path::Combine(cd, path);
		}
		return path;
	}
	else return "";
}

PAPI	XString PanPath_Resolve( intp n, const XString* roots, const XString& path, u32 flags )
{
	XString final = PanPath_Resolve( path, flags );
	if ( final != "" ) return final;
	
	for ( intp i = 0; i < n; i++ )
	{
		final = PanPath_Resolve( Path::Combine(roots[i], path), flags );
		if ( final != "" ) return final;
	}
	return "";
}

PAPI	bool	PanPath_GetFileTimes( const XString& name, AbcDate* timeCreate, AbcDate* timeAccess, AbcDate* timeWrite, UINT64* fileSize, AbcFileSysTxHandle transaction )
{
	AbcDate tcreate, taccess, twrite;
	return Panacea::IO::Path::GetFileTimes( name, timeCreate ? *timeCreate : tcreate, timeAccess ? *timeAccess : taccess, timeWrite ? *timeWrite : twrite, fileSize, transaction );
}

// Does everything necessary for a pair of rolling logs
// NOTE: This function does no locking, so there are race conditions here.
PAPI	XString	PanPath_RollingPair( const XString& filename, u64 maxFileSize )
{
	XString fa, fb;
	PanPath_RollingPair_Paths( filename, fa, fb );
	XString res;
	// Pick the smaller of the two files. If that file is GTE maxFileSize, rewind it.
	u64 sizea = Panacea::IO::Path::GetFileSize( fa );
	u64 sizeb = Panacea::IO::Path::GetFileSize( fb );
	// In the case where both log files are over-size, we want to use the oldest one
	if ( sizea < maxFileSize )		res = fa;
	else if ( sizeb < maxFileSize ) res = fb;
	else							res = Panacea::IO::Path::GetFileWriteTime( fa ) <= Panacea::IO::Path::GetFileWriteTime( fb ) ? fa : fb;
	u64 res_size = res == fa ? sizea : sizeb;
	if ( res_size >= maxFileSize )
	{
		//AbcTrace( "PanPath_RollingPair: Deleting %S\n", (LPCWSTR) res );
		//DeleteFile( res );
		// My original implementation here deleted the file, but that implementation produces strange
		// enough observations that I think it's worth avoiding. Truncating seems a lot saner, since
		// we have no worry about file handles changing or anything of that nature.
		bool truncateOK = PanPath_SetFileLength( res, 0 );
		AbcTrace( "PanPath_RollingPair: Truncation of %s %s\n", (const char*) res.ToUtf8(), truncateOK ? "succeeded" : "failed" );
	}
	Panacea::IO::Path::CreateFolder( DirOnly(res) );
	return res;
}

PAPI	void	PanPath_RollingPair_Paths( const XString& filename, XString& xpath, XString& ypath )
{
	XString ext = FileExtension( filename );
	XString raw = ChangeExtension( filename, "" );
	xpath = raw + L"-x." + ext;
	ypath = raw + L"-y." + ext;
}

PAPI	bool	PanPath_SetFileLength( const XString& filename, u64 size )
{
	HANDLE fh = CreateFile( filename, GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
	if ( fh == INVALID_HANDLE_VALUE )
		return false;
	LONG high = size >> 32;
	DWORD resLow = SetFilePointer( fh, size & 0xffffffff, &high, FILE_BEGIN );
	u64 res = resLow | ((u64) high) << 32;
	bool success = false;
	if ( res == size )
		success = !!SetEndOfFile( fh );
	CloseHandle( fh );
	return success;
}

PAPI	bool	PanPath_DeleteFile( const XString& filename )
{
#ifdef _WIN32
	return TRUE == DeleteFile( filename );
#else
	return 0 == unlink( filename.ToUtf8() );
#endif
}

int LastSlash( const XString& path )
{
	int slash1 = path.ReverseFind(L'/');
	int slash2 = path.ReverseFind(L'\\');
	return max(slash1, slash2);
}

PAPI	XString FileNameAndExtension( const XString& path )
{
	int slash = LastSlash( path );
	if ( slash == -1 ) return path;
	return path.Mid( slash + 1 );
}

PAPI	XString FileNameOnly( const XString& path )
{
	int slash = LastSlash( path );
	int dot = path.ReverseFind('.');
	if ( dot == -1 )	return path.Mid( slash + 1 );
	else				return path.Mid( slash + 1, dot - slash - 1 );
}

PAPI	XString DirOnly( const XString& path )
{
	int slash = LastSlash( path );
	if ( slash == -1 )	return L"";
	else				return path.Left( slash + 1 );
}

PAPI	XString ChangeFileNameOnly( const XString& path, const XString& newName )
{
	XString drive, dir, name, ext;
	Panacea::IO::Path::Split( path, &drive, &dir, &name, &ext );
	return drive + dir + newName + ext;
}

PAPI	XString ChangeExtension( const XString& path, const XString& extension )
{
	return Panacea::IO::Path::ChangeExtension( path, extension );
}


PAPI	XString JoinPath( const XString& a, const XString& b )
{
	return Panacea::IO::Path::Combine( a, b );
}
