#include "pch.h"
#include "xoDocGroup.h"
#include "xoDocUI.h"
#include "xoSysWnd.h"		// Imported only for SetSystemCursor. SetSystemCursor belongs in a different namespace
#include "Render/xoRenderDoc.h"
#include "Render/xoRenderDomEl.h"
#include "Render/xoRenderStack.h"
#include "Render/xoStyleResolve.h"

xoDocUI::xoDocUI( xoDoc* doc )
{
	Doc = doc;
	ViewportWidth = 0;
	ViewportHeight = 0;
	Cursor = xoCursorArrow;
}

xoDocUI::~xoDocUI()
{
}

// This is always called from the UI thread
// By the time this is called, the DocGroup->DocLock must already be held.
void xoDocUI::InternalProcessEvent( xoEvent& ev, const xoLayoutResult* layout )
{
	switch ( ev.Type )
	{
	case xoEventWindowSize:
		ViewportWidth = (uint32) ev.Points[0].x;
		ViewportHeight = (uint32) ev.Points[0].y;
		Doc->IncVersion();
		//XOTIME( "Processed WindowSize event. Document at version %d\n", Doc->GetVersion() );
		break;
	}

	// Give up processing any other events if we haven't run a layout yet
	if ( layout == nullptr )
		return;

	if ( BubbleEvent( ev, layout ) )
		Doc->IncVersion();
}

void xoDocUI::CloneSlowInto( xoDocUI& c ) const
{
	c.CurrentFocusID = CurrentFocusID;
	c.HoverNodes = HoverNodes;
	c.HoverSet = HoverSet;
	c.ViewportWidth = ViewportWidth;
	c.ViewportHeight = ViewportHeight;
	c.Cursor = Cursor;
}

// Returns true if the event was handled
bool xoDocUI::BubbleEvent( xoEvent& ev, const xoLayoutResult* layout )
{
	// The platform must just send MouseMove. It is our job here to synthesize MouseEnter for DOM nodes.
	// The platform must, however, send xoEventMouseLeave.
	XOASSERTDEBUG( ev.Type != xoEventMouseEnter );

	// TODO. My plan is to go with upward bubbling only. The inner-most
	// control gets the event first, then outward.
	// A return value of false means "cancel the bubble".
	// But ah.... downward bubbling is necessary for things like shortcut
	// keys. I'm not sure how one does that with HTML.
	// Right.. so "capturing" is the method where the event propagates inwards.
	// IE does not support capturing though, so nobody really use it.
	// We simply ignore the question of how to do shortcut keys for now.
	// Thinking more about shortcut keys - these are different from cursor events.
	// Keyboard events are always targeted at the object that has the focus,
	// so it is trivially easy to do a down + up bubble for that particular case.

	XOTRACE_EVENTS( "BubbleEvent type=%d\n", (int) ev.Type );

	bool stop = false;
	bool handled = false;

	pvect<const xoRenderDomNode*> nodeChain;
	if ( ev.Type == xoEventMouseLeave )
	{
		// When xoEventMouseLeave comes in from the window system, it means the cursor has left our
		// client area. This is not the same thing as the cursor leaving a DOM node.
		ev.Type = xoEventMouseMove;
	}
	else
	{
		FindTarget( ev.Points[0], nodeChain, layout );
	}

	if ( ev.Type == xoEventMouseMove )
	{
		UpdateCursorLocation( nodeChain );
	}

	//XOTRACE_EVENTS( "FindTarget chainlen = %d\n", (int) nodeChain.size() );

	// start at the inner-most node first
	for ( intp inode = nodeChain.size() - 1; inode >= 0; inode-- )
	{
		const xoRenderDomNode* rnode = nodeChain[inode];
		const xoDomNode* node = Doc->GetNodeByInternalID( rnode->InternalID );
		if ( node != nullptr )
		{
			if ( node->HandlesEvent(ev.Type) )
			{
				// Remember that SendEvent is allowed to do absolutely anything it wants to our DOM.
				// So it could be that the rest of the objects inside nodeChain are invalid once
				// SendEvent returns. We cater for this by always fetching DOM nodes based on InternalID.
				SendEvent( ev, node, &handled, &stop );
				if ( stop )
					break;
			}
		}
	}

	if ( ev.Type == xoEventClick )
		UpdateFocusWindow( nodeChain );

	return handled;
}

