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

void SysWndLinux::PostCursorChangedMessage() {
}

void SysWndLinux::PostRepaintMessage() {
	// See comment at top of MsgLoop_linux.cpp, for why XClientMessageEvent didn't work.
	/*
	XClientMessageEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type   = ClientMessage;
	ev.window = XWindow;
	ev.format = 32;
	ev.data.l[0] = XClientMessage_Repaint;
	int r = XSendEvent(XDisplay, XWindow, 0, 0, (XEvent*) &ev);
	XFlush(XDisplay);
	printf("posting repaint msg: %u (status %d)\n", nrepaint++, r);
	*/
	write(EventLoopWakePipe[1], "r", 1);
	//static uint32_t nrepaint = 0;
	//printf("posting repaint msg: %u\n", nrepaint++);
}

} // namespace xo