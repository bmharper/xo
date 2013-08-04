#include "pch.h"
#include "nuDoc.h"
#include "nuDocGroup.h"
#include "nuLayout.h"
#include "nuSysWnd.h"
#include "Render/nuRenderer.h"
#include "Render/nuRenderDoc.h"
#include "Render/nuRenderDomEl.h"

nuDocGroup::nuDocGroup()
{
	AbcCriticalSectionInitialize( DocLock );
	DestroyDocWithProcessor = false;
	Doc = NULL;
	Wnd = NULL;
	RenderDoc = new nuRenderDoc();
	RenderStats.Reset();
}

nuDocGroup::~nuDocGroup()
{
	delete RenderDoc;
	if ( DestroyDocWithProcessor )
		delete Doc;
	AbcCriticalSectionDestroy( DocLock );
}

// This is always called from the Render thread
nuRenderResult nuDocGroup::Render()
{
	bool haveLock = false;
	// I'm not quite sure how we should handle this. The idea is that you don't want to go without a UI update
	// for too long, even if the UI thread is taking its time, and being bombarded with messages.
	uint32 rDocAge = Doc->GetVersion() - RenderDoc->Doc.GetVersion(); 
	if ( rDocAge > 0 )
	{
		haveLock = true;
		AbcCriticalSectionEnter( DocLock );
	}
	else
		haveLock = AbcCriticalSectionTryEnter( DocLock );

	if ( !haveLock )
	{
		NUTRACE( "Render: Failed to acquire DocLock\n" );
		return nuRenderResultNeedMore;
	}

	// TODO: If AnyAnimationsRunning(), then we are not idle
	bool idle = (Doc->WindowWidth == 0 || Doc->WindowHeight == 0) || Doc->GetVersion() == RenderDoc->Doc.GetVersion();
	
	if ( idle )
	{
		AbcCriticalSectionLeave( DocLock );
	}
	else
	{
		//NUTRACE( "Render Version %u\n", Doc->GetVersion() );
		RenderDoc->CopyFromCanonical( *Doc, RenderStats );
		
		// Assume we are the only renderer of 'Doc'. If this assumption were not true, then you would need to update
		// all renderers simultaneously, so that you can guarantee that UsableIDs all go to FreeIDs atomically.
		//NUTRACE( "MakeFreeIDsUsable\n" );
		Doc->MakeFreeIDsUsable();
		Doc->ResetModifiedBitmap();			// AbcBitMap has an absolutely awful implementation of this (byte-filled)

		AbcCriticalSectionLeave( DocLock );
	}

	if ( !idle && Wnd != NULL )
	{
		//NUTRACE( "BeginRender\n" );
		if ( !Wnd->BeginRender() )
		{
			NUTRACE( "BeginRender failed\n" );
			return nuRenderResultNeedMore;
		}

		//NUTRACE( "Render DO\n" );
		RenderDoc->Render( Wnd->RGL );

		//NUTRACE( "Render Finish\n" );
		Wnd->FinishRender();
	}

	return idle ? nuRenderResultIdle : nuRenderResultNeedMore;
}

// This is always called from the UI thread
void nuDocGroup::ProcessEvent( nuEvent& ev )
{
	TakeCriticalSection lock( DocLock );
	switch ( ev.Type )
	{
	case nuEventWindowSize:
		Doc->WindowWidth = (uint32) ev.Points[0].x;
		Doc->WindowHeight = (uint32) ev.Points[0].y;
		Doc->IncVersion();
		NUTRACE( "Processed WindowSize event. Document at version %d\n", Doc->GetVersion() );
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

	nuDomEl* el = &Doc->Root;
	bool stop = false;
	bool handled = false;

	if ( el->HandlesEvent(ev.Type) )
	{
		const podvec<nuEventHandler>& h = el->GetHandlers();
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

void nuDocGroup::FindTarget( const nuVec2& p, pvect<nuRenderDomEl*>& chain )
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
