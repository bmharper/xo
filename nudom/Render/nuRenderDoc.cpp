#include "pch.h"
#include "../nuLayout.h"
#include "nuRenderDoc.h"
#include "nuRenderer.h"

nuRenderDoc::nuRenderDoc()
{
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

void nuRenderDoc::Render( nuRenderGL* rgl )
{
	//NUTRACE( "RenderDoc: Reset\n" );
	ResetRenderData();
	
	//NUTRACE( "RenderDoc: Layout\n" );
	nuLayout lay;
	lay.Layout( Doc, RenderRoot, &RenderPool );

	//NUTRACE( "RenderDoc: Render\n" );
	nuRenderer rend;
	// TODO: Don't use Doc.Images - use a locked temp copy
	rend.Render( &ClonedImages, &ClonedStrings, rgl, &RenderRoot, Doc.WindowWidth, Doc.WindowHeight );
}

void nuRenderDoc::CopyFromCanonical( const nuDoc& original )
{
	// Find nodes that have changed, so that we can apply transitions
	ModifiedNodeIDs.clear();
	FindAlteredNodes( &Doc, &original, ModifiedNodeIDs );

	Doc.Reset();
	original.CloneFastInto( Doc, 0 );

	// TODO: Don't do this dumb copying.
	ClonedStrings.CloneFrom( original.Strings );
	ClonedImages.CloneFrom( original.Images );
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
