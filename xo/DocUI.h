#pragma once
#include "Defs.h"
#include "Event.h"
#include "Style.h"

namespace xo {

// Chain of objects, always starting at document root, and following down a path of
// children, until we reach a final entity, which can either be a single glyph, or
// a DOM node. This is the output of the function that answers the question
// "what is underneath the mouse cursor".
struct XO_API SelectorChain {
	cheapvec<const RenderDomNode*> Nodes;           // Parallel to PosInNode
	cheapvec<Point>                PosInNode;       // Parallel to Nodes - contains position relative to node content box top-left
	const RenderDomText*           Text  = nullptr; // This is not necessarily populated
	const RenderCharEl*            Glyph = nullptr; // This is not necessarily populated
};

/* Document UI state.
Platform-level events are fed into this class, and inside here we generate
events for DOM elements. We also maintain the list of objects underneath the cursor, as well
as other things like which element currently has the keyboard focus.
*/
class XO_API DocUI {
public:
	DocUI(Doc* doc);
	~DocUI();

	void InternalProcessEvent(Event& ev, const LayoutResult* layout); // This is always called from the UI thread. Do not call this yourself. It is called only by DocGroup::ProcessEvent()
	void CloneSlowInto(DocUI& c) const;
	void DispatchDocLifecycle();

	uint32_t GetViewportWidth() const { return ViewportWidth; }
	uint32_t GetViewportHeight() const { return ViewportHeight; }

	bool    IsHovering(InternalID id) const { return HoverSet.contains(id); }
	bool    IsFocused(InternalID id) const { return CurrentFocusID == id; }
	bool    IsCaptured(InternalID id) const { return CurrentCaptureID == id; }
	Cursors GetCursor() const { return Cursor; }

	// Capture input, so that all UI events are dispatched only to this node, until ReleaseCapture is called.
	void SetCapture(InternalID id);
	// Release input capture. This does nothing if 'id' is not the item that currently holds the input capture
	void ReleaseCapture(InternalID id);

protected:
	struct HoverNode {
		xo::InternalID InternalID;    // Element beneath the cursor
		bool           HasHoverStyle; // Appearance of element beneath cursor depends upon cursor location
	};

	xo::Doc*               Doc;
	InternalID             CurrentFocusID   = InternalIDNull; // Element that has the keyboard focus
	InternalID             CurrentCaptureID = InternalIDNull; // Element that has the input captured
	InternalID             MouseDownID[NumMouseButtons];      // Element where a mouse button went down
	cheapvec<HoverNode>    HoverNodes;
	ohash::set<InternalID> HoverSet;
	uint32_t               ViewportWidth, ViewportHeight; // Device pixels

	// Cursor computed due to most recent mouse move message. Why volatile?
	// This is volatile so that we can read and write the cursor from any thread.
	// Look at the handling of WM_SETCURSOR to see why this is necessary. Basically,
	// WM_SETCURSOR needs to be responded to immediately. It can't be put onto a queue
	// and handled by another thread.
	volatile Cursors Cursor;

	bool  ProcessInputEvent(Event& ev, const LayoutResult* layout);
	bool  BubbleEvent(int nEvents, Event* events, SelectorChain& chain, const LayoutResult* layout);
	void  FindTarget(Vec2f p, const LayoutResult* layout, SelectorChain& selChain);
	void  SetupChainForDeepNode(InternalID deepNode, Vec2f p, const LayoutResult* layout, SelectorChain& selChain);
	void  UpdateCursorLocation(const SelectorChain& selChain);
	void  UpdateFocusWindow(const SelectorChain& selChain);
	Event MakeEvent(Events evType);
	void  InvalidateRenderForPseudoClass();
	bool  CanReceiveInputEvents(InternalID nodeID);
	void  RobustDispatchEventToHandlers(const Event& ev, const cheapvec<NodeEventIDPair>& handlers);

	static void SendEvent(const Event& ev, const DomNode* target, bool* handled = nullptr, bool* stop = nullptr);
};
} // namespace xo
