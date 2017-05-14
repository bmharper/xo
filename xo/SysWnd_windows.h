#pragma once
#include "SysWnd.h"

#if XO_PLATFORM_WIN_DESKTOP
namespace xo {

class XO_API SysWndWindows : public SysWnd {
public:
	enum WindowMessages {
		WM_XO_CURSOR_CHANGED = WM_USER,
	};

	HWND Wnd                        = NULL;
	bool QuitAppWhenWindowDestroyed = false; // This is here for multi-window applications. Close the first window, and the app exits.

	SysWndWindows();
	~SysWndWindows() override;

	void  PlatformInitialize() override;
	Error Create(uint32_t createFlags) override;
	void  Show() override;
	void  SetPosition(Box box, uint32_t setPosFlags) override;
	Box   GetRelativeClientRect() override;
	void  PostCursorChangedMessage() override;
	void  PostRepaintMessage() override;
	bool  CopySurfaceToImage(Box box, Image& img) override;
};

inline HWND GetHWND(xo::SysWnd& w) { return ((xo::SysWndWindows&) w).Wnd; }

} // namespace xo
#endif
