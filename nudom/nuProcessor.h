#pragma once
#include "nuDefs.h"

// Process UI input. Schedule rendering.
class NUAPI nuProcessor
{
public:
	nuDoc*				Doc;
	nuSysWnd*			Wnd;
	nuRenderDoc*		RenderDoc;

			nuProcessor();
			~nuProcessor();

#if NU_WIN_DESKTOP
	static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	LRESULT WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
#endif

	void	Render();

	void	BubbleEvent( nuEvent& ev );

protected:

	bool	CopyDoc();
	void	CopyDocAndQueueRender();
	void	CopyDocAndRenderNow();
	void	FindTarget( const nuVec2& p, pvect<nuRenderDomEl*>& chain );
};