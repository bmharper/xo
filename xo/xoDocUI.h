#pragma once

#include "xoDefs.h"
#include "xoEvent.h"
#include "xoStyle.h"

/* Document UI state.
Platform-level events are fed into this class, and inside here we generate
events for DOM elements. We also maintain the list of objects underneath the cursor, as well
as other things like which element currently has the keyboard focus.
*/
class XOAPI xoDocUI
{
public:
	xoDocUI(xoDoc* doc);
	~xoDocUI();

	void				InternalProcessEvent(xoEvent& ev, const xoLayoutResult* layout);		// This is always called from the UI thread. Do not call this yourself. It is called only by DocGroup::ProcessEvent()
	void				CloneSlowInto(xoDocUI& c) const;

	uint32				GetViewportWidth() const			{ return ViewportWidth; }
	uint32				GetViewportHeight() const			{ return ViewportHeight; }

	bool				IsHovering(xoInternalID id) const	{ return HoverSet.contains(id); }
	bool				IsFocused(xoInternalID id) const	{ return CurrentFocusID == id; }
	xoCursors			GetCursor() const					{ return Cursor; }

protected:
	struct HoverNode
	{
		xoInternalID	InternalID;		// Element beneath the cursor
		bool			HasHoverStyle;	// Appearance of element beneath cursor depends upon cursor location
	};

	xoDoc*					Doc;
	xoInternalID			CurrentFocusID = xoInternalIDNull;	// Element that has the keyboard focus
	podvec<HoverNode>		HoverNodes;
	fhashset<xoInternalID>	HoverSet;
	uint32					ViewportWidth, ViewportHeight;		// Device pixels

	// Cursor computed due to most recent mouse move message. Why volatile?
	// This is volatile so that we can read and write the cursor from any thread.
	// Look at the handling of WM_SETCURSOR to see why this is necessary. Basically,
	// WM_SETCURSOR needs to be responded to immediately. It can't be put onto a queue
	// and handled by another thread.
	volatile xoCursors		Cursor;

	bool			BubbleEvent(xoEvent& ev, const xoLayoutResult* layout);
	void			FindTarget(xoVec2f p, pvect<const xoRenderDomNode*>& nodeChain, const xoLayoutResult* layout);
	void			UpdateCursorLocation(const pvect<const xoRenderDomNode*>& nodeChain);
	void			UpdateFocusWindow(const pvect<const xoRenderDomNode*>& nodeChain);
	xoEvent			MakeEvent(xoEvents evType);
	void			InvalidateRenderForPseudoClass();

	static void		SendEvent(const xoEvent& ev, const xoDomNode* node, bool* handled = nullptr, bool* stop = nullptr);
};
