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

static Button WM_MouseButtonToXo(UINT message, WPARAM wParam) {
	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		return Button::MouseLeft;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		return Button::MouseRight;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		return Button::MouseMiddle;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		// If this assertion fails, then raise the enums above MouseButtonX4
		XO_DEBUG_ASSERT(GET_XBUTTON_WPARAM(wParam) < 4);
		return (Button)((int) Button::MouseX1 + (GET_XBUTTON_WPARAM(wParam) - XBUTTON1));
	}
	return Button::Null;
}

static void WM_KeyButtonToXo(WPARAM wp, LPARAM lp, Button& btn, int& codepoint) {
	if (wp >= 0x30 && wp <= 0x39) {
		btn = (Button)((int) Button::Key0 + (wp - 0x30));
		return;
	}

	if (wp >= 0x41 && wp <= 0x5A) {
		btn = (Button)((int) Button::KeyA + (wp - 0x41));
		return;
	}

	switch (wp) {
	case VK_BACK: btn = Button::KeyBack; break;
	case VK_TAB:
		btn       = Button::KeyTab;
		codepoint = 9;
		break;
	case VK_SPACE:
		btn       = Button::KeySpace;
		codepoint = 32;
		break;
	case VK_ESCAPE:
		btn       = Button::KeyEscape;
		codepoint = 27;
		break;
	case VK_LEFT: btn   = Button::KeyLeft; break;
	case VK_RIGHT: btn  = Button::KeyRight; break;
	case VK_UP: btn     = Button::KeyUp; break;
	case VK_DOWN: btn   = Button::KeyDown; break;
	case VK_DELETE: btn = Button::KeyDelete; break;
	}
}

static bool GeneratesCharMsg(WPARAM wp, LPARAM lp) {
	switch (wp) {
	case VK_BACK:
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	case VK_DELETE:
		return false;
	default:
		return true;
	}
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
		ev.Event.Button       = WM_MouseButtonToXo(message, wParam);
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
		ev.Event.Button       = WM_MouseButtonToXo(message, wParam);
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
		ev.Event.Button       = WM_MouseButtonToXo(message, wParam);
		ev.Event.PointCount   = 1;
		ev.Event.PointsAbs[0] = cursor;
		Global()->UIEventQueue.Add(ev);
		break;

	case WM_KEYDOWN:
		XOTRACE_LATENCY("KeyDown %u\n", (uint32_t) wParam);
		ev.Event.Type = EventKeyDown;
		WM_KeyButtonToXo(wParam, lParam, ev.Event.Button, ev.Event.KeyChar);
		Global()->UIEventQueue.Add(ev);
		// Also add an EventKeyChar message, so that one doesn't need to deal with two messages
		// that distinguish between arbitrary different keys. For example, why does BACKSPACE
		// generate a WM_CHAR, but VK_DELETE only generates WM_KEYDOWN? That seems pretty arbitrary.
		// In order to avoid duplicate messages from WM_CHAR, we need to only send messages that
		// get ignored by WM_CHAR (aka TranslateMessage)
		if (!GeneratesCharMsg(wParam, lParam)) {
			ev.Event.Type = EventKeyChar;
			Global()->UIEventQueue.Add(ev);
		}
		break;

	case WM_KEYUP:
		XOTRACE_LATENCY("KeyUp %u\n", (uint32_t) wParam);
		ev.Event.Type = EventKeyUp;
		WM_KeyButtonToXo(wParam, lParam, ev.Event.Button, ev.Event.KeyChar);
		Global()->UIEventQueue.Add(ev);
		break;

	// Although the docs for WM_UNICHAR seem ideal for us (an ANSI window), WM_UNICHAR messages
	// don't seem to be sent by a stock Windows 10 install. It looks like it may have disappeared
	// when Windows7 arrived. Various reports on the internet seem to suggest that WM_UNICHAR is
	// not a reliable way of receiving keyboard characters.
	// So how do we receive characters greater than 65535? At present, the only way I'm able
	// to synthesize this is with a tool called "unicodeinput.exe". The registry change to
	// HKCU\Control Panel\Input Method\EnableHexNumpad doesn't work for me on Windows 10.
	case WM_CHAR:
		// TODO: Combine two WM_CHAR keys into one message, for high unicode values that don't
		// fit into a 16-bit number.
		// http://www.catch22.net/tuts/unicode-text-editing
		//Trace("KeyChar %u\n", (uint32_t) wParam);
		XOTRACE_LATENCY("KeyChar %u\n", (uint32_t) wParam);
		ev.Event.Type    = EventKeyChar;
		ev.Event.KeyChar = (int32_t) wParam;
		Global()->UIEventQueue.Add(ev);
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
}
