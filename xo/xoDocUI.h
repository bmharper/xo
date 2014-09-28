#pragma once

#include "xoDefs.h"
#include "xoEvent.h"

// Document UI state
class XOAPI xoDocUI
{
public:
						xoDocUI( xoDoc* doc );
						~xoDocUI();

	void				InternalProcessEvent( xoEvent& ev, const xoLayoutResult* layout );		// This is always called from the UI thread. Do not call this yourself. It is called only by DocGroup::ProcessEvent()
	void				CloneSlowInto( xoDocUI& c ) const;

	uint32				GetViewportWidth() const	{ return ViewportWidth; }
	uint32				GetViewportHeight() const	{ return ViewportHeight; }
	
	bool				IsHovering( xoInternalID id ) const { return HoverSet.contains(id); }
	bool				IsFocused( xoInternalID id ) const	{ return CurrentFocusID == id; }

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

	bool			BubbleEvent( xoEvent& ev, const xoLayoutResult* layout );
	void			FindTarget( xoVec2f p, pvect<const xoRenderDomNode*>& nodeChain, const xoLayoutResult* layout );
	void			UpdateCursorLocation( const pvect<const xoRenderDomNode*>& nodeChain );
	void			UpdateFocusWindow( const pvect<const xoRenderDomNode*>& nodeChain );
	xoEvent			MakeEvent( xoEvents evType );
	void			InvalidateRenderForPseudoClass();
	
	static void		SendEvent( const xoEvent& ev, const xoDomNode* node, bool* handled = nullptr, bool* stop = nullptr );
};
