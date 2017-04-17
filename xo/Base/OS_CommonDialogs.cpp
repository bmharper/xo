#include "pch.h"
#include "../Doc.h"
#include "../DocGroup.h"
#include "../SysWnd.h"
#include "OS_CommonDialogs.h"

namespace xo {
namespace osdialogs {

#ifdef XO_PLATFORM_WIN_DESKTOP
XO_API bool BrowseForFile(BrowseFile& bf) {
	// See https://msdn.microsoft.com/en-us/library/bb776913(v=vs.85).aspx - titled "Common Item Dialog" for the example upon which this code was based.
	IFileDialog* pfd = NULL;
	const CLSID& id  = bf.IsOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
	HRESULT      hr  = CoCreateInstance(id, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	if (!SUCCEEDED(hr))
		return false;

	// Set the options on the dialog.
	DWORD dwFlags = 0;

	// Before setting, always get the options first in order not to override existing options.
	pfd->GetOptions(&dwFlags);
	pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);

	COMDLG_FILTERSPEC*        filters = new COMDLG_FILTERSPEC[bf.FileTypes.size()];
	std::vector<std::wstring> typeStorage;
	// Must load up storage first, so that interior pointers to strings are stable
	for (size_t i = 0; i < bf.FileTypes.size(); i++) {
		typeStorage.push_back(ConvertUTF8ToWide(bf.FileTypes[i].first));
		typeStorage.push_back(ConvertUTF8ToWide(bf.FileTypes[i].second));
	}
	for (size_t i = 0; i < bf.FileTypes.size(); i++) {
		filters[i].pszName = typeStorage[i * 2].c_str();
		filters[i].pszSpec = typeStorage[i * 2 + 1].c_str();
	}
	pfd->SetFileTypes((unsigned) bf.FileTypes.size(), filters);
	if (bf.FileTypeIndex != -1)
		pfd->SetFileTypeIndex((unsigned) bf.FileTypeIndex + 1);

	if (bf.Filename != "")
		pfd->SetFileName(ConvertUTF8ToWide(bf.Filename).c_str());

	// If we set the parent window here, then the process freezes. I don't know why this is, but I suspect
	// it might be due to the fact that xo event handler code runs on a different thread to the window
	// event processing (by design). It seems to be fine though - the OS freezes our window while the
	// the file browser dialog is open.
	//HWND parentWnd = bf.Parent ? bf.Parent->GetDocGroup()->Wnd->Wnd : NULL;
	hr = pfd->Show(NULL);
	if (!SUCCEEDED(hr)) {
		pfd->Release();
		return false;
	}

	bool good = false;

	IShellItem* res;
	if (SUCCEEDED(pfd->GetResult(&res))) {
		PWSTR path = nullptr;
		if (SUCCEEDED(res->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
			good        = true;
			bf.Filename = ConvertWideToUTF8(path);
			CoTaskMemFree(path);
		}
	}

	pfd->Release();
	return good;
}
#else
XO_API bool BrowseForFile(BrowseFile& bf) {
	return false;
}
#endif

XO_API bool BrowseForFileOpen(const Doc* doc, const std::vector<std::pair<std::string,std::string>>& types, std::string& filename) {
	BrowseFile bf;
	bf.Parent    = doc;
	bf.Filename  = filename;
	bf.FileTypes = types;
	if (!BrowseForFile(bf))
		return false;
	filename = bf.Filename;
	return true;
}

XO_API bool BrowseForFileSave(const Doc* doc, const std::vector<std::pair<std::string,std::string>>& types, std::string& filename) {
	BrowseFile bf;
	bf.IsOpen    = false;
	bf.Parent    = doc;
	bf.Filename  = filename;
	bf.FileTypes = types;
	if (!BrowseForFile(bf))
		return false;
	filename = bf.Filename;
	return true;
}
}
} // namespace xo
