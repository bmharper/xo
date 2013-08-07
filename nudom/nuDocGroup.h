#pragma once
#include "nuDefs.h"

// The umbrella class that houses a DOM tree.
// This processes UI input.
// It schedules rendering.
// It coordinates between DOM and Render threads.
class NUAPI nuDocGroup
{
public:
	nuDoc*				Doc;
	nuSysWnd*			Wnd;
	nuRenderDoc*		RenderDoc;
	bool				DestroyDocWithProcessor;

	nuRenderStats		RenderStats;

					nuDocGroup();
					~nuDocGroup();

#if NU_WIN_DESKTOP
	static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	LRESULT WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
#endif

	// These are our only two entry points into our content
	nuRenderResult	Render();							// This is always called from the Render thread
	void			ProcessEvent( nuEvent& ev );		// This is always called from the UI thread
	
	bool			IsDocNewerThanRenderer() const;

protected:
	AbcCriticalSection	DocLock;		// Mutation of 'Doc', or cloning of 'Doc' for the renderer

	void	FindTarget( const nuVec2& p, pvect<nuRenderDomEl*>& chain );
	bool	BubbleEvent( nuEvent& ev );
};