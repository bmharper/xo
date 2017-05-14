#include "pch.h"
#include "Defs.h"
#include "DocGroup_linux.h"
#include "SysWnd_linux.h"
#include "Event.h"
#include "Doc.h"

/*

We use select() to wait for X11 input messages, the same way one uses GetMessage() on Windows.
We need to be able to wake that select() up, for example if we have a redraw pending. For example,
the UI thread has made some changes to a document, and we want to get that document onto the
screen. The document is marked as dirty, but the select() doesn't know it. The first thing that
I tried, was to use inject XClientMessageEvent into the event stream, thereby waking up the
select(). That works most of the time, but fairly often it doesn't work. I have no idea
what's going on there, but I suspect it's something to do with message queues and the
inherent asynchronous nature of the X11 system. What I resorted to instead, is to create an
additional pipe, which I also add to the select() list. In order to wake up the select(), I
write a single byte into that pipe. Whenever I wake up from the select(), I drain the pipe.
This seems to work without any flaws.

-- OLD NEWS --
NOTE: We're doing *something* wrong in the way that we treat SysWndLinux::PostRepaintMessage.
For some reason, we can sometimes end up with a stale image, because our select() doesn't
get triggered by our XClientMessage_Repaint.

You can trigger the behaviour by running KitchenSink, DoKeyEventBubble function, and banging
on the keys (like qwe) for a second or two, and then rapidly pressing [] in sequence.
You'll notice that the screen stays on [ for a second, and then shows ]. Somehow the repaint
message that got sent after the ] key doesn't cause the select() to wake up. A hack is
to turn down the select timeout, but that's obviously nasty business.
*/

