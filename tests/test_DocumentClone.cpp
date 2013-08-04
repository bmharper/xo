#include "pch.h"

// erg.... intellisense on VS 2012 is broken without this bogus include here
#include "../nudom/nuDoc.h"

static AbcThreadReturnType AbcKernelCallbackDecl ui_thread( void* tp )
{
	int niter = 10000;
	for ( int iter = 0; iter < niter; iter++ )
	{

	}
	return 0;
}

static AbcThreadReturnType AbcKernelCallbackDecl rend_thread( void* tp )
{
	/*
	int niter = 10000;
	nuDoc* srcDoc = (nuDoc*) tp;
	nuRenderDoc rdoc;

	for ( int iter = 0; iter < niter; iter++ )
	{
		rdoc.UpdateDoc( *srcDoc );
	}
	*/
	return 0;
}

// Simulate a single document that is mutated by a UI thread, and a renderer thread that continually consumes it.
TESTFUNC(DocumentClone_Junk)
{
	nuDoc d1;
	nuDomEl* div = d1.Root.AddChild( nuTagDiv );
	div->StyleParse( "margin: 3px;" );

	AbcThreadHandle t_ui, t_render;
	AbcThreadCreate( &ui_thread, &d1, t_ui );
	AbcThreadCreate( &rend_thread, &d1, t_render );

	AbcThreadJoin( t_ui );
	AbcThreadJoin( t_render );
	AbcThreadCloseHandle( t_ui );
	AbcThreadCloseHandle( t_render );

	//nuDoc d2;
	//d1.CloneFastInto( d2, 0 );
}

TESTFUNC(DocumentClone)
{
	nuDocGroup g;
	g.Doc = new nuDoc();
	g.Render();
	g.Doc->WindowWidth = 16;
	g.Doc->WindowHeight = 16;
	TTASSERT( g.RenderStats.Clone_NumEls == 0 );
	nuDoc* d = g.Doc;

	nuDomEl* div1 = d->Root.AddChild( nuTagDiv );
	for ( int i = 0; i < 5; i++ )
	{
		g.Render();
		TTASSERT( g.RenderStats.Clone_NumEls == 2 ); // root and div1
	}

	div1->StyleParsef( "left: 10px;" );
	for ( int i = 0; i < 5; i++ )
	{
		g.Render();
		TTASSERT( g.RenderStats.Clone_NumEls == 3 ); // div1
	}

	d->Root.RemoveChild( div1 );
	for ( int i = 0; i < 5; i++ )
	{
		g.Render();
		TTASSERT( g.RenderStats.Clone_NumEls == 5 ); // root and div1
	}
}
