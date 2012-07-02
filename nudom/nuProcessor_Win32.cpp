#include "pch.h"
#include "nuDoc.h"
#include "nuProcessor.h"

LRESULT CALLBACK nuProcessor::StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	nuProcessor* proc = (nuProcessor*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
	if ( proc && proc->Doc )
	{
		return proc->WndProc( hWnd, message, wParam, lParam );
	}
	else
	{
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
	TimerRenderOutsideMainMsgPump = 1
};

LRESULT nuProcessor::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	NUASSERT( Doc != NULL );
	PAINTSTRUCT ps;
	HDC dc;
	nuEvent ev;
	ev.Processor = this;
	LRESULT result = 0;

	switch (message)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
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
		nuGlobal()->EventQueue.Add( ev );
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


