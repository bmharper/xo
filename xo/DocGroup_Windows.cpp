#include "pch.h"
#include "Doc.h"
#include "DocGroup.h"
#include "SysWnd.h"

namespace xo {

// TODO: Why is this functionality inside DocGroup_Windows.
// Perhaps it should be inside SysWnd instead?

LRESULT CALLBACK DocGroup::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	DocGroup* proc = (DocGroup*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (proc == NULL && lParam != NULL && message == WM_NCCREATE) {
		// This is passed in via CreateWindow()
		// The one message that we unfortunately miss is WM_GETMINMAXINFO, which gets sent before WM_NCCREATE
		CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
		proc             = (DocGroup*) cs->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) proc);
	}

	if (proc && proc->Doc) {
		return proc->WndProc(hWnd, message, wParam, lParam);
	} else {
		// This path gets hit before WM_CREATE
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

/*
Windows peculiarities
---------------------

WM_NCLBUTTONDOWN:
If you receive a WM_NCLBUTTONDOWN, and call DefWindowProc, it will enter a modal loop while allowing
the user to resize your window. Your application-level message loop will not get called until
the user finishes sizing the window.
Since we run our renderer from the application's main message pump, we cease to render while the window
is being resized. Our solution: Whenever we receive WM_NCLBUTTONDOWN, start a timer. Stop that timer
when DefWindowProc returns. Since rendering anyway happens on the main window message thread, we're not
violating any thread model principle here.

*/

enum XoWindowsTimers {
	XoWindowsTimerRenderOutsideMainMsgPump = 1, // Used to force repaint events when window is being sized
	XoWindowsTimerGenericEvent             = 2, // A user event, such as when you bind to DomNode.OnTimer
};

static MouseButton WM_ButtonToXo(UINT message, WPARAM wParam) {
	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		return MouseButtonLeft;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		return MouseButtonRight;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		return MouseButtonMiddle;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		// If this assertion fails, then raise the enums above MouseButtonX4
		XO_DEBUG_ASSERT(GET_XBUTTON_WPARAM(wParam) < 4);
		return (MouseButton)(MouseButtonX1 + (GET_XBUTTON_WPARAM(wParam) - XBUTTON1));
	}
	return MouseButtonNull;
}

static Box ToBox(RECT r) { return Box(r.left, r.top, r.right, r.bottom); }

// This is called the moment DocUI detects a change in cursor. Because that DocUI processing
// happens in a different thread to the one that responds to Windows messages, it effectively
// delays the updating of the system cursor by at least one mouse-move message. This shortcut
// is here to remove that delay.
static void UpdateWindowsCursor(HWND wnd, Cursors cursor, WPARAM wParam, LPARAM lParam) {
	POINT pos;
	RECT  cr;
	GetCursorPos(&pos);
	ScreenToClient(wnd, &pos);
	GetClientRect(wnd, &cr);
	if (pos.x < cr.left || pos.x >= cr.right || pos.y < cr.top || pos.y >= cr.bottom) {
		if (wParam == 0 && lParam == 0) {
			// We are being called from WM_XO_CURSOR_CHANGED, so we can't call through to DefWindowProc.
			// It is safe to ignore this, because the previous WM_SETCURSOR would have taken care of this correctly.
		} else {
			DefWindowProc(wnd, WM_SETCURSOR, wParam, lParam);
		}
		return;
	}
	LPTSTR wc = IDC_ARROW;
	switch (cursor) {
	case CursorArrow: wc = IDC_ARROW; break;
	case CursorHand: wc  = IDC_HAND; break;
	case CursorText: wc  = IDC_IBEAM; break;
	case CursorWait: wc  = IDC_WAIT; break;
	}
	static_assert(CursorWait == 3, "Implement new cursor");
	SetCursor(LoadCursor(NULL, wc));
}

void DocGroup::SetSysWndTimer(uint32_t periodMS) {
	if (periodMS == Wnd->TimerPeriodMS)
		return;

	Wnd->TimerPeriodMS = periodMS;

	if (periodMS == 0) {
		XOTRACE_OS_MSG_QUEUE("KillTimer\n");
		KillTimer(Wnd->Wnd, XoWindowsTimerGenericEvent);
	} else {
		XOTRACE_OS_MSG_QUEUE("SetTimer(%u)\n", periodMS);
		SetTimer(Wnd->Wnd, XoWindowsTimerGenericEvent, periodMS, nullptr);
	}
}

