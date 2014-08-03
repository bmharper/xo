#pragma once

#include <functional>

#ifdef _WIN32
#define ABC_DIR_SEP_STR		 "\\"
#define ABC_DIR_SEP_STR_W	L"\\"
#else
#define ABC_DIR_SEP_STR		 "/"
#define ABC_DIR_SEP_STR_W	L"/"
#endif

struct AbcFilesystemItem
{
	const char* Root;		// Directory of file
	const char* Name;		// Name of file
	double		TimeCreate;	// Seconds since unix epoch, UTC
	double		TimeModify;	// Seconds since unix epoch, UTC
	bool		IsDir : 1;
};

/*
Enumerate files and directories inside the given directory.
The directory name must be pure, without any wildcards.
This function appends a '*' itself.
The callback must respond in the following ways
- If 'item' is a directory
	Return true to cause the directory to be entered
	Return false to skip entering the directory
- If 'item' is a file
	Return true to cause iteration to continue
	Return false to stop iteration of this directory
This function returns false if an error occurred other than "no files found"
*/
bool AbcFilesystemFindFiles( const char* dir, std::function<bool(const AbcFilesystemItem& item)> callback );