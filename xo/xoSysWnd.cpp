#include "pch.h"
#include "xoSysWnd.h"
#include "xoDocGroup.h"
#include "xoDoc.h"
#include "Render/xoRenderGL.h"
#include "Render/xoRenderDX.h"

static const TCHAR*		WClass = _T("xo");

#if XO_PLATFORM_ANDROID
xoSysWnd*				SingleMainWnd = NULL;
#elif XO_PLATFORM_LINUX_DESKTOP
// TODO: multiple windows
xoSysWnd*				SingleMainWnd = NULL;
#endif

static int64			NumWindowsCreated = 0;

void xoSysWnd::PlatformInitialize()
{
#if XO_PLATFORM_WIN_DESKTOP
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_DBLCLKS; //CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= xoDocGroup::StaticWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetModuleHandle(NULL);
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= WClass;
	wcex.hIconSm		= NULL;

	ATOM wclass_atom = RegisterClassEx(&wcex);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
#else
	XOTODO_STATIC
#endif
}

xoSysWnd::xoSysWnd()
{
#if XO_PLATFORM_WIN_DESKTOP
	SysWnd = NULL;
	TimerPeriodMS = 0;
	QuitAppWhenWindowDestroyed = NumWindowsCreated == 0;
#elif XO_PLATFORM_ANDROID
	SingleMainWnd = this;
	RelativeClientRect = xoBox(0,0,0,0);
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
	XOTODO_STATIC
#endif
	NumWindowsCreated++;
	DocGroup = new xoDocGroup();
	DocGroup->Wnd = this;
	Renderer = NULL;
}

xoSysWnd::~xoSysWnd()
{
#if XO_PLATFORM_WIN_DESKTOP
	if (Renderer)
	{
		Renderer->DestroyDevice(*this);
		delete Renderer;
		Renderer = NULL;
	}
	DestroyWindow(SysWnd);
#elif XO_PLATFORM_ANDROID
	SingleMainWnd = NULL;
#elif XO_PLATFORM_LINUX_DESKTOP
	if (XDisplay != nullptr)
	{
		glXMakeCurrent(XDisplay, None, NULL);
		glXDestroyContext(XDisplay, GLContext);
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
	}
	SingleMainWnd = NULL;
#else
	XOTODO_STATIC
#endif
	xoGlobal()->DocRemoveQueue.Add(DocGroup);
	DocGroup = NULL;
}

xoSysWnd* xoSysWnd::Create(uint createFlags)
{
#if XO_PLATFORM_WIN_DESKTOP
	bool ok = false;
	xoSysWnd* w = new xoSysWnd();
	XOTRACE("DocGroup = %p\n", w->DocGroup);
	uint ws = 0;
	if (!!(createFlags & CreateMinimizeButton)) ws |= WS_CAPTION | WS_MINIMIZEBOX;
	if (!!(createFlags & CreateMaximizeButton)) ws |= WS_CAPTION | WS_MAXIMIZEBOX;
	if (!!(createFlags & CreateCloseButton)) ws |= WS_CAPTION | WS_SYSMENU;
	if (!!(createFlags & CreateBorder)) ws |= WS_THICKFRAME; 
	else ws |= WS_POPUP;
	uint wsx = 0;
	//w->SysWnd = CreateWindow(WClass, _T("xo"), ws, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), w->DocGroup);
	w->SysWnd = CreateWindowEx(wsx, WClass, _T("xo"), ws, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), w->DocGroup);
	if (w->SysWnd)
	{
		if (w->InitializeRenderer())
			ok = true;
	}
	if (!ok)
	{
		delete w;
		w = NULL;
	}
	return w;
#elif XO_PLATFORM_ANDROID
	xoSysWnd* w = new xoSysWnd();
	w->InitializeRenderer();
	return w;
#elif XO_PLATFORM_LINUX_DESKTOP
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
	xoSysWnd* w = new xoSysWnd();
	w->XDisplay = XOpenDisplay(NULL);
	if (w->XDisplay == NULL) { XOTRACE("Cannot connect to X server\n"); delete w; return nullptr; }
	w->XWindowRoot = DefaultRootWindow(w->XDisplay);
	w->VisualInfo = glXChooseVisual(w->XDisplay, 0, att);
	if (w->VisualInfo == NULL) { XOTRACE("no appropriate visual found\n"); delete w; return nullptr; }
	XOTRACE("visual %p selected\n", (void*) w->VisualInfo->visualid);
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
	XOTODO_STATIC
