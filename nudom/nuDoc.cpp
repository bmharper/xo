#include "pch.h"
#include "nuDoc.h"
#include "nuDocGroup.h"
#include "nuLayout.h"
#include "Render/nuRenderer.h"
#include "nuCloneHelpers.h"

nuDoc::nuDoc()
{
	IsReadOnly = false;
	Version = 0;
	WindowWidth = WindowHeight = 0;
	Root.SetDoc( this );
	Root.SetDocRoot();
	ResetInternalIDs();
}

nuDoc::~nuDoc()
{
	// TODO: Ensure that all of your events in the process-wide event queue have been dealt with,
	// because the event processor is going to try to access this doc.
	Reset();
}

void nuDoc::IncVersion()
{
	Version++;
}

void nuDoc::ResetModifiedBitmap()
{
	if ( ChildIsModified.Size() > 0 )
		ChildIsModified.Fill( 0, ChildIsModified.Size() - 1, false );
}

void nuDoc::MakeFreeIDsUsable()
{
	UsableIDs += FreeIDs;
	FreeIDs.clear();
}

void nuDoc::CloneFastInto( nuDoc& c, uint cloneFlags, nuRenderStats& stats ) const
{
	// this code path died...
	NUASSERT(false);

	//c.Reset();

	/*
	if ( c.ChildByInternalID.size() != ChildByInternalID.size() )
		c.ChildByInternalID.resize( ChildByInternalID.size() );
	c.ChildByInternalID.nullfill();
	Root.CloneFastInto( c.Root, &c.Pool, cloneFlags );
	
	ClassStyles.CloneFastInto( c.ClassStyles, &c.Pool );
	nuCloneStaticArrayWithCloneFastInto( c.TagStyles, TagStyles, &c.Pool );
	*/
}

// This clones only the objects that are marked as modified.
void nuDoc::CloneSlowInto( nuDoc& c, uint cloneFlags, nuRenderStats& stats ) const
{
	c.IsReadOnly = true;

	// Make sure the destination is large enough to hold all of our children
	while ( c.ChildByInternalID.size() < ChildByInternalID.size() )
		c.ChildByInternalID += nuInternalIDNull;

	// Although it would be trivial to parallelize the following two passes, I think it is unlikely to be worth it,
	// since I believe these passes are bandwidth limited.

	// Pass 1: Ensure that all objects that are present in the source document have a valid pointer in the target document
	for ( int i = 0; i < ChildIsModified.Size(); i++ )
	{
		if ( ChildIsModified.Get(i) )
		{
			stats.Clone_NumEls++;
			const nuDomEl* src = GetChildByInternalID( i );
			nuDomEl* dst = c.GetChildByInternalIDMutable( i );
			if ( src && !dst )
			{
				// create in destination
				c.ChildByInternalID[i] = c.AllocChild();
			}
			else if ( !src && dst )
			{
				// destroy destination. Make it forget its children, because this loop takes care of all elements.
				dst->ForgetChildren();
				c.FreeChild( dst );
				c.ChildByInternalID[i] = nuInternalIDNull;
			}
		}
	}
	
	// Pass 2: Clone the contents of all our modified objects into our target
	for ( int i = 0; i < ChildIsModified.Size(); i++ )
	{
		if ( ChildIsModified.Get(i) )
		{
			const nuDomEl* src = GetChildByInternalID( i );
			nuDomEl* dst = c.GetChildByInternalIDMutable( i );
			if ( src )
				src->CloneSlowInto( *dst, cloneFlags );
		}
	}

	ClassStyles.CloneSlowInto( c.ClassStyles );
	nuCloneStaticArrayWithCloneSlowInto( c.TagStyles, TagStyles );

	c.WindowWidth = WindowWidth;
	c.WindowHeight = WindowHeight;
	c.Version = Version;
}

nuDomEl* nuDoc::AllocChild()
{
	// we may want to use a more specialized heap in future, so we keep this path strict
	return new nuDomEl();
}

void nuDoc::FreeChild( const nuDomEl* el )
{
	// we may want to use a more specialized heap in future, so we keep this path strict
	delete el;
}

void nuDoc::ChildAdded( nuDomEl* el )
{
	NUASSERT(el->GetDoc() == this);
	NUASSERT(el->GetInternalID() == 0);
	if ( UsableIDs.size() != 0 )
	{
		el->SetInternalID( UsableIDs.rpop() );
		ChildByInternalID[el->GetInternalID()] = el;
	}
	else
	{
		el->SetInternalID( (nuInternalID) ChildByInternalID.size() );
		ChildByInternalID += el;
	}
	SetChildModified( el->GetInternalID() );
}

void nuDoc::ChildAddedFromDocumentClone( nuDomEl* el )
{
	nuInternalID elID = el->GetInternalID();
	NUASSERTDEBUG(elID != 0);
	NUASSERTDEBUG(elID < ChildByInternalID.size());		// The clone should have resized ChildByInternalID before copying the DOM elements
	ChildByInternalID[elID] = el;
}

void nuDoc::ChildRemoved( nuDomEl* el )
{
	nuInternalID elID = el->GetInternalID();
	NUASSERT(elID != 0);
	NUASSERT(el->GetDoc() == this);
	IncVersion();
	SetChildModified( elID );
	ChildByInternalID[elID] = NULL;
	el->SetDoc( NULL );
	el->SetInternalID( nuInternalIDNull );
	FreeIDs += elID;
}

void nuDoc::SetChildModified( nuInternalID id )
{
	ChildIsModified.SetAutoGrow( id, true, false );
	IncVersion();
}

void nuDoc::Reset()
{
	/*
	if ( IsReadOnly )
	{
		Root.Discard();
		ClassStyles.Discard();
	}
	*/
	WindowWidth = 0;
	WindowHeight = 0;
	Version = 0;
	Pool.FreeAll();
	Root.SetInternalID( nuInternalIDNull );	// Root will be assigned nuInternalIDRoot when we call ChildAdded() on it.
	ChildIsModified.Clear();
	ResetInternalIDs();
}

void nuDoc::ResetInternalIDs()
{
	FreeIDs.clear();
	UsableIDs.clear();
	ChildByInternalID.clear();
	ChildByInternalID += NULL;	// zero is NULL
	ChildAdded( &Root );
	NUASSERT( Root.GetInternalID() == nuInternalIDRoot );
}
