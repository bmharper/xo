#pragma once
#include "DocGroup.h"

namespace xo {

// The umbrella class that houses a DOM tree, as well as its rendered representation.
// This is not a bunch of different documents. It is one document and all it's different representations.
// It might be better to come up with a new name for this concept.
class XO_API DocGroupWindows : public DocGroup {
public:
	DocGroupWindows();
	~DocGroupWindows() override;

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT                 WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void                    SetSysWndTimer(uint32_t periodMS);
	HWND                    GetHWND();

protected:
	bool IsMouseTracking = false; // True if we called TrackMouseEvent when we first saw a WM_MOUSEMOVE message, and are waiting for a WM_MOUSELEAVE event.
};
} // namespace xo
