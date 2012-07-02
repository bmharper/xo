#pragma once

#include "nuDefs.h"

/* A system window, or view.
*/
class NUAPI nuSysWnd
{
public:
#if NU_WIN_DESKTOP
	HWND			SysWnd;
	HGLRC			GLRC;
#endif
	nuProcessor*	Processor;
	bool			DestroyDocWithWindow;
	nuRenderGL*		RGL;

	nuSysWnd();
	~nuSysWnd();

	static nuSysWnd*	Create();
	static nuSysWnd*	CreateWithDoc();
	static void			PlatformInitialize();

	void	Attach( nuDoc* doc, bool destroyDocWithWindow );
	void	Show();
	nuDoc*	Doc();
	bool	BeginRender();
	void	FinishRender();
	void	SurfaceLost();

protected:
#if NU_WIN_DESKTOP
	HDC		DC;
#endif
};