#endif
}

xoSysWnd* xoSysWnd::CreateWithDoc(uint createFlags)
{
	xoSysWnd* w = Create(createFlags);
	if (!w)
		return NULL;
	w->Attach(new xoDoc(), true);
	xoGlobal()->DocAddQueue.Add(w->DocGroup);
	return w;
}

void xoSysWnd::Show()
{
#if XO_PLATFORM_WIN_DESKTOP
	ShowWindow(SysWnd, SW_SHOW);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
#else
	XOTODO_STATIC
#endif
}

xoDoc* xoSysWnd::Doc()
{
	return DocGroup->Doc;
}

void xoSysWnd::Attach(xoDoc* doc, bool destroyDocWithGroup)
{
	DocGroup->Doc = doc;
	DocGroup->DestroyDocWithGroup = destroyDocWithGroup;
}

bool xoSysWnd::BeginRender()
{
	if (Renderer)
		return Renderer->BeginRender(*this);
	else
		return false;
}

void xoSysWnd::EndRender(uint endRenderFlags)
{
	if (Renderer)
	{
		XOTRACE_LATENCY("EndRender (begin) %s\n", !!(endRenderFlags & xoEndRenderNoSwap) ? "noswap" : "swap");
		Renderer->EndRender(*this, endRenderFlags);
		XOTRACE_LATENCY("EndRender (end)\n");
	}
}

void xoSysWnd::SurfaceLost()
{
	if (Renderer)
		Renderer->SurfaceLost();
}

void xoSysWnd::SetPosition(xoBox box, uint setPosFlags)
{
#if XO_PLATFORM_WIN_DESKTOP
	uint wflags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
	if (!!(setPosFlags & SetPosition_Move)) wflags = wflags & ~SWP_NOMOVE;
	if (!!(setPosFlags & SetPosition_Size)) wflags = wflags & ~SWP_NOSIZE;
	SetWindowPos(SysWnd, NULL, box.Left, box.Top, box.Width(), box.Height(), wflags);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
	XOTRACE("xoSysWnd.SetPosition is not implemented\n");
#else
	XOTODO_STATIC
#endif
}

xoBox xoSysWnd::GetRelativeClientRect()
{
#if XO_PLATFORM_WIN_DESKTOP
	RECT r;
	POINT p0 = {0,0};
	ClientToScreen(SysWnd, &p0);
	GetClientRect(SysWnd, &r);
	xoBox box = r;
	box.Offset(p0.x, p0.y);
	return box;
#elif XO_PLATFORM_ANDROID
	return RelativeClientRect;
#elif XO_PLATFORM_LINUX_DESKTOP
	XWindowAttributes wa;
	XGetWindowAttributes(XDisplay, XWindow, &wa);
	return xoBox(wa.x, wa.y, wa.x + wa.width, wa.y + wa.height);
#else
	XOTODO_STATIC
#endif
}

void xoSysWnd::PostCursorChangedMessage()
{
#if XO_PLATFORM_WIN_DESKTOP
	PostMessage(SysWnd, WM_XO_CURSOR_CHANGED, 0, 0);
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
	// See this thread for help on injecting a dummy message into the X queue
	// http://stackoverflow.com/questions/8592292/how-to-quit-the-blocking-of-xlibs-xnextevent
#else
	XOTODO_STATIC
#endif
}

bool xoSysWnd::InitializeRenderer()
{
#if XO_PLATFORM_WIN_DESKTOP
	if (xoGlobal()->PreferOpenGL)
	{
		if (InitializeRenderer_Any<xoRenderGL>(Renderer))
			return true;
		if (InitializeRenderer_Any<xoRenderDX>(Renderer))
			return true;
	}
	else
	{
		if (InitializeRenderer_Any<xoRenderDX>(Renderer))
			return true;
		if (InitializeRenderer_Any<xoRenderGL>(Renderer))
			return true;
	}
	return false;
#else
	InitializeRenderer_Any<xoRenderGL>(Renderer);
#endif
}

template<typename TRenderer>
bool xoSysWnd::InitializeRenderer_Any(xoRenderBase*& renderer)
{
	renderer = new TRenderer();
	if (renderer->InitializeDevice(*this))
	{
		XOTRACE("Successfully initialized %s renderer\n", renderer->RendererName());
		return true;
	}
	else
	{
		XOTRACE("Failed to initialize %s renderer\n", renderer->RendererName());
		delete renderer;
		renderer = NULL;
		return false;
	}
}
