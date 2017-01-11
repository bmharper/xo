#include "pch.h"

static void SetDocDims(xo::Doc* doc, int width, int height)
{
	xo::Event ev;
	ev.MakeWindowSize(width, height);
	doc->UI.InternalProcessEvent(ev, nullptr);
}

// Simulate a single document that is mutated by a UI thread, and a renderer thread that continually consumes it.
// TESTFUNC(DocumentClone_Junk)
// {
// }

TESTFUNC(DocumentClone)
{
	// I need to add a null renderer for these kind of tests
	xo::SysWnd* wnd = xo::SysWnd::CreateWithDoc();
	xo::AddOrRemoveDocsFromGlobalList();
	xo::DocGroup* g = wnd->DocGroup;
	g->Render();
	SetDocDims(g->Doc, 16, 16);
	TTASSERT(g->RenderStats.Clone_NumEls == 0);
	xo::Doc* d = g->Doc;

	xo::DomNode* div1 = d->Root.AddNode(xo::TagDiv);
	for (int i = 0; i < 5; i++)
	{
		g->Render();
		TTASSERT(g->RenderStats.Clone_NumEls == 2);   // root and div1
	}

	div1->StyleParsef("left: 10px;");
	for (int i = 0; i < 5; i++)
	{
		g->Render();
		TTASSERT(g->RenderStats.Clone_NumEls == 3);   // div1
	}

	d->Root.DeleteChild(div1);
	for (int i = 0; i < 5; i++)
	{
		g->Render();
		TTASSERT(g->RenderStats.Clone_NumEls == 5);   // root and div1
	}

	delete wnd;
	xo::AddOrRemoveDocsFromGlobalList();
}
