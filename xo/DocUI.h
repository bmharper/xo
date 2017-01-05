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
	cheapvec<const RenderDomNode*> Nodes;
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

	uint32_t GetViewportWidth() const { return ViewportWidth; }
	uint32_t GetViewportHeight() const { return ViewportHeight; }

	bool    IsHovering(InternalID id) const { return HoverSet.contains(id); }
	bool    IsFocused(InternalID id) const { return CurrentFocusID == id; }
	Cursors GetCursor() const { return Cursor; }

protected:
	struct HoverNode {
		InternalID InternalID;    // Element beneath the cursor
		bool       HasHoverStyle; // Appearance of element beneath cursor depends upon cursor location
	};

	Doc*                   Doc;
	InternalID             CurrentFocusID = InternalIDNull; // Element that has the keyboard focus
	cheapvec<HoverNode>    HoverNodes;
	ohash::set<InternalID> HoverSet;
	uint32_t               ViewportWidth, ViewportHeight; // Device pixels

	// Cursor computed due to most recent mouse move message. Why volatile?
	// This is volatile so that we can read and write the cursor from any thread.
	// Look at the handling of WM_SETCURSOR to see why this is necessary. Basically,
	// WM_SETCURSOR needs to be responded to immediately. It can't be put onto a queue
	// and handled by another thread.
	volatile Cursors Cursor;

	bool  BubbleEvent(Event& ev, const LayoutResult* layout);
	void  FindTarget(Vec2f p, SelectorChain& selChain, const LayoutResult* layout);
	void  UpdateCursorLocation(const SelectorChain& selChain);
	void  UpdateFocusWindow(const SelectorChain& selChain);
	Event MakeEvent(Events evType);
	void  InvalidateRenderForPseudoClass();

	static void SendEvent(const Event& ev, const DomNode* target, const SelectorChain* selChain = nullptr, bool* handled = nullptr, bool* stop = nullptr);
};
}
