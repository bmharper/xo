#include "pch.h"
#include "DocGroup.h"
#include "DocUI.h"
#include "Render/RenderDoc.h"
#include "Render/RenderDomEl.h"
#include "Render/RenderStack.h"
#include "Render/StyleResolve.h"

namespace xo {

DocUI::DocUI(Doc* doc) {
	Doc            = doc;
	ViewportWidth  = 0;
	ViewportHeight = 0;
	Cursor         = CursorArrow;
}

DocUI::~DocUI() {
}

// This is always called from the UI thread
// By the time this is called, the DocGroup->DocLock must already be held.
void DocUI::InternalProcessEvent(Event& ev, const LayoutResult* layout) {
	switch (ev.Type) {
	case EventWindowSize:
		ViewportWidth  = (uint32_t) ev.Points[0].x;
		ViewportHeight = (uint32_t) ev.Points[0].y;
		Doc->IncVersion();
		//XOTIME( "Processed WindowSize event. Document at version %d\n", Doc->GetVersion() );
		break;
	}

	// Give up processing any other events if we haven't run a layout yet
	if (layout == nullptr)
		return;

	if (BubbleEvent(ev, layout))
		Doc->IncVersion();
}

void DocUI::CloneSlowInto(DocUI& c) const {
	c.CurrentFocusID = CurrentFocusID;
	c.HoverNodes     = HoverNodes;
	c.HoverSet       = HoverSet;
	c.ViewportWidth  = ViewportWidth;
	c.ViewportHeight = ViewportHeight;
	c.Cursor         = Cursor;
}

// Returns true if the event was handled
bool DocUI::BubbleEvent(Event& ev, const LayoutResult* layout) {
	// The platform must just send MouseMove. It is our job here to synthesize MouseEnter for DOM nodes.
	// The platform must, however, send EventMouseLeave.
	XO_DEBUG_ASSERT(ev.Type != EventMouseEnter);

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

	XOTRACE_EVENTS("BubbleEvent type=%d\n", (int) ev.Type);

	bool stop    = false;
	bool handled = false;

	cheapvec<const RenderDomNode*> nodeChain;
	if (ev.Type == EventMouseLeave) {
		// When EventMouseLeave comes in from the window system, it means the cursor has left our
		// client area. This is not the same thing as the cursor leaving a DOM node.
		ev.Type = EventMouseMove;
	} else {
		FindTarget(ev.Points[0], nodeChain, layout);
	}

	if (ev.Type == EventMouseMove) {
		UpdateCursorLocation(nodeChain);
	}

	//XOTRACE_EVENTS( "FindTarget chainlen = %d\n", (int) nodeChain.size() );

	// start at the inner-most node first
	for (size_t inode = nodeChain.size() - 1; inode >= 0; inode--) {
		const RenderDomNode* rnode = nodeChain[inode];
		const DomNode*       node  = Doc->GetNodeByInternalID(rnode->InternalID);
		if (node != nullptr) {
			if (node->HandlesEvent(ev.Type)) {
				// Remember that SendEvent is allowed to do absolutely anything it wants to our DOM.
				// So it could be that the rest of the objects inside nodeChain are invalid once
				// SendEvent returns. We cater for this by always fetching DOM nodes based on InternalID.
				SendEvent(ev, node, &handled, &stop);
				if (stop)
					break;
			}
		}
	}

	if (ev.Type == EventClick)
		UpdateFocusWindow(nodeChain);

	return handled;
}

/* Given a point, return the chain of DOM elements (starting at the root) that leads down
to the inner-most DOM element beneath the cursor.

This code does not make provision for elements that are positioned outside of their parent,
such as relative-positioned or absolute-positioned.
*/
void DocUI::FindTarget(Vec2f p, cheapvec<const RenderDomNode*>& nodeChain, const LayoutResult* layout) {
	Point                pos  = {RealToPos(p.x), RealToPos(p.y)};
	const RenderDomNode* body = layout->Body();
	if (!body->BorderBox().IsInsideMe(pos))
		return;
	cheapvec<Point> posChain;
	size_t        stackPos = 0;
	nodeChain += body;
	posChain += pos - body->Pos.TopLeft();
	while (stackPos < nodeChain.size()) {
		const RenderDomNode* top    = nodeChain[stackPos];
		Point                relPos = posChain[stackPos];
		stackPos++;
		// walk backwards, yielding implicit z-order from child order
		for (size_t i = top->Children.size() - 1; i != 0; i--) {
			if (top->Children[i]->IsNode()) {
				const RenderDomNode* childNode = static_cast<const RenderDomNode*>(top->Children[i]);
				if (childNode->BorderBox().IsInsideMe(relPos)) {
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

void DocUI::UpdateCursorLocation(const cheapvec<const RenderDomNode*>& nodeChain) {
	// Update Hover states, send mouse leave and mouse enter events.
	bool                   anyHoverChanges = false;
	ohash::set<InternalID> oldNodeIDs, newNodeIDs;

	for (size_t i = 0; i < HoverNodes.size(); i++)
		oldNodeIDs.insert(HoverNodes[i].InternalID);

	for (size_t i = 0; i < nodeChain.size(); i++)
		newNodeIDs.insert(nodeChain[i]->InternalID);

	// Find objects that used to be under the cursor, but are no longer under the cursor
	for (size_t i = 0; i < HoverNodes.size(); i++) {
		if (!newNodeIDs.contains(HoverNodes[i].InternalID)) {
			anyHoverChanges = true;
			if (HoverNodes[i].HasHoverStyle)
				InvalidateRenderForPseudoClass();

			auto oldEl = Doc->GetNodeByInternalID(HoverNodes[i].InternalID);
			if (oldEl != nullptr && oldEl->HandlesEvent(EventMouseLeave))
				SendEvent(MakeEvent(EventMouseLeave), oldEl);
		}
	}

	decltype(HoverNodes) newHoverNodes;

	// Find new under-the-cursor objects
	for (size_t i = 0; i < nodeChain.size(); i++) {
		if (!oldNodeIDs.contains(nodeChain[i]->InternalID)) {
			anyHoverChanges = true;
			if (nodeChain[i]->Style.HasHoverStyle)
				InvalidateRenderForPseudoClass();

			auto newEl = Doc->GetNodeByInternalID(nodeChain[i]->InternalID);
			if (newEl != nullptr && newEl->HandlesEvent(EventMouseEnter))
				SendEvent(MakeEvent(EventMouseEnter), newEl);
		}

		newHoverNodes += HoverNode{nodeChain[i]->InternalID, nodeChain[i]->Style.HasHoverStyle};
	}

	HoverNodes = newHoverNodes;

	HoverSet.clear();
	for (size_t i = 0; i < HoverNodes.size(); i++)
		HoverSet.insert(HoverNodes[i].InternalID);

	// Update mouse cursor.
	// Note that cursor changes are monitored by DocGroup. If it detects that our cursor has changed during this function,
	// then it will notify the main thread to update it's cursor immediately.
	if (anyHoverChanges) {
		if (nodeChain.size() == 0)
			Cursor = CursorArrow;
		else {
			const DomNode* node = Doc->GetNodeByInternalID(nodeChain.back()->InternalID);
			if (node != nullptr) {
				StyleResolveOnceOff style(node);
				Cursor = style.RS->Get(CatCursor).GetCursor();
			}
		}
	}
}

void DocUI::UpdateFocusWindow(const cheapvec<const RenderDomNode*>& nodeChain) {
	// Find the window that should receive the focus
	const DomNode* newFocus = nullptr;
	for (size_t inode = nodeChain.size() - 1; inode > 0; inode--) {
		const RenderDomNode* rnode = nodeChain[inode];
		const DomNode*       node  = Doc->GetNodeByInternalID(rnode->InternalID);
		if (node != nullptr) {
			StyleResolveOnceOff style(node);
			if (style.RS->Get(CatCanFocus).GetCanFocus()) {
				newFocus = node;
				break;
			}
		}
	}

	if (CurrentFocusID != InternalIDNull && (newFocus == nullptr || newFocus->GetInternalID() != CurrentFocusID)) {
		// lose focus (aka blur)
		const DomNode* old = Doc->GetNodeByInternalID(CurrentFocusID);
		if (old != nullptr) {
			XOTRACE_EVENTS("LoseFocus %d\n", (int) CurrentFocusID);
			// assume that the previously focused object's rendering is dependent on its focal state.
			// The only reason we do this is because we don't have a convenient way of finding the RenderDomNode from
			// the ID. We'll probably want to add a lookup table inside LayoutResult for that.
			InvalidateRenderForPseudoClass();
			SendEvent(MakeEvent(EventLoseFocus), old);
		}
	}

	if (newFocus != nullptr && newFocus->GetInternalID() != CurrentFocusID) {
		XOTRACE_EVENTS("GetFocus %d\n", (int) newFocus->GetInternalID());
		// assume that the newly focused object's rendering is dependent on its focal state.
		// Same applies here as above.
		InvalidateRenderForPseudoClass();
		SendEvent(MakeEvent(EventGetFocus), newFocus);
	}

	CurrentFocusID = (newFocus == nullptr) ? InternalIDNull : newFocus->GetInternalID();
}

Event DocUI::MakeEvent(Events evType) {
	Event ev;
	ev.Doc  = Doc;
	ev.Type = evType;
	return ev;
}

void DocUI::InvalidateRenderForPseudoClass() {
	// This is heavy-handed. Ideally we could inform the layout system that it doesn't need to re-evaluate styles.
	Doc->IncVersion();
}

void DocUI::SendEvent(const Event& ev, const DomNode* node, bool* handled, bool* stop) {
	bool                localHandled = false;
	bool                localStop    = false;
	Event               localEv      = ev;
	const EventHandler* h;
	size_t              hcount;
	node->GetHandlers(h, hcount);
	XOTRACE_EVENTS("Found %d event handlers\n", (int) hcount);
	for (size_t i = 0; i < hcount; i++) {
		if (h[i].Handles(ev.Type)) {
			XOTRACE_EVENTS("Dispatching event to %d/%d (%p)\n", (int) i, (int) hcount, h[i].Func);
			localHandled    = true;
			localEv.Context = h[i].Context;
			localEv.Target  = const_cast<DomNode*>(node);
			if (!h[i].Func(localEv)) {
				localStop = true;
				break;
			}
		}
		// re-acquire set of handlers. This is not perfect - but if the handler is altering the list of
		// handlers, then I'm not sure what the most sensible course of action is. "do not crash" is
		// a reasonable requirement though.
		node->GetHandlers(h, hcount);
	}
	if (handled != nullptr)
		*handled = localHandled;
	if (stop != nullptr)
		*stop = localStop;
}
}
