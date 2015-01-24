#pragma once

#include "../xoStyle.h"
#include "../xoMem.h"
#include "../xoEvent.h"

/* DOM node in the tree.
*/
class XOAPI xoDomEl
{
public:
	xoDomEl(xoDoc* doc, xoTag tag, xoInternalID parentID = xoInternalIDNull);
	virtual ~xoDomEl();

	virtual void			SetText(const char* txt) = 0;		// Replace all children with a single xoTagText child, or set internal text if 'this' is xoTagText.
	virtual const char*		GetText() const = 0;				// Reverse behaviour of SetText()
	virtual void			CloneSlowInto(xoDomEl& c, uint cloneFlags) const = 0;
	virtual void			ForgetChildren() = 0;

	xoInternalID			GetInternalID() const		{ return InternalID; }
	xoTag					GetTag() const				{ return Tag; }
	xoDoc*					GetDoc() const				{ return Doc; }
	uint32					GetVersion() const			{ return Version; }
	xoInternalID			GetParentID() const			{ return ParentID; }
	const xoDomNode*		GetParent() const;

	xoDomNode*				ToNode();
	xoDomText*				ToText();
	const xoDomNode*		ToNode() const;
	const xoDomText*		ToText() const;

	//void					CloneFastInto( xoDomEl& c, xoPool* pool, uint cloneFlags ) const;

	void					SetInternalID(xoInternalID id)			{ InternalID = id; }	// Used by xoDoc during element creation.
	void					SetDoc(xoDoc* doc)						{ Doc = doc; }			// Used by xoDoc during element creation and destruction.
	bool					IsNode() const							{ return Tag != xoTagText; }
	bool					IsText() const							{ return Tag == xoTagText; }

protected:
	xoDoc*					Doc;				// Owning document
	xoInternalID			ParentID;			// Owning node
	xoInternalID			InternalID = 0;		// Internal 32-bit ID that is used to keep track of an object (memory address is not sufficient)
	xoTag					Tag;				// Tag, such <div>, etc
	uint32					Version = 0;		// Monotonic integer used to detect modified nodes

	void					IncVersion();
	void					CloneSlowIntoBase(xoDomEl& c, uint cloneFlags) const;
};
