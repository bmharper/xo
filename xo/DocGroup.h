#pragma once
#include "Defs.h"
#include "Event.h"

namespace xo {

// The umbrella class that houses a DOM tree, as well as its rendered representation.
// This is not a bunch of different documents. It is one document and all it's different representations.
// It might be better to come up with a new name for this concept.
// TODO: Pull the platform-specific stuff (ie WndProc, StaticWndProc, IsMouseTracking) out of this class.
class XO_API DocGroup {
	XO_DISALLOW_COPY_AND_ASSIGN(DocGroup);

public:
	Doc*       Doc; // Canonical Document, which the UI thread manipulates. Guarded by DocLock.
	SysWnd*    Wnd;
	RenderDoc* RenderDoc; // Copy of Canonical Document, as well as rendered state of document
	bool       DestroyDocWithGroup;
	RenderStats RenderStats;

	DocGroup();
	~DocGroup();

#if XO_PLATFORM_WIN_DESKTOP
	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT                 WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void                    SetSysWndTimer(uint32_t periodMS);
#endif

	// These are the only 3 entry points into our content
	RenderResult Render();                    // This is always called from the Render thread
	RenderResult RenderToImage(Image& image); // This is always called from the Render thread
	void         ProcessEvent(Event& ev);     // This is always called from the UI thread

	bool IsDirty() const;
	bool IsDocVersionDifferentToRenderer() const;

protected:
	std::mutex DocLock; // Mutation of 'Doc', or cloning of 'Doc' for the renderer

#if XO_PLATFORM_WIN_DESKTOP
	bool IsMouseTracking = false; // True if we called TrackMouseEvent when we first saw a WM_MOUSEMOVE message, and are waiting for a WM_MOUSELEAVE event.
#endif

	RenderResult RenderInternal(Image* targetImage);
	void         UploadImagesToGPU(bool& beganRender);
	uint32_t     DocAge() const;

	static void AddOrReplaceMessage(const OriginalEvent& ev);
};
}
