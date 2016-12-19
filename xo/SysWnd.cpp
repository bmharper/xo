#include "pch.h"
#include "SysWnd.h"
#include "DocGroup.h"
#include "Doc.h"
#include "Render/RenderGL.h"
#include "Render/RenderDX.h"

namespace xo {

static const TCHAR* WClass = _T("xo");

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
}

SysWnd* SysWnd::Create(uint32_t createFlags) {
#if XO_PLATFORM_WIN_DESKTOP
	bool    ok = false;
	SysWnd* w  = new SysWnd();
	Trace("DocGroup = %p\n", w->DocGroup);
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
	w->Wnd = CreateWindowEx(wsx, WClass, _T("xo"), ws, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), w->DocGroup);
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
	w->Attach(new xo::Doc(), true);
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
