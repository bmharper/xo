#include "pch.h"
#include "xoPlatform.h"

void* xoMallocOrDie(size_t bytes)
{
	void* b = malloc(bytes);
	XOASSERT(b);
	return b;
}

void* xoReallocOrDie(void* buf, size_t bytes)
{
	void* b = realloc(buf, bytes);
	XOASSERT(b);
	return b;
}

xoString xoDefaultCacheDir()
{
#if XO_PLATFORM_WIN_DESKTOP
	wchar_t* wpath;
	std::wstring path;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &wpath)))
		path = wpath;
	path.append(ABC_DIR_SEP_STR_W);
	path.append(L"xo");
	CreateDirectoryW(path.c_str(), NULL);
	path.append(ABC_DIR_SEP_STR_W);
	path.append(L"cache");
	CreateDirectoryW(path.c_str(), NULL);
	return ConvertWideToUTF8(path).c_str();
#elif XO_PLATFORM_LINUX_DESKTOP
	struct stat st = {0};
	struct passwd *pw = getpwuid(getuid());
	xoString path = pw->pw_dir;
	path += "/.xo";
	if (stat(path.Z, &st) == -1)
		mkdir(path.Z, 0700);
	return path;
#elif XO_PLATFORM_ANDROID
	return ""; // The cache dir on Android is fed in to us via JNI, through xoInitParams
#else
	XOTODO_STATIC
#endif
}
