#include "pch.h"
#include "nuSysWnd.h"
#include "nuDocGroup.h"
#include "nuDoc.h"
#include "Render/nuRenderGL.h"
#include "Render/nuRenderDX.h"

static const char*		WClass = "nuDom";

#if NU_PLATFORM_ANDROID
nuSysWnd*				SingleMainWnd = NULL;
#elif NU_PLATFORM_LINUX_DESKTOP
// TODO: multiple windows
nuSysWnd*				SingleMainWnd = NULL;
#endif

void nuSysWnd::PlatformInitialize()
{
#if NU_PLATFORM_WIN_DESKTOP
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;//CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= nuDocGroup::StaticWndProc;
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
#elif NU_PLATFORM_ANDROID
#elif NU_PLATFORM_LINUX_DESKTOP
#else
	NUTODO_STATIC
#endif
}

nuSysWnd::nuSysWnd()
{
#if NU_PLATFORM_WIN_DESKTOP
	SysWnd = NULL;
#elif NU_PLATFORM_ANDROID
	SingleMainWnd = this;
	RelativeClientRect = nuBox(0,0,0,0);
#elif NU_PLATFORM_LINUX_DESKTOP
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
	NUTODO_STATIC
#endif
	DocGroup = new nuDocGroup();
	DocGroup->Wnd = this;
	Renderer = NULL;
}

nuSysWnd::~nuSysWnd()
{
#if NU_PLATFORM_WIN_DESKTOP
	if ( Renderer )
	{
		Renderer->DestroyDevice( *this );
		delete Renderer;
		Renderer = NULL;
	}
	DestroyWindow( SysWnd );
#elif NU_PLATFORM_ANDROID
	SingleMainWnd = NULL;
#elif NU_PLATFORM_LINUX_DESKTOP
	if ( XDisplay != nullptr )
	{
		glXMakeCurrent( XDisplay, None, NULL );
		glXDestroyContext( XDisplay, GLContext );
		XDestroyWindow( XDisplay, XWindow );
		XCloseDisplay( XDisplay );
	}
	SingleMainWnd = NULL;
#else
	NUTODO_STATIC
#endif
	nuGlobal()->DocRemoveQueue.Add( DocGroup );
	DocGroup = NULL;
}

nuSysWnd* nuSysWnd::Create()
{
#if NU_PLATFORM_WIN_DESKTOP
	bool ok = false;
	nuSysWnd* w = new nuSysWnd();
	NUTRACE("DocGroup = %p\n", w->DocGroup);
	w->SysWnd = CreateWindow( WClass, "nuDom", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), w->DocGroup );
	if ( w->SysWnd )
	{
		if ( w->InitializeRenderer() )
			ok = true;
	}
	if ( !ok )
	{
		delete w;
		w = NULL;
	}
	return w;
#elif NU_PLATFORM_ANDROID
	nuSysWnd* w = new nuSysWnd();
	w->InitializeRenderer();
	return w;
#elif NU_PLATFORM_LINUX_DESKTOP
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
	nuSysWnd* w = new nuSysWnd();
	w->XDisplay = XOpenDisplay( NULL );
	if ( w->XDisplay == NULL ) { NUTRACE("Cannot connect to X server\n" ); delete w; return nullptr; }
	w->XWindowRoot = DefaultRootWindow( w->XDisplay );
	w->VisualInfo = glXChooseVisual( w->XDisplay, 0, att );
	if ( w->VisualInfo == NULL ) { NUTRACE("no appropriate visual found\n" ); delete w; return nullptr; }
	NUTRACE( "visual %p selected\n", (void*) w->VisualInfo->visualid );
	w->ColorMap = XCreateColormap( w->XDisplay, w->XWindowRoot, w->VisualInfo->visual, AllocNone );
	XSetWindowAttributes swa;
	swa.colormap = w->ColorMap;
	swa.event_mask = ExposureMask | KeyPressMask | PointerMotionMask;
	w->XWindow = XCreateWindow( w->XDisplay, w->XWindowRoot, 0, 0, 600, 600, 0, w->VisualInfo->depth, InputOutput, w->VisualInfo->visual, CWColormap | CWEventMask, &swa );
	XMapWindow( w->XDisplay, w->XWindow );
	XStoreName( w->XDisplay, w->XWindow, "nudom" );
	w->GLContext = glXCreateContext( w->XDisplay, w->VisualInfo, NULL, GL_TRUE );
	glXMakeCurrent( w->XDisplay, w->XWindow, w->GLContext );
 	w->InitializeRenderer();
	glXMakeCurrent( w->XDisplay, None, NULL );
	SingleMainWnd = w;
	return w;
