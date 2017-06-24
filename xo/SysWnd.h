#pragma once
#include "Defs.h"

namespace xo {

// A system window, or view.
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
	enum Event {
		EvDestroy,            // Called from the destructor of SysWnd
		EvHideToSysTray,      // Window is being hidden, but application remains in the system tray
		EvSysTrayContextMenu, // Right click on system tray icon
	};

	static int64_t NumWindowsCreated; // Incremented every time SysWnd constructor is called

	uint32_t                                   TimerPeriodMS = 0;
	xo::DocGroup*                              DocGroup      = nullptr;
	RenderBase*                                Renderer      = nullptr;
	std::vector<std::function<void(Event ev)>> EventListeners; // Listen to window events

	SysWnd();
	virtual ~SysWnd();

	static SysWnd* New(); // Just create a new SysWnd object, so that we can access it's virtual functions. Do not create the actual system window object.

	virtual void  PlatformInitialize(const InitParams* init);
	virtual Error Create(uint32_t createFlags = CreateDefault) = 0;
	virtual Box   GetRelativeClientRect()                      = 0; // Returns the client rectangle (in screen coordinates), relative to the non-client window
	virtual void  SetTitle(const char* title);
	virtual void  Show();
	virtual void  SetPosition(Box box, uint32_t setPosFlags);
	virtual void  PostCursorChangedMessage();
	virtual void  PostRepaintMessage();
	virtual bool  CopySurfaceToImage(Box box, Image& img);

	// Add an icon to the system tray.
	// If hideInsteadOfClose is true, then when you click on "X" to close the window, just hide it instead.
	// Use EvSysTrayContextMenu to respond to a right click on the tray icon.
	virtual void AddToSystemTray(const char* title, bool hideInsteadOfClose);

	// Show an alert on the desktop. On Windows, this pops up in the bottom right of the screen, above the system tray.
	// You must call AddToSystemTray() first, or this has no effect.
	virtual void ShowSystemTrayAlert(const char* msg);

	Error    CreateWithDoc(uint32_t createFlags = CreateDefault);
	void     Attach(Doc* doc, bool destroyDocWithProcessor);
	xo::Doc* Doc();
	bool     BeginRender();                      // Basically wglMakeCurrent()
	void     EndRender(uint32_t endRenderFlags); // SwapBuffers followed by wglMakeCurrent(NULL). Flags are EndRenderFlags
	void     SurfaceLost();                      // Surface lost, and now regained. Reinitialize GL state (textures, shaders, etc).
	void     SendEvent(Event ev);

	// Invalid rectangle management
	void InvalidateRect(Box box);
	Box  GetInvalidateRect();
	void ValidateWindow();

protected:
	std::mutex InvalidRect_Lock;
	Box        InvalidRect;

	Error InitializeRenderer();

	template <typename TRenderer>
	Error InitializeRenderer_Any(RenderBase*& renderer);
};
} // namespace xo
