#pragma once

#include "nuDomEl.h"
#include "nuMem.h"
#include "nuStringTable.h"

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
public:
	nuDomEl							Root;							// Root element of the document tree
	//nuStyleSet						Styles;							// All styles defined in this document
	nuStyleTable					Styles;							// All styles defined in this document
	uint32							WindowWidth, WindowHeight;		// Device pixels
	nuStringTable					Strings;						// Generic string table

					nuDoc();
					~nuDoc();
	void			Reset();
	void			IncVersion();
	uint32			GetVersion()		{ return Version; }					// Renderers use purposefully loose MT semantics on this.
	void			RenderSync();											// All of our dependent renderers have been updated, we can move FreeIDs over to UsableIDs.
	void			CloneFastInto( nuDoc& c, uint cloneFlags ) const;		// Used to make a read-only clone for the renderer.
	void			ChildAdded( nuDomEl* el );
	void			ChildRemoved( nuDomEl* el );

protected:
	volatile uint32				Version;
	nuPool						Pool;					// Used only when making a clone via CloneFast()
	bool						IsReadOnly;				// Read-only clone used for rendering
	pvect<nuDomEl*>				ChildByInternalID;
	podvec<nuInternalID>		UsableIDs;				// When we do a render sync, then FreeIDs are moved into UsableIDs
	podvec<nuInternalID>		FreeIDs;

	void	ResetInternalIDs();
};
