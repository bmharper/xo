#include "pch.h"
#include "Defs.h"
#include "DocGroup_windows.h"
#include "SysWnd.h"
#include "Event.h"
#include "Doc.h"
#include "MsgLoop.h"

namespace xo {

static void SetupTimerMessagesForAllDocs() {
	for (DocGroup* dg : Global()->Docs)
		((DocGroupWindows*) dg)->SetSysWndTimer(dg->Doc->FastestTimerMS());
}

XO_API void RunMessageLoop() {
	while (true) {
		SetupTimerMessagesForAllDocs();

		// Dispatch messages until queue is empty. This should work, because a message
		// dispatch is not actually running much code. It only generates our own
		// platform-agnostic event and places that event in our own queue. The actual
		// handling of those events occur on our UI thread. When the UI thread
		// changes our document state, so that it needs to be rendered, then we invalidate
		// our window using InvalidateRect, and we rely on a WM_PAINT message to wake
		// us up from the GetMessage() sleep. We don't really need any of the windows
		// WM_PAINT/invalidation infrastructure. All we really need is a jolt that causes
		// GetMessage() to return control to us, so that we can go ahead and draw ourselves.
		bool mustQuit = false;
		while (true) {
			MSG msg;
			if (!GetMessage(&msg, NULL, 0, 0)) {
				mustQuit = true;
				break;
			}
			if (msg.message == WM_KEYDOWN)
				int abc = 123;
			XOTRACE_OS_MSG_QUEUE("msg start: %x\n", msg.message);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			XOTRACE_OS_MSG_QUEUE("msg end: %x. AnyDocsDirty() ? %s\n", msg.message, AnyDocsDirty() ? "yes" : "no");
			if (!PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
				break;
		}
		if (mustQuit)
			break;

		for (DocGroup* dg : Global()->Docs) {
			if (dg->IsDirty()) {
				XOTRACE_OS_MSG_QUEUE("Render enter (%p)\n", dg);
				RenderResult rr = dg->Render();
				if (rr == RenderResultNeedMore) {
					dg->Wnd->PostRepaintMessage();
				} else {
					dg->Wnd->ValidateWindow();
				}
			}
		}

		// Add/remove items from the global list of windows. This only happens at Doc creation/destruction time.
		AddOrRemoveDocsFromGlobalList();
	}

	//timeEndPeriod( 5 );
}

} // namespace xo
