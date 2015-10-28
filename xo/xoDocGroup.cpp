#include "pch.h"
#include "xoDoc.h"
#include "xoDocGroup.h"
#include "Layout/xoLayout.h"
#include "Layout/xoLayout2.h"
#include "Layout/xoLayout3.h"
#include "xoSysWnd.h"
#include "Render/xoRenderer.h"
#include "Render/xoRenderDoc.h"
#include "Render/xoRenderDomEl.h"
#include "Render/xoRenderBase.h"
#include "Render/xoRenderGL.h"
#include "Render/xoStyleResolve.h"
#include "Image/xoImage.h"

xoDocGroup::xoDocGroup()
{
	AbcCriticalSectionInitialize(DocLock);
	DestroyDocWithGroup = false;
	Doc = NULL;
	Wnd = NULL;
	RenderDoc = new xoRenderDoc();
	RenderStats.Reset();
}

xoDocGroup::~xoDocGroup()
{
	delete RenderDoc;
	if (DestroyDocWithGroup)
		delete Doc;
	AbcCriticalSectionDestroy(DocLock);
}

xoRenderResult xoDocGroup::Render()
{
	return RenderInternal(nullptr);
}

xoRenderResult xoDocGroup::RenderToImage(xoImage& image)
{
	// The 10 here is an arbitrary thumbsuck. We'll see if we ever need a controllable limit.
	const int maxAttempts = 10;
	xoRenderResult res = xoRenderResultNeedMore;
	for (int attempt = 0; res == xoRenderResultNeedMore && attempt < maxAttempts; attempt++)
		res = RenderInternal(&image);
	return res;
}

/* This is always called from the Render thread
You might ask: Why do we copy from Doc to RenderDoc from here, running
in the Render thread? The only time that Doc is modified, is when an event
has been processed, and this always happens on the UI thread. Surely then,
immediately after processing an event, the UI thread should perform the
copy from Doc to RenderDoc? BUT - that leaves two problems.
Firstly, the Render thread would die, because you'd be modifying the RenderDoc
while the Render thread was busy using it.
Secondly, as part of the copy operation, we upload any altered textures
to the GPU. This is obviously a can of worms that we don't want to open
(ie multithreaded access to the GPU).
*/
xoRenderResult xoDocGroup::RenderInternal(xoImage* targetImage)
{
	bool wndDirty = Wnd->GetInvalidateRect().IsAreaPositive();
	bool haveLock = false;
	
	// If docAge = 0, then we do not need to make a new copy of Doc.
	// We merely need to run animations, or repaint our window.
	bool docModified = DocAge() >= 1;

	// I'm not quite sure how we should handle this. The idea is that you don't want to go without a UI update
	// for too long, even if the UI thread is taking its time, and being bombarded with messages.
	if (docModified || targetImage != NULL)
	{
		// If UI thread has performed even a single update since we last rendered, then pause our thread until we can gain the DocLock
		auto tstart = AbcTimeAccurateRTSeconds();
		AbcCriticalSectionEnter(DocLock);
		auto time = AbcTimeAccurateRTSeconds() - tstart;
		if (time > 0.001)
			XOTIME("xoDocGroup.RenderInternal took %d ms to acquire DocLock\n", (int) (time * 1000));
		haveLock = true;
	}

	// TODO: If AnyAnimationsRunning(), then we are not idle
	bool docValid = Doc->UI.GetViewportWidth() != 0 && Doc->UI.GetViewportHeight() != 0;
	bool beganRender = false;

	if (haveLock)
	{
		UploadImagesToGPU(beganRender);

		//XOTRACE( "Render Version %u\n", Doc->GetVersion() );
		RenderDoc->CopyFromCanonical(*Doc, RenderStats);

		// Assume we are the only renderer of 'Doc'. If this assumption were not true, then you would need to update
		// all renderers simultaneously, so that you can guarantee that UsableIDs all go to FreeIDs atomically.
		//XOTRACE( "MakeFreeIDsUsable\n" );
		Doc->MakeFreeIDsUsable();
		Doc->ResetModifiedBitmap();			// AbcBitMap has an absolutely awful implementation of this (byte-filled vs SSE or at least pointer-word-size-filled)

		AbcCriticalSectionLeave(DocLock);
	}

	xoRenderResult rendResult = xoRenderResultIdle;
	bool presentFrame = false;

	if (docValid && Wnd != nullptr)
	{
		//XOTIME( "Render start\n" );
		if (!beganRender && !Wnd->BeginRender())
		{
			XOTIME("BeginRender failed\n");
			return xoRenderResultNeedMore;
		}
		beganRender = true;

		//XOTIME( "Render DO\n" );
		rendResult = RenderDoc->Render(Wnd->Renderer);

		presentFrame = true;

		if (targetImage != nullptr)
			Wnd->Renderer->ReadBackbuffer(*targetImage);
	}

	if (beganRender)
	{
		// presentFrame will be false when the only action we've taken on the GPU is uploading textures.
		//XOTIME( "Render Finish\n" );
		Wnd->EndRender(presentFrame ? 0 : xoEndRenderNoSwap);
	}

	return rendResult;
}

