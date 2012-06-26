#pragma once
#include "nuDefs.h"

// Process UI input. Schedule rendering.
// Coordinates between DOM and Render threads.
class NUAPI nuProcessor
{
public:
	nuDoc*				Doc;
	nuSysWnd*			Wnd;
	nuRenderDoc*		RenderDoc;
	volatile uint32		MessagesSinceLastRender;

			nuProcessor();
			~nuProcessor();

#if NU_WIN_DESKTOP
	static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	LRESULT WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
#endif

	// These are our only two entry points into our content
	nuRenderResult	Render();							// This is always called from the Render thread
	void			ProcessEvent( nuEvent& ev );		// This is always called from the UI thread

protected:
	AbcCriticalSection	DocLock;

	bool	CopyDoc();
	void	CopyDocAndQueueRender();
	void	CopyDocAndRenderNow();
	void	FindTarget( const nuVec2& p, pvect<nuRenderDomEl*>& chain );
	void	BubbleEvent( nuEvent& ev );
};