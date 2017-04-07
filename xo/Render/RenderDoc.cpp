#include "pch.h"
#include "../Layout/Layout.h"
#include "RenderDoc.h"
#include "Renderer.h"
#include "RenderDX.h"
#include "RenderGL.h"

namespace xo {

LayoutResult::LayoutResult(const Doc& doc) {
	IsLocked = false;
	Root.SetPool(&Pool);
	Root.InternalID = doc.Root.GetInternalID();
}

LayoutResult::~LayoutResult() {
}

const RenderDomNode* LayoutResult::Body() const {
	if (Root.Children.size() == 0)
		return nullptr;
	XO_ASSERT(Root.Children[0]->IsNode());
	return static_cast<const RenderDomNode*>(Root.Children[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RenderDoc::RenderDoc(DocGroup* group) : Doc(group) {
}

RenderDoc::~RenderDoc() {
	for (uint32_t iter = 0; true; iter++) {
		std::lock_guard<std::mutex> lock(LayoutLock);
		if (LatestLayout != nullptr && !LatestLayout->IsLocked) {
			delete LatestLayout;
			LatestLayout = nullptr;
		}
		PurgeOldLayouts();
		if (LatestLayout == nullptr && OldLayouts.size() == 0)
			break;
		if (iter % 500 == 0)
			Trace("RenderDoc waiting for layouts to be released\n");
		SleepMS(1);
	}
}

RenderResult RenderDoc::Render(RenderBase* driver) {
	//XOTRACE_RENDER( "RenderDoc: Reset\n" );
	if (!HasExpandedClassVariables) {
		XOTRACE_RENDER("RenderDoc: Expand Class Variables\n");
		CodeTimer t;
		ExpandVerbatimClassVariables();
		TimeVariableBake = t.Measure();
	}

	LayoutResult* layout = new LayoutResult(Doc);

	XOTRACE_RENDER("RenderDoc: Layout\n");
	CodeTimer t;
	Layout    lay;
	lay.PerformLayout(Doc, layout->Root, &layout->Pool);
	TimeLayout = t.MeasureAndRestart();

	XOTRACE_RENDER("RenderDoc: Render\n");
	Renderer     rend;
	RenderResult res = rend.Render(&Doc, &ClonedImages, &Doc.GetSvgTable(), &VectorCache, &Doc.Strings, driver, &layout->Root);
	TimeRender       = t.MeasureAndRestart();

	layout->IDToNodeTable.resize(Doc.InternalIDSize());
	PopulateIDToNode(layout, &layout->Root);

	// Atomically publish the new layout
	{
		std::lock_guard<std::mutex> lock(LayoutLock);
		PurgeOldLayouts();
		if (LatestLayout != nullptr) {
			if (LatestLayout->IsLocked)
				OldLayouts += LatestLayout;
			else
				delete LatestLayout;
		}
		LatestLayout = layout;
	}
	TimePostRender = t.MeasureAndRestart();

	return res;
}

void RenderDoc::CopyFromCanonical(const xo::Doc& canonical, RenderStats& stats) {
	canonical.CloneSlowInto(Doc, 0, stats);

	// This must happen after textures are uploaded to the GPU. DocGroup ensures that.
	ClonedImages.CloneMetadataFrom(canonical.Images);

	HasExpandedClassVariables = false;
}

LayoutResult* RenderDoc::AcquireLatestLayout() {
	std::lock_guard<std::mutex> lock(LayoutLock);

	if (LatestLayout == nullptr)
		return nullptr;

	XO_ASSERT(!LatestLayout->IsLocked);
	LatestLayout->IsLocked = true;

	return LatestLayout;
}

void RenderDoc::ReleaseLayout(LayoutResult* layout) {
	if (layout == nullptr)
		return;
	std::lock_guard<std::mutex> lock(LayoutLock);
	XO_ASSERT(layout->IsLocked);
	layout->IsLocked = false;
	PurgeOldLayouts();
}

// Warning: This assumes LayoutLock is already held
void RenderDoc::PurgeOldLayouts() {
	for (size_t i = OldLayouts.size() - 1; i != -1; i--) {
		if (!OldLayouts[i]->IsLocked) {
			delete OldLayouts[i];
			OldLayouts.erase(i);
		}
	}
}

void RenderDoc::PopulateIDToNode(LayoutResult* res, RenderDomNode* node) {
	// populate 'node'
	res->IDToNodeTable[node->InternalID] = node;

	// recurse into children of 'node'
	for (size_t i = 0; i < node->Children.size(); i++) {
		RenderDomNode* cnode = node->Children[i]->ToNode();
		if (cnode)
			PopulateIDToNode(res, cnode);
	}
}

void RenderDoc::ExpandVerbatimClassVariables() {
	HasExpandedClassVariables = true;
	Doc.ClassStyles.ExpandVerbatimVariables(&Doc);
}
}
