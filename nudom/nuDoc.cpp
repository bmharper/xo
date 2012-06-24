#include "pch.h"
#include "nuDoc.h"
#include "nuProcessor.h"
#include "nuLayout.h"
#include "nuRender.h"

nuDoc::nuDoc()
{
	IsReadOnly = false;
	AppearanceDirty = true;
	LayoutDirty = true;
	WindowWidth = WindowHeight = 0;
	Root.Tag = nuTagBody;
	ResetInternalIDs();
}

nuDoc::~nuDoc()
{
	Reset();
}

void nuDoc::InvalidateAppearance()
{
	AppearanceDirty = true;
}

void nuDoc::InvalidateLayout()
{
	LayoutDirty = true;
	InvalidateAppearance();
}

void nuDoc::AppearanceClean()
{
	AppearanceDirty = false;
}

void nuDoc::LayoutClean()
{
	LayoutDirty = false;
}

void nuDoc::CloneFastInto( nuDoc& c, uint cloneFlags ) const
{
	c.Reset();
	c.IsReadOnly = true;
	Root.CloneFastInto( c.Root, &c.Pool, cloneFlags );
	Styles.CloneFastInto( c.Styles, &c.Pool );
	c.WindowWidth = WindowWidth;
	c.WindowHeight = WindowHeight;
	c.LayoutDirty = false;
	c.AppearanceDirty = false;
}

void nuDoc::ChildAdded( nuDomEl* el )
{
	NUASSERT(el->Doc == NULL);
	NUASSERT(el->InternalID == 0);
	el->Doc = this;
	if ( UsableIDs.size() != 0 )
	{
		el->InternalID = UsableIDs.rpop();
		ChildByInternalID[el->InternalID] = el;
	}
	else
	{
		el->InternalID = NextID++;
		ChildByInternalID += el;
		NUASSERT(ChildByInternalID.size() == el->InternalID + 1);
	}
}

void nuDoc::ChildRemoved( nuDomEl* el )
{
	NUASSERT(el->Doc == this);
	NUASSERT(el->InternalID != 0);
	ChildByInternalID[el->InternalID] = NULL;
	el->Doc = NULL;
	el->InternalID = 0;
	FreeIDs += el->InternalID;
}

void nuDoc::Reset()
{
	if ( IsReadOnly )
	{
		Root.Discard();
		Styles.Reset();
	}
	WindowWidth = 0;
	WindowHeight = 0;
	LayoutDirty = false;
	AppearanceDirty = false;
	Pool.FreeAll();
	Root.Doc = NULL;
	Root.InternalID = 0;
	ResetInternalIDs();
}

void nuDoc::ResetInternalIDs()
{
	NextID = 1;
	FreeIDs.clear();
	UsableIDs.clear();
	ChildByInternalID.clear();
	ChildByInternalID += NULL;	// zero is NULL
	ChildAdded( &Root );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuRenderDoc::nuRenderDoc()
{
	AbcCriticalSectionInitialize( Lock );
	RenderRoot.SetPool( &RenderPool );
	DocWidth = 0;
	DocHeight = 0;
}

nuRenderDoc::~nuRenderDoc()
{
	AbcCriticalSectionDestroy( Lock );
}

void nuRenderDoc::ResetRenderData()
{
	RenderRoot.Discard();
	RenderPool.FreeAll();
}

void nuRenderDoc::Render( nuRenderGL* rgl )
{
	{
		TakeCriticalSection lock( Lock );
		ResetRenderData();
		nuLayout lay;
		lay.Layout( Doc, RenderRoot, &RenderPool );
		DocWidth = Doc.WindowWidth;
		DocHeight = Doc.WindowHeight;
	}

	nuRenderer rend;
	rend.Render( rgl, &RenderRoot, DocWidth, DocHeight );
}

void nuRenderDoc::UpdateDoc( const nuDoc& original )
{
	TakeCriticalSection lock( Lock );
	Doc.Reset();
	original.CloneFastInto( Doc, 0 );
}

