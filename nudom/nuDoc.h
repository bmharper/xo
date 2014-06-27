#pragma once

#include "Dom/nuDomNode.h"
#include "Dom/nuDomText.h"
#include "nuMem.h"
#include "nuStringTable.h"
#include "Image/nuImageStore.h"

/* Document.

Version
-------

Every time an event handler runs, we increment the document version number. This is a conservative
way of detecting any change to the document, without requiring that all document updates go via
some kind of accessor function. Our renderer compares its last version to our current version,
and if the two differ, it knows that it needs to update.

*/
class NUAPI nuDoc
{
	DISALLOW_COPY_AND_ASSIGN(nuDoc);
public:
	nuDomNode					Root;							// Root element of the document tree
	nuStyleTable				ClassStyles;					// All style classes defined in this document
	nuStyle						TagStyles[nuTagEND];			// Styles of tags. For example, the style of <p>, or the style of <h1>.
	nuStringTable				Strings;						// Generic string table.
	nuImageStore				Images;							// All images. Some day we may want to be able to share these amongst different documents.

					nuDoc();
					~nuDoc();
	void			Reset();
	void			IncVersion();
	uint32			GetVersion()		{ return Version; }									// Renderers use purposefully loose MT semantics on this.
	void			ResetModifiedBitmap();													// Reset the 'ismodified' bitmap of all DOM elements.
	void			MakeFreeIDsUsable();													// All of our dependent renderers have been updated, we can move FreeIDs over to UsableIDs.
	void			CloneSlowInto( nuDoc& c, uint cloneFlags, nuRenderStats& stats ) const;	// Used to make a read-only clone for the renderer. Preserves existing.
	//void			CloneFastInto( nuDoc& c, uint cloneFlags, nuRenderStats& stats ) const;	// Used to make a read-only clone for the renderer. Starts from scratch.

	// Style Classes
	bool			ClassParse( const char* klass, const char* style );		// Set the class, overwriting any previously set style

	nuDomEl*		AllocChild( nuTag tag );
	void			FreeChild( const nuDomEl* el );

	void			ChildAdded( nuDomEl* el );
	void			ChildAddedFromDocumentClone( nuDomEl* el );
	void			ChildRemoved( nuDomEl* el );
	void			SetChildModified( nuInternalID id );
	intp			ChildByInternalIDListSize() const				{ return ChildByInternalID.size(); }
	const nuDomEl**	ChildByInternalIDList() const					{ return (const nuDomEl**) ChildByInternalID.data; }
	const nuDomEl*	GetChildByInternalID( nuInternalID id ) const	{ return ChildByInternalID[id]; }						// A NULL result means this child has been deleted
	nuDomEl*		GetChildByInternalIDMutable( nuInternalID id )	{ return ChildByInternalID[id]; }

protected:
	volatile uint32				Version;
	nuPool						Pool;					// Used only when making a clone via CloneFast()
	bool						IsReadOnly;				// Read-only clone used for rendering
	pvect<nuDomEl*>				ChildByInternalID;
	BitMap						ChildIsModified;		// Bit is set if child has been modified since we last synced with the renderer
	podvec<nuInternalID>		UsableIDs;				// When we do a render sync, then FreeIDs are moved into UsableIDs
	podvec<nuInternalID>		FreeIDs;

	void	ResetInternalIDs();
	void	InitializeDefaultTagStyles();
};
