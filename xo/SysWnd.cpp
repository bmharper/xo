#include "pch.h"
#include "SysWnd.h"
#include "DocGroup.h"
#include "Doc.h"
#include "Render/RenderGL.h"
#include "Render/RenderDX.h"
#include "Image//Image.h"

namespace xo {

static const wchar_t* WClass = L"xo";

#if XO_PLATFORM_ANDROID
SysWnd* SingleMainWnd = NULL;
#elif XO_PLATFORM_LINUX_DESKTOP
// TODO: multiple windows
SysWnd* SingleMainWnd = NULL;
#endif

static int64_t NumWindowsCreated = 0;

void SysWnd::PlatformInitialize() {
#if XO_PLATFORM_WIN_DESKTOP
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style         = CS_DBLCLKS; //CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc   = DocGroup::StaticWndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = GetModuleHandle(NULL);
	wcex.hIcon         = NULL;
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL; //(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName  = NULL;
	wcex.lpszClassName = WClass;
	wcex.hIconSm       = NULL;

	ATOM wclass_atom = RegisterClassEx(&wcex);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
#else
	XO_TODO_STATIC
#endif
}

SysWnd::SysWnd() {
	InvalidRect.SetInverted();
#if XO_PLATFORM_WIN_DESKTOP
	Wnd                        = NULL;
	TimerPeriodMS              = 0;
	QuitAppWhenWindowDestroyed = NumWindowsCreated == 0;
#elif XO_PLATFORM_ANDROID
	SingleMainWnd      = this;
	RelativeClientRect = Box(0, 0, 0, 0);
#elif XO_PLATFORM_LINUX_DESKTOP
	XDisplay = NULL;
	//XWindowRoot = NULL;
	VisualInfo = NULL;
	//ColorMap = NULL;
	//SetWindowAttributes = NULL;
	//XWindow = NULL;
	GLContext = NULL;
//WindowAttributes = NULL;
//Event = NULL;
#else
	XO_TODO_STATIC
#endif
	NumWindowsCreated++;
	DocGroup      = new xo::DocGroup();
	DocGroup->Wnd = this;
	Renderer      = NULL;
}

SysWnd::~SysWnd() {
#if XO_PLATFORM_WIN_DESKTOP
	if (Renderer) {
		Renderer->DestroyDevice(*this);
		delete Renderer;
		Renderer = NULL;
	}
	DestroyWindow(Wnd);
#elif XO_PLATFORM_ANDROID
	SingleMainWnd      = NULL;
#elif XO_PLATFORM_LINUX_DESKTOP
	if (XDisplay != nullptr) {
		glXMakeCurrent(XDisplay, None, NULL);
		glXDestroyContext(XDisplay, GLContext);
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
	}
	SingleMainWnd = NULL;
#else
	XO_TODO_STATIC
#endif
	Global()->DocRemoveQueue.Add(DocGroup);
	DocGroup = NULL;

	// For a simple, single-window application, by the time we get here, the message loop has already exited, so
	// the application can be confident that it can clean up resources by now, without worrying that it's still
	// going to be receiving UI input during that time. See RunAppLowLevel() and RunApp() to understand how
	// this works.
	for (auto cb : OnWindowClose)
		cb();
}

SysWnd* SysWnd::Create(uint32_t createFlags) {
#if XO_PLATFORM_WIN_DESKTOP
	bool    ok = false;
	SysWnd* w  = new SysWnd();
	Trace("DocGroup = %p\n", (void*) w->DocGroup);
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
	//w->Wnd = CreateWindow(WClass, _T("xo"), ws, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), w->DocGroup);
	w->Wnd = CreateWindowEx(wsx, WClass, L"xo", ws, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), w->DocGroup);
	if (w->Wnd) {
		if (w->InitializeRenderer())
			ok = true;
	}
	if (!ok) {
		delete w;
		w = NULL;
	}
	return w;
#elif XO_PLATFORM_ANDROID
	SysWnd* w          = new SysWnd();
	w->InitializeRenderer();
	return w;
#elif XO_PLATFORM_LINUX_DESKTOP
	GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};
	SysWnd* w = new SysWnd();
	w->XDisplay = XOpenDisplay(NULL);
	if (w->XDisplay == NULL) {
		Trace("Cannot connect to X server\n");
		delete w;
		return nullptr;
	}
	w->XWindowRoot = DefaultRootWindow(w->XDisplay);
	w->VisualInfo = glXChooseVisual(w->XDisplay, 0, att);
	if (w->VisualInfo == NULL) {
		Trace("no appropriate visual found\n");
		delete w;
		return nullptr;
	}
	Trace("visual %p selected\n", (void*) w->VisualInfo->visualid);
	w->ColorMap = XCreateColormap(w->XDisplay, w->XWindowRoot, w->VisualInfo->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = w->ColorMap;
	swa.event_mask = ExposureMask | KeyPressMask | PointerMotionMask;
	w->XWindow = XCreateWindow(w->XDisplay, w->XWindowRoot, 0, 0, 600, 600, 0, w->VisualInfo->depth, InputOutput, w->VisualInfo->visual, CWColormap | CWEventMask, &swa);
	XMapWindow(w->XDisplay, w->XWindow);
	XStoreName(w->XDisplay, w->XWindow, "xo");
	w->GLContext = glXCreateContext(w->XDisplay, w->VisualInfo, NULL, GL_TRUE);
	glXMakeCurrent(w->XDisplay, w->XWindow, w->GLContext);
	w->InitializeRenderer();
	glXMakeCurrent(w->XDisplay, None, NULL);
	SingleMainWnd = w;
	return w;
#else
	XO_TODO_STATIC
#endif
}

