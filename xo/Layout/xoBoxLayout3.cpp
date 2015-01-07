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

	FlowStates.Init(maxDomTreeDepth);
	NodeStates.Init(maxDomTreeDepth);
}

xoBoxLayout3::~xoBoxLayout3()
{
}

void xoBoxLayout3::BeginDocument()
{
	// Add a dummy root node. This root node has no limits. This does not house the
	// top-level <body> tag; it is the parent of the <body> tag.
	// This is really just a dummy so that <body> can go through the same code paths as all of its children,
	// and so that we never have an empty NodeStates stack.
	NodeState& s = NodeStates.Add();
	s.Input.NewFlowContext = false;
	FlowState& flow = FlowStates.Add();
	flow.Reset();
}

void xoBoxLayout3::EndDocument()
{
	XOASSERT(NodeStates.Count == 1);
	XOASSERT(FlowStates.Count == 1);
	NodeStates.Pop();
	FlowStates.Pop();
}

void xoBoxLayout3::BeginNode(const NodeInput& in)
{
	NodeState& parentNode = NodeStates.Back();
	NodeState& ns = NodeStates.Add();
	ns.Input = in;
	ns.MarginBox = xoBox(0,0,0,0);
	if (in.NewFlowContext)
	{
		FlowState& flow = FlowStates.Add();
		flow.Reset();
		flow.MaxMinor = in.ContentWidth;
		flow.MaxMajor = in.ContentHeight;
	}
}

void xoBoxLayout3::EndNode(xoBox& marginBox)
{
	NodeState* ns = &NodeStates.Back();
	if (ns->Input.NewFlowContext)
	{
		FlowStates.Pop();
		Flow(*ns, FlowStates.Back(), ns->MarginBox);
	}

	marginBox = ns->MarginBox;

	NodeStates.Pop(); // ns is invalid after Pop()
}

/* One of two things can happen when you add a word:
1. The word fits on the current line.
	* The returned rtxt is an existing value.
	* The returned posX is the offset inside rtxt where this word starts.
2. The word needs to go onto a new line.
	* The returned rtxt is a new value.
	* The returned posX is zero (which is consistent with the definition of the above case #1).
*/
void xoBoxLayout3::AddWord(const WordInput& in, xoBox& marginBox)
{
	NodeState ns;
	ns.Input.ContentWidth = in.Width;
	ns.Input.ContentHeight = in.Height;
	ns.Input.MarginAndPadding = xoBox(0, 0, 0, 0);
	Flow(ns, FlowStates.Back(), marginBox);
}

void xoBoxLayout3::AddSpace(xoPos width)
{
	FlowStates.Back().PosMinor += width;
}

void xoBoxLayout3::AddLinebreak()
{
	NewLine(FlowStates.Back());
}

void xoBoxLayout3::Flow(const NodeState& ns, FlowState& flow, xoBox& marginBox)
{
	xoPos marginBoxWidth = ns.Input.MarginAndPadding.Left + ns.Input.MarginAndPadding.Right + (ns.Input.ContentWidth != xoPosNULL ? ns.Input.ContentWidth : 0);
	xoPos marginBoxHeight = ns.Input.MarginAndPadding.Top + ns.Input.MarginAndPadding.Bottom + (ns.Input.ContentHeight != xoPosNULL ? ns.Input.ContentHeight : 0);
	bool overflow = flow.PosMinor + marginBoxWidth > flow.MaxMinor;
	bool onNewLine = flow.PosMinor == 0;
	if (flow.MaxMinor != xoPosNULL && overflow && !onNewLine)
		NewLine(flow);

	marginBox.Left = flow.PosMinor;
	marginBox.Top = flow.PosMajor;
	marginBox.Right = flow.PosMinor + marginBoxWidth;
	marginBox.Bottom = flow.PosMajor + marginBoxHeight;

	flow.PosMinor = marginBox.Right;
	flow.HighMajor = xoMax(flow.HighMajor, marginBox.Bottom);
}

void xoBoxLayout3::NewLine(FlowState& flow)
{
	flow.Lines += LineBox::Make(0, 0, 0);
	flow.PosMajor = flow.HighMajor;
	flow.PosMinor = 0;
}

void xoBoxLayout3::FlowState::Reset()
{
	PosMinor = 0;
	PosMajor = 0;
	HighMajor = 0;
	MaxMajor = xoPosNULL;
	MaxMinor = xoPosNULL;
	Lines.clear_noalloc();
}
