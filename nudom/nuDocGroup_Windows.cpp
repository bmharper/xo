#include "pch.h"
#include "nuDoc.h"
#include "nuDocGroup.h"

LRESULT CALLBACK nuDocGroup::StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	nuDocGroup* proc = (nuDocGroup*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
	if ( proc == NULL && lParam != NULL && message == WM_NCCREATE )
	{
		// This is passed in via CreateWindow()
		// The one message that we unfortunately miss is WM_GETMINMAXINFO, which gets sent before WM_NCCREATE
		CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
		proc = (nuDocGroup*) cs->lpCreateParams;
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

LRESULT nuDocGroup::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	NUASSERT( Doc != NULL );
	PAINTSTRUCT ps;
	HDC dc;
	nuEvent ev;
	ev.DocGroup = this;
	LRESULT result = 0;

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
		nuGlobal()->EventQueue.Add( ev );
		break;

	case WM_TIMER:
		if ( wParam == TimerRenderOutsideMainMsgPump )
			Render();
		else if ( wParam == TimerGenericEvent )
		{
			ev.Type = nuEventTimer;
			nuGlobal()->EventQueue.Add( ev );
		}
		break;

	case WM_NCLBUTTONDOWN:
		// Explanation above titled 'WM_NCLBUTTONDOWN'
		SetTimer( hWnd, TimerRenderOutsideMainMsgPump, 1000 / nuGlobal()->TargetFPS, NULL );
		result = DefWindowProc(hWnd, message, wParam, lParam);
		KillTimer( hWnd, TimerRenderOutsideMainMsgPump );
		return result;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEMOVE:
		ev.Type = nuEventMouseMove;
		ev.PointCount = 1;
		ev.Points[0] = NUVEC2( LOWORD(lParam), HIWORD(lParam) );
		NUTRACE_LATENCY("MouseMove\n");
		nuGlobal()->EventQueue.Add( ev );
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


