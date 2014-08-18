#include "pch.h"
#include "xoDoc.h"
#include "xoDocGroup.h"

LRESULT CALLBACK xoDocGroup::StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	xoDocGroup* proc = (xoDocGroup*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
	if ( proc == NULL && lParam != NULL && message == WM_NCCREATE )
	{
		// This is passed in via CreateWindow()
		// The one message that we unfortunately miss is WM_GETMINMAXINFO, which gets sent before WM_NCCREATE
		CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
		proc = (xoDocGroup*) cs->lpCreateParams;
		SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR) proc );
	}

	if ( proc && proc->Doc )
	{
		return proc->WndProc( hWnd, message, wParam, lParam );
	}
	else
	{
		// This path gets hit before WM_CREATE
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

/*
Windows peculiarities
---------------------

WM_NCLBUTTONDOWN:
If you receive a WM_NCLBUTTONDOWN, and call DefWindowProc, it will enter a modal loop. So your application-level message loop will not
get called until the user finishes sizing the window.
Since we run our renderer from the application's main message pump, we cease to render while the window is being resized.
Our solution: Whenever we receive WM_NCLBUTTONDOWN, start a timer. Stop that timer when DefWindowProc returns.
Since rendering happens on the main window message thread, we're not violating any thread model principle here.

*/

enum Timers
{
	TimerRenderOutsideMainMsgPump	= 1,
	TimerGenericEvent				= 2,
};

LRESULT xoDocGroup::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	XOASSERT( Doc != NULL );
	PAINTSTRUCT ps;
	HDC dc;
	xoEvent ev;
	ev.DocGroup = this;
	LRESULT result = 0;
	auto cursor = XOVEC2( (float) GET_X_LPARAM(lParam), (float) GET_Y_LPARAM(lParam) );

	switch (message)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		// TODO: Adjust this on the fly, using the minimum interval of all generic timer subscribers
		// Also.. find a better place to put this
		//SetTimer( hWnd, TimerGenericEvent, 15, NULL );
		//SetTimer( hWnd, TimerGenericEvent, 150, NULL );

		dc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_SIZE:
		ev.MakeWindowSize( int(lParam & 0xffff), int((lParam >> 16) & 0xffff) );
		xoGlobal()->EventQueue.Add( ev );
		break;

	case WM_TIMER:
		if ( wParam == TimerRenderOutsideMainMsgPump )
			Render();
		else if ( wParam == TimerGenericEvent )
		{
			ev.Type = xoEventTimer;
			xoGlobal()->EventQueue.Add( ev );
		}
		break;

	case WM_NCLBUTTONDOWN:
		// Explanation above titled 'WM_NCLBUTTONDOWN'
		SetTimer( hWnd, TimerRenderOutsideMainMsgPump, 1000 / xoGlobal()->TargetFPS, NULL );
		result = DefWindowProc(hWnd, message, wParam, lParam);
		KillTimer( hWnd, TimerRenderOutsideMainMsgPump );
		return result;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEMOVE:
		ev.Type = xoEventMouseMove;
		ev.PointCount = 1;
		ev.Points[0] = cursor;
		XOTRACE_LATENCY("MouseMove\n");
		xoGlobal()->EventQueue.Add( ev );
		break;

	case WM_LBUTTONUP:
		// Click event needs refinement (ie on down, capture, etc)
		ev.Type = xoEventClick;
		ev.PointCount = 1;
		XOTRACE_LATENCY("LButtonUp\n");
		ev.Points[0] = cursor;
		xoGlobal()->EventQueue.Add( ev );
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


