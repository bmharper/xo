#include "pch.h"
#include "../Layout/xoLayout.h"
#include "../Layout/xoLayout2.h"
#include "xoRenderDoc.h"
#include "xoRenderer.h"
#include "xoRenderDX.h"
#include "xoRenderGL.h"

xoLayoutResult::xoLayoutResult( const xoDoc& doc )
{
	IsLocked = false;
	Root.SetPool( &Pool );
	Root.InternalID = doc.Root.GetInternalID();
}

xoLayoutResult::~xoLayoutResult()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoRenderDoc::xoRenderDoc()
{
	AbcCriticalSectionInitialize( LayoutLock );
}

xoRenderDoc::~xoRenderDoc()
{
	for ( u32 iter = 0; true; iter++ )
	{
		TakeCriticalSection lock( LayoutLock );
		if ( LatestLayout != nullptr && !LatestLayout->IsLocked )
		{
			delete LatestLayout;
			LatestLayout = nullptr;
		}
		PurgeOldLayouts();
		if ( LatestLayout == nullptr && OldLayouts.size() == 0 )
			break;
		if ( iter % 500 == 0 )
			XOTRACE( "xoRenderDoc waiting for layouts to be released\n" );
		AbcSleep( 1 );
	}
	AbcCriticalSectionDestroy( LayoutLock );
}

//void xoRenderDoc::ResetRenderData()
//{
//	RenderRoot.Discard();
//	RenderPool.FreeAll();
//	RenderRoot.InternalID = Doc.Root.GetInternalID();
//}

xoRenderResult xoRenderDoc::Render( xoRenderBase* driver )
{
	//XOTRACE_RENDER( "RenderDoc: Reset\n" );
	//ResetRenderData();

	xoLayoutResult* layout = new xoLayoutResult( Doc );
	
	XOTRACE_RENDER( "RenderDoc: Layout\n" );
	xoLayout2 lay;
	lay.Layout( Doc, layout->Root, &layout->Pool );

	XOTRACE_RENDER( "RenderDoc: Render\n" );
	xoRenderer rend;
	xoRenderResult res = rend.Render( &Doc, &ClonedImages, &Doc.Strings, driver, &layout->Root );

	// Atomically publish the new layout
	{
		TakeCriticalSection lock( LayoutLock );
		PurgeOldLayouts();
		if ( LatestLayout != nullptr )
		{
			if ( LatestLayout->IsLocked )
				OldLayouts += LatestLayout;
			else
				delete LatestLayout;
		}
		LatestLayout = layout;
	}

	return res;
}

void xoRenderDoc::CopyFromCanonical( const xoDoc& canonical, xoRenderStats& stats )
{
	// Find nodes that have changed, so that we can apply transitions
	//ModifiedNodeIDs.clear();
	//FindAlteredNodes( &Doc, &canonical, ModifiedNodeIDs );

	canonical.CloneSlowInto( Doc, 0, stats );

	// TODO: Don't do this dumb copying.
	//ClonedStrings.CloneFrom( canonical.Strings );
	ClonedImages.CloneFrom( canonical.Images );
}

xoLayoutResult*	xoRenderDoc::AcquireLatestLayout()
{
	TakeCriticalSection lock( LayoutLock );

	if ( LatestLayout == nullptr )
		return nullptr;

	XOASSERT( !LatestLayout->IsLocked );
	LatestLayout->IsLocked = true;

	return LatestLayout;
}

void xoRenderDoc::ReleaseLayout( xoLayoutResult* layout )
{
	if ( layout == nullptr )
		return;
	TakeCriticalSection lock( LayoutLock );
	XOASSERT( LatestLayout->IsLocked );
	layout->IsLocked = false;
	PurgeOldLayouts();
}

// Warning: This assumes LayoutLock is already held
void xoRenderDoc::PurgeOldLayouts()
{
	for ( intp i = OldLayouts.size() - 1; i >= 0; i-- )
	{
		if ( !OldLayouts[i]->IsLocked )
		{
			delete OldLayouts[i];
			OldLayouts.erase( i );
		}
	}
}

/*
xoInternalID xoRenderDoc::FindElement( const xoRenderDomEl& el, xoPoint pos )
{
	if ( el.Children.size() == 0 )
		return el.InternalID;

	for ( intp i = 0; i < el.Children.size(); i++ )
	{
		if ( el.Children[i]->Pos.IsInsideMe( pos ) )
		{
			xoInternalID id = FindElement( *el.Children[i], pos );
			if ( id != xoInternalIDNull )
				return id;
		}
	}

	return xoInternalIDNull;
}
*/

/*
void xoRenderDoc::FindAlteredNodes( const xoDoc* original, const xoDoc* modified, podvec<xoInternalID>& alteredNodeIDs )
{
	int top = (int) min( original->ChildByInternalIDListSize(), modified->ChildByInternalIDListSize() );
	const xoDomEl** org = original->ChildByInternalIDList();
	const xoDomEl** mod = modified->ChildByInternalIDList();
	for ( int i = 0; i < top; i++ )
	{
		if (	(org[i] && mod[i]) &&
				(org[i]->GetVersion() != mod[i]->GetVersion()) )
		{
			alteredNodeIDs += i;
		}
	}
}
*/