SysWnd* SysWnd::CreateWithDoc(uint32_t createFlags) {
	SysWnd* w = Create(createFlags);
	if (!w)
		return NULL;
	w->Attach(new xo::Doc(w->DocGroup), true);
	Global()->DocAddQueue.Add(w->DocGroup);
	return w;
}

void SysWnd::Show() {
#if XO_PLATFORM_WIN_DESKTOP
	ShowWindow(Wnd, SW_SHOW);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
#else
	XO_TODO_STATIC
#endif
}

Doc* SysWnd::Doc() {
	return DocGroup->Doc;
}

void SysWnd::Attach(xo::Doc* doc, bool destroyDocWithGroup) {
	DocGroup->Doc                 = doc;
	DocGroup->DestroyDocWithGroup = destroyDocWithGroup;
}

bool SysWnd::BeginRender() {
	if (Renderer)
		return Renderer->BeginRender(*this);
	else
		return false;
}

void SysWnd::EndRender(uint32_t endRenderFlags) {
	if (Renderer) {
		XOTRACE_LATENCY("EndRender (begin) %s\n", !!(endRenderFlags & EndRenderNoSwap) ? "noswap" : "swap");
		Renderer->EndRender(*this, endRenderFlags);
		XOTRACE_LATENCY("EndRender (end)\n");
	}
}

void SysWnd::SurfaceLost() {
	if (Renderer)
		Renderer->SurfaceLost();
}

void SysWnd::SetPosition(Box box, uint32_t setPosFlags) {
#if XO_PLATFORM_WIN_DESKTOP
	uint32_t wflags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
	if (!!(setPosFlags & SetPosition_Move))
		wflags = wflags & ~SWP_NOMOVE;
	if (!!(setPosFlags & SetPosition_Size))
		wflags = wflags & ~SWP_NOSIZE;
	SetWindowPos(Wnd, NULL, box.Left, box.Top, box.Width(), box.Height(), wflags);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
	Trace("SysWnd.SetPosition is not implemented\n");
#else
	XO_TODO_STATIC
#endif
}

