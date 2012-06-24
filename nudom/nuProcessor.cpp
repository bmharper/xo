#include "pch.h"
#include "nuDoc.h"
#include "nuProcessor.h"
#include "nuRender.h"
#include "nuLayout.h"
#include "nuSysWnd.h"

nuProcessor::nuProcessor()
{
	Doc = NULL;
	Wnd = NULL;
	RenderDoc = new nuRenderDoc();
}

nuProcessor::~nuProcessor()
{
	delete RenderDoc;
}

// This will come in either from the main thread (on Win32), or from the Renderer thread.
// Win32 is main thread OR renderer thread
// Android is always renderer thread
void nuProcessor::Render()
{
	if ( !Wnd->BeginRender() )
		return;

	RenderDoc->Render( Wnd->RGL );

	Wnd->FinishRender();
}

bool nuProcessor::CopyDoc()
{
	if ( Doc->WindowWidth == 0 || Doc->WindowHeight == 0 )
		return false;

	RenderDoc->UpdateDoc( *Doc );

	return true;
}

void nuProcessor::CopyDocAndQueueRender()
{
	if ( !CopyDoc() )
		return;
	nuQueueRender( this );
}

void nuProcessor::CopyDocAndRenderNow()
{
	if ( !CopyDoc() )
		return;
	Render();
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
