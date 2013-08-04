#include "pch.h"
#include "../Strings/strings.h"
#include "../Strings/wildcard.h"
#include "PackFile.h"

#ifdef _WIN32
#include <io.h>
#include <wchar.h>
#endif

PackFile::PackFile()
{
	// DirChar is our own dir separator - independent of the OS
	DirChar = L'/';
	CF = NULL;
	CaseSense = false;
	OwnMem = NULL;
	OwnMemSize = 0;
}

PackFile::~PackFile()
{
	Close();
}

void PackFile::SetCaseSensitivity( bool on )
{
	if ( CF )
		VERIFY( CF->SetCaseSensitive( on ) );
	CaseSense = on;
}

bool PackFile::Create( LPCWSTR pakfile )
{
	Close();
	CF = new CompoundFile();
	CF->SetCaseSensitive( CaseSense );
	return CF->Open( pakfile, CompoundFile::OpenFlagCreate | CompoundFile::OpenFlagCompressAll );
}

// Adds a path to the compound file.
// Returns the number of files added.
int PackFile::AddPath( LPCWSTR _root, LPCWSTR _wildcard, LPCWSTR _ignore )
{
	if ( _root == NULL ) { ASSERT(false); return 0; }
	vector< XStringW > files;
	vector< int > filesizes;
	XStringW wildcard = _wildcard ? _wildcard : L"*";
	XStringW root = _root;
	root.TrimRight( '\\' );
	root.TrimRight( '/' );

	XStringW ignore = _ignore;
	vector< XStringW > ignoreList;
	Split< wchar_t, vector<XStringW> > ( ignore, ';', ignoreList );

	// collect
	int cc = AddPath( root, L"", wildcard, ignoreList, files, filesizes );
	ASSERT( cc == files.size() );

	// add
	int fail = 0;
	for ( size_t i = 0; i < files.size(); i++ )
	{
		XStringW ss = files[i];
		XStringW tt1 = XString(DIR_SEP) + files[i];
		XStringW fullname = root + XString(DIR_SEP) + files[i];
		fullname.Replace( '\\', '/' );
		FILE* of = fopen( fullname.ToUtf8(), "rb" );
		if ( of == NULL ) 
		{
			fail++;
			continue;
		}
		void* buff = AbcMallocOrDie( filesizes[i] );
		fread( buff, filesizes[i], 1, of );
		fclose( of );
		// use the relative paths of the files as their names in the compound file
		CF->WriteSub( files[i], buff, filesizes[i] );
		free(buff);
	}
	return (int) (files.size() - fail);
}

bool PackFile::AddFile( LPCWSTR nameInPakFile, LPCWSTR filepath )
{
	XStringA buf;
	if ( !AbcIO_ReadWholeFile( filepath, buf ) )
		return false;
	return CF->WriteSub( nameInPakFile, buf.GetRawBufferConst(), buf.Length() );
}

// recursively collect
int PackFile::AddPath( const XStringW& root, const XStringW& relpath, const XStringW& wildcard, const vector< XStringW >& ignore, vector< XStringW >& files, vector< int >& filesizes )
{
	dvect<XString> relativePaths;
	Panacea::IO::Path::FindFilesOnly( root + DIR_SEP_WS + L"*", relativePaths, Panacea::IO::FindRecursiveOnAllFolders );
	int nadded = 0;
	for ( int i = 0; i < relativePaths.size(); i++ )
	{
		bool ignoreThis = false;
		for ( size_t j = 0; j < ignore.size(); j++ )
		{
			if ( MatchWildcard( relativePaths[i], ignore[j], CaseSense ) )
			{
				ignoreThis = true;
				break;
			}
		}
		if ( ignoreThis ) continue;

		if ( MatchWildcard( relativePaths[i], wildcard, CaseSense ) )
		{
			filesizes.push_back( (int) Panacea::IO::Path::GetFileSize( root + DIR_SEP_WS + relativePaths[i] ) );
			XStringW sname = relativePaths[i];
			sname.Replace( '\\', DirChar );
			files.push_back( sname );
			nadded++;
		}
	}
	return nadded;

	/*
	int cc = 0;
#if _MSC_VER >= 1400
	_wfinddata64_t ft;
#else 
	__wfinddata64_t ft;
#endif

	intptr_t handle;
	XStringW dirsearch_path = root + XStringW(DIR_SEP) + L"*";
	wchar_t buff[MAX_PATH];

#if _MSC_VER >= 1400
	wcscpy_s( buff, MAX_PATH - 1, dirsearch_path );
#else
	wcscpy( buff, dirsearch_path );
#endif

	handle = _wfindfirst64( buff, &ft );
	do 
	{
		XStringW rname = ft.name;
		if ( rname == L".." || rname == L"." ) continue;

		// peruse ignore list
		bool ignoreFile = false;
		for ( size_t i = 0; i < ignore.size(); i++ )
		{
			if ( MatchWildcard( rname, ignore[i], CaseSense ) )
			{
				ignoreFile = true;
				break;
			}
		}
		if ( ignoreFile ) continue;

		if ( ft.attrib & _A_SUBDIR )
		{
			XStringW npp = root + XStringW(DIR_SEP) + ft.name;
			XStringW np2 = relpath + ft.name + XStringW(DirChar);
			cc += AddPath( npp, np2, wildcard, ignore, files, filesizes );
		}
		else
		{
			bool wc = MatchWildcard( ft.name, wildcard, CaseSense );
			if ( wc )
			{
				filesizes.push_back( ft.size );
				XStringW sname = relpath + ft.name;
				sname.Replace( '\\', '/' );
				files.push_back( sname );
				cc++;
			}
		}
	} while ( _wfindnext64( handle, &ft ) == 0 );
	_findclose(handle);

	return cc;
	*/
}