void xoDocGroup::UploadImagesToGPU(bool& beganRender)
{
	beganRender = false;
	pvect<xoImage*> invalidImages = Doc->Images.InvalidList();
	if (invalidImages.size() != 0)
	{
		if (!Wnd->BeginRender())
			return;

		beganRender = true;

		for (intp i = 0; i < invalidImages.size(); i++)
		{
			if (Wnd->Renderer->LoadTexture(invalidImages[i], 0))
			{
				invalidImages[i]->TexClearInvalidRect();
			}
			else
			{
				XOTRACE_WARNING("Failed to upload image to GPU\n");
			}
		}
	}
}

uint32 xoDocGroup::DocAge() const
{
	return Doc->GetVersion() - RenderDoc->Doc.GetVersion();
}

struct AddOrReplaceMessage_Context
{
	const xoOriginalEvent*	NewEvent;
	bool					DidReplace;
};

static bool AddOrReplaceMessage_Scan(void* context, xoOriginalEvent* iter)
{
	AddOrReplaceMessage_Context* cx = (AddOrReplaceMessage_Context*) context;
	if (iter->DocGroup == cx->NewEvent->DocGroup && iter->Event.Type == cx->NewEvent->Event.Type)
	{
		// TODO: Keep a history of the replaced events, so that a program that needs to have
		// smooth cursor input can use all of the events that were sent by the OS. To do this, we'll
		// want to store time of events, as well as extend xoEvent to be able to store a chain of
		// missed events that came before it.
		cx->DidReplace = true;
		*iter = *cx->NewEvent;
		return false;
	}
	return true;
}

// Why do we do this? Normally the OS does this for us - it coalesces mouse move messages into
// a single message, when we ask for it. However, because our message polling loop runs on a different
// thread to our 'program' thread, we can consume mouse move messages faster than the 'program'
// can process them. So we can end up with a massive backlog of messages to process. Right now
// all we do is replace old events from the queue, but in future.. see the TODO message above.
void xoDocGroup::AddOrReplaceMessage(const xoOriginalEvent& ev)
{
	AddOrReplaceMessage_Context cx = { &ev, false };
	xoGlobal()->UIEventQueue.Scan(false, &cx, &AddOrReplaceMessage_Scan);
	if (!cx.DidReplace)
	{
		xoGlobal()->UIEventQueue.Add(ev);
	}
}

void xoDocGroup::ProcessEvent(xoEvent& ev)
{
	// NOTE: I think the use of a Windows CRITICAL_SECTION is not great, because I get the
	// feeling that the UI thread can starve the render thread if the UI thread is processing
	// a ton of messages, and taking a long time to do so.
	// My first idea was to add a check in here to see whether the document had outstanding
	// rendering work to be done (ie rDocAge > 1 -- higher in this source file ). If so,
	// then pause for a millisecond or two to allow the render thread to acquire the doclock.
	// That's obviously heresy - the only correct thing is to have a fair mutex that operates
	// like a queue instead of first-come-first-serve. I don't know what the semantics are
	// of the Windows CRITICAL_SECTION. Also, if I recall correctly, Jeff Preshing may have
	// had a great article on using semaphores to implement a fair queue like this.

	// This is indeed a terrible solution to the above problem .I tried it out of curiosity.
	// It merely adds a lag to the processing of all messages.
	// if (DocAge() >= 1) AbcSleep(1);

	TakeCriticalSection lock(DocLock);

	if (ev.Type != xoEventTimer)
		XOTRACE_LATENCY("ProcessEvent (not a timer)\n");

	xoLayoutResult* layout = RenderDoc->AcquireLatestLayout();

	xoCursors oldCursor = Doc->UI.GetCursor();

	Doc->UI.InternalProcessEvent(ev, layout);

	// Get the main thread to update it's cursor now
	if (Doc->UI.GetCursor() != oldCursor)
	{
		auto blah = Doc->UI.GetCursor();
		Wnd->PostCursorChangedMessage();
	}

	RenderDoc->ReleaseLayout(layout);
}

bool xoDocGroup::IsDirty() const
{
	return IsDocVersionDifferentToRenderer() || Wnd->GetInvalidateRect().IsAreaPositive();
}

bool xoDocGroup::IsDocVersionDifferentToRenderer() const
{
	return Doc->GetVersion() != RenderDoc->Doc.GetVersion();
}

