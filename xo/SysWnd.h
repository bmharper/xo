#pragma once
#include "Defs.h"

namespace xo {

// A system window, or view.
class XO_API SysWnd {
public:
	enum SetPositionFlags {
		SetPosition_Move = 1,
		SetPosition_Size = 2,
	};
	enum CreateFlags {
		CreateMinimizeButton = 1,
		CreateMaximizeButton = 2,
		CreateCloseButton    = 4,
		CreateBorder         = 8,
		CreateDefault        = CreateMinimizeButton | CreateMaximizeButton | CreateCloseButton | CreateBorder,
	};

	static int64_t NumWindowsCreated; // Incremented every time SysWnd constructor is called

	uint32_t                           TimerPeriodMS = 0;
	xo::DocGroup*                      DocGroup      = nullptr;
	RenderBase*                        Renderer      = nullptr;
	std::vector<std::function<void()>> OnWindowClose; // You can use this in a simple application to detect when the application is closing

	SysWnd();
	virtual ~SysWnd();

	static SysWnd* New(); // Just create a new SysWnd object, so that we can access it's virtual functions. Do not create the actual system window object.

	virtual void  PlatformInitialize();
	virtual Error Create(uint32_t createFlags = CreateDefault) = 0;
	virtual Box   GetRelativeClientRect()                      = 0; // Returns the client rectangle (in screen coordinates), relative to the non-client window
	virtual void  Show();
	virtual void  SetPosition(Box box, uint32_t setPosFlags);
	virtual void  PostCursorChangedMessage();
	virtual void  PostRepaintMessage();
	virtual bool  CopySurfaceToImage(Box box, Image& img);

	Error    CreateWithDoc(uint32_t createFlags = CreateDefault);
	void     Attach(Doc* doc, bool destroyDocWithProcessor);
	xo::Doc* Doc();
	bool     BeginRender();                      // Basically wglMakeCurrent()
	void     EndRender(uint32_t endRenderFlags); // SwapBuffers followed by wglMakeCurrent(NULL). Flags are EndRenderFlags
	void     SurfaceLost();                      // Surface lost, and now regained. Reinitialize GL state (textures, shaders, etc).

	// Invalid rectangle management
	void InvalidateRect(Box box);
	Box  GetInvalidateRect();
	void ValidateWindow();

protected:
	std::mutex InvalidRect_Lock;
	Box        InvalidRect;

	Error InitializeRenderer();

	template <typename TRenderer>
	Error InitializeRenderer_Any(RenderBase*& renderer);
};
} // namespace xo
