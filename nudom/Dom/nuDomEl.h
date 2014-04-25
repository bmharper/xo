#pragma once

#include "nuStyle.h"
#include "nuMem.h"
#include "nuEvent.h"

/* DOM node in the tree.
*/
class NUAPI nuDomEl
{
public:
					nuDomEl( nuDoc* doc, nuTag tag );
					virtual ~nuDomEl();

	virtual void			SetText( const char* txt ) = 0;		// Replace all children with a single nuTagText child, or set internal text if 'this' is nuTagText.
	virtual const char*		GetText() const = 0;				// Reverse behaviour of SetText()
	virtual void			CloneSlowInto( nuDomEl& c, uint cloneFlags ) const = 0;
	virtual void			ForgetChildren() = 0;

	nuInternalID					GetInternalID() const		{ return InternalID; }
	nuTag							GetTag() const				{ return Tag; }
	nuDoc*							GetDoc() const				{ return Doc; }
	uint32							GetVersion() const			{ return Version; }

	//void			CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const;

	void			SetInternalID( nuInternalID id )			{ InternalID = id; }	// Used by nuDoc at element creation time.
	void			SetDoc( nuDoc* doc )						{ Doc = doc; }			// Used by nuDoc at element creation and destruction time.
	void			SetDocRoot()								{ Tag = nuTagBody; }	// Used by nuDoc at initialization time.

protected:
	nuDoc*					Doc;			// Owning document
	nuInternalID			InternalID;		// Internal 32-bit ID that is used to keep track of an object (memory address is not sufficient)
	nuTag					Tag;			// Tag, such <div>, etc
	uint32					Version;		// Monotonic integer used to detect modified nodes

	void			IncVersion();
	void			CloneSlowIntoBase( nuDomEl& c, uint cloneFlags ) const;
};
