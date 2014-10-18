#include "pch.h"
#include <string>
#include "filesystem.h"
#include "timeprims.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>	// Added for Android
#endif

#ifndef _WIN32
	#ifdef ANDROID
		// unsigned long       st_mtime;
		// unsigned long       st_mtime_nsec;
		// #define STAT_TIME(st, x) (st.st_ ## x ## time_nsec) * (1.0 / 1000000000)
		#define STAT_TIME(st, x) (st.st_ ## x ## time) + ((st.st_ ## x ## time_nsec) * (1.0 / 1000000000))
	#else
		// struct timespec st_mtim;  /* time of last modification */
		#define STAT_TIME(st, x) (st.st_ ## x ## tim.tv_sec) + ((st.st_ ## x ## tim.tv_nsec) * (1.0 / 1000000000))
	#endif
#endif

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
	std::string fixed = _dir;
	if ( _dir[dirLen - 1] == '/' )
		fixed.pop_back();

	DIR* d = opendir( fixed.c_str() );
	if ( d )
	{
		AbcFilesystemItem item;
		item.Root = fixed.c_str();
		struct dirent block;
		struct dirent* iter = nullptr;
		while ( readdir_r(d, &block, &iter) == 0 && iter != nullptr )
		{
			if ( strcmp(iter->d_name, ".") == 0 ) continue;
			if ( strcmp(iter->d_name, "..") == 0 ) continue;
			item.IsDir = iter->d_type == DT_DIR;
			item.Name = iter->d_name;
			struct stat st;
			if ( stat( (fixed + "/" + iter->d_name).c_str(), &st ) != 0 )
				continue;
			item.TimeModify = STAT_TIME(st, m); // st.st_mtim.tv_sec + st.st_mtim.tv_nsec * (1.0 / 1000000000);
			bool res = callback( item );
			if ( item.IsDir )
			{
				if ( res )
					AbcFilesystemFindFiles( (fixed + "/" + item.Name).c_str(), callback );
			}
			else
			{
				if ( !res )
					break;
			}
		}
		closedir( d );
	}
#endif
}

#ifndef _WIN32
#undef STAT_TIME
#endif