LRESULT DocGroup::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	XO_ASSERT(Doc != NULL);
	RECT          rect;
	OriginalEvent ev;
	ev.DocGroup    = this;
	ev.Event.Doc   = Doc;
	LRESULT result = 0;
	auto    cursor = VEC2((float) GET_X_LPARAM(lParam), (float) GET_Y_LPARAM(lParam));

	// Remember that we are the Main Thread, so we do not own Doc.
	// Doc is owned and manipulated by the UI Thread.
	// We are only allowed to read niche volatile stuff from Doc, such as the latest cursor.

	switch (message) {
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		if (!!GetUpdateRect(hWnd, &rect, false)) {
			// We will paint from RunWin32MessageLoop.
			Wnd->InvalidateRect(ToBox(rect));
			ValidateRect(hWnd, &rect);
		}
		break;

	case WM_SIZE:
		ev.Event.MakeWindowSize(int(lParam & 0xffff), int((lParam >> 16) & 0xffff));
		Global()->UIEventQueue.Add(ev);
		break;

	case WM_TIMER:
		if (wParam == XoWindowsTimerRenderOutsideMainMsgPump) {
			Render();
		} else if (wParam == XoWindowsTimerGenericEvent) {
			ev.Event.Type = EventTimer;
			AddOrReplaceMessage(ev);
		}
		break;

	case WM_NCLBUTTONDOWN:
		// Explanation above titled 'WM_NCLBUTTONDOWN'
		SetTimer(hWnd, XoWindowsTimerRenderOutsideMainMsgPump, (uint32_t)(1000.0 / Global()->TargetFPS), NULL);
		result = DefWindowProc(hWnd, message, wParam, lParam);
		KillTimer(hWnd, XoWindowsTimerRenderOutsideMainMsgPump);
		return result;

	case WM_DESTROY:
		if (Wnd->QuitAppWhenWindowDestroyed)
			PostQuitMessage(0);
		break;

	case WM_SETCURSOR:
		// Note that this is always at least one mouse move message behind, because we only
		// update the Doc->UI.Cursor on WM_MOUSEMOVE, which is sent after WM_SETCURSOR.
		// We COULD process this message inside this thread, but that would mean that we
		// process mouse move messages twice - once in the UI thread and once in the main thread.
		// If necessary for latency sake, just do it, because it's not too expensive to
		// do a hit-test in the main thread. I have simply omitted doing it in the name
		// of simplicity.
		UpdateWindowsCursor(hWnd, Doc->UI.GetCursor(), wParam, lParam);
		break;

	case SysWnd::WM_XO_CURSOR_CHANGED:
		// The UI thread will post this message to us after it has processed a mouse move
		// message, and detected that the cursor changed as a result of that.
		UpdateWindowsCursor(hWnd, Doc->UI.GetCursor(), 0, 0);
		break;

	case WM_MOUSEMOVE:
		if (!IsMouseTracking) {
			TRACKMOUSEEVENT tme = {0};
			tme.cbSize          = sizeof(tme);
			tme.dwFlags         = TME_LEAVE;
			tme.hwndTrack       = hWnd;
			TrackMouseEvent(&tme);
			IsMouseTracking = true;
			// We don't send EventMouseEnter from here. It is the DocUI's job to synthesize that message
			// on a per-DOM-node basis. It determines this when it receives mousemove messages.
		}
		ev.Event.Type         = EventMouseMove;
		ev.Event.PointCount   = 1;
		ev.Event.PointsAbs[0] = cursor;
		AddOrReplaceMessage(ev);
		XOTRACE_LATENCY("MouseMove\n");
		break;

	case WM_MOUSELEAVE:
		IsMouseTracking       = false;
		ev.Event.Type         = EventMouseLeave;
		ev.Event.PointCount   = 1;
		ev.Event.PointsAbs[0] = cursor;
		Global()->UIEventQueue.Add(ev);
		break;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:
		XOTRACE_LATENCY("ButtonDown\n");
		ev.Event.Type         = EventMouseDown;
		ev.Event.Button       = WM_ButtonToXo(message, wParam);
		ev.Event.PointCount   = 1;
		ev.Event.PointsAbs[0] = cursor;
		Global()->UIEventQueue.Add(ev);
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
		XOTRACE_LATENCY("ButtonUp\n");
		ev.Event.Type         = EventMouseUp;
		ev.Event.Button       = WM_ButtonToXo(message, wParam);
		ev.Event.PointCount   = 1;
		ev.Event.PointsAbs[0] = cursor;
		Global()->UIEventQueue.Add(ev);
		// Click event needs refinement (ie on down, capture, etc)
		ev.Event.Type = EventClick;
		Global()->UIEventQueue.Add(ev);
		break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		XOTRACE_LATENCY("ButtonDblClick\n");
		ev.Event.Type         = EventDblClick;
		ev.Event.Button       = WM_ButtonToXo(message, wParam);
		ev.Event.PointCount   = 1;
		ev.Event.PointsAbs[0] = cursor;
		Global()->UIEventQueue.Add(ev);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
}
