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
	std::string fixed = _dir;
	if ( _dir[dirLen - 1] == '\\' )
		fixed.pop_back();

	WIN32_FIND_DATAA fd;
	HANDLE handle = FindFirstFileA( (fixed + "\\*").c_str(), &fd );
	AbcFilesystemItem item;
	item.Root = fixed.c_str();
	while ( handle != INVALID_HANDLE_VALUE )
	{
		bool ignore =	(fd.cFileName[0] == '.' && fd.cFileName[1] == 0) ||
						(fd.cFileName[0] == '.' && fd.cFileName[1] == '.' && fd.cFileName[2] == 0);
		if ( !ignore )
		{
			item.IsDir = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			item.Name = fd.cFileName;
			item.TimeCreate = AbcFileTimeToUnixSeconds( fd.ftCreationTime );
			item.TimeModify = AbcFileTimeToUnixSeconds( fd.ftLastWriteTime );
			bool res = callback( item );
			if ( item.IsDir )
			{
				if ( res )
					AbcFilesystemFindFiles( (fixed + "\\" + item.Name).c_str(), callback );
			}
			else
			{
				if ( !res )
					break;
			}
		}
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
