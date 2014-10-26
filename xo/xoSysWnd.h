#pragma once

#include "xoDefs.h"

/* A system window, or view.
*/
class XOAPI xoSysWnd
{
public:
	enum SetPositionFlags
	{
		SetPosition_Move = 1,
		SetPosition_Size = 2,
	};
#if XO_PLATFORM_WIN_DESKTOP
	HWND					SysWnd;
	bool					QuitAppWhenWindowDestroyed;		// This is here for multi-window applications. Close the first window, and the app exits.
#elif XO_PLATFORM_ANDROID
	xoBox					RelativeClientRect;		// Set by XoLib_init
#elif XO_PLATFORM_LINUX_DESKTOP
	Display*				XDisplay;
	Window					XWindowRoot;
	//GLint					att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	XVisualInfo*			VisualInfo;
	Colormap				ColorMap;
	Window					XWindow;
	GLXContext				GLContext;
	XEvent					Event;
#else
	XOTODO_STATIC;
#endif
	xoDocGroup*			DocGroup;
	xoRenderBase*		Renderer;

	xoSysWnd();
	~xoSysWnd();

	static xoSysWnd*	Create();
	static xoSysWnd*	CreateWithDoc();
	static void			PlatformInitialize();
	// Called by xoDocUI whenever it detects a change in cursor. We want to update the cursor now, instead of waiting for the next WM_SETCURSOR.
	// I'm not happy with this function here. We need to introduce a separate namespace for these type of platform abstractions.
	static void			SetSystemCursor( xoCursors cursor );	

	void	Attach( xoDoc* doc, bool destroyDocWithProcessor );
	void	Show();
	xoDoc*	Doc();
	bool	BeginRender();							// Basically wglMakeCurrent()
	void	EndRender( uint endRenderFlags );		// SwapBuffers followed by wglMakeCurrent(NULL). Flags are xoEndRenderFlags
	void	SurfaceLost();							// Surface lost, and now regained. Reinitialize GL state (textures, shaders, etc).
	void	SetPosition( xoBox box, uint setPosFlags );
	xoBox	GetRelativeClientRect();				// Returns the client rectangle (in screen coordinates), relative to the non-client window

protected:
	bool	InitializeRenderer();

	template<typename TRenderer>
	bool	InitializeRenderer_Any( xoRenderBase*& renderer );
};
