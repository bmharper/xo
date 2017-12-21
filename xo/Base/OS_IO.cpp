#include "pch.h"
#include "OS_IO.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h> // Added for Android
#endif

#ifdef __APPLE__
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <unistd.h>
#endif

#ifndef _WIN32
#ifdef ANDROID
// unsigned long       st_mtime;
// unsigned long       st_mtime_nsec;
// #define STAT_TIME(st, x) (st.st_ ## x ## time_nsec) * (1.0 / 1000000000)
#define STAT_TIME(st, x) (st.st_##x##time) + ((st.st_##x##time_nsec) * (1.0 / 1000000000))
#elif defined(__APPLE__)
// struct timespec st_mtimespec;	/* time of last data modification */
#define STAT_TIME(st, x) (st.st_##x##timespec.tv_sec) + ((st.st_##x##timespec.tv_nsec) * (1.0 / 1000000000))
#else
// struct timespec st_mtim;  /* time of last modification */
#define STAT_TIME(st, x) (st.st_##x##tim.tv_sec) + ((st.st_##x##tim.tv_nsec) * (1.0 / 1000000000))
#endif
#endif

namespace xo {

XO_API String DefaultCacheDir() {
#if XO_PLATFORM_WIN_DESKTOP
	wchar_t*     wpath;
	std::wstring path;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &wpath)))
		path = wpath;
	path.append(L"\\");
	path.append(L"xo");
	CreateDirectoryW(path.c_str(), NULL);
	path.append(L"\\");
	path.append(L"cache");
	CreateDirectoryW(path.c_str(), NULL);
	return ConvertWideToUTF8(path).c_str();
#elif XO_PLATFORM_LINUX_DESKTOP || XO_PLATFORM_OSX
	struct stat    st   = {0};
	struct passwd* pw   = getpwuid(getuid());
	String         path = pw->pw_dir;
	path += "/.xo";
	if (stat(path.Z, &st) == -1)
		mkdir(path.Z, 0700);
	return path;
#elif XO_PLATFORM_ANDROID
	return ""; // The cache dir on Android is fed in to us via JNI, through InitParams
#else
	XO_TODO_STATIC
#endif
}

#ifdef _WIN32
inline int64_t FileTimeToMicroseconds(const FILETIME& ft) {
	uint64_t time  = ((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	int64_t  stime = (int64_t) time;
	return stime / 10;
}

inline double FileTimeToUnixSeconds(const FILETIME& ft) {
	const int64_t days_from_1601_to_1970 = 370 * 365 - 276;
	const int64_t microsecondsPerDay     = 24 * 3600 * (int64_t) 1000000;
	int64_t       micro                  = FileTimeToMicroseconds(ft);
	return (micro - (days_from_1601_to_1970 * microsecondsPerDay)) * (1.0 / 1000000.0);
}
#endif

XO_API bool FindFiles(const char* _dir, std::function<bool(const FilesystemItem& item)> callback) {
	size_t dirLen = strlen(_dir);
	if (dirLen == 0)
		return false;

#ifdef _WIN32
	std::string fixed = _dir;
	if (_dir[dirLen - 1] == '\\')
		fixed.pop_back();

	WIN32_FIND_DATAA fd;
	HANDLE           handle = FindFirstFileA((fixed + "\\*").c_str(), &fd);
	FilesystemItem   item;
	item.Root = fixed.c_str();
	while (handle != INVALID_HANDLE_VALUE) {
		bool ignore = (fd.cFileName[0] == '.' && fd.cFileName[1] == 0) ||
		              (fd.cFileName[0] == '.' && fd.cFileName[1] == '.' && fd.cFileName[2] == 0);
		if (!ignore) {
			item.IsDir      = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			item.Name       = fd.cFileName;
			item.TimeModify = FileTimeToUnixSeconds(fd.ftLastWriteTime);
			bool res        = callback(item);
			if (item.IsDir) {
				if (res)
					FindFiles((fixed + "\\" + item.Name).c_str(), callback);
			} else {
				if (!res)
					break;
			}
		}
		if (FALSE == FindNextFileA(handle, &fd))
			break;
	}
	bool ok = handle != INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND;
	if (handle != INVALID_HANDLE_VALUE)
		FindClose(handle);
	return ok;
#else
	std::string fixed = _dir;
	if (_dir[dirLen - 1] == '/')
		fixed.pop_back();

	DIR* d  = opendir(fixed.c_str());
	bool ok = d != nullptr;
	if (d) {
		FilesystemItem item;
		item.Root = fixed.c_str();
		struct dirent  block;
		struct dirent* iter = nullptr;
		while (readdir_r(d, &block, &iter) == 0 && iter != nullptr) {
			if (strcmp(iter->d_name, ".") == 0)
				continue;
			if (strcmp(iter->d_name, "..") == 0)
				continue;
			item.IsDir = iter->d_type == DT_DIR;
			item.Name  = iter->d_name;
			struct stat st;
			if (stat((fixed + "/" + iter->d_name).c_str(), &st) != 0)
				continue;
			item.TimeModify = STAT_TIME(st, m); // st.st_mtim.tv_sec + st.st_mtim.tv_nsec * (1.0 / 1000000000);
			bool res        = callback(item);
			if (item.IsDir) {
				if (res)
					FindFiles((fixed + "/" + item.Name).c_str(), callback);
			} else {
				if (!res)
					break;
			}
		}
		closedir(d);
	}
	return ok;
#endif
}

#ifndef _WIN32
#undef STAT_TIME
#endif
} // namespace xo
