#include "pch.h"
#include "OS_Clipboard.h"

namespace xo {

#if XO_PLATFORM_WIN_DESKTOP
XO_API std::string ClipboardRead() {
	if (!OpenClipboard(NULL))
		return "";
	std::string res;
	HANDLE h = GetClipboardData(CF_UNICODETEXT);
	if (h) {
		const wchar_t* src = (const wchar_t*) GlobalLock((HGLOBAL) h);
		res = ConvertWideToUTF8(std::wstring(src));
		GlobalUnlock((HGLOBAL) h);
	}
	CloseClipboard();
	return res;
}
#else
XO_API std::string ClipboardRead() {
	return "";
}
#endif

}

