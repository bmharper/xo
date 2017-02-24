#include "pch.h"
#include "Doc.h"
#include "DocGroup.h"
#include "Layout/Layout.h"
#include "SysWnd.h"
#include "Render/Renderer.h"
#include "Render/RenderDoc.h"
#include "Render/RenderDomEl.h"
#include "Render/RenderBase.h"
#include "Render/RenderGL.h"
#include "Render/StyleResolve.h"
#include "Image/Image.h"

namespace xo {

DocGroup::DocGroup() {
	DestroyDocWithGroup = false;
	Doc                 = NULL;
	Wnd                 = NULL;
	RenderDoc           = new xo::RenderDoc(this);
	RenderStats.Reset();
}

DocGroup::~DocGroup() {
	delete RenderDoc;
	if (DestroyDocWithGroup)
		delete Doc;
}

RenderResult DocGroup::Render() {
	return RenderInternal(nullptr);
}

RenderResult DocGroup::RenderToImage(Image& image) {
	// The 10 here is an arbitrary thumbsuck. We'll see if we ever need a controllable limit.
	const int    maxAttempts = 10;
	RenderResult res         = RenderResultNeedMore;
	for (int attempt = 0; res == RenderResultNeedMore && attempt < maxAttempts; attempt++)
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
RenderResult DocGroup::RenderInternal(Image* targetImage) {
	bool wndDirty = Wnd->GetInvalidateRect().IsAreaPositive();
	bool haveLock = false;

	// If docAge = 0, then we do not need to make a new copy of Doc.
	// We merely need to run animations, or repaint our window.
	bool docModified = DocAge() >= 1;

	// I'm not quite sure how we should handle this. The idea is that you don't want to go without a UI update
	// for too long, even if the UI thread is taking its time, and being bombarded with messages.
	if (docModified || targetImage != NULL) {
		// If UI thread has performed even a single update since we last rendered, then pause our thread until we can gain the DocLock
		auto tstart = TimeAccurateSeconds();
		DocLock.lock();
		auto time = TimeAccurateSeconds() - tstart;
		if (time > 0.001)
			TimeTrace("DocGroup.RenderInternal took %d ms to acquire DocLock\n", (int) (time * 1000));
		haveLock = true;
	}

	bool docValid    = Doc->UI.GetViewportWidth() != 0 && Doc->UI.GetViewportHeight() != 0;
	bool beganRender = false;

	double timeCopyDoc = 0;

	if (haveLock) {
		UploadImagesToGPU(beganRender);

		//Trace( "Render Version %u\n", Doc->GetVersion() );
		CodeTimer t;
		RenderDoc->CopyFromCanonical(*Doc, RenderStats);

		// Assume we are the only renderer of 'Doc'. If this assumption were not true, then you would need to update
		// all renderers simultaneously, so that you can guarantee that UsableIDs all go to FreeIDs atomically.
		//Trace( "MakeFreeIDsUsable\n" );
		Doc->MakeFreeIDsUsable();
		Doc->ResetModifiedBitmap(); // AbcBitMap has an absolutely awful implementation of this (uint8_t-filled vs SSE or at least pointer-word-size-filled)
		timeCopyDoc = t.Measure();

		DocLock.unlock();
	}

	RenderResult rendResult   = RenderResultDone;
	bool         presentFrame = false;

	if (docValid && Wnd != nullptr) {
		//TimeTrace( "Render start\n" );
		if (!beganRender && !Wnd->BeginRender()) {
			TimeTrace("BeginRender failed\n");
			return RenderResultNeedMore;
		}
		beganRender = true;

		//TimeTrace( "Render DO\n" );
		rendResult = RenderDoc->Render(Wnd->Renderer);

		presentFrame = true;

		if (targetImage != nullptr)
			Wnd->Renderer->ReadBackbuffer(*targetImage);
	}

	if (beganRender) {
		// presentFrame will be false when the only action we've taken on the GPU is uploading textures.
		//TimeTrace( "Render Finish\n" );
		Wnd->EndRender(presentFrame ? 0 : EndRenderNoSwap);
	}

	// If anybody is listening, queue a "post render" event
	if (rendResult == RenderResultDone) {
		DocLock.lock();
		if (Doc->AnyRenderHandlers() != 0) {
			OriginalEvent ev;
			ev.DocGroup   = this;
			ev.Event.Type = EventRender;
			Global()->UIEventQueue.Add(ev);
		}
		DocLock.unlock();
	}

	if (Global()->ShowCoarseTimes)
		xo::Trace("Copy: %.1f, Bake: %.1f, Layout: %.1f, Render: %.1f, PostRender: %.1f\n",
			timeCopyDoc * 1000, RenderDoc->TimeVariableBake * 1000, RenderDoc->TimeLayout * 1000, RenderDoc->TimeRender * 1000, RenderDoc->TimePostRender * 1000);

	return rendResult;
}

void DocGroup::UploadImagesToGPU(bool& beganRender) {
	beganRender                    = false;
	cheapvec<Image*> invalidImages = Doc->Images.InvalidList();
	if (invalidImages.size() != 0) {
		if (!Wnd->BeginRender())
			return;

		beganRender = true;

		for (size_t i = 0; i < invalidImages.size(); i++) {
			if (Wnd->Renderer->LoadTexture(invalidImages[i], 0)) {
				invalidImages[i]->TexClearInvalidRect();
			} else {
				XOTRACE_WARNING("Failed to upload image to GPU\n");
			}
		}
	}
}

uint32_t DocGroup::DocAge() const {
	return Doc->GetVersion() - RenderDoc->Doc.GetVersion();
}

struct AddOrReplaceMessage_Context {
	const OriginalEvent* NewEvent;
	bool                 DidReplace;
};

static bool AddOrReplaceMessage_Scan(void* context, OriginalEvent* iter) {
	AddOrReplaceMessage_Context* cx = (AddOrReplaceMessage_Context*) context;
	if (iter->DocGroup == cx->NewEvent->DocGroup && iter->Event.Type == cx->NewEvent->Event.Type) {
		// TODO: Keep a history of the replaced events, so that a program that needs to have
		// smooth cursor input can use all of the events that were sent by the OS. To do this, we'll
		// want to store time of events, as well as extend Event to be able to store a chain of
		// missed events that came before it.
		cx->DidReplace = true;
		*iter          = *cx->NewEvent;
		return false;
	}
	return true;
}

// Why do we do this? Normally the OS does this for us - it coalesces mouse move messages into
// a single message, when we ask for it. However, because our message polling loop runs on a different
// thread to our 'program' thread, we can consume mouse move messages faster than the 'program'
// can process them. By 'program' here, we mean the DOM event handlers that run from our UI thread.
// Because of this, we can end up with a massive backlog of messages to process. Right now
// all we do is replace old events from the queue, but in future.. see the TODO message above.
void DocGroup::AddOrReplaceMessage(const OriginalEvent& ev) {
	AddOrReplaceMessage_Context cx = {&ev, false};
	Global()->UIEventQueue.Scan(false, &cx, &AddOrReplaceMessage_Scan);
	if (!cx.DidReplace) {
		Global()->UIEventQueue.Add(ev);
	}
}

void DocGroup::ProcessEvent(Event& ev) {
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
	// if (DocAge() >= 1) SleepMS(1);

	std::lock_guard<std::mutex> lock(DocLock);

	ev.Doc = Doc;

	if (ev.Type != EventTimer)
		XOTRACE_LATENCY("ProcessEvent (not a timer)\n");

	LayoutResult* layout = RenderDoc->AcquireLatestLayout();

	Cursors oldCursor  = Doc->UI.GetCursor();
	auto    oldVersion = Doc->GetVersion();

	Doc->UI.InternalProcessEvent(ev, layout);

	// Get the main thread to update it's cursor now
	if (Doc->UI.GetCursor() != oldCursor) {
		Wnd->PostCursorChangedMessage();
	}

	if (Doc->GetVersion() != oldVersion) {
		Wnd->PostRepaintMessage();
	}

	RenderDoc->ReleaseLayout(layout);
}

bool DocGroup::IsDirty() const {
	return IsDocVersionDifferentToRenderer() || Wnd->GetInvalidateRect().IsAreaPositive();
}

bool DocGroup::IsDocVersionDifferentToRenderer() const {
	return Doc->GetVersion() != RenderDoc->Doc.GetVersion();
}
}
