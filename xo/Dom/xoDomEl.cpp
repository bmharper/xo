#include "pch.h"
#include "nuDoc.h"
#include "nuDomEl.h"
#include "nuCloneHelpers.h"

nuDomEl::nuDomEl( nuDoc* doc, nuTag tag )
{
	Doc = doc;
	Tag = tag;
	InternalID = 0;
	Version = 0;
}

nuDomEl::~nuDomEl()
{
}

// all memory allocations come from the pool. The also happens to be recursive.
/*
void nuDomEl::CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const
{
	nuDoc* cDoc = c.GetDoc();
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
	Style.CloneFastInto( c.Style, pool );

	NUASSERT(false); // Text?

	// copy classes
	nuClonePodvecWithMemCopy( c.Classes, Classes, pool );

	// alloc list of pointers to children
	nuClonePvectPrepare( c.Children, Children, pool );
	
	// alloc children
	for ( int i = 0; i < Children.size(); i++ )
		c.Children[i] = pool->AllocT<nuDomEl>( true );
	
	// copy children
	for ( int i = 0; i < Children.size(); i++ )
	{
		c.Children[i]->Doc = c.Doc;
		Children[i]->CloneFastInto( *c.Children[i], pool, cloneFlags );
	}

	if ( !!(cloneFlags & nuCloneFlagEvents) )
		NUPANIC("clone events is TODO");

	cDoc->ChildAddedFromDocumentClone( &c );
}
*/

void nuDomEl::IncVersion()
{
	Version++;
	Doc->SetChildModified( InternalID );
}

// memory allocations come from the regular heap. This also happens to not be recursive.
void nuDomEl::CloneSlowIntoBase( nuDomEl& c, uint cloneFlags ) const
{
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
}