/* Given a point, return the chain of DOM elements (starting at the root) that leads down
to the inner-most DOM element beneath the cursor.

This code does not make provision for elements that are positioned outside of their parent,
such as relative-positioned or absolute-positioned.
*/
void xoDocUI::FindTarget( xoVec2f p, pvect<const xoRenderDomNode*>& nodeChain, const xoLayoutResult* layout )
{
	xoPoint pos = { xoRealToPos(p.x), xoRealToPos(p.y) };
	const xoRenderDomNode* body = layout->Body();
	if ( !body->BorderBox().IsInsideMe(pos) )
		return;
	podvec<xoPoint> posChain;
	intp stackPos = 0;
	nodeChain += body;
	posChain += pos - body->Pos.TopLeft();
	while ( stackPos < nodeChain.size() )
	{
		const xoRenderDomNode* top = nodeChain[stackPos];
		xoPoint relPos = posChain[stackPos];
		stackPos++;
		// walk backwards, yielding implicit z-order from child order
		for ( intp i = top->Children.size() - 1; i >= 0; i-- )
		{
			if ( top->Children[i]->IsNode() )
			{
				const xoRenderDomNode* childNode = static_cast<const xoRenderDomNode*>(top->Children[i]);
				if ( childNode->BorderBox().IsInsideMe( relPos ) )
				{
					nodeChain += childNode;
					posChain += relPos - childNode->Pos.TopLeft();
					// Only allow a single path to the inner-most object.
					// This code will need to be extended to handle explicit z-order
					break;
				}
			}
		}
	}
}

void xoDocUI::UpdateCursorLocation( const pvect<const xoRenderDomNode*>& nodeChain )
{
	// Update Hover states, send mouse leave and mouse enter events.
	bool anyHoverChanges = false;
	fhashset<xoInternalID> oldNodeIDs, newNodeIDs;

	for ( intp i = 0; i < HoverNodes.size(); i++ )
		oldNodeIDs.insert( HoverNodes[i].InternalID );

	for ( intp i = 0; i < nodeChain.size(); i++ )
		newNodeIDs.insert( nodeChain[i]->InternalID );

	// Find objects that used to be under the cursor, but are no longer under the cursor
	for ( intp i = 0; i < HoverNodes.size(); i++ )
	{
		if ( !newNodeIDs.contains(HoverNodes[i].InternalID) )
		{
			anyHoverChanges = true;
			if ( HoverNodes[i].HasHoverStyle )
				InvalidateRenderForPseudoClass();

			auto oldEl = Doc->GetNodeByInternalID(HoverNodes[i].InternalID);
			if ( oldEl != nullptr && oldEl->HandlesEvent( xoEventMouseLeave ) )
				SendEvent( MakeEvent(xoEventMouseLeave), oldEl );
		}
	}

	decltype(HoverNodes) newHoverNodes;
	
	// Find new under-the-cursor objects
	for ( intp i = 0; i < nodeChain.size(); i++ )
	{
		if ( !oldNodeIDs.contains(nodeChain[i]->InternalID) )
		{
			anyHoverChanges = true;
			if ( nodeChain[i]->Style.HasHoverStyle )
				InvalidateRenderForPseudoClass();

			auto newEl = Doc->GetNodeByInternalID(nodeChain[i]->InternalID);
			if ( newEl != nullptr && newEl->HandlesEvent( xoEventMouseEnter ) )
				SendEvent( MakeEvent(xoEventMouseEnter), newEl );
		}

		newHoverNodes += HoverNode{ nodeChain[i]->InternalID, nodeChain[i]->Style.HasHoverStyle };
	}

	HoverNodes = newHoverNodes;

	HoverSet.clear();
	for ( intp i = 0; i < HoverNodes.size(); i++ )
		HoverSet.insert( HoverNodes[i].InternalID );

	// Update mouse cursor
	if ( anyHoverChanges )
	{
		xoCursors oldCursor = Cursor;
		if ( nodeChain.size() == 0 )
			Cursor = xoCursorArrow;
		else
		{
			const xoDomNode* node = Doc->GetNodeByInternalID( nodeChain.back()->InternalID );
			if ( node != nullptr )
			{
				xoStyleResolveOnceOff style( node );
				Cursor = style.RS->Get( xoCatCursor ).GetCursor();
			}
		}

		// This does not work, because we are running on a different thread to the 
		// thread that owns the Window's message queue. We need a more sophisticated mechanism
		// of asynchronously updating the Windows cursor. I haven't figured out yet how
		// that should look.
		//if ( Cursor != oldCursor )
		//	xoSysWnd::SetSystemCursor( Cursor );
	}
}

