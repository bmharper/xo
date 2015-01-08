#pragma once

#include "../xoDoc.h"
#include "xoRenderDomEl.h"

// Output from layout
class XOAPI xoLayoutResult
{
public:
	xoLayoutResult(const xoDoc& doc);
	~xoLayoutResult();

	bool					IsLocked;		// True if we are being used by the UI thread to do things like hit-testing
	xoRenderDomNode			Root;			// This is a dummy node that is above Body. Use Body() to get the true root of the tree.
	xoPool					Pool;

	const xoRenderDomNode*	Body() const;	// This is the effective root of the DOM
};

/* Document used by renderer.
The 'Doc' member is a complete clone of the original document.
*/
class XOAPI xoRenderDoc
{
public:
	// Defining state
	xoDoc					Doc;

	xoRenderDoc();
	~xoRenderDoc();

	xoRenderResult	Render(xoRenderBase* driver);
	void			CopyFromCanonical(const xoDoc& canonical, xoRenderStats& stats);

	// Acquire the latest layout object. Call ReleaseLayout when you are done using it. Returns nullptr if no layouts exist.
	// Panics if the latest layout has already been acquired.
	xoLayoutResult*	AcquireLatestLayout();
	void			ReleaseLayout(xoLayoutResult* layout);

protected:
	// Cloned image metadata
	xoImageStore			ClonedImages;

	// Rendered state
	AbcCriticalSection		LayoutLock;				// This guards the pointers LayoutResult and OldLayouts (but not necessarily the content that is pointed to)
	xoLayoutResult*			LatestLayout = nullptr;	// Most recent layout performed
	pvect<xoLayoutResult*>	OldLayouts;				// Layouts there were busy being used by the UI thread while the rendering thread progressed onto doing another layout

	void			PurgeOldLayouts();

};
