#include "pch.h"
#include "DocGroup.h"
#include "DocUI.h"
#include "Render/RenderDoc.h"
#include "Render/RenderDomEl.h"
#include "Render/RenderStack.h"
#include "Render/StyleResolve.h"

namespace xo {

DocUI::DocUI(xo::Doc* doc) {
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
		ViewportWidth  = (uint32_t) ev.PointsAbs[0].x;
		ViewportHeight = (uint32_t) ev.PointsAbs[0].y;
		Doc->IncVersion();
		//TimeTrace( "Processed WindowSize event. Document at version %d\n", Doc->GetVersion() );
		break;
	}

	// Give up processing any other events if we haven't run a layout yet
	if (layout == nullptr)
		return;

	ev.LayoutResult = layout;

	switch (ev.Type) {
	case EventTimer: {
		// Remember that any callback can do *anything* to our DOM, so we cannot assume that
		// anything is still alive between calls to different callbacks.
		int64_t                   nowTicksMS    = MilliTicks();
		uint32_t                  nowTicksMS_32 = MilliTicks_32(nowTicksMS);
		cheapvec<NodeEventIDPair> handlers;
		Doc->ReadyTimers(nowTicksMS, handlers);

		Event localEv = ev;
		for (NodeEventIDPair h : handlers) {
			DomNode* target = Doc->GetNodeByInternalIDMutable(h.NodeID);
			if (!target)
				continue;
			EventHandler* eh = target->HandlerByID(h.EventID);
			if (!eh)
				continue;
			localEv.Context              = eh->Context;
			localEv.Target               = target;
			localEv.IsCancelTimerToggled = false;
			eh->Func(localEv);
			// If the timer has destroyed it's owning DOM element, then 'eh' and 'target' will be dead.
			if (!Doc->GetNodeByInternalIDMutable(h.NodeID))
				continue;
			eh->TimerLastTickMS = nowTicksMS_32;
			if (localEv.IsCancelTimerToggled)
				target->RemoveHandler(h.EventID);
		}
		break;
	}
	case EventRender: {
		// Very similar principles apply here as with timers - we need to be very careful that objects
		// are still alive, at every step of the way.
		cheapvec<NodeEventIDPair> handlers;
		Doc->RenderHandlers(handlers);
		Event localEv = ev;
		for (NodeEventIDPair h : handlers) {
			DomNode* target = Doc->GetNodeByInternalIDMutable(h.NodeID);
			if (!target)
				continue;
			EventHandler* eh = target->HandlerByID(h.EventID);
			if (!eh)
				continue;
			localEv.Context = eh->Context;
			localEv.Target  = target;
			eh->Func(localEv);
		}
		break;
	}
	default:
		BubbleEvent(ev, layout);
	}
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

	if (ev.Type == EventKeyChar || ev.Type == EventKeyDown || ev.Type == EventKeyUp) {
		// TODO: build upward chain and do bubbling... Possibly downward bubbling for keyboard shortcut management?
		DomNode* focus = (DomNode*) Doc->GetChildByInternalID(CurrentFocusID);
		if (!focus)
			focus = &Doc->Root;
		SendEvent(ev, focus, &handled, &stop);
		return handled;
	}

	SelectorChain selChain;
	Vec2f         posInTarget;
	if (ev.Type == EventMouseLeave) {
		// When EventMouseLeave comes in from the window system, it means the cursor has left our
		// client area. This is not the same thing as the cursor leaving a DOM node.
		ev.Type = EventMouseMove;
	} else {
		FindTarget(ev.PointsAbs[0], layout, selChain);
	}

	if (ev.Type == EventMouseMove)
		UpdateCursorLocation(selChain);

	//XOTRACE_EVENTS( "FindTarget chainlen = %d\n", (int) nodeChain.size() );

	// Start at the inner-most node first.
	// Remember that SendEvent is allowed to do absolutely anything it wants to our DOM.
	// So it could be that the rest of the objects inside nodeChain are invalid once
	// SendEvent returns. We cater for this by always fetching DOM nodes based on InternalID.
	for (size_t inode = selChain.Nodes.size() - 1; inode != -1; inode--) {
		bool                 isDeepestNode = inode == selChain.Nodes.size() - 1;
		const RenderDomNode* rnode         = selChain.Nodes[inode];
		const DomNode*       node          = Doc->GetNodeByInternalID(rnode->InternalID);
		if (node != nullptr) {
			if (node->HandlesEvent(ev.Type)) {
				if (isDeepestNode && selChain.Text) {
					ev.TargetText = (DomText*) Doc->GetChildByInternalID(selChain.Text->InternalID);
					if (selChain.Glyph)
						ev.TargetChar = selChain.Glyph->OriginalCharIndex;
				} else {
					ev.TargetText = nullptr;
					ev.TargetChar = -1;
				}
				ev.PointsRel[0] = selChain.PosInNode[inode].ToReal();
				SendEvent(ev, node, &handled, &stop);
				if (stop)
					break;
			}
		}
	}

