#include "pch.h"
#include "Error.h"

namespace xo {

XO_API StaticError ErrEACCESS("Tried to open a read-only file for writing, file's sharing mode does not allow the specified operations, or the given path is a directory");
XO_API StaticError ErrEEXIST("File already exists");
XO_API StaticError ErrEINVAL("Invalid oflag or pmode argument");
XO_API StaticError ErrEMFILE("No more file descriptors are available (too many files are open)");
XO_API StaticError ErrENOENT("File or path not found");


#ifdef _WIN32
XO_API Error ErrorFrom_GetLastError() {
	return ErrorFrom_GetLastError(GetLastError());
}

#pragma warning(push)
#pragma warning(disable : 6031) // snprintf return value ignored
XO_API Error ErrorFrom_GetLastError(DWORD err) {
	switch (err) {
	case ERROR_ACCESS_DENIED: return ErrEACCESS;
	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS: return ErrEEXIST;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_NO_MORE_FILES: return ErrENOENT;
	default: break;
	}

	char   szBuf[1024];
	LPVOID lpMsgBuf;

	FormatMessageA(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER |
	        FORMAT_MESSAGE_FROM_SYSTEM,
	    nullptr,
	    err,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    (LPSTR) &lpMsgBuf,
	    0, nullptr);

	snprintf(szBuf, sizeof(szBuf), "(%u) %s", err, (const char*) lpMsgBuf);
	szBuf[sizeof(szBuf) - 1] = 0;
	LocalFree(lpMsgBuf);

	// chop off trailing carriage returns
	std::string r = szBuf;
	while (r.length() > 0 && (r[r.length() - 1] == 10 || r[r.length() - 1] == 13))
		r.resize(r.length() - 1);

	return Error(r);
}
#pragma warning(pop)
#endif
}