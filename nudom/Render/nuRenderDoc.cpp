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
	rend.Render( rgl, &RenderRoot, Doc.WindowWidth, Doc.WindowHeight );
}

void nuRenderDoc::UpdateDoc( const nuDoc& original )
{
	Doc.Reset();
	original.CloneFastInto( Doc, 0 );
}

