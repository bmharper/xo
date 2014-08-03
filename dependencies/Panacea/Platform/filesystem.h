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
	const char* Name;
	double		TimeCreate;	// Seconds since unix epoch, UTC
	double		TimeModify;	// Seconds since unix epoch, UTC
	bool		IsDir : 1;
};

// Enumerate files and directories inside the given directory
// Return false from the callback to stop enumeration
// This function returns false if an error occurred other than "no files found"
bool AbcFilesystemFindFiles( const char* dir, std::function<bool(const AbcFilesystemItem& item)> callback );