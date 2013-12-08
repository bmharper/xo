#include "pch.h"
#include "../nuLayout.h"
#include "nuRenderDoc.h"
#include "nuRenderer.h"

nuRenderDoc::nuRenderDoc()
{
	WindowWidth = 0;
	WindowHeight = 0;
	RenderRoot.SetPool( &RenderPool );
}

nuRenderDoc::~nuRenderDoc()
{
}

void nuRenderDoc::ResetRenderData()
{
	RenderRoot.Discard();
	RenderPool.FreeAll();
}

nuRenderResult nuRenderDoc::Render( nuRenderGL* rgl )
{
	NUTRACE_RENDER( "RenderDoc: Reset\n" );
	ResetRenderData();
	
	NUTRACE_RENDER( "RenderDoc: Layout\n" );
	nuLayout lay;
	lay.Layout( Doc, WindowWidth, WindowHeight, RenderRoot, &RenderPool );

	NUTRACE_RENDER( "RenderDoc: Render\n" );
	nuRenderer rend;
	nuRenderResult res = rend.Render( &ClonedImages, &ClonedStrings, rgl, &RenderRoot, WindowWidth, WindowHeight );

	return res;
}

void nuRenderDoc::CopyFromCanonical( const nuDoc& canonical, nuRenderStats& stats )
{
	// Find nodes that have changed, so that we can apply transitions
	//ModifiedNodeIDs.clear();
	//FindAlteredNodes( &Doc, &canonical, ModifiedNodeIDs );

	canonical.CloneSlowInto( Doc, 0, stats );

	// TODO: Don't do this dumb copying.
	ClonedStrings.CloneFrom( canonical.Strings );
	ClonedImages.CloneFrom( canonical.Images );
}

nuInternalID nuRenderDoc::FindElement( const nuRenderDomEl& el, nuPoint pos )
{
	if ( el.Children.size() == 0 )
		return el.InternalID;

	for ( intp i = 0; i < el.Children.size(); i++ )
	{
		if ( el.Children[i]->Pos.IsInsideMe( pos ) )
		{
			nuInternalID id = FindElement( *el.Children[i], pos );
			if ( id != nuInternalIDNull )
				return id;
		}
	}

	return nuInternalIDNull;
}

/*
void nuRenderDoc::FindAlteredNodes( const nuDoc* original, const nuDoc* modified, podvec<nuInternalID>& alteredNodeIDs )
{
	int top = (int) min( original->ChildByInternalIDListSize(), modified->ChildByInternalIDListSize() );
	const nuDomEl** org = original->ChildByInternalIDList();
	const nuDomEl** mod = modified->ChildByInternalIDList();
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