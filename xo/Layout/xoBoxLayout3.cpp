#include "pch.h"
#include "xoBoxLayout3.h"
#include "xoDoc.h"
#include "Dom/xoDomNode.h"
#include "Dom/xoDomText.h"
#include "Render/xoRenderDomEl.h"
#include "Render/xoStyleResolve.h"
#include "Text/xoFontStore.h"
#include "Text/xoGlyphCache.h"

xoBoxLayout3::xoBoxLayout3()
{
	// This constant is arbitrarily chosen. The program will panic if the limit is exceeded.
	int maxDomTreeDepth = 100;

	FlowStates.Init( maxDomTreeDepth );
	NodeStates.Init( maxDomTreeDepth );
}

xoBoxLayout3::~xoBoxLayout3()
{
}

//void xoBoxLayout3::BeginDocument( int vpWidth, int vpHeight, xoRenderDomNode* root )
void xoBoxLayout3::BeginDocument( xoRenderDomNode* root )
{
	// Add a dummy root node. This root node has no limits. This does not house the
	// top-level <body> tag. This is really just a dummy so that <body> can
	// go through the same code paths as all of its children.
	NodeState& s = NodeStates.Add();
	s.Input.NewFlowContext = false;
	s.RNode = root;
	FlowState& flow = FlowStates.Add();
	flow.Reset();
}

void xoBoxLayout3::EndDocument()
{
	XOASSERT( NodeStates.Count == 1 );
	XOASSERT( FlowStates.Count == 1 );
	NodeStates.Pop();
	FlowStates.Pop();
}

// Eventually we'll want to add a method AddWordBox, which will be used to add
// word boxes. This is for efficiency, because we know that word boxes are simple
// and have no children.

void xoBoxLayout3::BeginNode( const NodeInput& in )
{
	NodeState& parentNode = NodeStates.Back();
	NodeState& ns = NodeStates.Add();
	ns.Input = in;
	ns.MarginBox = xoBox(0,0,0,0);
	if ( in.NewFlowContext )
	{
		FlowState& flow = FlowStates.Add();
		flow.Reset();
		flow.MaxMinor = in.ContentWidth;
		flow.MaxMajor = in.ContentHeight;
	}

	//if ( in.Tag == xoTagText )
	//{
	//	xoRenderDomText* rchild = new (Pool->AllocT<xoRenderDomText>(false)) xoRenderDomText( ns.Input.InternalID, Pool );
	//	parentNode.RNode->Children += rchild;
	//}
	//else
	{
		xoRenderDomNode* rchild = new (Pool->AllocT<xoRenderDomNode>(false)) xoRenderDomNode( ns.Input.InternalID, ns.Input.Tag, Pool );
		parentNode.RNode->Children += rchild;
		ns.RNode = rchild;
	}
}

void xoBoxLayout3::EndNode()
{
	NodeState& ns = NodeStates.Back();
	if ( ns.Input.NewFlowContext )
	{
		FlowStates.Pop();
		Flow( ns, FlowStates.Back(), ns.MarginBox );
	}
	else
	{

	}
	NodeStates.Pop();
}

xoRenderDomText* xoBoxLayout3::AddWord( const WordInput& in )
{
	NodeState ns;
	ns.Input.ContentWidth = in.Width;
	ns.Input.ContentHeight = in.Height;
	ns.Input.Margin = xoBox(0,0,0,0);
	ns.Input.Padding = xoBox(0,0,0,0);
	xoBox marginBox;
	bool isNewLine = Flow( ns, FlowStates.Back(), marginBox );

	NodeState& parentNode = NodeStates.Back();
	xoRenderDomText* lastChild = nullptr;
	if ( parentNode.RNode->Children.size() != 0 && parentNode.RNode->Children.back()->IsText() )
		lastChild = static_cast<xoRenderDomText*>(parentNode.RNode->Children.back());

	if ( isNewLine || lastChild == nullptr )
	{
		// For a new line, we need to start a new xoRenderDomText object.
		xoRenderDomText* rchild = new (Pool->AllocT<xoRenderDomText>(false)) xoRenderDomText(parentNode.Input.InternalID, Pool);
		parentNode.RNode->Children += rchild;
		rchild->Pos = marginBox;
		return rchild;
	}
	else
	{
		// Reuse the existing text object
		lastChild->Pos.Right = marginBox.Right;
		return lastChild;
	}
}

void xoBoxLayout3::AddSpace( xoPos width )
{
	FlowStates.Back().PosMinor += width;
}

void xoBoxLayout3::AddLinebreak()
{
	NewLine( FlowStates.Back() );
}

bool xoBoxLayout3::Flow( const NodeState& ns, FlowState& flow, xoBox& marginBox )
{
	bool isNewLine = false;
	xoPos marginBoxWidth = ns.Input.Margin.Left + ns.Input.Margin.Right + ns.Input.Padding.Left + ns.Input.Padding.Right + (ns.Input.ContentWidth != xoPosNULL ? ns.Input.ContentWidth : 0);
	xoPos marginBoxHeight = ns.Input.Margin.Top + ns.Input.Margin.Bottom + ns.Input.Padding.Top + ns.Input.Padding.Bottom + (ns.Input.ContentHeight != xoPosNULL ? ns.Input.ContentHeight : 0);
	bool overflow = flow.PosMinor + marginBoxWidth > flow.MaxMinor;
	bool onNewLine = flow.PosMinor == 0;
	if ( flow.MaxMinor != xoPosNULL && overflow && !onNewLine )
	{
		isNewLine = true;
		NewLine( flow );
	}

	marginBox.Left = flow.PosMinor;
	marginBox.Top = flow.PosMajor;
	marginBox.Right = flow.PosMinor + marginBoxWidth;
	marginBox.Bottom = flow.PosMajor + marginBoxHeight;

	flow.PosMinor = marginBox.Right;
	return isNewLine;
}

void xoBoxLayout3::NewLine( FlowState& flow )
{
	flow.Lines += LineBox::Make( 0, 0, 0 );
	flow.PosMajor = flow.MaxMajor;
	flow.PosMinor = 0;
}

void xoBoxLayout3::FlowState::Reset()
{
	PosMinor = 0;
	PosMajor = 0;
	MaxMajor = xoPosNULL;
	MaxMinor = xoPosNULL;
	Lines.clear_noalloc();
}
