#pragma once
#include "Defs.h"
#include "Event.h"

namespace xo {

// The umbrella class that houses a DOM tree, as well as its rendered representation.
// This is not a bunch of different documents. It is one document and all it's different representations.
// It might be better to come up with a new name for this concept.
class XO_API DocGroup {
	XO_DISALLOW_COPY_AND_ASSIGN(DocGroup);

public:
	xo::Doc*        Doc                 = nullptr; // Canonical Document, which the UI thread manipulates. Guarded by DocLock.
	SysWnd*         Wnd                 = nullptr;
	xo::RenderDoc*  RenderDoc           = nullptr; // Copy of Canonical Document, as well as rendered state of document
	bool            DestroyDocWithGroup = false;
	xo::RenderStats RenderStats;

	static DocGroup* New(); // Create a new platform-specific DocGroup object

	DocGroup();
	virtual ~DocGroup();

	// These are the only 3 entry points into our content
	RenderResult Render();                    // This is always called from the Render thread
	RenderResult RenderToImage(Image& image); // This is always called from the Render thread
	void         ProcessEvent(Event& ev);     // This is always called from the UI thread

	bool IsDirty() const;
	bool IsDocVersionDifferentToRenderer() const;

	// This is called by rx::Control when it receives an ObservableTouched() callback from a thread that is not our UI thread.
	// This is a paradigm that gets used whenever there are threads doing background work, and there are UI components
	// that show state that is altered by those threads.
	// To see this another way, what we're doing here is making the interaction between background threads and our DOM
	// very strict. A different approach would be to allow any thread to manipulate the DOM. However, this is fraught
	// will complications, and I can't imagine a world where you'd get that right without causing a world of pain
	// for your users. Instead, we have a very narrow communication path between background threads and our DOM.
	// Firstly, observables are allowed to be mutated by background threads, but it's up to the observable objects
	// to make sure that their observable state is safe to access from multiple threads. Secondly, when an observable
	// is modified, one must call Touch() on it, and this has a clear path up to the Controls that are monitoring
	// that observable, and those controls set themselves dirty, and end up calling this function. This function
	// then goes through the necessary motions to create an OS message that will wake our message loop, dispatch
	// the necessary event handler, and then re-render the world if necessary.
	void TouchedByOtherThread();

protected:
	std::mutex        DocLock; // Mutation of 'Doc', or cloning of 'Doc' for the renderer
	std::atomic<bool> IsTouchedByOtherThread;

	virtual void InternalTouchedByOtherThread() = 0;

	RenderResult RenderInternal(Image* targetImage);
	void         UploadImagesToGPU(bool& beganRender);
	uint32_t     DocAge() const;

	static void AddOrReplaceMessage(const OriginalEvent& ev);
};
} // namespace xo
