#include "pch.h"
#include "nuDoc.h"
#include "nuProcessor.h"
#include "nuRender.h"
#include "nuLayout.h"
#include "nuSysWnd.h"

nuProcessor::nuProcessor()
{
	AbcCriticalSectionInitialize( DocLock );
	MessagesSinceLastRender = 0;
	Doc = NULL;
	Wnd = NULL;
	RenderDoc = new nuRenderDoc();
}

nuProcessor::~nuProcessor()
{
	delete RenderDoc;
	AbcCriticalSectionDestroy( DocLock );
}

// This is always called from the Render thread
nuRenderResult nuProcessor::Render()
{
	bool haveLock = false;
	// I'm not quite sure how we should handle this. The idea is that you don't want to go without a UI update
	// for too long, even if the UI thread is taking its time, and being bombarded with messages.
	if ( MessagesSinceLastRender > 0 )
	{
		haveLock = true;
		AbcCriticalSectionEnter( DocLock );
	}
	else
		haveLock = AbcCriticalSectionTryEnter( DocLock );

	if ( !haveLock )
		return nuRenderResultNeedMore;

	// TODO: If AnyAnimationsRunning(), then we are not idle
	bool idle = (Doc->WindowWidth == 0 || Doc->WindowHeight == 0) ||
				!(Doc->IsAppearanceDirty() || Doc->IsLayoutDirty());
	
	if ( idle )
	{
		AbcCriticalSectionLeave( DocLock );
	}
	else
	{
		Doc->AppearanceClean();
		Doc->LayoutClean();
		RenderDoc->UpdateDoc( *Doc );
		InterlockedExchange( &MessagesSinceLastRender,  0 );
		AbcCriticalSectionLeave( DocLock );

		if ( !Wnd->BeginRender() )
			return nuRenderResultNeedMore;

		RenderDoc->Render( Wnd->RGL );

		Wnd->FinishRender();
	}

	return idle ? nuRenderResultIdle : nuRenderResultNeedMore;
}

// This is always called from the UI thread
void nuProcessor::ProcessEvent( nuEvent& ev )
{
	{
		TakeCriticalSection lock( DocLock );
		switch ( ev.Type )
		{
		case nuEventWindowSize:
			Doc->WindowWidth = (uint32) ev.Points[0].x;
			Doc->WindowHeight = (uint32) ev.Points[0].y;
			Doc->InvalidateLayout();
			break;
		}
		// HACK. This analysis needs to be performed by the renderer, after it has copied our document over.
		// Doc shouldn't actually have these flags. It should have only one flag, which is "An user-controllable event ran".
		// If that flag is true (or if we've had a systemic event such as WindowSize), then we should proceed to the render
		// phase, and perform differential analysis there. First just compare the two linear tables of dom elements. From
		// there you can know whether you need to perform layout.
		Doc->InvalidateLayout();
		Doc->InvalidateAppearance();
		BubbleEvent( ev );
	}
	InterlockedIncrement( &MessagesSinceLastRender );
}

void nuProcessor::BubbleEvent( nuEvent& ev )
{
	// TODO. My plan is to go with upward bubbling only. The inner-most
	// control gets the event first, then outward.
	// A return value of false means "cancel the bubble".
	// But ah.... downward bubbling is necessary for things like shortcut
	// keys. I'm not sure how one does that with HTML.

	nuDomEl* el = &Doc->Root;
	bool stop = false;

	if ( el->HandlesEvent(ev.Type) )
	{
		const podvec<nuEventHandler>& h = el->GetHandlers();
		for ( intp i = 0; i < h.size() && !stop; i++ )
		{
			if ( h[i].Handles( ev.Type ) )
			{
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
}

void nuProcessor::FindTarget( const nuVec2& p, pvect<nuRenderDomEl*>& chain )
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
