#pragma once
#include "xoDefs.h"

// The umbrella class that houses a DOM tree.
// This processes UI input.
// It schedules rendering.
// It coordinates between DOM and Render threads.
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

	xoRenderResult	RenderInternal( xoImage* targetImage );
	//void			FindTarget( const xoVec2f& p, pvect<xoRenderDomEl*>& chain );
	bool			BubbleEvent( xoEvent& ev );
};