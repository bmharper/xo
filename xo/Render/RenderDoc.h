#pragma once
#include "../Doc.h"
#include "RenderDomEl.h"

namespace xo {

// Output from layout
class XO_API LayoutResult {
public:
	LayoutResult(const Doc& doc);
	~LayoutResult();

	bool                     IsLocked; // True if we are being used by the UI thread to do things like hit-testing
	RenderDomNode            Root;     // This is a dummy node that is above Body. Use Body() to get the true root of the tree.
	Pool                     Pool;
	cheapvec<RenderDomNode*> IDToNodeTable; // Mapping from InternalID to Node. Use Node() function rather than this directly.

	const RenderDomNode* Body() const; // This is the effective root of the DOM

	// Returns null if invalid
	const RenderDomNode* Node(InternalID id) const {
		if ((size_t) id >= (size_t) IDToNodeTable.size())
			return nullptr;
		return IDToNodeTable[id];
	}
};

/* Document used by renderer.
The 'Doc' member is a complete clone of the original document.
*/
class XO_API RenderDoc {
public:
	Doc Doc; // Defining state

	// Timings of most recent render
	double TimeVariableBake = 0;
	double TimeLayout       = 0;
	double TimeRender       = 0;
	double TimePostRender   = 0;

	RenderDoc(DocGroup* group);
	~RenderDoc();

	RenderResult Render(RenderBase* driver);
	void         CopyFromCanonical(const xo::Doc& canonical, RenderStats& stats);

	// Acquire the latest layout object. Call ReleaseLayout when you are done using it. Returns nullptr if no layouts exist.
	// Panics if the latest layout has already been acquired.
	LayoutResult* AcquireLatestLayout();
	void          ReleaseLayout(LayoutResult* layout);

protected:
	// Cloned image metadata
	ImageStore ClonedImages;

	// Tracks whether style variables (eg $dark = #aaa) have been baked into the class styles yet
	// Variables on individual DOM element styles are baked in at final resolve time
	bool HasExpandedClassVariables = false;

	// Rendered state
	std::mutex              LayoutLock;             // This guards the pointers LayoutResult and OldLayouts (but not necessarily the content that is pointed to)
	LayoutResult*           LatestLayout = nullptr; // Most recent layout performed
	cheapvec<LayoutResult*> OldLayouts;             // Layouts there were busy being used by the UI thread while the rendering thread progressed onto doing another layout

	void PurgeOldLayouts();
	void PopulateIDToNode(LayoutResult* res, RenderDomNode* node);
	void ExpandVerbatimClassVariables(); // Expand and parse the value of style variables such as $dark-outline = #333
};
}
