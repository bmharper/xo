#include "pch.h"
#include "SysWnd_windows.h"
#include "DocGroup_windows.h"
#include "Render/RenderGL.h"
#include "Render/RenderDX.h"
#include "Image/Image.h"

namespace xo {

static const wchar_t* WClass        = L"xo";
static bool           IsFirstWindow = true;
static HICON          LargeAppIcon  = nullptr;
static HICON          SmallAppIcon  = nullptr;

SysWndWindows::SysWndWindows() {
}

SysWndWindows::~SysWndWindows() {
	if (Renderer) {
		Renderer->DestroyDevice(*this);
		delete Renderer;
	}
	if (Wnd)
		DestroyWindow(Wnd);
}

void SysWndWindows::PlatformInitialize(const InitParams* init) {
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	if (init) {
		LargeAppIcon = init->WindowsAppIconLarge;
		SmallAppIcon = init->WindowsAppIconSmall;
	}

	wcex.style         = CS_DBLCLKS; //CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc   = DocGroupWindows::StaticWndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = GetModuleHandle(NULL);
	wcex.hIcon         = LargeAppIcon;
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL; //(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName  = NULL;
	wcex.lpszClassName = WClass;
	wcex.hIconSm       = SmallAppIcon;

	ATOM wclass_atom = RegisterClassEx(&wcex);
}

Error SysWndWindows::Create(uint32_t createFlags) {
	Trace("DocGroup = %p\n", (void*) DocGroup);
	uint32_t ws = 0;
	if (!!(createFlags & CreateMinimizeButton))
		ws |= WS_CAPTION | WS_MINIMIZEBOX;
	if (!!(createFlags & CreateMaximizeButton))
		ws |= WS_CAPTION | WS_MAXIMIZEBOX;
	if (!!(createFlags & CreateCloseButton))
		ws |= WS_CAPTION | WS_SYSMENU;
	if (!!(createFlags & CreateBorder))
		ws |= WS_THICKFRAME;
	else
		ws |= WS_POPUP;
	uint32_t wsx = 0;
	Wnd          = CreateWindowEx(wsx, WClass, L"xo", ws, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), DocGroup);
	if (!Wnd)
		return Error::Fmt("Error creating window: %v", ErrorFrom_GetLastError().Message());
	auto err = InitializeRenderer();
	if (!err.OK()) {
		DestroyWindow(Wnd);
		Wnd = nullptr;
		return err;
	}
	QuitAppWhenWindowDestroyed = IsFirstWindow;
	IsFirstWindow              = false;
	return Error();
}

void SysWndWindows::Show() {
	ShowWindow(Wnd, SW_SHOW);
}

void SysWndWindows::SetTitle(const char* title) {
	SetWindowTextW(Wnd, ConvertUTF8ToWide(title).c_str());
}

void SysWndWindows::SetPosition(Box box, uint32_t setPosFlags) {
	uint32_t wflags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
	if (!!(setPosFlags & SetPosition_Move))
		wflags = wflags & ~SWP_NOMOVE;
	if (!!(setPosFlags & SetPosition_Size))
		wflags = wflags & ~SWP_NOSIZE;
	SetWindowPos(Wnd, NULL, box.Left, box.Top, box.Width(), box.Height(), wflags);
}

Box SysWndWindows::GetRelativeClientRect() {
	RECT  r;
	POINT p0 = {0, 0};
	ClientToScreen(Wnd, &p0);
	GetClientRect(Wnd, &r);
	Box box = r;
	box.Offset(p0.x, p0.y);
	return box;
}

void SysWndWindows::PostCursorChangedMessage() {
	PostMessage(Wnd, WM_XO_CURSOR_CHANGED, 0, 0);
}

void SysWndWindows::PostRepaintMessage() {
	::InvalidateRect(Wnd, nullptr, false);
}

bool SysWndWindows::CopySurfaceToImage(Box box, Image& img) {
	RECT r;
	GetClientRect(Wnd, &r);
	int width  = (int) box.Width();
	int height = (int) box.Height();
	if (width <= 0 || height <= 0)
		return false;
	if (!img.Alloc(TexFormatRGBA8, width, height))
		return false;
	BITMAPINFO bmpInfoLoc;
	HBITMAP    dibSec = nullptr;
	void*      bits   = nullptr;
	memset(&bmpInfoLoc.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmpInfoLoc.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bmpInfoLoc.bmiHeader.biBitCount      = 32;
	bmpInfoLoc.bmiHeader.biWidth         = width;
	bmpInfoLoc.bmiHeader.biHeight        = -height; // negative = top-down
	bmpInfoLoc.bmiHeader.biPlanes        = 1;
	bmpInfoLoc.bmiHeader.biCompression   = BI_RGB;
	bmpInfoLoc.bmiHeader.biSizeImage     = 0;
	bmpInfoLoc.bmiHeader.biXPelsPerMeter = 72;
	bmpInfoLoc.bmiHeader.biYPelsPerMeter = 72;
	bmpInfoLoc.bmiHeader.biClrUsed       = 0;
	bmpInfoLoc.bmiHeader.biClrImportant  = 0;
	dibSec                               = CreateDIBSection(NULL, &bmpInfoLoc, DIB_RGB_COLORS, &bits, NULL, NULL);
	if (!dibSec)
		return false;

	HDC dc = GetDC(Wnd);
	if (!dc) {
		DeleteObject(dibSec);
		return false;
	}

	HDC capDC = CreateCompatibleDC(dc);
	if (!capDC) {
		DeleteObject(dibSec);
		ReleaseDC(Wnd, dc);
		return false;
	}

	SelectObject(capDC, dibSec);
	BitBlt(capDC, 0, 0, width, height, dc, box.Left, box.Top, SRCCOPY);

	for (int y = 0; y < height; y++) {
		uint8_t* src     = (uint8_t*) bits + y * width * 4;
		uint8_t* dst     = (uint8_t*) img.DataAtLine(y);
		auto     src_end = src + width * 4;
		for (; src != src_end; src += 4, dst += 4) {
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst[3] = 0xff;
		}
	}

	DeleteObject(capDC);
	ReleaseDC(Wnd, dc);
	DeleteObject(dibSec);
	return true;
}

void SysWndWindows::MinimizeToSystemTray(const char* title, std::function<void()> showContextMenu) {
	SysWnd::MinimizeToSystemTray(title, showContextMenu);
	HideWindowOnClose = true;
	HasSysTrayIcon = true;

	NOTIFYICONDATA nd;
	// Add icon
	memset(&nd, 0, sizeof(nd));
	nd.cbSize = sizeof(nd);
	nd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
	nd.hWnd = Wnd;
	nd.uID = SysTrayIconID;
	nd.hIcon = LargeAppIcon;
	nd.uCallbackMessage = WM_XO_SYSTRAY_ICON;
	auto titlew = ConvertUTF8ToWide(title);
	wcsncpy(nd.szTip, titlew.c_str(), 127);
	BOOL ok = Shell_NotifyIcon(NIM_ADD, &nd);

	// toggle new (Vista+) behaviour
	memset(&nd, 0, sizeof(nd));
	nd.cbSize = sizeof(nd);
	nd.hWnd = Wnd;
	nd.uID = SysTrayIconID;
	nd.uVersion = NOTIFYICON_VERSION_4;
	ok = Shell_NotifyIcon(NIM_SETVERSION, &nd);

	int abc = 123;
}

} // namespace xo