Box SysWnd::GetRelativeClientRect() {
#if XO_PLATFORM_WIN_DESKTOP
	RECT  r;
	POINT p0 = {0, 0};
	ClientToScreen(Wnd, &p0);
	GetClientRect(Wnd, &r);
	Box box = r;
	box.Offset(p0.x, p0.y);
	return box;
#elif XO_PLATFORM_ANDROID
	return RelativeClientRect;
#elif XO_PLATFORM_LINUX_DESKTOP
	XWindowAttributes wa;
	XGetWindowAttributes(XDisplay, XWindow, &wa);
	return Box(wa.x, wa.y, wa.x + wa.width, wa.y + wa.height);
#else
	XO_TODO_STATIC
#endif
}

void SysWnd::PostCursorChangedMessage() {
#if XO_PLATFORM_WIN_DESKTOP
	PostMessage(Wnd, WM_XO_CURSOR_CHANGED, 0, 0);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
// See this thread for help on injecting a dummy message into the X queue
// http://stackoverflow.com/questions/8592292/how-to-quit-the-blocking-of-xlibs-xnextevent
#else
	XO_TODO_STATIC
#endif
}

void SysWnd::PostRepaintMessage() {
#if XO_PLATFORM_WIN_DESKTOP
	::InvalidateRect(Wnd, nullptr, false);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
#else
	XO_TODO_STATIC
#endif
}

bool SysWnd::CopySurfaceToImage(Box box, Image& img) {
#if XO_PLATFORM_WIN_DESKTOP
	RECT r;
	GetClientRect(Wnd, &r);
	int width = (int) box.Width();
	int height = (int) box.Height();
	if (width <= 0 || height <= 0)
		return false;
	if (!img.Alloc(TexFormatRGBA8, width, height))
		return false;
	BITMAPINFO bmpInfoLoc;
	HBITMAP    dibSec = nullptr;
	void* bits = nullptr;
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
	dibSec                           = CreateDIBSection(NULL, &bmpInfoLoc, DIB_RGB_COLORS, &bits, NULL, NULL);
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
		uint8_t* src = (uint8_t*) bits + y * width * 4;
		uint8_t* dst = (uint8_t*) img.DataAtLine(y);
		auto src_end = src + width * 4;
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
#elif XO_PLATFORM_ANDROID
	return false;
#elif XO_PLATFORM_LINUX_DESKTOP
	return false;
#else
	XO_TODO_STATIC
#endif
}

void SysWnd::InvalidateRect(Box box) {
	std::lock_guard<std::mutex> lock(InvalidRect_Lock);
	InvalidRect.ExpandToFit(box);
}

Box SysWnd::GetInvalidateRect() {
	std::lock_guard<std::mutex> lock(InvalidRect_Lock);
	return InvalidRect;
}

void SysWnd::ValidateWindow() {
	std::lock_guard<std::mutex> lock(InvalidRect_Lock);
	InvalidRect.SetInverted();
}

bool SysWnd::InitializeRenderer() {
#if XO_PLATFORM_WIN_DESKTOP
	if (Global()->PreferOpenGL) {
		if (InitializeRenderer_Any<RenderGL>(Renderer))
			return true;
		if (InitializeRenderer_Any<RenderDX>(Renderer))
			return true;
	} else {
		if (InitializeRenderer_Any<RenderDX>(Renderer))
			return true;
		if (InitializeRenderer_Any<RenderGL>(Renderer))
			return true;
	}
	return false;
#else
	InitializeRenderer_Any<RenderGL>(Renderer);
#endif
}

template <typename TRenderer>
bool SysWnd::InitializeRenderer_Any(RenderBase*& renderer) {
	renderer = new TRenderer();
	if (renderer->InitializeDevice(*this)) {
		Trace("Successfully initialized %s renderer\n", renderer->RendererName());
		return true;
	} else {
		Trace("Failed to initialize %s renderer\n", renderer->RendererName());
		delete renderer;
		renderer = NULL;
		return false;
	}
}
}
