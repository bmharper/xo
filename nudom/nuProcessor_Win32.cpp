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

LRESULT nuProcessor::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	NUASSERT( Doc != NULL );
	PAINTSTRUCT ps;
	HDC dc;
	nuEvent ev;
	ev.Processor = this;

	switch (message)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		dc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		//CopyDocAndRenderNow();
		break;

	case WM_SIZE:
		ev.Type = nuEventWindowSize;
		ev.Points[0].x = float(lParam & 0xffff);
		ev.Points[0].y = float((lParam >> 16) & 0xffff);
		nuGlobal()->EventQueue.Add( ev );
		//Doc->InvalidateLayout();
		//CopyDocAndQueueRender();
		//CopyDocAndRenderNow();
		break;

	case WM_SIZING:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEMOVE:
		ev.Type = nuEventMouseMove;
		ev.PointCount = 1;
		ev.Points[0] = NUVEC2( LOWORD(lParam), HIWORD(lParam) );
		nuGlobal()->EventQueue.Add( ev );
		//BubbleEvent( ev );
		//CopyDocAndRenderNow();
		//CopyDocAndQueueRender();
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
