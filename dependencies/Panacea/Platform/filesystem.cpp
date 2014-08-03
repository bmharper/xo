#include "pch.h"
#include <string>
#include "filesystem.h"
#include "timeprims.h"

bool AbcFilesystemFindFiles( const char* _dir, std::function<bool(const AbcFilesystemItem& item)> callback )
{
	size_t dirLen = strlen(_dir);
	if ( dirLen == 0 )
		return false;

#ifdef _WIN32
	std::string fixed;
	const char* dir = _dir;
	if ( _dir[dirLen - 1] == '\\' )
	{
		fixed = _dir;
		fixed.pop_back();
		dir = fixed.c_str();
	}

	WIN32_FIND_DATAA fd;
	HANDLE handle = FindFirstFileA( dir, &fd );
	while ( handle != INVALID_HANDLE_VALUE )
	{
		AbcFilesystemItem item;
		item.IsDir = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		item.Name = fd.cFileName;
		item.TimeCreate = AbcFileTimeToUnixSeconds( fd.ftCreationTime );
		item.TimeModify = AbcFileTimeToUnixSeconds( fd.ftLastWriteTime );
		if ( !callback( item ) )
			break;
		if ( FALSE == FindNextFileA( handle, &fd ) )
			break;
	}
	bool ok = handle != INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND;
	if ( handle != INVALID_HANDLE_VALUE )
		FindClose( handle );
	return ok;
#else
#endif
}
