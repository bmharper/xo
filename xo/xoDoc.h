#pragma once

#include "Dom/xoDomNode.h"
#include "Dom/xoDomText.h"
#include "xoMem.h"
#include "xoStringTable.h"
#include "Image/xoImageStore.h"
#include "xoDocUI.h"

/* Document.

Version
-------
Every time an event handler runs, we increment the document version number. This is a conservative
way of detecting any change to the document, without requiring that all document updates go via
some kind of accessor function. Our renderer compares its last version to our current version,
and if the two differ, it knows that it needs to update.

*/
class XOAPI xoDoc
{
	DISALLOW_COPY_AND_ASSIGN(xoDoc);
public:
	xoDomNode					Root;							// Root element of the document tree
	xoStyleTable				ClassStyles;					// All style classes defined in this document
	xoStyle						TagStyles[xoTagEND];			// Styles of tags. For example, the style of <p>, or the style of <h1>.
	xoStringTable				Strings;						// Generic string table.
	xoImageStore				Images;							// All images. Some day we may want to be able to share these amongst different documents.
	xoDocUI						UI;								// UI state (which element has the focus, over which elements is the cursor, etc)

						xoDoc();
						~xoDoc();
	void				Reset();
	void				IncVersion();
	uint32				GetVersion()		{ return Version; }									// Renderers use purposefully loose MT semantics on this.
	void				ResetModifiedBitmap();													// Reset the 'ismodified' bitmap of all DOM elements.
	void				MakeFreeIDsUsable();													// All of our dependent renderers have been updated, we can move FreeIDs over to UsableIDs.
	void				CloneSlowInto( xoDoc& c, uint cloneFlags, xoRenderStats& stats ) const;	// Used to make a read-only clone for the renderer. Preserves existing.
	//void				CloneFastInto( xoDoc& c, uint cloneFlags, xoRenderStats& stats ) const;	// Used to make a read-only clone for the renderer. Starts from scratch.

	// Style Classes
	bool				ClassParse( const char* klass, const char* style );		// Set the class, overwriting any previously set style

	xoDomEl*			AllocChild( xoTag tag, xoInternalID parentID );
	void				FreeChild( const xoDomEl* el );

	xoString			Parse( const char* src ); // Set the entire document from a single xml-like string. Returns empty string on success, or error message.

	void				ChildAdded( xoDomEl* el );
	//void				ChildAddedFromDocumentClone( xoDomEl* el );
	void				ChildRemoved( xoDomEl* el );
	void				SetChildModified( xoInternalID id );
	intp				ChildByInternalIDListSize() const				{ return ChildByInternalID.size(); }
	const xoDomEl**		ChildByInternalIDList() const					{ return (const xoDomEl**) ChildByInternalID.data; }
	const xoDomEl*		GetChildByInternalID( xoInternalID id ) const	{ return ChildByInternalID[id]; }						// A NULL result means this child has been deleted
	const xoDomNode*	GetNodeByInternalID( xoInternalID id ) const	{ return ChildByInternalID[id] ? ChildByInternalID[id]->ToNode() : nullptr; }
	xoDomEl*			GetChildByInternalIDMutable( xoInternalID id )	{ return ChildByInternalID[id]; }
	xoDomNode*			GetNodeByInternalIDMutable( xoInternalID id )	{ return ChildByInternalID[id] ? ChildByInternalID[id]->ToNode() : nullptr; }

protected:
	volatile uint32				Version;
	xoPool						Pool;					// Used only when making a clone via CloneFast()
	bool						IsReadOnly;				// Read-only clone used for rendering
	pvect<xoDomEl*>				ChildByInternalID;
	BitMap						ChildIsModified;		// Bit is set if child has been modified since we last synced with the renderer
	podvec<xoInternalID>		UsableIDs;				// When we do a render sync, then FreeIDs are moved into UsableIDs
	podvec<xoInternalID>		FreeIDs;

	void	ResetInternalIDs();
	void	InitializeDefaultTagStyles();
};
