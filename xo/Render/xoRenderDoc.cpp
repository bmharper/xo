#include "pch.h"
#include "../Layout/xoLayout.h"
#include "../Layout/xoLayout2.h"
#include "../Layout/xoLayout3.h"
#include "xoRenderDoc.h"
#include "xoRenderer.h"
#include "xoRenderDX.h"
#include "xoRenderGL.h"

xoLayoutResult::xoLayoutResult(const xoDoc& doc)
{
	IsLocked = false;
	Root.SetPool(&Pool);
	Root.InternalID = doc.Root.GetInternalID();
}

xoLayoutResult::~xoLayoutResult()
{
}

const xoRenderDomNode* xoLayoutResult::Body() const
{
	if (Root.Children.size() == 0)
		return nullptr;
	XOASSERT(Root.Children[0]->IsNode());
	return static_cast<const xoRenderDomNode*>(Root.Children[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoRenderDoc::xoRenderDoc()
{
	AbcCriticalSectionInitialize(LayoutLock);
}

xoRenderDoc::~xoRenderDoc()
{
	for (u32 iter = 0; true; iter++)
	{
		TakeCriticalSection lock(LayoutLock);
		if (LatestLayout != nullptr && !LatestLayout->IsLocked)
		{
			delete LatestLayout;
			LatestLayout = nullptr;
		}
		PurgeOldLayouts();
		if (LatestLayout == nullptr && OldLayouts.size() == 0)
			break;
		if (iter % 500 == 0)
			XOTRACE("xoRenderDoc waiting for layouts to be released\n");
		AbcSleep(1);
	}
	AbcCriticalSectionDestroy(LayoutLock);
}

xoRenderResult xoRenderDoc::Render(xoRenderBase* driver)
{
	//XOTRACE_RENDER( "RenderDoc: Reset\n" );

	xoLayoutResult* layout = new xoLayoutResult(Doc);

	XOTRACE_RENDER("RenderDoc: Layout\n");
	xoLayout3 lay;
	lay.Layout(Doc, layout->Root, &layout->Pool);

	XOTRACE_RENDER("RenderDoc: Render\n");
	xoRenderer rend;
	xoRenderResult res = rend.Render(&Doc, &ClonedImages, &Doc.Strings, driver, &layout->Root);

	// Atomically publish the new layout
	{
		TakeCriticalSection lock(LayoutLock);
		PurgeOldLayouts();
		if (LatestLayout != nullptr)
		{
			if (LatestLayout->IsLocked)
				OldLayouts += LatestLayout;
			else
				delete LatestLayout;
		}
		LatestLayout = layout;
	}

	return res;
}

void xoRenderDoc::CopyFromCanonical(const xoDoc& canonical, xoRenderStats& stats)
{
	canonical.CloneSlowInto(Doc, 0, stats);

	// This must happen after textures are uploaded to the GPU. xoDocGroup ensures that.
	ClonedImages.CloneMetadataFrom(canonical.Images);
}

xoLayoutResult*	xoRenderDoc::AcquireLatestLayout()
{
	TakeCriticalSection lock(LayoutLock);

	if (LatestLayout == nullptr)
		return nullptr;

	XOASSERT(!LatestLayout->IsLocked);
	LatestLayout->IsLocked = true;

	return LatestLayout;
}

void xoRenderDoc::ReleaseLayout(xoLayoutResult* layout)
{
	if (layout == nullptr)
		return;
	TakeCriticalSection lock(LayoutLock);
	XOASSERT(layout->IsLocked);
	layout->IsLocked = false;
	PurgeOldLayouts();
}

// Warning: This assumes LayoutLock is already held
void xoRenderDoc::PurgeOldLayouts()
{
	for (intp i = OldLayouts.size() - 1; i >= 0; i--)
	{
		if (!OldLayouts[i]->IsLocked)
		{
			delete OldLayouts[i];
			OldLayouts.erase(i);
		}
	}
}
