#include "pch.h"
#include "nuSysWnd.h"
#include "nuDocGroup.h"
#include "nuDoc.h"
#include "Render/nuRenderGL.h"

void initGLExt();

static const char*		WClass = "nuDom";
static bool				GLIsBooted = false;

#if NU_PLATFORM_ANDROID
nuSysWnd*				MainWnd = NULL;
#endif

#if NU_PLATFORM_WIN_DESKTOP

typedef BOOL (*_wglChoosePixelFormatARB) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

static void nuBootGL_FillPFD( PIXELFORMATDESCRIPTOR& pfd )
{
	const DWORD flags = 0
		| PFD_DRAW_TO_WINDOW	// support window
		| PFD_SUPPORT_OPENGL	// support OpenGL 
		| PFD_DOUBLEBUFFER		// double buffer
		| 0;

	// Note that this must match the attribs used by wglChoosePixelFormatARB (I find that strange).
	PIXELFORMATDESCRIPTOR base = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		flags,
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // alpha bits
		0,                     // shift bit ignored 
		0,                     // no accumulation buffer 
		0, 0, 0, 0,            // accum bits ignored 
		16,                    // z-buffer 
		0,                     // no stencil buffer 
		0,                     // no auxiliary buffer 
		PFD_MAIN_PLANE,        // main layer 
		0,                     // reserved 
		0, 0, 0                // layer masks ignored 
	};

	pfd = base;
}

static bool nuBootGL( HWND wnd )
{
	HDC dc = GetDC( wnd );

	// get the best available match of pixel format for the device context  
	PIXELFORMATDESCRIPTOR pfd;
	nuBootGL_FillPFD( pfd );
	int iPixelFormat = ChoosePixelFormat( dc, &pfd );

	if ( iPixelFormat != 0 )
	{
		// make that the pixel format of the device context 
		BOOL setOK = SetPixelFormat(dc, iPixelFormat, &pfd); 
		HGLRC rc = wglCreateContext( dc );
		wglMakeCurrent( dc, rc );
		initGLExt();
		GLIsBooted = true;
		//biggleInit();
		wglMakeCurrent( NULL, NULL ); 
		wglDeleteContext( rc );
	}

	ReleaseDC( wnd, dc );

	return true;
}

static HGLRC nuInitGL( HWND wnd, nuRenderGL* rgl )
{
	if ( !GLIsBooted )
	{
		if ( !nuBootGL( wnd ) )
			return NULL;
	}

	bool allGood = false;
	HGLRC rc = NULL;
	HDC dc = GetDC( wnd );
	if ( !dc ) return NULL;

	int attribs[] =
	{
		WGL_DRAW_TO_WINDOW_ARB,		1,
		WGL_SUPPORT_OPENGL_ARB,		1,
		WGL_DOUBLE_BUFFER_ARB,		1,
		WGL_PIXEL_TYPE_ARB,			WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,			24,
		WGL_ALPHA_BITS_ARB,			0,
		WGL_DEPTH_BITS_ARB,			16,
		WGL_STENCIL_BITS_ARB,		0,
		WGL_SWAP_METHOD_ARB,		WGL_SWAP_EXCHANGE_ARB,	// This was an attempt to lower latency on Windows 8.0, but it seems to have no effect
		0
	};
	PIXELFORMATDESCRIPTOR pfd;
	nuBootGL_FillPFD( pfd );
	int formats[20];
	uint numformats = 0;
	BOOL chooseOK = wglChoosePixelFormatARB( dc, attribs, NULL, arraysize(formats), formats, &numformats );
	if ( chooseOK && numformats != 0 )
	{
		if ( SetPixelFormat( dc, formats[0], &pfd ) )
		{
			rc = wglCreateContext( dc );
			wglMakeCurrent( dc, rc );
			allGood = rgl->CreateShaders();
			wglMakeCurrent( NULL, NULL );
		}
		else
		{
			NUTRACE( "SetPixelFormat failed: %d\n", GetLastError() );
		}
	}

	ReleaseDC( wnd, dc );

	if ( !allGood )
	{
		if ( rc ) wglDeleteContext( rc );
		rc = NULL;
	}

	return rc;
}
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
#endif
}

