#include "pch.h"
#include "nuSysWnd.h"
#include "nuProcessor.h"
#include "nuDoc.h"
#include "Render/nuRenderGL.h"

void initGLExt();

static const char*		WClass = "nuDom";
static bool				GLExtInitialized = false;

#if NU_ANDROID
nuSysWnd*				MainWnd = NULL;
#endif

#if NU_WIN_DESKTOP
static HGLRC nuInitGL( HWND wnd, nuRenderGL* rgl )
{
	bool allGood = false;

	HGLRC rc = NULL;

	HDC dc = GetDC( wnd );
	if ( !dc ) return NULL;

	DWORD flags = 0;
	flags |= PFD_DRAW_TO_WINDOW;	// support window
	flags |= PFD_SUPPORT_OPENGL;	// support OpenGL 
	flags |= PFD_DOUBLEBUFFER;		// double buffer

	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		flags,
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // no alpha buffer 
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

	// get the best available match of pixel format for the device context  
	int iPixelFormat = ChoosePixelFormat( dc, &pfd );

	if ( iPixelFormat != 0 )
	{
		// make that the pixel format of the device context 
		SetPixelFormat(dc, iPixelFormat, &pfd); 
		rc = wglCreateContext( dc );
		wglMakeCurrent( dc, rc );
		if ( !GLExtInitialized )
		{
			GLExtInitialized = true;
			initGLExt();
			//biggleInit();
		}
		allGood = rgl->CreateShaders();
		wglMakeCurrent( NULL, NULL ); 
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
#if NU_WIN_DESKTOP
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;//CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= nuProcessor::StaticWndProc;
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
#if NU_WIN_DESKTOP
	SysWnd = NULL;
	DC = NULL;
#endif
#if NU_ANDROID
	MainWnd = this;
#endif
	Processor = new nuProcessor();
	Processor->Wnd = this;
	RGL = new nuRenderGL();
	DestroyDocWithWindow = false;
}

nuSysWnd::~nuSysWnd()
{
#if NU_WIN_DESKTOP
	if ( GLRC )
	{
		HDC dc = GetDC( SysWnd );
		wglMakeCurrent( dc, GLRC );
		RGL->DeleteShaders();
		wglMakeCurrent( NULL, NULL );
		ReleaseDC( SysWnd, dc );
	}
	DestroyWindow( SysWnd );
#endif
#if NU_ANDROID
	MainWnd = NULL;
#endif
	nuGlobal()->DocRemoveQueue.Add( Processor );
	Processor = NULL;
	delete RGL;
	RGL = NULL;
}

nuSysWnd* nuSysWnd::Create()
{
#if NU_WIN_DESKTOP
	bool ok = false;
	nuSysWnd* w = new nuSysWnd();
	w->SysWnd = CreateWindow( WClass, "nuDom", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	if ( w->SysWnd )
	{
		w->GLRC = nuInitGL( w->SysWnd, w->RGL );
		if ( w->GLRC )
		{
			SetWindowLongPtr( w->SysWnd, GWLP_USERDATA, (LONG_PTR) w->Processor );
			ok = true;
		}
	}
	if ( !ok )
	{
		delete w;
		w = NULL;
	}
	return w;
#endif
#if NU_ANDROID
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
	nuGlobal()->DocAddQueue.Add( w->Processor );
	return w;
}

void nuSysWnd::Show()
{
#if NU_WIN_DESKTOP
	ShowWindow( SysWnd, SW_SHOW );
#endif
}

nuDoc* nuSysWnd::Doc()
{
	return Processor->Doc;
}

void nuSysWnd::Attach( nuDoc* doc, bool destroyDocWithWindow )
{
	Processor->Doc = doc;
	DestroyDocWithWindow = destroyDocWithWindow;
}

bool nuSysWnd::BeginRender()
{
#if NU_WIN_DESKTOP
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
#if NU_WIN_DESKTOP
	SwapBuffers( DC );
	wglMakeCurrent( NULL, NULL );
	ReleaseDC( SysWnd, DC );
	DC = NULL;
#endif
}

void nuSysWnd::SurfaceLost()
{
	if ( RGL ) RGL->SurfaceLost();
}