#else
	NUTODO_STATIC
#endif
}

nuSysWnd* nuSysWnd::CreateWithDoc()
{
	nuSysWnd* w = Create();
	if ( !w )
		return NULL;
	w->Attach( new nuDoc(), true );
	nuGlobal()->DocAddQueue.Add( w->DocGroup );
	return w;
}

void nuSysWnd::Show()
{
#if NU_PLATFORM_WIN_DESKTOP
	ShowWindow( SysWnd, SW_SHOW );
#elif NU_PLATFORM_ANDROID
#elif NU_PLATFORM_LINUX_DESKTOP
#else
	NUTODO_STATIC
#endif
}

nuDoc* nuSysWnd::Doc()
{
	return DocGroup->Doc;
}

void nuSysWnd::Attach( nuDoc* doc, bool destroyDocWithGroup )
{
	DocGroup->Doc = doc;
	DocGroup->DestroyDocWithGroup = destroyDocWithGroup;
}

bool nuSysWnd::BeginRender()
{
	if ( Renderer )
		return Renderer->BeginRender( *this );
	else
		return false;
}

void nuSysWnd::EndRender()
{
	if ( Renderer )
		Renderer->EndRender( *this );
}

void nuSysWnd::SurfaceLost()
{
	if ( Renderer )
		Renderer->SurfaceLost();
}

void nuSysWnd::SetPosition( nuBox box, uint setPosFlags )
{
#if NU_PLATFORM_WIN_DESKTOP
	uint wflags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
	if ( !!(setPosFlags & SetPosition_Move) ) wflags = wflags & ~SWP_NOMOVE;
	if ( !!(setPosFlags & SetPosition_Size) ) wflags = wflags & ~SWP_NOSIZE;
	SetWindowPos( SysWnd, NULL, box.Left, box.Top, box.Width(), box.Height(), wflags );
#elif NU_PLATFORM_ANDROID
#elif NU_PLATFORM_LINUX_DESKTOP
	NUTRACE( "nuSysWnd.SetPosition is not implemented\n" );
#else
	NUTODO_STATIC
#endif
}

nuBox nuSysWnd::GetRelativeClientRect()
{
#if NU_PLATFORM_WIN_DESKTOP
	RECT r;
	POINT p0 = {0,0};
	ClientToScreen( SysWnd, &p0 );
	GetClientRect( SysWnd, &r );
	nuBox box = r;
	box.Offset( p0.x, p0.y );
	return box;
#elif NU_PLATFORM_ANDROID
	return RelativeClientRect;
#elif NU_PLATFORM_LINUX_DESKTOP
	XWindowAttributes wa;
	XGetWindowAttributes( XDisplay, XWindow, &wa );
	return nuBox( wa.x, wa.y, wa.x + wa.width, wa.y + wa.height );
#else
	NUTODO_STATIC
#endif
}

bool nuSysWnd::InitializeRenderer()
{
#if NU_PLATFORM_WIN_DESKTOP
	if ( nuGlobal()->PreferOpenGL )
	{
		if ( InitializeRenderer_Any<nuRenderGL>( Renderer ) )
			return true;
		if ( InitializeRenderer_Any<nuRenderDX>( Renderer ) )
			return true;
	}
	else
	{
		if ( InitializeRenderer_Any<nuRenderDX>( Renderer ) )
			return true;
		if ( InitializeRenderer_Any<nuRenderGL>( Renderer ) )
			return true;
	}
	return false;
#else
	InitializeRenderer_Any<nuRenderGL>( Renderer );
#endif
}

template<typename TRenderer>
bool nuSysWnd::InitializeRenderer_Any( nuRenderBase*& renderer )
{
	renderer = new TRenderer();
	if ( renderer->InitializeDevice( *this ) )
	{
		NUTRACE( "Successfully initialized %s renderer\n", renderer->RendererName() );
		return true;
	}
	else
	{
		delete renderer;
		renderer = NULL;
		return false;
	}
}
