#pragma once
#include "xoDefs.h"
#include "xoEvent.h"

// The umbrella class that houses a DOM tree.
// This processes UI input.
// It schedules rendering.
// It coordinates between DOM and Render threads.
// TODO: Pull the platform-specific stuff (ie WndProc, StaticWndProc, IsMouseTracking) out of this class.
class XOAPI xoDocGroup
{
	DISALLOW_COPY_AND_ASSIGN(xoDocGroup);
public:
	xoDoc*				Doc;
	xoSysWnd*			Wnd;
	xoRenderDoc*		RenderDoc;
	bool				DestroyDocWithGroup;

	xoRenderStats		RenderStats;

					xoDocGroup();
					~xoDocGroup();

#if XO_PLATFORM_WIN_DESKTOP
	static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	LRESULT WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
#endif

	// These are the only 3 entry points into our content
	xoRenderResult	Render();							// This is always called from the Render thread
	xoRenderResult	RenderToImage( xoImage& image );	// This is always called from the Render thread
	void			ProcessEvent( xoEvent& ev );		// This is always called from the UI thread
	
	bool			IsDocVersionDifferentToRenderer() const;

protected:
	AbcCriticalSection	DocLock;		// Mutation of 'Doc', or cloning of 'Doc' for the renderer

#if XO_PLATFORM_WIN_DESKTOP
	bool				IsMouseTracking = false;	// True if we called TrackMouseEvent when we first saw a WM_MOUSEMOVE message, and are waiting for a WM_MOUSELEAVE event.
#endif

	xoRenderResult	RenderInternal( xoImage* targetImage );
};