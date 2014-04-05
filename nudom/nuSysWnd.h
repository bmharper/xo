#pragma once

#include "nuDefs.h"

/* A system window, or view.
*/
class NUAPI nuSysWnd
{
public:
	enum SetPositionFlags
	{
		SetPosition_Move = 1,
		SetPosition_Size = 2,
	};
#if NU_PLATFORM_WIN_DESKTOP
	HWND				SysWnd;
#elif NU_PLATFORM_ANDROID
	nuBox				RelativeClientRect;		// Set by NuLib_init
#endif
	nuDocGroup*			DocGroup;
	nuRenderBase*		Renderer;

	nuSysWnd();
	~nuSysWnd();

	static nuSysWnd*	Create();
	static nuSysWnd*	CreateWithDoc();
	static void			PlatformInitialize();

	void	Attach( nuDoc* doc, bool destroyDocWithProcessor );
	void	Show();
	nuDoc*	Doc();
	bool	BeginRender();				// Basically wglMakeCurrent()
	void	EndRender();				// SwapBuffers followed by wglMakeCurrent(NULL)
	void	SurfaceLost();				// Surface lost, and now regained. Reinitialize GL state (textures, shaders, etc).
	void	SetPosition( nuBox box, uint setPosFlags );
	nuBox	GetRelativeClientRect();	// Returns the client rectangle (in screen coordinates), relative to the non-client window

protected:
	bool	InitializeRenderer();

	template<typename TRenderer>
	bool	InitializeRenderer_Any( nuRenderBase*& renderer );
};
