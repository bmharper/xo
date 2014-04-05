#include "pch.h"
#include "nuSysWnd.h"
#include "nuDocGroup.h"
#include "nuDoc.h"
#include "Render/nuRenderGL.h"
#include "Render/nuRenderDX.h"

static const char*		WClass = "nuDom";

#if NU_PLATFORM_ANDROID
nuSysWnd*				MainWnd = NULL;
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
#else
	NUTODO_STATIC
#endif
}

nuSysWnd::nuSysWnd()
{
#if NU_PLATFORM_WIN_DESKTOP
	SysWnd = NULL;
#elif NU_PLATFORM_ANDROID
	MainWnd = this;
	RelativeClientRect = nuBox(0,0,0,0);
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
	MainWnd = NULL;
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
	if ( !renderer->InitializeDevice( *this ) )
	{
		delete renderer;
		renderer = NULL;
	}
	return renderer != NULL;
}
