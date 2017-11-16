#include "pch.h"
#include "SysWnd_linux.h"
#include "DocGroup.h"
#include "Render/RenderGL.h"
#include "Render/RenderDX.h"

namespace xo {

SysWndLinux::SysWndLinux() {
	EventLoopWakePipe[0] = 0;
	EventLoopWakePipe[1] = 0;
}

SysWndLinux::~SysWndLinux() {
	if (XDisplay) {
		glXMakeCurrent(XDisplay, None, nullptr);
		glXDestroyContext(XDisplay, GLContext);
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
	}
	XDisplay    = nullptr;
	XDisplay_FD = -1;
	if (EventLoopWakePipe[0]) {
		close(EventLoopWakePipe[0]);
		close(EventLoopWakePipe[1]);
		EventLoopWakePipe[0] = 0;
		EventLoopWakePipe[1] = 0;
	}
}

Error SysWndLinux::Create(uint32_t createFlags) {
	GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};
	XDisplay    = XOpenDisplay(nullptr);
	if (XDisplay == nullptr)
		return Error("Cannot connect to X server");
	XWindowRoot = DefaultRootWindow(XDisplay);
	VisualInfo  = glXChooseVisual(XDisplay, 0, att);
	if (VisualInfo == nullptr) {
		XCloseDisplay(XDisplay);
		XDisplay = nullptr;
		return Error("no appropriate XVisual found");
	}
	pipe2(EventLoopWakePipe, O_NONBLOCK);
	XDisplay_FD = ConnectionNumber(XDisplay);
	Trace("visual %p selected\n", (void*) VisualInfo->visualid);
	ColorMap = XCreateColormap(XDisplay, XWindowRoot, VisualInfo->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = ColorMap;
	swa.event_mask =
	    ExposureMask |
	    VisibilityChangeMask |
	    KeyPressMask |
	    PointerMotionMask |
	    ButtonMotionMask |
	    Button1MotionMask |
	    Button2MotionMask |
	    Button3MotionMask |
	    Button4MotionMask |
	    Button5MotionMask |
	    ButtonPressMask |
	    ButtonReleaseMask |
	    EnterWindowMask |
	    LeaveWindowMask;
	XWindow = XCreateWindow(XDisplay, XWindowRoot, 0, 0, 600, 600, 0, VisualInfo->depth, InputOutput, VisualInfo->visual, CWColormap | CWEventMask, &swa);
	XMapWindow(XDisplay, XWindow);
	XStoreName(XDisplay, XWindow, "xo");
	GLContext = glXCreateContext(XDisplay, VisualInfo, NULL, GL_TRUE);
	glXMakeCurrent(XDisplay, XWindow, GLContext);
	auto err = InitializeRenderer();
	glXMakeCurrent(XDisplay, None, NULL);
	return err;
}

Box SysWndLinux::GetRelativeClientRect() {
	XWindowAttributes wa;
	XGetWindowAttributes(XDisplay, XWindow, &wa);
	return Box(wa.x, wa.y, wa.x + wa.width, wa.y + wa.height);
}

void SysWndLinux::SetTitle(const char* title) {
	XStoreName(XDisplay, XWindow, title);
}

void SysWndLinux::Show() {
	// I don't know exactly why this is necessary.
	// What we're doing here is we're forcing the select() statement in SelectDocsWithEventsInQueue()
	// to come up with a positive result for this window. Once the select() wakes and the OS tells it
	// that our Window has new events, then the code ends up calling into ProcessEventsForDoc(),
	// which in turn calls XPending() and XNextEvent(). Both XPending and XNextEvent then return a bunch
	// of messages that have been queued up for us, such as Expose. I don't understand why the select()
	// would fail when there are actually messages in the queue, but then again, it doesn't surprise me.
	// The origin of this failure may be related to the behaviour described at the top of MsgLoop_Linux.cpp,
	// where we describe why we use pipes to wake up our processor, instead of a custom message.
	write(EventLoopWakePipe[1], "r", 1);
	// This is also sufficient (ie send any new message to the window)
	//SendExampleClientMessage();
}

void SysWndLinux::SetPosition(Box box, uint32_t setPosFlags) {
	if (!!(setPosFlags & SetPositionFlags::SetPosition_Move) && !!(setPosFlags & SetPositionFlags::SetPosition_Size))
		XMoveResizeWindow(XDisplay, XWindow, box.Left, box.Top, box.Width(), box.Height());
	else if (!!(setPosFlags & SetPositionFlags::SetPosition_Size))
		XResizeWindow(XDisplay, XWindow, box.Width(), box.Height());
	else if (!!(setPosFlags & SetPositionFlags::SetPosition_Move))
		XMoveWindow(XDisplay, XWindow, box.Left, box.Top);
}

void SysWndLinux::PostCursorChangedMessage() {
}

void SysWndLinux::PostRepaintMessage() {
	// See comment at top of MsgLoop_linux.cpp, for why XClientMessage_Example doesn't work for this.
	// So we use our special wake pipe, which does work.
	write(EventLoopWakePipe[1], "r", 1);
}

// Example code for sending a user-defined message
// I keep this around because I have a strong feeling that it's going to be necessary to use this at some point,
// and it's a useful debugging tool.
void SysWndLinux::SendExampleClientMessage() {
	XClientMessageEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type      = ClientMessage;
	ev.window    = XWindow;
	ev.format    = 32;
	ev.data.l[0] = XClientMessage_Example;
	int r        = XSendEvent(XDisplay, XWindow, 0, 0, (XEvent*) &ev);
	XFlush(XDisplay);
}

} // namespace xo