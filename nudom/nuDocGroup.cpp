#include "pch.h"
#include "nuDoc.h"
#include "nuDocGroup.h"
#include "Layout/nuLayout.h"
#include "nuSysWnd.h"
#include "Render/nuRenderer.h"
#include "Render/nuRenderDoc.h"
#include "Render/nuRenderDomEl.h"
#include "Render/nuRenderBase.h"
#include "Render/nuRenderGL.h"

nuDocGroup::nuDocGroup()
{
	AbcCriticalSectionInitialize( DocLock );
	DestroyDocWithGroup = false;
	Doc = NULL;
	Wnd = NULL;
	RenderDoc = new nuRenderDoc();
	RenderStats.Reset();
}

nuDocGroup::~nuDocGroup()
{
	delete RenderDoc;
	if ( DestroyDocWithGroup )
		delete Doc;
	AbcCriticalSectionDestroy( DocLock );
}

nuRenderResult nuDocGroup::Render()
{
	return RenderInternal( NULL );
}

nuRenderResult nuDocGroup::RenderToImage( nuImage& image )
{
	// The 10 here is an arbitrary thumbsuck. We'll see if we ever need a controllable limit.
	const int maxAttempts = 10;
	nuRenderResult res = nuRenderResultNeedMore;
	for ( int attempt = 0; res == nuRenderResultNeedMore && attempt < maxAttempts; attempt++ )
		res = RenderInternal( &image );
	return res;
}

// This is always called from the Render thread
nuRenderResult nuDocGroup::RenderInternal( nuImage* targetImage )
{
	bool haveLock = false;
	// I'm not quite sure how we should handle this. The idea is that you don't want to go without a UI update
	// for too long, even if the UI thread is taking its time, and being bombarded with messages.
	uint32 rDocAge = Doc->GetVersion() - RenderDoc->Doc.GetVersion(); 
	if ( rDocAge > 0 || targetImage != NULL )
	{
		// If UI thread has performed many updates since we last rendered,
		// then pause our thread until we can gain the DocLock
		haveLock = true;
		AbcCriticalSectionEnter( DocLock );
	}
	else
	{
		// The UI thread has not done much since we last rendered, so do not wait for the lock
		haveLock = AbcCriticalSectionTryEnter( DocLock );
	}

	if ( !haveLock )
	{
		NUTIME( "Render: Failed to acquire DocLock\n" );
		return nuRenderResultNeedMore;
	}

	// TODO: If AnyAnimationsRunning(), then we are not idle
	bool docValid = RenderDoc->WindowWidth != 0 && RenderDoc->WindowHeight != 0;
	bool docModified = Doc->GetVersion() != RenderDoc->Doc.GetVersion();
	
	if ( docModified && docValid )
	{
		//NUTRACE( "Render Version %u\n", Doc->GetVersion() );
		RenderDoc->CopyFromCanonical( *Doc, RenderStats );
		
		// Assume we are the only renderer of 'Doc'. If this assumption were not true, then you would need to update
		// all renderers simultaneously, so that you can guarantee that UsableIDs all go to FreeIDs atomically.
		//NUTRACE( "MakeFreeIDsUsable\n" );
		Doc->MakeFreeIDsUsable();
		Doc->ResetModifiedBitmap();			// AbcBitMap has an absolutely awful implementation of this (byte-filled vs SSE or at least pointer-word-size-filled)
	}
	AbcCriticalSectionLeave( DocLock );

	nuRenderResult rendResult = nuRenderResultIdle;

	if ( (docModified || targetImage != NULL) && docValid && Wnd != NULL )
	{
		//NUTIME( "Render start\n" );
		if ( !Wnd->BeginRender() )
		{
			NUTIME( "BeginRender failed\n" );
			return nuRenderResultNeedMore;
		}

		//NUTIME( "Render DO\n" );
		rendResult = RenderDoc->Render( Wnd->Renderer );

		if ( targetImage != NULL )
			Wnd->Renderer->ReadBackbuffer( *targetImage );

		//NUTIME( "Render Finish\n" );
		Wnd->EndRender();
	}

	return rendResult;
}

// This is always called from the UI thread
void nuDocGroup::ProcessEvent( nuEvent& ev )
{
	TakeCriticalSection lock( DocLock );
	uint32 initialVersion = Doc->GetVersion();
	if ( ev.Type != nuEventTimer )
		NUTRACE_LATENCY("ProcessEvent (not a timer)\n");
	switch ( ev.Type )
	{
	case nuEventWindowSize:
		RenderDoc->WindowWidth = (uint32) ev.Points[0].x;
		RenderDoc->WindowHeight = (uint32) ev.Points[0].y;
		Doc->IncVersion();
		//NUTIME( "Processed WindowSize event. Document at version %d\n", Doc->GetVersion() );
		break;
	}
	if ( BubbleEvent( ev ) )
		Doc->IncVersion();
}

// Returns true if the event was handled
bool nuDocGroup::BubbleEvent( nuEvent& ev )
{
	// TODO. My plan is to go with upward bubbling only. The inner-most
	// control gets the event first, then outward.
	// A return value of false means "cancel the bubble".
	// But ah.... downward bubbling is necessary for things like shortcut
	// keys. I'm not sure how one does that with HTML.
	// Right.. so "capturing" is the method where the event propagates inwards.
	// IE does not support capturing though, so nobody really use it.
	// We simply ignore the question of how to do shortcut keys for now.

	NUTRACE_EVENTS( "BubbleEvent type=%d\n", (int) ev.Type );

	nuDomNode* el = &Doc->Root;
	bool stop = false;
	bool handled = false;

	if ( el->HandlesEvent(ev.Type) )
	{
		const podvec<nuEventHandler>& h = el->GetHandlers();
		NUTRACE_EVENTS( "BubbleEvent found %d event handlers\n", (int) h.size() );
		for ( intp i = 0; i < h.size() && !stop; i++ )
		{
			if ( h[i].Handles( ev.Type ) )
			{
				handled = true;
				nuEvent c = ev;
				c.Context = h[i].Context;
				c.Target = el;
				if ( !h[i].Func( c ) )
				{
					stop = true;
				}
			}
		}
	}

	return handled;
}

/*
void nuDocGroup::FindTarget( const nuVec2f& p, pvect<nuRenderDomEl*>& chain )
{
	chain += &RenderDoc->RenderRoot;
	while ( true )
	{
		nuRenderDomEl* top = chain.back();
		for ( intp i = 0; i < top->Children.size(); i++ )
		{
			//if ( top->Children[i]->Pos )
		}
	}
}
*/

bool nuDocGroup::IsDocVersionDifferentToRenderer() const
{
	return Doc->GetVersion() != RenderDoc->Doc.GetVersion();
}
