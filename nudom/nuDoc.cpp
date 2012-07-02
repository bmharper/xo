#include "pch.h"
#include "nuDoc.h"
#include "nuProcessor.h"
#include "nuLayout.h"
#include "Render/nuRenderer.h"

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

void nuDoc::RenderSync()
{
	UsableIDs += FreeIDs;
	FreeIDs.clear();
}

void nuDoc::CloneFastInto( nuDoc& c, uint cloneFlags ) const
{
	c.Reset();
	c.IsReadOnly = true;
	Root.CloneFastInto( c.Root, &c.Pool, cloneFlags );
	Styles.CloneFastInto( c.Styles, &c.Pool );
	c.WindowWidth = WindowWidth;
	c.WindowHeight = WindowHeight;
	c.Version = Version;
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
}

void nuDoc::ChildRemoved( nuDomEl* el )
{
	nuInternalID elID = el->GetInternalID();
	NUASSERT(el->GetDoc() == this);
	NUASSERT(elID != 0);
	ChildByInternalID[el->GetInternalID()] = NULL;
	el->SetDoc( NULL );
	el->SetInternalID( 0 );
	FreeIDs += elID;
}

void nuDoc::Reset()
{
	if ( IsReadOnly )
	{
		Root.Discard();
		Styles.Discard();
	}
	WindowWidth = 0;
	WindowHeight = 0;
	Version = 0;
	Pool.FreeAll();
	Root.SetInternalID( 0 );
	ResetInternalIDs();
}

void nuDoc::ResetInternalIDs()
{
	FreeIDs.clear();
	UsableIDs.clear();
	ChildByInternalID.clear();
	ChildByInternalID += NULL;	// zero is NULL
	ChildAdded( &Root );
}
