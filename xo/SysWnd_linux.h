#pragma once
#include "SysWnd.h"

#if XO_PLATFORM_LINUX_DESKTOP

#include <X11/Xlib.h> // X11 definitions are needed by SysWnd.h
#include <X11/Xutil.h>

// Remove ridiculous X11 defines
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Success
#undef Success
#endif
#ifdef Bool
#undef Bool
#endif

namespace xo {

namespace X11Constants {
enum {
	// These are copied from the macros defined inside the X11 headers. I hope they never change!
	None = 0,
	Success = 0,
};
}

class XO_API SysWndLinux : public SysWnd {
public:
	enum XClientMessages {
		XClientMessage_Repaint = 1,
	};

	Display*     XDisplay    = nullptr;
	int          XDisplay_FD = -1; // file descriptor from ConnectionNumber(XDisplay)
	Window       XWindowRoot;
	XVisualInfo* VisualInfo = nullptr;
	Colormap     ColorMap;
	Window       XWindow;
	GLXContext   GLContext = nullptr;
	XEvent       Event;
	int          EventLoopWakePipe[2]; // read,write ends of pipe used to wake message loop's select()

	SysWndLinux();
	~SysWndLinux() override;

	Error Create(uint32_t createFlags) override;
	Box   GetRelativeClientRect() override;
	void  PostCursorChangedMessage() override;
	void  PostRepaintMessage() override;
};
} // namespace xo
#endif