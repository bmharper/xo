#include "pch.h"
#include "xoDoc.h"
#include "xoDomEl.h"
#include "xoCloneHelpers.h"

xoDomEl::xoDomEl( xoDoc* doc, xoTag tag )
{
	Doc = doc;
	Tag = tag;
	InternalID = 0;
	Version = 0;
}

xoDomEl::~xoDomEl()
{
}

// all memory allocations come from the pool. The also happens to be recursive.
/*
void xoDomEl::CloneFastInto( xoDomEl& c, xoPool* pool, uint cloneFlags ) const
{
	xoDoc* cDoc = c.GetDoc();
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
	Style.CloneFastInto( c.Style, pool );

	XOASSERT(false); // Text?

	// copy classes
	xoClonePodvecWithMemCopy( c.Classes, Classes, pool );

	// alloc list of pointers to children
	xoClonePvectPrepare( c.Children, Children, pool );
	
	// alloc children
	for ( int i = 0; i < Children.size(); i++ )
		c.Children[i] = pool->AllocT<xoDomEl>( true );
	
	// copy children
	for ( int i = 0; i < Children.size(); i++ )
	{
		c.Children[i]->Doc = c.Doc;
		Children[i]->CloneFastInto( *c.Children[i], pool, cloneFlags );
	}

	if ( !!(cloneFlags & xoCloneFlagEvents) )
		XOPANIC("clone events is TODO");

	cDoc->ChildAddedFromDocumentClone( &c );
}
*/

void xoDomEl::IncVersion()
{
	Version++;
	Doc->SetChildModified( InternalID );
}

// memory allocations come from the regular heap. This also happens to not be recursive.
void xoDomEl::CloneSlowIntoBase( xoDomEl& c, uint cloneFlags ) const
{
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
}
