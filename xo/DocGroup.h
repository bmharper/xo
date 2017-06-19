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

	DocGroup();
	virtual ~DocGroup();

	// These are the only 3 entry points into our content
	RenderResult Render();                    // This is always called from the Render thread
	RenderResult RenderToImage(Image& image); // This is always called from the Render thread
	void         ProcessEvent(Event& ev);     // This is always called from the UI thread

	bool IsDirty() const;
	bool IsDocVersionDifferentToRenderer() const;

protected:
	std::mutex DocLock; // Mutation of 'Doc', or cloning of 'Doc' for the renderer

	RenderResult RenderInternal(Image* targetImage);
	void         UploadImagesToGPU(bool& beganRender);
	uint32_t     DocAge() const;

	static void AddOrReplaceMessage(const OriginalEvent& ev);
};
} // namespace xo
