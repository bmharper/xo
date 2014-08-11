#include "pch.h"
#include "nuPlatform.h"

void* nuMallocOrDie( size_t bytes )
{
	void* b = malloc( bytes );
	NUASSERT(b);
	return b;
}

void* nuReallocOrDie( void* buf, size_t bytes )
{
	void* b = realloc( buf, bytes );
	NUASSERT(b);
	return b;
}

nuString nuCacheDir()
{
#if NU_PLATFORM_WIN_DESKTOP
	wchar_t* wpath;
	std::wstring path;
	if ( SUCCEEDED(SHGetKnownFolderPath( FOLDERID_LocalAppData, 0, NULL, &wpath )) )
		path = wpath;
	path.append( ABC_DIR_SEP_STR_W );
	path.append( L"xo" );
	CreateDirectoryW( path.c_str(), NULL );
	path.append( ABC_DIR_SEP_STR_W );
	path.append( L"cache" );
	CreateDirectoryW( path.c_str(), NULL );
	return ConvertWideToUTF8( path ).c_str();
#elif NU_PLATFORM_LINUX_DESKTOP
	NUTODO_STATIC
#elif NU_PLATFORM_ANDROID
	NUTODO_STATIC
#else
	NUTODO_STATIC
#endif
}
