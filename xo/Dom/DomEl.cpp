#include "pch.h"
#include "Doc.h"
#include "DomEl.h"
#include "DomNode.h"
#include "DomText.h"
#include "CloneHelpers.h"

namespace xo {

DomEl::DomEl(Doc* doc, Tag tag, InternalID parentID)
    : Doc(doc), Tag(tag), ParentID(parentID) {
}

DomEl::~DomEl() {
}

const DomNode* DomEl::GetParent() const {
	return Doc->GetNodeByInternalID(ParentID);
}

DomNode* DomEl::ToNode() {
	return IsNode() ? static_cast<DomNode*>(this) : nullptr;
}

DomText* DomEl::ToText() {
	return IsText() ? static_cast<DomText*>(this) : nullptr;
}

const DomNode* DomEl::ToNode() const {
	return IsNode() ? static_cast<const DomNode*>(this) : nullptr;
}

const DomText* DomEl::ToText() const {
	return IsText() ? static_cast<const DomText*>(this) : nullptr;
}

// all memory allocations come from the pool. The also happens to be recursive.
/*
void DomEl::CloneFastInto( DomEl& c, Pool* pool, uint32_t cloneFlags ) const
{
	Doc* cDoc = c.GetDoc();
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
	Style.CloneFastInto( c.Style, pool );

	XO_ASSERT(false); // Text?

	// copy classes
	ClonePodvecWithMemCopy( c.Classes, Classes, pool );

	// alloc list of pointers to children
	ClonePvectPrepare( c.Children, Children, pool );

	// alloc children
	for ( int i = 0; i < Children.size(); i++ )
		c.Children[i] = pool->AllocT<DomEl>( true );

	// copy children
	for ( int i = 0; i < Children.size(); i++ )
	{
		c.Children[i]->Doc = c.Doc;
		Children[i]->CloneFastInto( *c.Children[i], pool, cloneFlags );
	}

	if ( !!(cloneFlags & CloneFlagEvents) )
		XO_DIE_MSG("clone events is TODO");

	cDoc->ChildAddedFromDocumentClone( &c );
}
*/

void DomEl::IncVersion() {
	Version++;
	Doc->SetChildModified(InternalID);
}

// memory allocations come from the regular heap. This also happens to not be recursive.
void DomEl::CloneSlowIntoBase(DomEl& c, uint32_t cloneFlags) const {
	c.InternalID = InternalID;
	c.Tag        = Tag;
	c.Version    = Version;
}
}