nuSysWnd::nuSysWnd()
{
#if NU_PLATFORM_WIN_DESKTOP
	SysWnd = NULL;
	DC = NULL;
#elif NU_PLATFORM_ANDROID
	MainWnd = this;
#endif
	DocGroup = new nuDocGroup();
	DocGroup->Wnd = this;
	RGL = new nuRenderGL();
}

nuSysWnd::~nuSysWnd()
{
#if NU_PLATFORM_WIN_DESKTOP
	if ( GLRC )
	{
		HDC dc = GetDC( SysWnd );
		wglMakeCurrent( dc, GLRC );
		RGL->DeleteShadersAndTextures();
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( GLRC );
		ReleaseDC( SysWnd, dc );
		GLRC = NULL;
	}
	DestroyWindow( SysWnd );
#elif NU_PLATFORM_ANDROID
	MainWnd = NULL;
#endif
	nuGlobal()->DocRemoveQueue.Add( DocGroup );
	DocGroup = NULL;
	delete RGL;
	RGL = NULL;
}

nuSysWnd* nuSysWnd::Create()
{
#if NU_PLATFORM_WIN_DESKTOP
	bool ok = false;
	nuSysWnd* w = new nuSysWnd();
	w->SysWnd = CreateWindow( WClass, "nuDom", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	if ( w->SysWnd )
	{
		w->GLRC = nuInitGL( w->SysWnd, w->RGL );
		if ( w->GLRC )
		{
			SetWindowLongPtr( w->SysWnd, GWLP_USERDATA, (LONG_PTR) w->DocGroup );
			ok = true;
		}
	}
	if ( !ok )
	{
		delete w;
		w = NULL;
	}
	return w;
#elif NU_PLATFORM_ANDROID
	nuSysWnd* w = new nuSysWnd();
	w->RGL->CreateShaders();
	return w;
#endif
}

nuSysWnd* nuSysWnd::CreateWithDoc()
{
	nuSysWnd* w = Create();
	if ( !w ) return NULL;
	w->Attach( new nuDoc(), true );
	nuGlobal()->DocAddQueue.Add( w->DocGroup );
	return w;
}

void nuSysWnd::Show()
{
#if NU_PLATFORM_WIN_DESKTOP
	ShowWindow( SysWnd, SW_SHOW );
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
#if NU_PLATFORM_WIN_DESKTOP
	if ( GLRC )
	{
		DC = GetDC( SysWnd );
		if ( DC )
		{
			wglMakeCurrent( DC, GLRC );
			return true;
		}
	}
#endif
	return true;
}

void nuSysWnd::FinishRender()
{
#if NU_PLATFORM_WIN_DESKTOP
	NUTRACE_LATENCY( "SwapBuffers (begin)\n" );
	SwapBuffers( DC );
	wglMakeCurrent( NULL, NULL );
	ReleaseDC( SysWnd, DC );
	DC = NULL;
	NUTRACE_LATENCY( "SwapBuffers (done)\n" );
#endif
}

void nuSysWnd::SurfaceLost()
{
	if ( RGL ) RGL->SurfaceLost();
}

void nuSysWnd::SetPosition( nuBox box, uint setPosFlags )
{
#if NU_PLATFORM_WIN_DESKTOP
	uint wflags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
	if ( !!(setPosFlags & SetPosition_Move) ) wflags = wflags & ~SWP_NOMOVE;
	if ( !!(setPosFlags & SetPosition_Size) ) wflags = wflags & ~SWP_NOSIZE;
	SetWindowPos( SysWnd, NULL, box.Left, box.Top, box.Width(), box.Height(), wflags );
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
#else
	// TODO
	nuBox box(0,0,0,0);
	return box;
#endif
}