namespace xo {

static cheapvec<DocGroupLinux*> SelectDocsWithEventsInQueue() {
	fd_set readFd;
	FD_ZERO(&readFd);

	int xf_max = 0;
	for (DocGroup* dg : Global()->Docs) {
		SysWndLinux* wnd = (SysWndLinux*) dg->Wnd;
		FD_SET(wnd->XDisplay_FD, &readFd);
		FD_SET(wnd->EventLoopWakePipe[0], &readFd);
		xf_max = std::max(xf_max, wnd->XDisplay_FD);
		xf_max = std::max(xf_max, wnd->EventLoopWakePipe[0]);
	}

	cheapvec<DocGroupLinux*> ready;

	timeval timeout = {2, 0}; // seconds, microseconds
	int     nsel    = select(xf_max + 1, &readFd, nullptr, nullptr, &timeout);
	//printf("select: %d\n", nsel);
	if (nsel == -1)
		return ready;

	for (DocGroup* dg : Global()->Docs) {
		SysWndLinux* wnd         = (SysWndLinux*) dg->Wnd;
		bool         hasXEvents  = FD_ISSET(wnd->XDisplay_FD, &readFd);
		bool         hasPipeData = FD_ISSET(wnd->EventLoopWakePipe[0], &readFd);
		if (hasXEvents || hasPipeData) {
			ready.push((DocGroupLinux*) dg);
			if (hasPipeData) {
				// drain the pipe, because we're going to be re-rendering this document now.
				char buf[64];
				while (read(wnd->EventLoopWakePipe[0], buf, sizeof(buf)) > 0) {
				}
			}
		}
	}
	return ready;
}

static void MapKeyToEvent(XKeyEvent& xkey, Event& ev, bool& dispatch, bool& isChar) {
	dispatch      = true;
	isChar        = false;
	char   str[8] = {0};
	KeySym keysym;
	int    len = XLookupString(&xkey, str, sizeof(str), &keysym, nullptr);
	//printf("keycode = %d, (len = %d), str[0] = '%c', str[0] = %d\n", xkey.keycode, len, str[0], str[0]);
	if (len != 1) {
		dispatch = false;
		return;
	}

	switch (str[0]) {
	case 8: ev.Button = Button::KeyBack; break;
	case 9: ev.Button = Button::KeyEscape; break;
	case 127: ev.Button = Button::KeyDelete; break;
	default:
		isChar = true;
	}
	ev.KeyChar = str[0];

	if (ev.KeyChar >= 'a' && ev.KeyChar <= 'z')
		ev.Button = (Button)((int) Button::KeyA + (ev.KeyChar - 'a'));

	if (ev.KeyChar >= 'A' && ev.KeyChar <= 'Z')
		ev.Button = (Button)((int) Button::KeyA + (ev.KeyChar - 'A'));

	if (ev.KeyChar >= '0' && ev.KeyChar <= '9')
		ev.Button = (Button)((int) Button::Key0 + (ev.KeyChar - '0'));
}

static void MapButton(XButtonEvent& xb, Event& ev) {
	//printf("x=%d y=%d x_root=%d y_root=%d state=%u button=%u\n", xb.x, xb.y, xb.x_root, xb.y_root, xb.state, xb.button);
	switch (xb.button) {
	case 1: ev.Button = xo::Button::MouseLeft; break;
	case 2: ev.Button = xo::Button::MouseMiddle; break;
	case 3: ev.Button = xo::Button::MouseRight; break;
	case 4: ev.Button = xo::Button::MouseWheelScrollUp; break;
	case 5: ev.Button = xo::Button::MouseWheelScrollDown; break;
	case 8: ev.Button = xo::Button::MouseX1; break;
	case 9: ev.Button = xo::Button::MouseX2; break;
	}
}

bool ProcessEventsForDoc(DocGroupLinux* dg) {
	SysWndLinux* wnd = (SysWndLinux*) dg->Wnd;

	while (XPending(wnd->XDisplay)) {
		XEvent xev;
		XNextEvent(wnd->XDisplay, &xev);

		OriginalEvent ev;
		ev.DocGroup  = dg;
		ev.Event.Doc = dg->Doc;

		// I don't know where this delta comes from in Ubuntu Unity.
		int cursorOffX = -1;
		int cursorOffY = -2;

		switch (xev.type) {
		case ClientMessage: {
			// This is no longer used. We use a pipe instead. See comment at top of file.
			int msg = xev.xclient.data.l[0];
			if (msg == SysWndLinux::XClientMessage_Repaint) {
				static uint32_t nrepaint = 0;
				//printf("received repaint %u\n", nrepaint++);
			}
			break;
		}
		case Expose: {
			//printf("ev: Expose %d %d %d %d %d\n", xev.xexpose.x, xev.xexpose.y, xev.xexpose.width, xev.xexpose.height, xev.xexpose.count);
			XWindowAttributes wa;
			XGetWindowAttributes(wnd->XDisplay, wnd->XWindow, &wa);
			//printf("x,y = %d,%d borderwidth: %d\n", wa.x, wa.y, wa.border_width);
			ev.Event.Type           = EventWindowSize;
			ev.Event.PointsAbs[0].x = wa.width;
			ev.Event.PointsAbs[0].y = wa.height;
			Global()->UIEventQueue.Add(ev);
			break;
		}
		case KeymapNotify:
			XRefreshKeyboardMapping(&xev.xmapping);
			break;
		case KeyPress:
		case KeyRelease: {
			//printf("ev: key %d\n", (int) xev.xkey.keycode);
			//XOTRACE_OS_MSG_QUEUE("key = %d\n", xev.xkey.keycode);
			//if (xev.xkey.keycode == 24) // 'q'
			//	return false;
			ev.Event.Type = xev.type == KeyPress ? EventKeyDown : EventKeyUp;
			bool dispatch, isChar;
			MapKeyToEvent(xev.xkey, ev.Event, dispatch, isChar);
			if (dispatch) {
				Global()->UIEventQueue.Add(ev);
				if (isChar && ev.Event.Type == EventKeyDown) {
					ev.Event.Type = EventKeyChar;
					Global()->UIEventQueue.Add(ev);
				}
			}
			break;
		}
		case MotionNotify:
			//printf("x,y = %d,%d\n", xev.xmotion.x, xev.xmotion.y);
			ev.Event.Type           = EventMouseMove;
			ev.Event.PointsAbs[0].x = xev.xmotion.x + cursorOffX;
			ev.Event.PointsAbs[0].y = xev.xmotion.y + cursorOffY;
			Global()->UIEventQueue.Add(ev);
			break;
		case ButtonPress:
		case ButtonRelease:
			ev.Event.Type           = xev.type == ButtonPress ? EventMouseDown : EventMouseUp;
			ev.Event.PointsAbs[0].x = xev.xbutton.x + cursorOffX;
			ev.Event.PointsAbs[0].y = xev.xbutton.y + cursorOffY;
			MapButton(xev.xbutton, ev.Event);
			Global()->UIEventQueue.Add(ev);
			break;
		}
	}

	return true;
}

XO_API void RunMessageLoop() {
	AddOrRemoveDocsFromGlobalList();

	while (1) {
		bool quit = false;
		for (DocGroupLinux* dg : SelectDocsWithEventsInQueue()) {
			if (!ProcessEventsForDoc(dg)) {
				quit = true;
				break;
			}
		}
		if (quit)
			break;

		for (DocGroup* dg : Global()->Docs) {
			if (dg->IsDirty()) {
				RenderResult rr = dg->Render();
				if (rr == RenderResultNeedMore) {
					dg->Wnd->PostRepaintMessage();
				} else {
					dg->Wnd->ValidateWindow();
				}
			}
		}

		AddOrRemoveDocsFromGlobalList();
	}
}

} // namespace xo
