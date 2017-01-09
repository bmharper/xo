#pragma once
#include "Defs.h"

namespace xo {

/* A system window, or view.
TODO: Get rid of the ifdefs, and move them out into separate platform-specific implementations.
*/
class XO_API SysWnd {
public:
	enum SetPositionFlags {
		SetPosition_Move = 1,
		SetPosition_Size = 2,
	};
	enum CreateFlags {
		CreateMinimizeButton = 1,
		CreateMaximizeButton = 2,
		CreateCloseButton    = 4,
		CreateBorder         = 8,
		CreateDefault        = CreateMinimizeButton | CreateMaximizeButton | CreateCloseButton | CreateBorder,
	};
#if XO_PLATFORM_WIN_DESKTOP
	HWND     Wnd;
	uint32_t TimerPeriodMS;
	bool     QuitAppWhenWindowDestroyed; // This is here for multi-window applications. Close the first window, and the app exits.
	enum WindowMessages {
		WM_XO_CURSOR_CHANGED = WM_USER,
	};
#elif XO_PLATFORM_ANDROID
	Box RelativeClientRect; // Set by XoLib_init
#elif XO_PLATFORM_LINUX_DESKTOP
	Display* XDisplay;
	Window   XWindowRoot;
	//GLint					att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	XVisualInfo* VisualInfo;
	Colormap     ColorMap;
	Window       XWindow;
	GLXContext   GLContext;
	XEvent       Event;
#else
	XO_TODO_STATIC;
#endif
	DocGroup*                          DocGroup;
	RenderBase*                        Renderer;
	std::vector<std::function<void()>> OnWindowClose; // You can use this in a simple application to detect when the application is closing

	SysWnd();
	~SysWnd();

	static SysWnd* Create(uint32_t createFlags = CreateDefault);
	static SysWnd* CreateWithDoc(uint32_t createFlags = CreateDefault);
	static void    PlatformInitialize();

	void Attach(Doc* doc, bool destroyDocWithProcessor);
	void Show();
	Doc* Doc();
	bool BeginRender();                      // Basically wglMakeCurrent()
	void EndRender(uint32_t endRenderFlags); // SwapBuffers followed by wglMakeCurrent(NULL). Flags are EndRenderFlags
	void SurfaceLost();                      // Surface lost, and now regained. Reinitialize GL state (textures, shaders, etc).
	void SetPosition(Box box, uint32_t setPosFlags);
	Box  GetRelativeClientRect(); // Returns the client rectangle (in screen coordinates), relative to the non-client window
	void PostCursorChangedMessage();
	void PostRepaintMessage();

	// Invalid rectangle management
	void InvalidateRect(Box box);
	Box  GetInvalidateRect();
	void ValidateWindow();

protected:
	std::mutex InvalidRect_Lock;
	Box        InvalidRect;

	bool InitializeRenderer();

	template <typename TRenderer>
	bool InitializeRenderer_Any(RenderBase*& renderer);
};
}