	if (ev.Type == EventClick)
		UpdateFocusWindow(selChain);

	return handled;
}

/* Given a point, return the chain of DOM elements (starting at the root) that leads down
to the inner-most DOM element beneath the cursor.

This code does not make provision for elements that are positioned outside of their parent,
such as relative-positioned or absolute-positioned.
*/
void DocUI::FindTarget(Vec2f p, const LayoutResult* layout, SelectorChain& selChain) {
	Point                pos  = {RealToPos(p.x), RealToPos(p.y)};
	const RenderDomNode* body = layout->Body();
	if (!body->BorderBox().IsInsideMe(pos))
		return;
	size_t stackPos = 0;
	selChain.Nodes.push(body);
	selChain.PosInNode.push(pos - body->Pos.TopLeft());
	// The condition "stackPos < nodeChain.size()" is just another way of saying "we have a deeper node to recurse into"
	while (stackPos < selChain.Nodes.size()) {
		const RenderDomNode* top    = selChain.Nodes[stackPos];
		Point                relPos = selChain.PosInNode[stackPos];
		stackPos++;
		// Walk backwards, yielding implicit z-order from child order.
		// Pick the last (ie the top-most) child who's border-box contains
		// this point, and continue recursing down into that node.
		for (size_t i = top->Children.size() - 1; i != -1; i--) {
			if (top->Children[i]->IsNode()) {
				const RenderDomNode* childNode = static_cast<const RenderDomNode*>(top->Children[i]);
				if (childNode->BorderBox().IsInsideMe(relPos)) {
					selChain.Nodes.push(childNode);
					selChain.PosInNode.push(relPos - childNode->Pos.TopLeft());
					// Only allow a single path to the inner-most object.
					// This code would need to be extended to handle explicit z-order
					break;
				}
			} else if (top->Children[i]->IsText()) {
				const RenderDomText* childTxt = static_cast<const RenderDomText*>(top->Children[i]);
				if (childTxt->Pos.IsInsideMe(relPos)) {
					// Find the nearest glyph inside this rendertext object.
					// Because we're not adding anything new to selChain.Nodes here,
					// this is the final stop in our walk down the DOM tree.
					selChain.PosInNode.push(relPos - childTxt->Pos.TopLeft());
					Point relPosToText = selChain.PosInNode.back();
					selChain.Text      = childTxt;
					for (size_t j = 0; j < childTxt->Text.size(); j++) {
						const auto& g = childTxt->Text[j];
						if (g.X <= relPosToText.X && relPosToText.X < g.X + g.Width) {
							selChain.Glyph = &g;
							break;
						}
					}
					break;
				}
			}
		}
	}
}

void DocUI::UpdateCursorLocation(const SelectorChain& selChain) {
	// Update Hover states, send mouse leave and mouse enter events.
	bool                   anyHoverChanges = false;
	ohash::set<InternalID> oldNodeIDs, newNodeIDs;
	const auto&            nodeChain = selChain.Nodes;

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

void DocUI::UpdateFocusWindow(const SelectorChain& selChain) {
	// Find the window that should receive the focus
	const DomNode* newFocus = nullptr;
	for (size_t inode = selChain.Nodes.size() - 1; inode != -1; inode--) {
		const RenderDomNode* rnode = selChain.Nodes[inode];
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

void DocUI::SendEvent(const Event& ev, const DomNode* target, bool* handled, bool* stop) {
	bool       localHandled = false;
	bool       localStop    = false;
	Event      localEv      = ev;
	auto       doc          = target->GetDoc();
	InternalID targetID     = target->GetInternalID();
	localEv.Target          = const_cast<DomNode*>(target);
	const EventHandler* h;
	size_t              hcount;
	target->GetHandlers(h, hcount);
	XOTRACE_EVENTS("Found %d event handlers\n", (int) hcount);
	for (size_t i = 0; i < hcount; i++) {
		if (h[i].Handles(ev.Type)) {
			XOTRACE_EVENTS("Dispatching event to %d/%d (%p)\n", (int) i, (int) hcount, h[i].Func);
			localHandled                     = true;
			localEv.IsStopPropagationToggled = false;
			localEv.Context                  = h[i].Context;
			h[i].Func(localEv);
			if (localEv.IsStopPropagationToggled) {
				localStop = true;
				break;
			}
			// The handler may have deleted the DOM element itself, so cancel all other events if that has happened.
			if (!doc->GetChildByInternalID(targetID))
				break;
		}
		// re-acquire set of handlers. This is not perfect - but if the handler is altering the list of
		// handlers, then I'm not sure what the most sensible course of action is. "do not crash" is
		// a reasonable requirement though.
		target->GetHandlers(h, hcount);
	}
	if (handled != nullptr)
		*handled = localHandled;
	if (stop != nullptr)
		*stop = localStop;
}
}