void xoDocUI::UpdateFocusWindow( const pvect<const xoRenderDomNode*>& nodeChain )
{
	// Find the window that should receive the focus
	const xoDomNode* newFocus = nullptr;
	for ( intp inode = nodeChain.size() - 1; inode > 0; inode-- )
	{
		const xoRenderDomNode* rnode = nodeChain[inode];
		const xoDomNode* node = Doc->GetNodeByInternalID( rnode->InternalID );
		if ( node != nullptr )
		{
			xoStyleResolveOnceOff style( node );
			if ( style.RS->Get( xoCatCanFocus ).GetCanFocus() )
			{
				newFocus = node;
				break;
			}
		}
	}

	if ( CurrentFocusID != xoInternalIDNull && (newFocus == nullptr || newFocus->GetInternalID() != CurrentFocusID) )
	{
		// lose focus (aka blur)
		const xoDomNode* old = Doc->GetNodeByInternalID( CurrentFocusID );
		if ( old != nullptr )
		{
			XOTRACE_EVENTS( "LoseFocus %d\n", (int) CurrentFocusID );
			// assume that the previously focused object's rendering is dependent on its focal state.
			// The only reason we do this is because we don't have a convenient way of finding the xoRenderDomNode from
			// the ID. We'll probably want to add a lookup table inside xoLayoutResult for that.
			InvalidateRenderForPseudoClass();
			SendEvent( MakeEvent(xoEventLoseFocus), old );
		}
	}

	if ( newFocus != nullptr && newFocus->GetInternalID() != CurrentFocusID )
	{
		XOTRACE_EVENTS( "GetFocus %d\n", (int) newFocus->GetInternalID() );
		// assume that the newly focused object's rendering is dependent on its focal state.
		// Same applies here as above.
		InvalidateRenderForPseudoClass();
		SendEvent( MakeEvent(xoEventGetFocus), newFocus );
	}

	CurrentFocusID = (newFocus == nullptr) ? xoInternalIDNull : newFocus->GetInternalID();
}

xoEvent xoDocUI::MakeEvent( xoEvents evType )
{
	xoEvent ev;
	ev.Doc = Doc;
	ev.Type = evType;
	return ev;
}

void xoDocUI::InvalidateRenderForPseudoClass()
{
	// This is heavy-handed. Ideally we could inform the layout system that it doesn't need to re-evaluate styles.
	Doc->IncVersion();
}

void xoDocUI::SendEvent( const xoEvent& ev, const xoDomNode* node, bool* handled, bool* stop )
{
	bool localHandled = false;
	bool localStop = false;
	xoEvent localEv = ev;
	const xoEventHandler* h;
	intp hcount;
	node->GetHandlers( h, hcount );
	XOTRACE_EVENTS( "Found %d event handlers\n", (int) hcount );
	for ( intp i = 0; i < hcount; i++ )
	{
		if ( h[i].Handles( ev.Type ) )
		{
			XOTRACE_EVENTS( "Dispatching event to %d/%d (%p)\n", (int) i, (int) hcount, h[i].Func );
			localHandled = true;
			localEv.Context = h[i].Context;
			localEv.Target = const_cast<xoDomNode*>(node);
			if ( !h[i].Func( localEv ) )
			{
				localStop = true;
				break;
			}
		}
		// re-acquire set of handlers. This is not perfect - but if the handler is altering the list of
		// handlers, then I'm not sure what the most sensible course of action is. "do not crash" is
		// a reasonable requirement though.
		node->GetHandlers( h, hcount );
	}
	if ( handled != nullptr )
		*handled = localHandled;
	if ( stop != nullptr )
		*stop = localStop;
}

