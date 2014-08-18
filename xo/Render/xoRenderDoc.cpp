#include "pch.h"
#include "../Layout/xoLayout.h"
#include "../Layout/xoLayout2.h"
#include "xoRenderDoc.h"
#include "xoRenderer.h"
#include "xoRenderDX.h"
#include "xoRenderGL.h"

xoRenderDoc::xoRenderDoc()
{
	WindowWidth = 0;
	WindowHeight = 0;
	RenderRoot.SetPool( &RenderPool );
}

xoRenderDoc::~xoRenderDoc()
{
}

void xoRenderDoc::ResetRenderData()
{
	RenderRoot.Discard();
	RenderPool.FreeAll();
	RenderRoot.InternalID = Doc.Root.GetInternalID();
}

xoRenderResult xoRenderDoc::Render( xoRenderBase* driver )
{
	XOTRACE_RENDER( "RenderDoc: Reset\n" );
	ResetRenderData();
	
	XOTRACE_RENDER( "RenderDoc: Layout\n" );
	xoLayout2 lay;
	lay.Layout( Doc, WindowWidth, WindowHeight, RenderRoot, &RenderPool );

	XOTRACE_RENDER( "RenderDoc: Render\n" );
	xoRenderer rend;
	xoRenderResult res = rend.Render( &ClonedImages, &Doc.Strings, driver, &RenderRoot, WindowWidth, WindowHeight );

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