bool PackFile::Save()
{
	return CF->Save();
}

void PackFile::Close()
{
	if (CF) 
	{
		CF->Close();
		delete CF;
	}
	CF = NULL;
	if ( OwnMem != NULL )
		free( OwnMem );
	OwnMem = NULL;
	OwnMemSize = 0;
}

bool PackFile::Open( LPCWSTR pakfile )
{
	Close();
	CF = new CompoundFile();
	if ( !CF->Open( pakfile, false ) ) return false;
	return true;
}

bool PackFile::Open( void* mem, size_t bytes, bool copy_memory )
{
	Close();
	if ( copy_memory )
	{
		OwnMem = malloc( bytes );
		if ( OwnMem == NULL ) { ASSERT(false); return false; }
		memcpy( OwnMem, mem, bytes );
		mem = OwnMem;
		OwnMemSize = bytes;
	}
	CF = new CompoundFile();
	if ( !CF->Open( mem, bytes, CompoundFile::OpenFlagNone ) ) return false;
	return true;
}

int PackFile::GetFileCount()
{
	if (CF == NULL) { ASSERT(false); return 0; }
	return CF->GetSubCount();
}

XStringW PackFile::GetFileName( int index )
{
	if (CF == NULL) { ASSERT(false); return L""; }
	return CF->GetSubName( index );
}

size_t PackFile::GetFileSize( LPCWSTR fname )
{
	if (CF == NULL) { ASSERT(false); return 0; }
	return (size_t) CF->SubSize(fname);
}

bool PackFile::ReadFile( LPCWSTR fname, void* buff )
{
	if (CF == NULL) { ASSERT(false); return false; }
	INT64 ss = CF->SubSize( fname );
	CF->SeekSub( fname, 0 );
	return CF->ReadSub( fname, buff, ss );
}

XStringA PackFile::ReadTextFileA( LPCWSTR fname )
{
	if (CF == NULL) { ASSERT(false); return ""; }
	INT64 ss = CF->SubSize(fname);
	if (ss == 0) return "";
	try
	{
		char *mem = new char[ ss + 8 ];
		if (mem)
		{
			CF->SeekSub( fname, 0 );
			CF->ReadSub( fname, mem, ss );
			mem[ss] = 0; // terminator
			XStringA str = mem;
			delete []mem;
			return str;
		}
	}
	catch (...)
	{
		return "";
	}
	return "";
}

XStringW PackFile::ReadTextFileW( LPCWSTR fname )
{
	if (CF == NULL) { ASSERT(false); return L""; }
	INT64 ss = CF->SubSize(fname);
	if (ss == 0) return L"";
	try
	{
		wchar_t *mem = new wchar_t[ ss + 8 ];
		if (mem)
		{
			CF->SeekSub( fname, 0 );
			CF->ReadSub( fname, mem, ss );
			mem[ss] = 0; // terminator
			XStringW str = mem;
			delete []mem;
			return str;
		}
	}
	catch (...)
	{
		return L"";
	}
	return L"";
}

size_t PackFile::MemoryUsed()
{
	size_t sum = OwnMemSize;
	sum += CF->MemoryUsed();
	return sum;
}

