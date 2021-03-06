#pragma once
#include "SysWnd.h"

#if XO_PLATFORM_WIN_DESKTOP
namespace xo {

class XO_API SysWndWindows : public SysWnd {
public:
	enum WindowMessages {
		WM_XO_CURSOR_CHANGED = WM_USER,
		WM_XO_SYSTRAY_ICON,
		WM_XO_TOUCHED_BY_OTHER_THREAD,
	};
	enum {
		SysTrayIconID = 1,
	};

	HWND Wnd                        = NULL;
	bool QuitAppWhenWindowDestroyed = false; // This is here for multi-window applications. Close the first window, and the app exits.
	bool HideWindowOnClose          = false; // On WM_CLOSE (ie top-right X button of window), do not close window, but hide instead. AddToSystemTray() sets this to true.
	bool HasSysTrayIcon             = false; // Set to true by AddToSystemTray()

	SysWndWindows();
	~SysWndWindows() override;

	void  PlatformInitialize(const InitParams* init) override;
	Error Create(uint32_t createFlags) override;
	void  Show() override;
	void  SetTitle(const char* title) override;
	void  SetPosition(Box box, uint32_t setPosFlags) override;
	Box   GetRelativeClientRect() override;
	void  PostCursorChangedMessage() override;
	void  PostRepaintMessage() override;
	bool  CopySurfaceToImage(Box box, Image& img) override;
	void  AddToSystemTray(const char* title, bool hideInsteadOfClose) override;
	void  ShowSystemTrayAlert(const char* msg) override;
};

inline HWND GetHWND(xo::SysWnd& w) { return ((xo::SysWndWindows&) w).Wnd; }

} // namespace xo
#endif
